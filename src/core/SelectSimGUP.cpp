#include "SelectSimGUP.h"
#include "PluginDescriptor.h"
#include "../selectsim/SelectSimilarAction.h"

SelectSimGUP SelectSimGUP::sInstance;

DWORD SelectSimGUP::Start()
{
    if (!mStarted)
    {
        // Force construction + activation of the action table so Shift+G works
        // for the whole Max session, even before any Customize UI enumeration.
        SelectSimilarActionTable::GetInstance();
        mStarted = TRUE;
    }
    return GUPRESULT_KEEP;
}

void SelectSimGUP::Stop()
{
    mStarted = FALSE;
}

DWORD_PTR SelectSimGUP::Control(DWORD parameter)
{
    return 0;
}

void SelectSimGUP::DeleteThis()
{
}
