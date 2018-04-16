#pragma once

#include "Config.h"

class StateMachine {
public:
	enum MouseButtonState { Nothing, Resizing, Moving };
	enum Direction { None = 0, Top = 1, Right = 2, Bottom = 4, Left = 8};

public:
	MouseButtonState* m_penState;
	HWND*             m_phWndDrag;
	HWND*             m_phWndOldCapture;
	POINT*            m_pptPrevPosition;
	bool*             m_pbHasMouseCapture;
	int*              m_pnResizeEdge;
	bool*             m_pbAlt;
	bool*             m_pbShift;
	bool*             m_pbCtrl;
	bool*             m_pbActivateWindows;
	RECT*             m_prcWindow;
	int*              m_pnSnapDistance;

	StateMachine();

	// Returns true if the message should not be propagated to 
	// destination window
	bool Execute(UINT nMessage, MOUSEHOOKSTRUCT* pMHS);
	void ResetState();

private:
	bool InitiateMove(MOUSEHOOKSTRUCT* pMHS);
	bool InitiateResize(MOUSEHOOKSTRUCT* pMHS);
	bool MaximizeOrNormalize(MOUSEHOOKSTRUCTEX* pMHS);
	void Move(MOUSEHOOKSTRUCT* pMHS);
	void Resize(MOUSEHOOKSTRUCT* pMHS);
	bool CloseWindow(MOUSEHOOKSTRUCT* pMHS);
	bool CorrectKeyStroke();
};