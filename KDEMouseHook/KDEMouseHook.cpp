// KDEMouseHook.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "KDEMouseHook.h"
#include "StateMachine.h"
#include "Debug.h"
#include <new>

static HINSTANCE hInstance = 0;
#pragma data_seg(".HOOKDAT") // Shared data among all instances.
HHOOK            g_hHook = 0;

// The state machine
StateMachine     g_statemachine;

// This is a GIGANTIC cludge. It seems that eventhough the statemachine object
// is shared between all processes, its constructor is called for each attaching
// process. This makes keeping state consistent pretty hard! Hence, the statemachine
// object does not hold any state of its own, but rather pointers to the variables
// declared below. It seems that static initializers are executed only once.
// These variables are bound to the statemachine object down in DllMain/DLL_PROCESS_ATTACH.
StateMachine::MouseButtonState g_enState = StateMachine::Nothing;
HWND             g_hWndDrag = 0;
HWND             g_hWndOldCapture = 0;
POINT            g_ptPrevPosition = { 0, 0 };
bool             g_bHasMouseCapture = false;
int              g_nResizeEdge = StateMachine::None;
bool             g_bAlt = true;
bool             g_bCtrl = true;
bool             g_bShift = false;
bool             g_bActivateWindows = false;
RECT			 g_rcWindow = { 0, 0, 0, 0 };
int              g_nSnapDistance = 20;
#pragma data_seg()

BOOL APIENTRY DllMain( HINSTANCE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		hInstance = hModule;

		// This is where the state is bound to the statemachine....
		g_statemachine.m_pbAlt = &g_bAlt;
		g_statemachine.m_pbCtrl = &g_bCtrl;
		g_statemachine.m_pbShift = &g_bShift;
		g_statemachine.m_pbActivateWindows = &g_bActivateWindows;
		g_statemachine.m_penState = &g_enState;
		g_statemachine.m_phWndDrag = &g_hWndDrag;
		g_statemachine.m_phWndOldCapture = &g_hWndOldCapture;
		g_statemachine.m_pptPrevPosition = &g_ptPrevPosition;
		g_statemachine.m_pbHasMouseCapture = &g_bHasMouseCapture;
		g_statemachine.m_pnResizeEdge = &g_nResizeEdge;
		g_statemachine.m_prcWindow = &g_rcWindow;
		g_statemachine.m_pnSnapDistance = &g_nSnapDistance;
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
    return TRUE;
}

//linker directive
#pragma comment(linker, "/SECTION:.HOOKDAT,RWS")

static LRESULT CALLBACK HookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	// We're not interested in messages generated with ::PeekMessage
	if(nCode == HC_NOREMOVE)
		return ::CallNextHookEx(g_hHook, nCode, wParam, lParam);

	//statemachine.SetConfig(config);
	if(g_statemachine.Execute(UINT(wParam), LPMOUSEHOOKSTRUCT(lParam))) {
		::CallNextHookEx(g_hHook, nCode, wParam, lParam);
		return 1; // This suppresses message propagation (non-zero return value that is)
	} else
		return ::CallNextHookEx(g_hHook, nCode, wParam, lParam);
}

bool InstallKDEMouseHook(CONFIG conf)
{
	if(g_hHook)
		return false;

	g_hHook = ::SetWindowsHookEx(WH_MOUSE, HookProc, hInstance, 0);
	if(!g_hHook)
		return false;
	g_bAlt = conf.bAlt;
	g_bCtrl = conf.bCtrl;
	g_bShift = conf.bShift;
	g_bActivateWindows = conf.bActivateWindows;
	g_nSnapDistance = conf.nSnapDistance;
	g_statemachine.ResetState();
	return true;
}

void UninstallKDEMouseHook()
{
	if(g_hHook) {
		g_statemachine.ResetState();
		::UnhookWindowsHookEx(g_hHook);
		g_hHook = 0;
	}
}

bool IsInstalled()
{
	return g_hHook != 0;
}

void SetConfig(CONFIG conf)
{
	g_bAlt = conf.bAlt;
	g_bCtrl = conf.bCtrl;
	g_bShift = conf.bShift;
	g_bActivateWindows = conf.bActivateWindows;
	g_nSnapDistance = conf.nSnapDistance;
	g_statemachine.ResetState();
}