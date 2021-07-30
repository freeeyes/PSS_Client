#include "framework.h"
#include "windows.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        //插件的建立
        //load_module();
        break;
    case DLL_PROCESS_DETACH:
        //插件的销毁
        unload_module();
        break;
    }
    return TRUE;
}

