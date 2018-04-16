#pragma once

#ifdef _DEBUG
void TRACE(LPCTSTR lpszFmt, ...);
#else
#define TRACE __noop
#endif
