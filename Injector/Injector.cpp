#include <cstdio>
#include <Windows.h>
#include <TlHelp32.h>
#include <conio.h>

int main() {
	printf("DO NOT USE THIS IN MULTIPLAYER!!!!!!!!\n");

	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);
	auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	bool found = false;

	if (Process32First(snapshot, &entry)) {
		while (Process32Next(snapshot, &entry)) {
			if (_stricmp(entry.szExeFile, "hl.exe") == 0) {
				found = true;
				printf("Injecting into hl.exe...\n");
				auto process = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, false, entry.th32ProcessID);
				char dllPath[MAX_PATH];

				GetCurrentDirectory(MAX_PATH, dllPath);

				snprintf(dllPath, MAX_PATH, "%s\\HLFixes.dll", dllPath);

				auto attrib = GetFileAttributes(dllPath);

				if (attrib == INVALID_FILE_ATTRIBUTES) {
					printf("Couldn't find HLFixes.dll! Make sure it's next to the injector.\n");
					CloseHandle(process);
					break;
				}

				auto mem = VirtualAllocEx(process, nullptr, strlen(dllPath), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

				if (!mem) {
					printf("Couldn't allocate memory in hl.exe!\n");
					CloseHandle(process);
					break;
				}

				WriteProcessMemory(process, mem, dllPath, strlen(dllPath), nullptr);

				auto func = GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");

				CreateRemoteThread(process, nullptr, 0, (LPTHREAD_START_ROUTINE)func, mem, 0, nullptr);

				CloseHandle(process);

				printf("Done!\n");
				break;
			}
		}
	}

	if (!found) printf("hl.exe doesn't seem to be running\n");

	printf("Press any key to exit...");
	_getch();

	CloseHandle(snapshot);
}
