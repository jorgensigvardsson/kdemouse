#include "stdafx.h"
#include "StateMachine.h"
#include "Debug.h"

StateMachine::StateMachine() 
{
}

bool
StateMachine::Execute(UINT nMessage, MOUSEHOOKSTRUCT* pMHS)
{
	bool bSuppressMessagePropagation = true;

	switch(*m_penState) {
		case Nothing: 
			switch(nMessage) {
				case WM_LBUTTONDOWN:
				case WM_NCLBUTTONDOWN:
					bSuppressMessagePropagation = InitiateMove(pMHS);
					break;
				case WM_RBUTTONDOWN:
				case WM_NCRBUTTONDOWN:
					bSuppressMessagePropagation = InitiateResize(pMHS);
					break;
				case WM_MOUSEWHEEL:
					bSuppressMessagePropagation = MaximizeOrNormalize((MOUSEHOOKSTRUCTEX*)pMHS);
					break;
				case WM_MBUTTONDOWN:
				case WM_NCMBUTTONDOWN:
					bSuppressMessagePropagation = CloseWindow(pMHS);
					break;
				default:
					bSuppressMessagePropagation = false;
					break;
			}
			break;
		case Moving:
			switch(nMessage) {
				case WM_MOUSEMOVE:
				case WM_NCMOUSEMOVE:
					Move(pMHS);
					break;
				case WM_LBUTTONUP:
				case WM_NCLBUTTONUP:
					ResetState();
					break;
			}
			break;
		case Resizing:
			switch(nMessage) {
				case WM_MOUSEMOVE:
				case WM_NCMOUSEMOVE:
					Resize(pMHS);
					break;
				case WM_RBUTTONUP:
				case WM_NCRBUTTONUP:
					ResetState();
					break;
			}
			break;
		default:
			assert(!"This should never occur!");
			ResetState();
			bSuppressMessagePropagation = false;
			break;
	}

	return bSuppressMessagePropagation;
}

void StateMachine::ResetState()
{
	if(*m_phWndOldCapture)
		::SetCapture(*m_phWndOldCapture);
	else if(*m_pbHasMouseCapture)
		::ReleaseCapture();

	*m_phWndOldCapture = 0;
	*m_pbHasMouseCapture = false;
	*m_phWndDrag = 0;
	*m_penState = Nothing;
	*m_pnResizeEdge = None;
}

static HWND GetTopLevelParent(HWND hWnd)
{
	if(!hWnd)
		return NULL;

	HWND hWndParent = ::GetParent(hWnd);
	while(hWndParent != 0) {
		hWnd = hWndParent;
		hWndParent = ::GetParent(hWnd);
	}

	return hWnd;
}

bool StateMachine::InitiateMove(MOUSEHOOKSTRUCT* pMHS)
{
	// Is the master control key pressed?
	if(!CorrectKeyStroke())
		return false;

	HWND hWndTopLevelParent = ::GetTopLevelParent(::WindowFromPoint(pMHS->pt));
	if(!hWndTopLevelParent)
		return false;

	// If the window is maximized, turn off the "maximize bit"
	DWORD dwStyle = ::GetWindowLong(hWndTopLevelParent, GWL_STYLE);
	if(dwStyle & WS_MAXIMIZE)
		::SetWindowLong(hWndTopLevelParent, GWL_STYLE, dwStyle & ~WS_MAXIMIZE);

	if(*m_pbActivateWindows)
		::SetForegroundWindow(hWndTopLevelParent);

	*m_penState = Moving;
	*m_phWndDrag = hWndTopLevelParent;
	*m_phWndOldCapture = ::SetCapture(hWndTopLevelParent);
	*m_pbHasMouseCapture = true;
	*m_pptPrevPosition = pMHS->pt;
	::GetWindowRect(hWndTopLevelParent, m_prcWindow);
	return true;
}

bool StateMachine::InitiateResize(MOUSEHOOKSTRUCT* pMHS)
{
	// Is the master control key pressed?
	if(!CorrectKeyStroke())
		return false;

	HWND hWndTopLevelParent = ::GetTopLevelParent(::WindowFromPoint(pMHS->pt));
	if(!hWndTopLevelParent)
		return false;

	DWORD dwStyle = ::GetWindowLong(hWndTopLevelParent, GWL_STYLE);
	if(!(dwStyle & WS_THICKFRAME))
		return false;

	// If the window is maximized, turn off the "maximize bit"
	if(dwStyle & WS_MAXIMIZE)
		::SetWindowLong(hWndTopLevelParent, GWL_STYLE, dwStyle & ~WS_MAXIMIZE);

	// Figure out which edge to resize
	RECT rcWindow;
	::GetWindowRect(hWndTopLevelParent, &rcWindow);
	
	// Normalize point and rectangle
	POINT pt = pMHS->pt;
	pt.x -= rcWindow.left;
	pt.y -= rcWindow.top;

	rcWindow.right -= rcWindow.left;
	rcWindow.left = 0;
	rcWindow.bottom -= rcWindow.top;
	rcWindow.top = 0;

	int nWidth = rcWindow.right - rcWindow.left;
	int nHeight = rcWindow.bottom - rcWindow.top;

	*m_pnResizeEdge = None;
	if(pt.x < nWidth / 3)
		*m_pnResizeEdge |= Left;
	else if(pt.x > nWidth - nWidth / 3)
		*m_pnResizeEdge |= Right;

	if(pt.y < nHeight / 3)
		*m_pnResizeEdge |= Top;
	else if(pt.y > nHeight - nHeight / 3)
		*m_pnResizeEdge |= Bottom;

	if(*m_pnResizeEdge == None)
		*m_pnResizeEdge = Bottom | Right; // Good default

	if(*m_pbActivateWindows)
		::SetForegroundWindow(hWndTopLevelParent);

	*m_penState = Resizing;
	*m_phWndDrag = hWndTopLevelParent;
	*m_phWndOldCapture = ::SetCapture(hWndTopLevelParent);
	*m_pbHasMouseCapture = true;
	*m_pptPrevPosition = pMHS->pt;
	::GetWindowRect(hWndTopLevelParent, m_prcWindow);
	return true;
}

bool StateMachine::MaximizeOrNormalize(MOUSEHOOKSTRUCTEX* pMHS)
{
	if(!CorrectKeyStroke())
		return false;

	HWND hWndTopLevelParent = ::GetTopLevelParent(::WindowFromPoint(pMHS->pt));
	if(!hWndTopLevelParent)
		return false;

	DWORD dwStyle = ::GetWindowLong(hWndTopLevelParent, GWL_STYLE);
	short nDelta = short(HIWORD(pMHS->mouseData));
	if((dwStyle & WS_MAXIMIZE) && nDelta < 0) {
		::ShowWindow(hWndTopLevelParent, SW_SHOWNORMAL);
		return true;
	} else if(!(dwStyle & WS_MAXIMIZE) && nDelta > 0) {
		::ShowWindow(hWndTopLevelParent, SW_SHOWMAXIMIZED);
		return true;
	} else
		return false;
}

void StateMachine::Move(MOUSEHOOKSTRUCT* pMHS)
{
	int nDeltaX = pMHS->pt.x - m_pptPrevPosition->x;
	int nDeltaY = pMHS->pt.y - m_pptPrevPosition->y;

	m_prcWindow->left += nDeltaX;
	m_prcWindow->right += nDeltaX;
	m_prcWindow->top += nDeltaY;
	m_prcWindow->bottom += nDeltaY;

	int x = m_prcWindow->left;
	int y = m_prcWindow->top;
	int width = m_prcWindow->right - m_prcWindow->left;
	int height = m_prcWindow->bottom - m_prcWindow->top;

	//HMONITOR hMon = ::MonitorFromWindow(*m_phWndDrag, MONITOR_DEFAULTTONEAREST);
	HMONITOR hMon = MonitorFromPoint(pMHS->pt, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi;
	mi.cbSize = sizeof(MONITORINFO);
	if (::GetMonitorInfo(hMon, &mi))
	{
		if (x + width > mi.rcWork.right - *m_pnSnapDistance && x + width < mi.rcWork.right + *m_pnSnapDistance)
			x = mi.rcWork.right - width;
		if (x > mi.rcWork.left - *m_pnSnapDistance && x < mi.rcWork.left + *m_pnSnapDistance)
			x = mi.rcWork.left;
		if (y + height > mi.rcWork.bottom - *m_pnSnapDistance && y + height < mi.rcWork.bottom + *m_pnSnapDistance)
			y = mi.rcWork.bottom - height;
		if (y > mi.rcWork.top - *m_pnSnapDistance && y  < mi.rcWork.top + *m_pnSnapDistance)
			y = mi.rcWork.top;
	}

	::MoveWindow(*m_phWndDrag, x, y, width, height, TRUE);

	*m_pptPrevPosition = pMHS->pt;
}

void StateMachine::Resize(MOUSEHOOKSTRUCT* pMHS)
{
	int nDeltaX = pMHS->pt.x - m_pptPrevPosition->x;
	int nDeltaY = pMHS->pt.y - m_pptPrevPosition->y;

	if(*m_pnResizeEdge & Right)
		m_prcWindow->right += nDeltaX;
	else if(*m_pnResizeEdge & Left)
		m_prcWindow->left += nDeltaX;

	if(*m_pnResizeEdge & Bottom)
		m_prcWindow->bottom += nDeltaY;
	else if(*m_pnResizeEdge & Top)
		m_prcWindow->top += nDeltaY;

	// Restrict size to a minimum of 20x20 pixels
	if(m_prcWindow->right - m_prcWindow->left < 20) {
		if(*m_pnResizeEdge & Right)
			m_prcWindow->right = m_prcWindow->left + 20;
		else
			m_prcWindow->left = m_prcWindow->right - 20;
	}

	if(m_prcWindow->bottom - m_prcWindow->top < 20) {
		if(*m_pnResizeEdge & Bottom)
			m_prcWindow->bottom = m_prcWindow->top + 20;
		else
			m_prcWindow->top = m_prcWindow->bottom - 20;
	}

	RECT rcWindow = *m_prcWindow;

	//HMONITOR hMon = ::MonitorFromWindow(*m_phWndDrag, MONITOR_DEFAULTTONEAREST);
	HMONITOR hMon = MonitorFromPoint(pMHS->pt, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi;
	mi.cbSize = sizeof(MONITORINFO);
	if (::GetMonitorInfo(hMon, &mi))
	{
		if(*m_pnResizeEdge & Right) {
			if(rcWindow.right > mi.rcWork.right - *m_pnSnapDistance && rcWindow.right < mi.rcWork.right + *m_pnSnapDistance)
				rcWindow.right = mi.rcWork.right;
		} else if(*m_pnResizeEdge & Left) {
			if(rcWindow.left > mi.rcWork.left - *m_pnSnapDistance && rcWindow.left < mi.rcWork.left + *m_pnSnapDistance)
				rcWindow.left = mi.rcWork.left;
		}
		if(*m_pnResizeEdge & Bottom) {
			if(rcWindow.bottom > mi.rcWork.bottom - *m_pnSnapDistance && rcWindow.bottom < mi.rcWork.bottom + *m_pnSnapDistance)
				rcWindow.bottom = mi.rcWork.bottom;
		} else if(*m_pnResizeEdge & Top) {
			if(rcWindow.top > mi.rcWork.top - *m_pnSnapDistance && rcWindow.top < mi.rcWork.top + *m_pnSnapDistance)
				rcWindow.top = mi.rcWork.top;
		}
	}

	::MoveWindow(*m_phWndDrag, rcWindow.left, rcWindow.top, rcWindow.right - rcWindow.left, rcWindow.bottom - rcWindow.top, TRUE);
	*m_pptPrevPosition = pMHS->pt;
}

bool StateMachine::CloseWindow(MOUSEHOOKSTRUCT* pMHS)
{
	if(!CorrectKeyStroke())
		return false;

	HWND hWndTopLevelParent = ::GetTopLevelParent(pMHS->hwnd);
	if(!hWndTopLevelParent)
		return false;

	::PostMessage(hWndTopLevelParent, WM_CLOSE, 0, 0);
	return true;
}

bool StateMachine::CorrectKeyStroke()
{
	bool bAlt   = ::GetAsyncKeyState(VK_MENU) < 0;
	bool bCtrl  = ::GetAsyncKeyState(VK_CONTROL) < 0;
	bool bShift = ::GetAsyncKeyState(VK_SHIFT) < 0;

	bool bResult =
		bAlt == *m_pbAlt &&
		bCtrl == *m_pbCtrl &&
		bShift == *m_pbShift;

	return bResult;
}