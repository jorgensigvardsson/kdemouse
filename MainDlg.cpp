// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "aboutdlg.h"
#include "MainDlg.h"
#include "ConfigDialog.h"

BOOL CMainDlg::PreTranslateMessage(MSG* pMsg)
{
	return CWindow::IsDialogMessage(pMsg);
}

BOOL CMainDlg::OnIdle()
{
	return FALSE;
}

static HMODULE hModule = 0;
static InstallKDEMouseHook_t   pInstallKDEMouseHook = 0;
static UninstallKDEMouseHook_t pUninstallKDEMouseHook = 0;
static IsInstalled_t           pIsInstalled = 0;
static SetConfig_t             pSetConfig = 0;

LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// set icons
	HICON hIcon = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
		IMAGE_ICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
		IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
	SetIcon(hIconSmall, FALSE);

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	UIAddChildWindowContainer(m_hWnd);

	CONFIG conf;
	if(!LoadConfig(conf))
		GetDefaultConfig(conf);

	hModule = ::LoadLibrary(_T("KDEMouseHook.dll"));
	if(!hModule) {
		MessageBox(_T("Failed to load KDEMouseHook.dll"), _T("Error"), MB_OK | MB_ICONERROR);
		DestroyWindow();
		::PostQuitMessage(0);
		return TRUE;
	} else {
		pInstallKDEMouseHook = (InstallKDEMouseHook_t)::GetProcAddress(hModule, "InstallKDEMouseHook");
		pUninstallKDEMouseHook = (UninstallKDEMouseHook_t)::GetProcAddress(hModule, "UninstallKDEMouseHook");
		pIsInstalled = (IsInstalled_t)::GetProcAddress(hModule, "IsInstalled");
		pSetConfig = (SetConfig_t)::GetProcAddress(hModule, "SetConfig");

		if(!pInstallKDEMouseHook || !pUninstallKDEMouseHook || !pIsInstalled || !pSetConfig) {
			MessageBox(_T("KDEMouseHook.dll does not contain the needed entry points"), _T("Error"), MB_OK | MB_ICONERROR);
			pInstallKDEMouseHook = 0;
			pUninstallKDEMouseHook = 0;

			::FreeLibrary(hModule);
			hModule = 0;
			DestroyWindow();
			::PostQuitMessage(0);
			return TRUE;
		} else {
			if(!pInstallKDEMouseHook(conf)) {
				MessageBox(_T("KDEMouseHook.dll failed to install mouse hook"), _T("Error"), MB_OK | MB_ICONERROR);
				pInstallKDEMouseHook = 0;
				pUninstallKDEMouseHook = 0;
				::FreeLibrary(hModule);
				hModule = 0;
				DestroyWindow();
				::PostQuitMessage(0);
				return TRUE;
			}
		}
	}

	InstallIcon(
		_T("KDE Mouse Emulation"), 
		hIconSmall, 
		IDR_TRAYMENU
	);

	SetDefaultItem(ID_CONFIGURE);

	return TRUE;
}

LRESULT CMainDlg::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CAboutDlg dlg;
	dlg.DoModal();
	return 0;
}

LRESULT CMainDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CloseDialog(wID);
	return 0;
}

LRESULT CMainDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CloseDialog(wID);
	return 0;
}

void CMainDlg::CloseDialog(int nVal)
{
	if(pUninstallKDEMouseHook)
		pUninstallKDEMouseHook();
	pUninstallKDEMouseHook = 0;
	pInstallKDEMouseHook = 0;
	if(hModule)
		::FreeLibrary(hModule);
	hModule = 0;
	DestroyWindow();
	::PostQuitMessage(nVal);
}

void CMainDlg::PrepareMenu(HMENU hMenu)
{
	CMenuHandle menu(hMenu);
    menu.EnableMenuItem(ID_QUIT, MF_ENABLED);
	if(pIsInstalled()) {
		menu.EnableMenuItem(ID_ENABLE, MF_GRAYED);
		menu.EnableMenuItem(ID_DISABLE, MF_ENABLED);
		menu.CheckMenuItem(ID_ENABLE, MF_CHECKED);
		menu.CheckMenuItem(ID_DISABLE, MF_UNCHECKED);
	} else {
		menu.EnableMenuItem(ID_ENABLE, MF_ENABLED);
		menu.EnableMenuItem(ID_DISABLE, MF_GRAYED);
		menu.CheckMenuItem(ID_ENABLE, MF_UNCHECKED);
		menu.CheckMenuItem(ID_DISABLE, MF_CHECKED);
    }
}

LRESULT CMainDlg::OnEnable(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CONFIG conf;
	if(!LoadConfig(conf))
		GetDefaultConfig(conf);

	if(!pIsInstalled()) {
		if(!pInstallKDEMouseHook(conf)) {
			::MessageBox(NULL, _T("Failed to install mouse hook"), _T("Error"), MB_OK | MB_ICONERROR);
		}
	}
	return 0;
}

LRESULT CMainDlg::OnDisable(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if(pIsInstalled()) {
		pUninstallKDEMouseHook();
	}
	return 0;
}

LRESULT CMainDlg::OnConfigure(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	ConfigDialog dlg;
	if(!LoadConfig(dlg.Config))
		GetDefaultConfig(dlg.Config);

	if(dlg.DoModal()) {
		if(!StoreConfig(dlg.Config)) {
			MessageBox(_T("Failed to store configuration in the registry"), _T("Warning!"), MB_OK | MB_ICONWARNING);
		} else
			pSetConfig(dlg.Config);
	}

	return 0;
}

bool CMainDlg::LoadConfig(CONFIG& conf)
{
	CRegKey key;
	long nError = key.Open(HKEY_CURRENT_USER, _T("Software\\KDEMouse"), KEY_READ);
	if(nError != ERROR_SUCCESS)
		return false;

	DWORD dwActivate;
	DWORD dwCtrl;
	DWORD dwAlt;
	DWORD dwShift;
	DWORD dwSnapDistance;

	bool bSuccess =
		ERROR_SUCCESS == key.QueryDWORDValue(_T("Alt"), dwAlt) &&
		ERROR_SUCCESS == key.QueryDWORDValue(_T("Ctrl"), dwCtrl) &&
		ERROR_SUCCESS == key.QueryDWORDValue(_T("Shift"), dwShift) &&
		ERROR_SUCCESS == key.QueryDWORDValue(_T("ActivateWin"), dwActivate) &&
		ERROR_SUCCESS == key.QueryDWORDValue(_T("SnapDistance"), dwSnapDistance);

	key.Close();

	if(!bSuccess)
		return false;

	conf.bActivateWindows = dwActivate != 0;
	conf.bAlt = dwAlt != 0;
	conf.bCtrl = dwCtrl != 0;
	conf.bShift = dwShift != 0;
	conf.nSnapDistance = dwSnapDistance;

	return true;
}

bool CMainDlg::StoreConfig(const CONFIG& conf)
{
	CRegKey key;
	long nError = key.Create(HKEY_CURRENT_USER, _T("Software\\KDEMouse"), REG_NONE, REG_OPTION_NON_VOLATILE, KEY_WRITE);
	if(nError != ERROR_SUCCESS)
		return false;

	bool bSuccess = 
		ERROR_SUCCESS == key.SetDWORDValue(_T("Alt"), conf.bAlt ? 1 : 0) &&
		ERROR_SUCCESS == key.SetDWORDValue(_T("Shift"), conf.bShift ? 1 : 0) &&
		ERROR_SUCCESS == key.SetDWORDValue(_T("Ctrl"), conf.bCtrl ? 1 : 0) &&
		ERROR_SUCCESS == key.SetDWORDValue(_T("ActivateWin"), conf.bActivateWindows ? 1 : 0) &&
		ERROR_SUCCESS == key.SetDWORDValue(_T("SnapDistance"), conf.nSnapDistance);

	key.Close();

	return bSuccess;
}

void CMainDlg::GetDefaultConfig(CONFIG& conf)
{
	conf.bActivateWindows = false;
	conf.bAlt = true;
	conf.bShift = false;
	conf.bCtrl = false;
	conf.nSnapDistance = 20;
}