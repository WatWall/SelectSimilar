#pragma once

#include <max.h>

extern HINSTANCE hInstance;

// "SSI" = ASCII 'S','S','I'  -> 0x535349  ("Select Similar")
#define SELECTSIM_GUP_CLASS_ID        Class_ID(0x53534900, 0x47555000)
#define SELECTSIM_ACTION_TABLE_ID     ((ActionTableId)0x53534901)

#define SELECTSIM_PLUGIN_NAME     L"Select Similar"
#define SELECTSIM_PLUGIN_DESC     L"Select Similar (Shift+G) - selects subobjects with similar attributes"
#define SELECTSIM_PLUGIN_AUTHOR   L"Eray"
#define SELECTSIM_PLUGIN_VERSION  1
#define SELECTSIM_CATEGORY        L"Select Similar"
