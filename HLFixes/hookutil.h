#pragma once

u32 FindSig(u32 start, u32 end, u8 sig[], u32 siglen);
u32 FindSig(const char* dll, std::string_view sig);
void MakeHook(void* addr, void* func);
void MakeHook(void* addr, void* func, void** origFunc);
void MakeHook(const char* dll, std::string_view sig, void* func);
void MakeHook(const char* dll, std::string_view sig, void* func, void** origFunc);
void MakeHook(void* vtable, u32 index, void* func);
void MakePatch(void* addr, u8 patch[], u32 patchlen);
void MakePatch(void* addr, std::string_view patch);
void MakePatch(const char* dll, std::string_view sig, std::string_view patch);
u32 RelativeToAbsolute(u32 relAddr, u32 nextInstructionAddr);
u32 AbsoluteToRelative(u32 absAddr, u32 nextInstructionAddr);