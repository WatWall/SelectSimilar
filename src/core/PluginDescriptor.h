#pragma once

#include <max.h>
#include <actiontable.h>
#include "PluginDef.h"

class SelectSimGUPDesc : public ClassDesc2
{
public:
    int IsPublic() override { return TRUE; }
    void* Create(BOOL loading = FALSE) override;
    SClass_ID SuperClassID() override { return GUP_CLASS_ID; }
    Class_ID ClassID() override { return SELECTSIM_GUP_CLASS_ID; }
    const TCHAR* Category() override { return SELECTSIM_CATEGORY; }
    const TCHAR* InternalName() override { return _T("SelectSimGUP"); }
    HINSTANCE HInstance() override { return hInstance; }
    const MCHAR* ClassName() override { return _M("Select Similar GUP"); }
    const MCHAR* NonLocalizedClassName() override { return _M("Select Similar GUP"); }

    // Action-table integration: Max calls NumActionTables/GetActionTable on load.
    int NumActionTables() override;
    ActionTable* GetActionTable(int i) override;
};

SelectSimGUPDesc& GetSelectSimGUPDesc();
