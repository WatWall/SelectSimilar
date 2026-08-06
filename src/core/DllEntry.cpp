#include "PluginDef.h"
#include "PluginDescriptor.h"

HINSTANCE hInstance = nullptr;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        hInstance = hinstDLL;
        DisableThreadLibraryCalls(hinstDLL);
    }
    return TRUE;
}

extern "C" {

__declspec(dllexport) int LibNumberClasses()
{
    return 1;
}

__declspec(dllexport) ClassDesc* LibClassDesc(int i)
{
    switch (i)
    {
    case 0:  return &GetSelectSimGUPDesc();
    default: return nullptr;
    }
}

__declspec(dllexport) const TCHAR* LibDescription()
{
    return SELECTSIM_PLUGIN_DESC;
}

__declspec(dllexport) ULONG LibVersion()
{
    return VERSION_3DSMAX;
}

__declspec(dllexport) ULONG CanAutoDefer()
{
    return 1;
}

}
