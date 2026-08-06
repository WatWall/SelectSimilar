#include "PluginDescriptor.h"
#include "../selectsim/SelectSimilarAction.h"

SelectSimGUPDesc& GetSelectSimGUPDesc()
{
    static SelectSimGUPDesc desc;
    return desc;
}

int SelectSimGUPDesc::NumActionTables()
{
    return 1;
}

ActionTable* SelectSimGUPDesc::GetActionTable(int i)
{
    if (i == 0)
    {
        return SelectSimilarActionTable::GetInstance();
    }
    return nullptr;
}

void* SelectSimGUPDesc::Create(BOOL loading)
{
    return SelectSimGUP::GetInstance();
}
