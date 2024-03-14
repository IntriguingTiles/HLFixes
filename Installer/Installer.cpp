#include <Windows.h>
#include <ShObjIdl.h>
#include <ShlObj.h>
#include <string>
#include <cstdio>
#include <string_view>
#include "resource.h"

#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")  

#define HLTITLE L"HLFixes Installer"
#define HLCANTWRITE L"The installer was unable write to Half-Life's directory.\nTry restarting the installer as an Administrator."

// lifted from VersionHelpers.h
// we're restricted to the windows 7 sdk which doesn't have that header
BOOL IsWindowsVersionOrGreater(WORD wMajorVersion, WORD wMinorVersion, WORD wServicePackMajor)
{
	OSVERSIONINFOEXW osvi = { sizeof(osvi), 0, 0, 0, 0, {0}, 0, 0 };
	DWORDLONG        const dwlConditionMask = VerSetConditionMask(
		VerSetConditionMask(
			VerSetConditionMask(
				0, VER_MAJORVERSION, VER_GREATER_EQUAL),
			VER_MINORVERSION, VER_GREATER_EQUAL),
		VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);

	osvi.dwMajorVersion = wMajorVersion;
	osvi.dwMinorVersion = wMinorVersion;
	osvi.wServicePackMajor = wServicePackMajor;

	return VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR, dwlConditionMask) != FALSE;
}

std::wstring OpenFolderPicker(HWND hWindow) {
	wchar_t buffer[MAX_PATH];
	memset(buffer, 0, sizeof(buffer));

	if (SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED))) {
		if (IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_VISTA), LOBYTE(_WIN32_WINNT_VISTA), 0)) {
			// use modern folder picker
			IFileOpenDialog* fileDialog;
			if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, (LPVOID*)&fileDialog))) {
				fileDialog->SetTitle(L"Select your Half-Life folder");
				fileDialog->SetOptions(FOS_PICKFOLDERS | FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST);
				if (SUCCEEDED(fileDialog->Show(hWindow))) {
					IShellItem* item;
					if (SUCCEEDED(fileDialog->GetResult(&item))) {
						PWSTR pszFilePath = NULL;
						// it seems that older versions of IShellItem don't like to use already allocated buffers
						item->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
						item->Release();
						fileDialog->Release();
						std::wstring ret(pszFilePath);
						CoTaskMemFree(pszFilePath);
						CoUninitialize();
						return ret;
					}
				}
				fileDialog->Release();
			}
		}
		else {
			// use the crusty old one
			OleInitialize(NULL);
			BROWSEINFO bi{};
			bi.lpszTitle = L"Select your Half-Life folder.";
			bi.ulFlags = BIF_USENEWUI;
			PIDLIST_ABSOLUTE pid = SHBrowseForFolder(&bi);

			if (pid) {
				SHGetPathFromIDList(pid, buffer);
				CoTaskMemFree(pid);
			}
		}
		CoUninitialize();
	}

	return std::wstring(buffer);
}

std::wstring LocateHalfLifeFromRegistry() {
	wchar_t buffer[MAX_PATH];
	memset(buffer, 0, sizeof(buffer));
	DWORD size = MAX_PATH;
	DWORD type = 0;
	HKEY key = NULL;

	RegOpenKeyEx(HKEY_CURRENT_USER, L"SOFTWARE\\Valve\\Steam", 0, KEY_READ, &key);
	RegQueryValueEx(key, L"ModInstallPath", NULL, &type, (LPBYTE)buffer, &size);
	RegCloseKey(key);

	return std::wstring(buffer);
}

BOOL FileExists(const wchar_t* path) {
	DWORD attribs = GetFileAttributes(path);
	return attribs != INVALID_FILE_ATTRIBUTES && ((attribs & FILE_ATTRIBUTE_DIRECTORY) == 0);
}

BOOL DirExists(const wchar_t* path) {
	DWORD attribs = GetFileAttributes(path);
	return attribs != INVALID_FILE_ATTRIBUTES && attribs & FILE_ATTRIBUTE_DIRECTORY;
}

int FindInArray(BYTE* arr, int length, const char* sig, int siglen) {
	for (int i = 0; i < length - siglen; i++) {
		for (int j = 0; j < siglen; j++) {
			if (arr[i + j] != sig[j]) break;
			if (j == siglen - 1) return i;
		}
	}

	return -1;
}

void MakePatch(BYTE* arr, int offset, const char* patch, int patchlen) {
	for (int i = offset, j = 0; i < offset + patchlen; i++, j++) {
		arr[i] = patch[j];
	}
}

BOOL IsLauncherPatched(std::wstring path) {
	FILE* file;
	_wfopen_s(&file, path.c_str(), L"rb");
	if (file) {
		fseek(file, 0, SEEK_END);
		long size = ftell(file);
		rewind(file);
		BYTE* data = (BYTE*)malloc(size);
		fread(data, 1, size, file);
		fclose(file);

		if (FindInArray(data, size, "hl.fix", 6) != -1 || FindInArray(data, size, "sw.fix", 6) != -1) {
			free(data);
			return TRUE;
		}
		else {
			free(data);
			return FALSE;
		}
	}

	return FALSE;
}

void SetPathText(HWND hWindow, std::wstring path, BOOL resize) {
	RECT rect{ 0, 0, 0, 0 };
	DrawText(GetDC(hWindow), path.c_str(), -1, &rect, DT_CALCRECT);

	if (resize) {
		// resize window to fit the status text and center it while we're at it
		RECT wRect{ 0, 0, 0, 0 };
		RECT dRect{ 0, 0, 0, 0 };
		RECT dRect2{ 0, 0, 0, 0 };
		GetWindowRect(hWindow, &wRect);
		SetWindowPos(hWindow, NULL, 0, 0, max(wRect.right - wRect.left, rect.right + 80), wRect.bottom, SWP_NOMOVE);
		GetWindowRect(hWindow, &wRect);
		GetWindowRect(GetDesktopWindow(), &dRect);
		CopyRect(&dRect2, &dRect);
		OffsetRect(&wRect, -wRect.left, -wRect.top);
		OffsetRect(&dRect2, -dRect2.left, -dRect2.top);
		OffsetRect(&dRect2, -wRect.right, -wRect.bottom);
		SetWindowPos(hWindow, NULL, dRect.left + (dRect2.right / 2), dRect.top + (dRect2.bottom / 2), 0, 0, SWP_NOSIZE);
	}

	SetWindowText(GetDlgItem(hWindow, IDC_HLDIR), path.c_str());
}

void SetStatusText(HWND hWindow, std::wstring statusText) {
	RECT rect{ 0, 0, 0, 0 };
	DrawText(GetDC(hWindow), statusText.c_str(), -1, &rect, DT_CALCRECT);
	SetWindowPos(GetDlgItem(hWindow, IDC_HLSTATUS), NULL, 0, 0, rect.right, rect.bottom, SWP_NOMOVE);
	SetWindowText(GetDlgItem(hWindow, IDC_HLSTATUS), statusText.c_str());
}

BOOL UpdateWindowFromPath(HWND hWindow, std::wstring path, BOOL firstUpdate) {
	SetPathText(hWindow, path, firstUpdate);
	// reset install button text
	SendMessage(GetDlgItem(hWindow, IDINSTALL), WM_SETTEXT, NULL, (LPARAM)L"Install");

	if (!DirExists(path.c_str())) {
		// no path, disable buttons and set status text
		EnableWindow(GetDlgItem(hWindow, IDUNINSTALL), FALSE);
		EnableWindow(GetDlgItem(hWindow, IDINSTALL), FALSE);
		SetStatusText(hWindow, L"Select your Half-Life folder.");
		return FALSE;
	}

	if (!FileExists((path + L"\\hl.exe").c_str())) {
		// no hl.exe, disable buttons and set status text
		EnableWindow(GetDlgItem(hWindow, IDUNINSTALL), FALSE);
		EnableWindow(GetDlgItem(hWindow, IDINSTALL), FALSE);
		SetStatusText(hWindow, L"The Half-Life launcher (hl.exe) is not in the specified path.");
		return FALSE;
	}

	if (IsLauncherPatched(path + L"\\hl.exe")) {
		// hl.exe is present and patched
		EnableWindow(GetDlgItem(hWindow, IDUNINSTALL), TRUE);
		EnableWindow(GetDlgItem(hWindow, IDINSTALL), TRUE);
		SendMessage(GetDlgItem(hWindow, IDINSTALL), WM_SETTEXT, NULL, (LPARAM)L"Reinstall");
		SetStatusText(hWindow, L"HLFixes is currently installed.");
		return TRUE;
	}

	// hl.exe is present and unpatched
	EnableWindow(GetDlgItem(hWindow, IDUNINSTALL), FALSE);
	EnableWindow(GetDlgItem(hWindow, IDINSTALL), TRUE);
	SetStatusText(hWindow, L"HLFixes is currently not installed.");
	return TRUE;
}

INT_PTR CALLBACK DialogProc(HWND hWindow, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static std::wstring path;
	static BYTE* data = NULL;
	static long size = 0;

	switch (uMsg) {
	case WM_CLOSE:
		if (data) free(data);
		EndDialog(hWindow, NULL);
		DestroyWindow(hWindow);
		return TRUE;
	case WM_SIZE: {
		// clueless win32 programmers be like
		RECT rect{ 0,0,0,0 };
		GetWindowRect(hWindow, &rect);
		SetWindowPos(GetDlgItem(hWindow, IDINSTALL), NULL, 11, rect.bottom - rect.top - 91, 0, 0, SWP_NOSIZE);
		SetWindowPos(GetDlgItem(hWindow, IDUNINSTALL), NULL, rect.right - rect.left - 162, rect.bottom - rect.top - 91, 0, 0, SWP_NOSIZE);
		SetWindowPos(GetDlgItem(hWindow, IDC_HLDIR), NULL, 0, 0, rect.right - rect.left - 207, 21, SWP_NOMOVE);
		SetWindowPos(GetDlgItem(hWindow, IDC_BROWSE), NULL, rect.right - rect.left - 102, 11, 0, 0, SWP_NOSIZE);
		return TRUE;
	}
	case WM_GETMINMAXINFO: {
		MINMAXINFO* mmInfo = (MINMAXINFO*)lParam;
		mmInfo->ptMinTrackSize.x = 350;
		mmInfo->ptMinTrackSize.y = 150;
		break;
	}
	case WM_INITDIALOG: {
		// i can't quite get the positioning right in the dialog editor so i'm doing this instead :sob:
		SetWindowPos(GetDlgItem(hWindow, IDC_HLDIR), NULL, 100, 12, 0, 0, SWP_NOSIZE);
		// check if HLFixes.dll exists
		if (!FileExists(L"HLFixes.dll")) {
			MessageBox(hWindow, L"The installer was unable to locate HLFixes.dll.", HLTITLE, MB_OK | MB_ICONERROR);
			_exit(-1);
			return FALSE;
		}

		PostMessage(hWindow, WM_NEXTDLGCTL, 0, FALSE);

		// get HL path from registry
		path = LocateHalfLifeFromRegistry();

		if (UpdateWindowFromPath(hWindow, path, TRUE)) {
			// load hl.exe into memory
			FILE* file;
			_wfopen_s(&file, (path + L"\\hl.exe").c_str(), L"rb");

			if (!file) {
				MessageBox(hWindow, L"The installer was unable to read the Half-Life launcher.\nTry restarting the installer as an Administrator.", HLTITLE, MB_OK | MB_ICONERROR);
				return TRUE;
			}

			fseek(file, 0, SEEK_END);
			size = ftell(file);
			rewind(file);
			data = (BYTE*)malloc(size);
			fread(data, 1, size, file);
			fclose(file);
		}

		return TRUE;
	}
	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED) {
			switch (LOWORD(wParam)) {
			case IDINSTALL: {
				// drop the binaries
				if (!CopyFile(L"HLFixes.dll", (path + L"\\hl.fix").c_str(), FALSE) || !CopyFile(L"HLFixes.dll", (path + L"\\sw.fix").c_str(), FALSE)) {
					MessageBox(hWindow, HLCANTWRITE, HLTITLE, MB_OK | MB_ICONERROR);
					return TRUE;
				}

				// find "hw.dll" in the launcher
				int hwLoc = FindInArray(data, size, "hw.dll", 6);

				// patch it to "hl.fix"
				if (hwLoc != -1) {
					MakePatch(data, hwLoc, "hl.fix", 6);
				}

				// find "sw.dll" in the launcher
				int swLoc = FindInArray(data, size, "sw.dll", 6);

				// patch it to "sw.fix"
				if (swLoc != -1) {
					MakePatch(data, swLoc, "sw.fix", 6);
				}

				// validate the patch
				if (FindInArray(data, size, "hl.fix", 6) == -1 || FindInArray(data, size, "sw.fix", 6) == -1) {
					MessageBox(hWindow, L"The installer was unable to patch the Half-Life launcher.\nPlease verify integrity in Steam and try again.", HLTITLE, MB_OK | MB_ICONERROR);
					return TRUE;
				}

				// make a backup if one doesn't already exist
				if (!FileExists((path + L"\\hl.exe.bak").c_str())) {
					if (!CopyFile((path + L"\\hl.exe").c_str(), (path + L"\\hl.exe.bak").c_str(), FALSE)) {
						MessageBox(hWindow, HLCANTWRITE, HLTITLE, MB_OK | MB_ICONERROR);
						return TRUE;
					}
				} else {
					// also make a backup if the version of the current launcher is different than the backup
					FILE* file;
					_wfopen_s(&file, (path + L"\\hl.exe.bak").c_str(), L"rb");

					if (!file) {
						MessageBox(hWindow, L"The installer was unable to read the backup of the Half-Life launcher.\nTry restarting the installer as an Administrator.", HLTITLE, MB_OK | MB_ICONERROR);
						return TRUE;
					}

					fseek(file, 0, SEEK_END);
					long backupSize = ftell(file);
					rewind(file);
					BYTE* backupData = (BYTE*)malloc(backupSize);
					fread(backupData, 1, backupSize, file);
					fclose(file);

					auto newDos = (IMAGE_DOS_HEADER*)data;
					auto newNt = (IMAGE_NT_HEADERS*)((uint8_t*)newDos + newDos->e_lfanew);
					auto oldDos = (IMAGE_DOS_HEADER*)backupData;
					auto oldNt = (IMAGE_NT_HEADERS*)((uint8_t*)oldDos + oldDos->e_lfanew);

					if (oldNt->FileHeader.TimeDateStamp != newNt->FileHeader.TimeDateStamp) {
						if (!CopyFile((path + L"\\hl.exe").c_str(), (path + L"\\hl.exe.bak").c_str(), FALSE)) {
							MessageBox(hWindow, HLCANTWRITE, HLTITLE, MB_OK | MB_ICONERROR);
							return TRUE;
						}
					}
				}

				// write to disk
				FILE* file;
				errno_t err = _wfopen_s(&file, (path + L"\\hl.exe").c_str(), L"wb");

				if (!file) {
					MessageBox(hWindow, HLCANTWRITE, HLTITLE, MB_OK | MB_ICONERROR);
					return TRUE;
				}

				fwrite(data, 1, size, file);
				fclose(file);

				// update buttons
				UpdateWindowFromPath(hWindow, path, FALSE);

				MessageBox(hWindow, L"Successfully installed.", HLTITLE, MB_OK | MB_ICONINFORMATION);
				return TRUE;
			}
			case IDUNINSTALL: {
				// check to see if there's a backup
				if (!FileExists((path + L"\\hl.exe.bak").c_str())) {
					MessageBox(hWindow, L"The installer was unable to locate the backup of the Half-Life launcher.\nPlease verify integrity in Steam to uninstall.", HLTITLE, MB_OK | MB_ICONERROR);
					return TRUE;
				}

				// restore the backup
				if (!CopyFile((path + L"\\hl.exe.bak").c_str(), (path + L"\\hl.exe").c_str(), FALSE)) {
					MessageBox(hWindow, HLCANTWRITE, HLTITLE, MB_OK | MB_ICONERROR);
					return TRUE;
				}

				// clean up files we dropped
				// we don't care if these fail as long the launcher backup was restored
				DeleteFile((path + L"\\hl.exe.bak").c_str());
				DeleteFile((path + L"\\hl.fix").c_str());
				DeleteFile((path + L"\\sw.fix").c_str());

				// update buttons
				UpdateWindowFromPath(hWindow, path, FALSE);

				MessageBox(hWindow, L"Successfully uninstalled.", HLTITLE, MB_OK | MB_ICONINFORMATION);
				return TRUE;
			}
			case IDC_BROWSE: {
				std::wstring newPath = OpenFolderPicker(hWindow);

				if (newPath.empty()) return TRUE;
				else path = newPath;

				if (UpdateWindowFromPath(hWindow, path, FALSE)) {
					// load hl.exe into memory
					FILE* file;
					_wfopen_s(&file, (path + L"\\hl.exe").c_str(), L"rb");

					if (!file) {
						MessageBox(hWindow, L"The installer was unable to read the Half-Life launcher.\nTry restarting the installer as an Administrator.", HLTITLE, MB_OK | MB_ICONERROR);
						return TRUE;
					}

					fseek(file, 0, SEEK_END);
					long newSize = ftell(file);
					rewind(file);
					if (size == 0) {
						data = (BYTE*)malloc(newSize);
					}
					else {
						data = (BYTE*)realloc(data, newSize);
					}
					size = newSize;
					fread(data, 1, size, file);
					fclose(file);
				}

				return TRUE;
			}
			}
		}
		return FALSE;
	}

	return FALSE;
}

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow) {
	InitCommonControls();
	DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG1), GetDesktopWindow(), DialogProc);
}