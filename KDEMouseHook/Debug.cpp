#include "stdafx.h"
#include "Debug.h"
#include <stdarg.h>

#ifdef _DEBUG
void TRACE(LPCTSTR lpszFmt, ...)
{
	TCHAR buf[512+1] = { 0 };
	va_list va;
	va_start(va, lpszFmt);
#if _MSC_VER < 1400
	_vsntprintf(buf, 512, lpszFmt, va);
#else
	_vsntprintf_s(buf, _countof(buf), 512, lpszFmt, va);
#endif
	va_end(va);
	OutputDebugString(buf);
}
#endif