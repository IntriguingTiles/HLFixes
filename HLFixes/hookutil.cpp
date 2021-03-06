#include <string_view>
#include "types.h"
#include "hookutil.h"

#define WIN32_LEAN_AND_MEAN
#include <MinHook.h>

u32 FindSig(u32 start, u32 end, u8 sig[], u32 siglen) {
	for (u32 cur = start; cur < end; cur++) {
		for (u32 i = 0; i < siglen; i++) {
			u8* b = (u8*)(cur + i);
			if (*b != sig[i] && sig[i] != '*') break;
			if (i == siglen - 1) return cur;
		}
	}

	return 0;
}

u32 FindSig(const char* dll, std::string_view sig) {
	u32 base = (u32)GetModuleHandle(dll);

	auto dos = (IMAGE_DOS_HEADER*)base;
	auto nt = (IMAGE_NT_HEADERS*)((u8*)dos + dos->e_lfanew);

	return FindSig(base, base + nt->OptionalHeader.SizeOfImage, (u8*)sig.data(), sig.length());
}

void MakeHook(void* addr, void* func) {
	MH_CreateHook(addr, func, nullptr);
	MH_EnableHook(addr);
}

void MakeHook(void* addr, void* func, void** origFunc) {
	auto status = MH_CreateHook(addr, func, origFunc);
	if (status != MH_OK) printf("error: %d\n", status);
	MH_EnableHook(addr);
}

void MakeHook(const char* dll, std::string_view sig, void* func) {
	u32 addr = FindSig(dll, sig);

	if (addr != 0) MakeHook((void*)addr, func);
}

void MakeHook(const char* dll, std::string_view sig, void* func, void** origFunc) {
	u32 addr = FindSig(dll, sig);

	if (addr != 0) MakeHook((void*)addr, func, origFunc);
}

void MakeHook(void* vtable, u32 index, void* func) {
	DWORD oldProtect;
	void** vt = *(void***)vtable;
	VirtualProtect(&vt[index], sizeof(u32), PAGE_EXECUTE_READWRITE, &oldProtect);
	vt[index] = func;
	VirtualProtect(&vt[index], sizeof(u32), oldProtect, &oldProtect);
}

void MakePatch(void* addr, u8 patch[], u32 patchlen) {
	DWORD oldProtect;
	VirtualProtect(addr, patchlen, PAGE_EXECUTE_READWRITE, &oldProtect);

	for (u32 i = 0; i < patchlen; i++) {
		u8* byte = (u8*)((u32)addr + i);
		*byte = patch[i];
	}

	VirtualProtect(addr, patchlen, oldProtect, &oldProtect);
}

void MakePatch(void* addr, std::string_view patch) {
	MakePatch(addr, (u8*)patch.data(), patch.length());
}