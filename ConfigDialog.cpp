#include "stdafx.h"
#include "resource.h"
#include "ConfigDialog.h"

LRESULT ConfigDialog::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	m_chkActivateWindows.Attach(GetDlgItem(IDC_ACTIVATE_WIN));
	m_chkCtrl.Attach(GetDlgItem(IDC_CTRL));
	m_chkAlt.Attach(GetDlgItem(IDC_ALT));
	m_chkShift.Attach(GetDlgItem(IDC_SHIFT));
	m_edtSnapDist.Attach(GetDlgItem(IDC_SNAP_DISTANCE));
	m_btnOK.Attach(GetDlgItem(IDOK));

	m_chkActivateWindows.SetCheck(Config.bActivateWindows ? 1 : 0);
	m_chkCtrl.SetCheck(Config.bCtrl ? 1 : 0);
	m_chkAlt.SetCheck(Config.bAlt ? 1 : 0);
	m_chkShift.SetCheck(Config.bShift ? 1 : 0);

	std::basic_stringstream<TCHAR> snapDistanceText;
	snapDistanceText << Config.nSnapDistance;
	m_edtSnapDist.SetWindowText(snapDistanceText.str().c_str());

	if(!Config.bCtrl && !Config.bAlt && !Config.bShift)
		m_chkAlt.SetCheck(1);

	CenterWindow(::GetDesktopWindow());

	SetGUIState();

	return TRUE;
}

LRESULT ConfigDialog::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	Config.bActivateWindows = m_chkActivateWindows.GetCheck() != 0;
	Config.bCtrl = m_chkCtrl.GetCheck() != 0;
	Config.bAlt = m_chkAlt.GetCheck() != 0;
	Config.bShift = m_chkShift.GetCheck() != 0;

	TCHAR buf[20+1] = { 0 };
	m_edtSnapDist.GetWindowText(buf, 20);
	Config.nSnapDistance = _ttoi(buf);

	EndDialog(IDOK);
	return 0;
}

LRESULT ConfigDialog::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	EndDialog(IDCANCEL);
	return 0;
}

LRESULT ConfigDialog::OnCtrl(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if(m_chkCtrl.GetCheck()) {
		if(m_chkAlt.GetCheck() || m_chkShift.GetCheck())
			m_chkCtrl.SetCheck(0);
	} else {
		m_chkCtrl.SetCheck(1);
	}

	return 0;
}

LRESULT ConfigDialog::OnAlt(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if(m_chkAlt.GetCheck()) {
		if(m_chkCtrl.GetCheck() || m_chkShift.GetCheck())
			m_chkAlt.SetCheck(0);
	} else {
		m_chkAlt.SetCheck(1);
	}

	return 0;
}

LRESULT ConfigDialog::OnShift(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if(m_chkShift.GetCheck()) {
		if(m_chkCtrl.GetCheck() || m_chkAlt.GetCheck())
			m_chkShift.SetCheck(0);
	} else {
		m_chkShift.SetCheck(1);
	}

	return 0;
}

LRESULT ConfigDialog::OnSnapChange(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if(m_edtSnapDist.m_hWnd)
		SetGUIState();	
	return 0;
}

void ConfigDialog::SetGUIState()
{
	m_btnOK.EnableWindow(m_edtSnapDist.GetWindowTextLength() > 0);
}