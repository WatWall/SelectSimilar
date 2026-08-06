#pragma once

#include <max.h>
#include <actiontable.h>
#include "../core/PluginDef.h"
#include "../core/SelectSimGUP.h"

// Combined ActionTable + ActionCallback (per the SDK's resettm.cpp pattern).
// A single instance is owned by ClassDesc::GetActionTable(0). Its constructor
// builds the operation list, sets "this" as the callback, and registers +
// activates the table so Shift+G works for the whole Max session.
class SelectSimilarActionTable : public ActionTable, public ActionCallback
{
public:
    static SelectSimilarActionTable* GetInstance();

    // ---- ActionCallback overrides ----
    BOOL ExecuteAction(int id) override;
    BOOL IsEnabled(int cmdId) override;

    // Bridge used by the menu callback to actually perform the work.
    static void DoSelectSimilar();

private:
    SelectSimilarActionTable();

    static SelectSimilarActionTable* sInstance;
};
