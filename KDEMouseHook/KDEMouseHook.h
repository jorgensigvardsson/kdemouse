#pragma once

#include "Config.h"

extern "C" {
	bool __declspec(dllexport) InstallKDEMouseHook(CONFIG conf);
	void __declspec(dllexport) UninstallKDEMouseHook();
	bool __declspec(dllexport) IsInstalled();
	void __declspec(dllexport) SetConfig(CONFIG conf);
}

typedef bool (*InstallKDEMouseHook_t)(CONFIG);
typedef void (*UninstallKDEMouseHook_t)();
typedef bool (*IsInstalled_t)();
typedef void (*SetConfig_t)(CONFIG conf);
