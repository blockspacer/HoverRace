// CheckUpdateServerDialog.cpp
// Checks the update server for any updates, and asks the user if they should
// be applied (if necessary).
//
// Copyright (c) 2009 Ryan Curtin
//
// Licensed under GrokkSoft HoverRace SourceCode License v1.0(the "License");
// you may not use this file except in compliance with the License.
//
// A copy of the license should have been attached to the package from which
// you have taken this file. If you can not find the license you can not use
// this file.
//
//
// The author makes no representations about the suitability of
// this software for any purpose.  It is provided "as is" "AS IS",
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied.
//
// See the License for the specific language governing permissions
// and limitations under the License.

#include "StdAfx.h"

#include "CheckUpdateServerDialog.h"

#include "../../engine/Net/Agent.h"
#include "../../engine/Net/NetExn.h"
#include "../../engine/Util/Str.h"

#include "resource.h"

using namespace HoverRace;
using namespace HoverRace::Client;
using namespace HoverRace::Util;

namespace {
	// CancelFlag that checks if the thread has been interrupted.
	// Note: This cannot be moved to the engine since it must be in the same
	//       module as where the thread was created, due to limitations of
	//       boost::thread.
	class ThreadInterruptedCancelFlag : public Net::CancelFlag
	{
		virtual bool IsCanceled()
		{
			try {
				boost::this_thread::interruption_point();
			}
			catch (boost::thread_interrupted&) {
				return true;
			}
			return false;
		}
	};
}

CheckUpdateServerDialog::CheckUpdateServerDialog(string url) :
	url(url)
{
	// nothing to do, we hope
}

CheckUpdateServerDialog::~CheckUpdateServerDialog()
{
	// also nothing to do, we hope
}

void CheckUpdateServerDialog::CheckUpdatesFromUrl(const std::string &url,
					Net::CancelFlagPtr cancelFlag)
{

}

void CheckUpdateServerDialog::ShowModal(HINSTANCE hinst, HWND parent)
{
	result_t result = (result_t) DialogBoxParamW(hinst,
		MAKEINTRESOURCEW(IDD_PERFORMING_TASK),
		parent, DlgFunc, reinterpret_cast<LPARAM>(this));

	string msg;
	switch (result) {
		case RESULT_UPDATE:
			// ask user if they want to update
			{
				msg = _("New version of HoverRace available.");
				msg += "\n\n";
				msg += _("Current version: ");
				msg += dlPtr->currentVersion.toString();
				msg += '\n';
				msg += _("Available version: ");
				msg += dlPtr->updatedVersion.toString();
				msg += "\n\nUpdate HoverRace?";

				if(MessageBoxW(parent, Str::UW(msg.c_str()), PACKAGE_NAME_L,
					MB_YESNO) == IDYES) {
						// only update if the user hit ok
						dlPtr->DownloadUpdate(".");
						dlPtr->ApplyUpdate(); // at this point HoverRace will
							// exit and restart (unless the user chooses not to
							// apply the patch for some reason)
				}
				// do not do anything if the user chooses not to update
			}
			break;
		case RESULT_FAILED:
			{
				msg = _("Error while checking for updates:");
				msg += '\n';
				msg += url;
				msg += "\n\n";
				msg += errMsg;

				MessageBoxW(parent, Str::UW(msg.c_str()), PACKAGE_NAME_L,
					MB_ICONWARNING | MB_OK);
			}
			break;
	}
}

BOOL CALLBACK CheckUpdateServerDialog::DlgProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	BOOL retv = FALSE;

	CheckUpdateServerDialog *dlg = reinterpret_cast<CheckUpdateServerDialog *>(lparam);

	switch(message) {
		case WM_INITDIALOG:
			SetWindowTextW(hwnd, PACKAGE_NAME_L);
			SetDlgItemTextW(hwnd, IDC_MSG_LBL, Str::UW(_("Checking server for updates.  Please wait.")));
			SetDlgItemTextW(hwnd, IDCANCEL, Str::UW(_("Cancel")));
			retv = TRUE;

			dlg->loadThread = boost::thread(boost::bind(&CheckUpdateServerDialog::ThreadProc, dlg, hwnd));
			break;

		case WM_COMMAND:
			switch (LOWORD(wparam)) {
				case IDCANCEL:
					dlg->loadThread.interrupt();
					SetDlgItemTextW(hwnd, IDC_MSG_LBL, Str::UW(_("Canceling...")));
					EnableWindow(GetDlgItem(hwnd, IDCANCEL), FALSE);
					retv = TRUE;
					break;
			}
			break;

		case WM_APP_UPDATE_CHECK_DONE:
			EnableWindow(GetDlgItem(hwnd, IDCANCEL), FALSE);
			EndDialog(hwnd, wparam);
			break;
	}

	return retv;
}

/// Global dialog callback dispatcher.
BOOL CALLBACK CheckUpdateServerDialog::DlgFunc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	// Determine which instance to route the message to.
	CheckUpdateServerDialog *dlg;
	if (message == WM_INITDIALOG) {
		dlg = reinterpret_cast<CheckUpdateServerDialog *>(lparam);
		SetWindowLong(hwnd, GWL_USERDATA, lparam);
	} else {
		dlg = reinterpret_cast<CheckUpdateServerDialog *>(GetWindowLong(hwnd, GWL_USERDATA));
		if (message == WM_DESTROY) {
			SetWindowLong(hwnd, GWL_USERDATA, 0);
		}
	}

	return (dlg == NULL) ? FALSE : dlg->DlgProc(hwnd, message, wparam, lparam);
}

void CheckUpdateServerDialog::ThreadProc(HWND hwnd) {
	result_t result;
	Net::CancelFlagPtr cancelFlag((Net::CancelFlag *) new ThreadInterruptedCancelFlag());

	try {
		dlPtr = UpdateDownloaderPtr(new UpdateDownloader());
		if(dlPtr->CheckUrl("http://www.hoverrace.com/updates/updates.php", cancelFlag))
			result = RESULT_UPDATE;
		else
			result = RESULT_UPTODATE;
	}
	catch (Net::CanceledExn &) {
		result = RESULT_CANCELED;
	}
	catch (Net::NetExn &ex) {
		errMsg = ex.what();
		result = RESULT_FAILED;
	}

	PostMessageW(hwnd, WM_APP_UPDATE_CHECK_DONE, result, 0);
}