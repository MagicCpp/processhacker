/*
 * Process Hacker Plugins -
 *   Update Checker Plugin
 *
 * Copyright (C) 2016 dmex
 *
 * This file is part of Process Hacker.
 *
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "updater.h"
#include <shellapi.h>
#include <shlobj.h>

static TASKDIALOG_BUTTON TaskDialogButtonArray[] =
{
    { IDYES, L"Install Update" }
};

HRESULT CALLBACK FinalTaskDialogCallbackProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR dwRefData
    )
{
    PPH_UPDATER_CONTEXT context = (PPH_UPDATER_CONTEXT)dwRefData;

    switch (uMsg)
    {
    case TDN_NAVIGATED:
        {
            if (!PhGetOwnTokenAttributes().Elevated)
            {
                SendMessage(hwndDlg, TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE, IDYES, TRUE);
            }
        }
        break;
    case TDN_BUTTON_CLICKED:
        {
            if ((INT)wParam == IDRETRY)
            {
                ShowCheckingForUpdatesDialog(hwndDlg, dwRefData);
                return S_FALSE;
            }

            if ((INT)wParam == IDYES)
            {
                SHELLEXECUTEINFO info = { sizeof(SHELLEXECUTEINFO) };

                if (PhIsNullOrEmptyString(context->SetupFilePath))
                    break;

                info.lpFile = context->SetupFilePath->Buffer;
                info.lpVerb = PhGetOwnTokenAttributes().Elevated ? NULL : L"runas";
                info.nShow = SW_SHOW;
                info.hwnd = hwndDlg;

                ProcessHacker_PrepareForEarlyShutdown(PhMainWndHandle);

                if (!ShellExecuteEx(&info))
                {
                    // Install failed, cancel the shutdown.
                    ProcessHacker_CancelEarlyShutdown(PhMainWndHandle);

                    // Set button text for next action
                    //Button_SetText(GetDlgItem(hwndDlg, IDOK), L"Retry");

                    return S_FALSE;
                }
                else
                {
                    ProcessHacker_Destroy(PhMainWndHandle);
                }
            }
        }
        break;
    case TDN_HYPERLINK_CLICKED:
        {
            TaskDialogLinkClicked(context);
        }
        break;
    }

    return S_OK;
}

VOID ShowUpdateInstallDialog(
    _In_ HWND hwndDlg,
    _In_ LONG_PTR Context
    )
{
    PPH_UPDATER_CONTEXT context;
    TASKDIALOGCONFIG config;

    context = (PPH_UPDATER_CONTEXT)Context;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = context->IconLargeHandle;

    config.pszWindowTitle = L"Process Hacker - Updater";
    config.pszMainInstruction = L"Ready to install update";
    config.pszContent = L"The update has been sucessfully downloaded, hashed and the digital signature verified.\r\n\r\nClick install to continue update...";

    config.pButtons = TaskDialogButtonArray;
    config.cButtons = ARRAYSIZE(TaskDialogButtonArray);

    config.cxWidth = 200;
    config.pfCallback = FinalTaskDialogCallbackProc;
    config.lpCallbackData = Context;

    SendMessage(hwndDlg, TDM_NAVIGATE_PAGE, 0, (LPARAM)&config);
}

VOID ShowLatestVersionDialog(
    _In_ HWND hwndDlg,
    _In_ LONG_PTR Context
    )
{
    PPH_UPDATER_CONTEXT context;
    TASKDIALOGCONFIG config;

    context = (PPH_UPDATER_CONTEXT)Context;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED | TDF_ENABLE_HYPERLINKS;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = context->IconLargeHandle;

    config.pszWindowTitle = L"Process Hacker - Updater";
    config.pszMainInstruction = L"You're running the latest version.";
        config.pszContent = PhaFormatString(
        L"Stable release build: v%lu.%lu.%lu\r\n\r\n<A HREF=\"executablestring\">View Changelog</A>",
        context->CurrentMajorVersion,
        context->CurrentMinorVersion,
        context->CurrentRevisionVersion
        )->Buffer;

    config.cxWidth = 200;
    config.pfCallback = FinalTaskDialogCallbackProc;
    config.lpCallbackData = Context;

    SendMessage(hwndDlg, TDM_NAVIGATE_PAGE, 0, (LPARAM)&config);
}

VOID ShowNewerVersionDialog(
    _In_ HWND hwndDlg,
    _In_ LONG_PTR Context
    )
{
    PPH_UPDATER_CONTEXT context;
    TASKDIALOGCONFIG config;

    context = (PPH_UPDATER_CONTEXT)Context;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = context->IconLargeHandle;

    config.pszWindowTitle = L"Process Hacker - Updater";
    config.pszMainInstruction = L"You're running a pre-release version!";
    config.pszContent = PhaFormatString(
        L"Pre-release build: v%lu.%lu.%lu\r\n",
        context->CurrentMajorVersion,
        context->CurrentMinorVersion,
        context->CurrentRevisionVersion
        )->Buffer;

    config.cxWidth = 200;
    config.pfCallback = FinalTaskDialogCallbackProc;
    config.lpCallbackData = Context;

    SendMessage(hwndDlg, TDM_NAVIGATE_PAGE, 0, (LPARAM)&config);
}

VOID ShowUpdateFailedDialog(
    _In_ HWND hwndDlg,
    _In_ LONG_PTR Context,
    _In_ BOOLEAN HashFailed,
    _In_ BOOLEAN SignatureFailed
    )
{
    PPH_UPDATER_CONTEXT context;
    TASKDIALOGCONFIG config;

    context = (PPH_UPDATER_CONTEXT)Context;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    //config.pszMainIcon = MAKEINTRESOURCE(65529);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON | TDCBF_RETRY_BUTTON;
    config.hMainIcon = context->IconLargeHandle;

    config.pszWindowTitle = L"Process Hacker - Updater";
    config.pszMainInstruction = L"An error was encountered while downloading the update.";

    if (SignatureFailed)
    {
        config.pszContent = L"Signature check failed. Please click Retry and download the update again.";
    }
    else if (HashFailed)
    {
        config.pszContent = L"Hash check failed. Please click Retry and download the update again.";
    }
    else
    {
        config.pszContent = L"Please click Retry and download the update again.";
    }

    config.cxWidth = 200;
    config.pfCallback = FinalTaskDialogCallbackProc;
    config.lpCallbackData = Context;

    SendMessage(hwndDlg, TDM_NAVIGATE_PAGE, 0, (LPARAM)&config);
}