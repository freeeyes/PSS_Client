#ifndef _TIMEMANAGER_H
#define _TIMEMANAGER_H

#include <functional>
#include <queue>
#include <memory>
#include <vector>
#include <chrono>
#include <mutex>
#include <iostream>
#include <condition_variable>
#include <thread>

//当定时器中没有数据，最多等待的时间
const int timer_default_wait = 300;

//定时器组件
namespace brynet {

    enum class EM_TIMER_STATE
    {
        TIMER_STATE_ADD_TIMER = 0,
        TIMER_STATE_EXECUTE_TIMER,
    };

    enum class ENUM_WHILE_STATE
    {
        WHILE_STATE_CONTINUE = 0,
        WHILE_STATE_BREAK,
    };

    enum class ENUM_TIMER_TYPE
    {
        TIMER_TYPE_ONCE = 0,
        TIMER_TYPE_LOOP,
    };

    class TimerMgr;

    class Timer final
    {
    public:
        using Ptr = std::shared_ptr<Timer>;
        using WeakPtr = std::weak_ptr<Timer>;
        using Callback = std::function<void(void)>;

        Timer(std::chrono::steady_clock::time_point startTime,
            std::chrono::nanoseconds lastTime,
            std::chrono::seconds delayTime,
            ENUM_TIMER_TYPE timertype,
            Callback&& callback)
            :
            mCallback(std::move(callback)),
            mStartTime(startTime),
            mLastTime(lastTime),
            mDelayTime(delayTime),
            mTimerType(timertype)
        {
        }

        const std::chrono::steady_clock::time_point& getStartTime() const
        {
            return mStartTime;
        }

        const std::chrono::nanoseconds& getLastTime() const
        {
            return mLastTime;
        }

        std::chrono::nanoseconds getLeftTime()
        {
            const auto now = std::chrono::steady_clock::now();
            auto delayTime = mDelayTime;
            mDelayTime = std::chrono::seconds(0);
            return getLastTime() - (now - getStartTime()) + delayTime;
        }

        void    cancel()
        {
            std::call_once(mExecuteOnceFlag, [this]() {
                mCallback = nullptr;
                });
        }

        ENUM_TIMER_TYPE get_timer_type()
        {
            return mTimerType;
        }

    private:
        void operator() ()
        {
            if (mTimerType == ENUM_TIMER_TYPE::TIMER_TYPE_ONCE)
            {
                Callback callback;
                std::call_once(mExecuteOnceFlag, [&callback, this]() {
                    //一次运行
                    callback = std::move(mCallback);
                    mCallback = nullptr;
                    });

                if (callback != nullptr)
                {
                    callback();
                }
            }
            else
            {
                if (mCallback != nullptr)
                {
                    mCallback();
                }
                mStartTime = std::chrono::steady_clock::now();
            }
        }

        std::once_flag                                  mExecuteOnceFlag;
        Callback                                        mCallback;
        std::chrono::steady_clock::time_point           mStartTime;
        std::chrono::nanoseconds                        mLastTime;
        std::chrono::seconds                            mDelayTime;
        ENUM_TIMER_TYPE                                 mTimerType = ENUM_TIMER_TYPE::TIMER_TYPE_ONCE;

        friend class TimerMgr;
    };

    class TimerMgr final
    {
    public:
        using Ptr = std::shared_ptr<TimerMgr>;

        template<typename F, typename ...TArgs>
        Timer::WeakPtr  addTimer_loop(
            std::chrono::seconds delayseconds,
            std::chrono::nanoseconds timeout,
            F&& callback,
            TArgs&& ...args)
        {
            auto timer = std::make_shared<Timer>(
                std::chrono::steady_clock::now(),
                std::chrono::nanoseconds(timeout),
                delayseconds,
                ENUM_TIMER_TYPE::TIMER_TYPE_LOOP,
                std::bind(std::forward<F>(callback), std::forward<TArgs>(args)...));
            mtx_queue.lock();
            mTimers.push(timer);
            mtx_queue.unlock();

            //唤醒线程
            timer_wakeup_state = EM_TIMER_STATE::TIMER_STATE_ADD_TIMER;
            cv.notify_one();
            return timer;
        }

        template<typename F, typename ...TArgs>
        Timer::WeakPtr  addTimer(
            std::chrono::nanoseconds timeout,
            F&& callback,
            TArgs&& ...args)
        {
            auto timer = std::make_shared<Timer>(
                std::chrono::steady_clock::now(),
                std::chrono::nanoseconds(timeout),
                std::chrono::seconds(0),
                ENUM_TIMER_TYPE::TIMER_TYPE_ONCE,
                std::bind(std::forward<F>(callback), std::forward<TArgs>(args)...));
            mtx_queue.lock();
            mTimers.push(timer);
            mtx_queue.unlock();

            //唤醒线程
            timer_wakeup_state = EM_TIMER_STATE::TIMER_STATE_ADD_TIMER;
            cv.notify_one();
            return timer;
        }

        void addTimer(const Timer::Ptr& timer)
        {
            std::unique_lock <std::mutex> lck(mtx);
            mTimers.push(timer);

            //唤醒线程
            timer_wakeup_state = EM_TIMER_STATE::TIMER_STATE_ADD_TIMER;
            cv.notify_one();
        }

        void Close()
        {
            std::unique_lock <std::mutex> lck(mtx);
            timer_run_ = false;
            //唤醒线程
            timer_wakeup_state = EM_TIMER_STATE::TIMER_STATE_ADD_TIMER;
            cv.notify_one();
        }

        ENUM_WHILE_STATE timer_run()
        {
            std::unique_lock <std::mutex> lck(mtx);

            if (mTimers.empty())
            {
                //当前没有定时器，等待唤醒
                cv.wait_for(lck, std::chrono::seconds(timer_default_wait));
                // std::cout << "wake up" << std::endl;
            }

            if (!timer_run_)
            {
                //std::cout << "end up" << std::endl;
                return ENUM_WHILE_STATE::WHILE_STATE_BREAK;
            }

            if (mTimers.empty())
            {
                return ENUM_WHILE_STATE::WHILE_STATE_CONTINUE;
            }

            mtx_queue.lock();
            auto tmp = mTimers.top();
            mtx_queue.unlock();

            auto timer_wait = tmp->getLeftTime();
            if (timer_wait > std::chrono::nanoseconds::zero())
            {
                //还需要等待下一个到期时间
                cv.wait_for(lck, timer_wait);
                //std::cout << "[cv]cv.wait_for=" << timer_wait.count() << std::endl;

                if (timer_wakeup_state == EM_TIMER_STATE::TIMER_STATE_ADD_TIMER)
                {
                    return ENUM_WHILE_STATE::WHILE_STATE_CONTINUE;
                }
            }

            if (!timer_run_)
            {
                return ENUM_WHILE_STATE::WHILE_STATE_BREAK;
            }

            mtx_queue.lock();
            mTimers.pop();

            (*tmp)();

            //如果是循环消息，则自动添加。
            if (ENUM_TIMER_TYPE::TIMER_TYPE_LOOP == tmp->get_timer_type() && tmp->mCallback != nullptr)
            {
                //重新插入
                mTimers.push(tmp);
            }

            mtx_queue.unlock();



            timer_wakeup_state = EM_TIMER_STATE::TIMER_STATE_EXECUTE_TIMER;

            return ENUM_WHILE_STATE::WHILE_STATE_CONTINUE;
        }

        void schedule()
        {
            timer_run_ = true;

            while (timer_run_)
            {
                auto while_result = timer_run();

                if (while_result == ENUM_WHILE_STATE::WHILE_STATE_BREAK)
                {
                    break;
                }
                else
                {
                    continue;
                }
            }

            //std::cout << "[TimerMgr::schedule]Time manager is end.\n" << std::endl;
        }

        bool isEmpty()
        {
            std::unique_lock <std::mutex> lck(mtx);
            return mTimers.empty();
        }

        // if timer empty, return zero
        std::chrono::nanoseconds nearLeftTime()
        {
            std::unique_lock <std::mutex> lck(mtx);
            if (mTimers.empty())
            {
                return std::chrono::nanoseconds::zero();
            }

            auto result = mTimers.top()->getLeftTime();

            if (result < std::chrono::nanoseconds::zero())
            {
                return std::chrono::nanoseconds::zero();
            }

            return result;
        }

        void clear()
        {
            std::unique_lock <std::mutex> lck(mtx);
            while (!mTimers.empty())
            {
                mTimers.pop();
            }
        }

    private:
        class CompareTimer
        {
        public:
            bool operator() (const Timer::Ptr& left,
                const Timer::Ptr& right) const
            {
                const auto startDiff = left->getStartTime() - right->getStartTime();
                const auto lastDiff = left->getLastTime() - right->getLastTime();
                const auto diff = startDiff.count() + lastDiff.count();
                return diff > 0;
            }
        };

        std::priority_queue<Timer::Ptr, std::vector<Timer::Ptr>, CompareTimer>  mTimers;
        std::mutex mtx;
        std::recursive_mutex mtx_queue;
        std::condition_variable cv;
        bool timer_run_ = false;
        EM_TIMER_STATE timer_wakeup_state = EM_TIMER_STATE::TIMER_STATE_EXECUTE_TIMER;
    };

}

#endif
