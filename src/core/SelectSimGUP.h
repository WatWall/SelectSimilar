#pragma once

#include <max.h>
#include <gup.h>

class SelectSimGUP : public GUP
{
public:
    DWORD Start() override;
    void Stop() override;
    DWORD_PTR Control(DWORD parameter) override;
    void DeleteThis() override;

    static SelectSimGUP* GetInstance() { return &sInstance; }

private:
    SelectSimGUP() = default;
    static SelectSimGUP sInstance;
    BOOL mStarted = FALSE;
};
