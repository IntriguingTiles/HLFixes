#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <gelf.h>

#include <unordered_map>
#include <string_view>
#include <string>

#include "types.h"

void readSymbols(const char *lib, std::unordered_map<std::string, u32> &symbols)
{
    elf_version(EV_CURRENT);
    int fd = open(lib, O_RDONLY);
    Elf *elf = elf_begin(fd, ELF_C_READ, nullptr);
    Elf_Scn *scn = nullptr;
    GElf_Shdr shdr;

    while ((scn = elf_nextscn(elf, scn)) != nullptr)
    {
        gelf_getshdr(scn, &shdr);

        if (shdr.sh_type == SHT_SYMTAB)
            break;
    }

    Elf_Data *data = elf_getdata(scn, nullptr);
    int count = shdr.sh_size / shdr.sh_entsize;

    for (int i = 0; i < count; i++)
    {
        GElf_Sym sym;
        gelf_getsym(data, i, &sym);
        symbols.insert({elf_strptr(elf, shdr.sh_link, sym.st_name), sym.st_value});
    }

    elf_end(elf);
    close(fd);
}

// this sucks but it works for now
bool getModuleAddress(const char *lib, u32* startOfModule, u32* endOfModule)
{
    FILE *maps = fopen("/proc/self/maps", "r");
    char line[PATH_MAX];
    u32 start;
    u32 end;
    char permissions[5];
    int offset;
    char dev[12];
    int inode;
    char path[PATH_MAX];

    while (fgets(line, PATH_MAX, maps))
    {
        // clear the path in case there isn't one on this line
        memset(path, 0, PATH_MAX);
        int ret = sscanf(line, "%x-%x %s %x %s %d %s", &start, &end, permissions, &offset, dev, &inode, path);
        if (strstr(path, lib))
        {
            if (permissions[2] = 'x')
            {
                fclose(maps);
                if (startOfModule) *startOfModule = start;
                if (endOfModule) *endOfModule = end;
                return true;
            }
        }
    }

    fclose(maps);
    return false;
}

bool checkArg(const char *arg, int argc, char **argv)
{
    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], arg) == 0)
            return true;
    }

    return false;
}

u32 RelativeToAbsolute(u32 relAddr, u32 nextInstructionAddr)
{
    return relAddr + nextInstructionAddr;
}

u32 AbsoluteToRelative(u32 absAddr, u32 nextInstructionAddr)
{
    return absAddr - nextInstructionAddr;
}

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
    u32 start;
    u32 end;
	getModuleAddress(dll, &start, &end);

	return FindSig(start, end, (u8*)sig.data(), sig.length());
}

void MakePatch(u32 addr, u8 patch[], u32 patchlen)
{
    int pageSize = sysconf(_SC_PAGESIZE);
    int pageDiff = addr % pageSize;
    mprotect((void *)(addr - pageDiff), patchlen + pageDiff, PROT_READ | PROT_WRITE);

    for (u32 i = 0; i < patchlen; i++)
    {
        u8 *byte = (u8 *)((u32)addr + i);
        *byte = patch[i];
    }

    mprotect((void *)(addr - pageDiff), patchlen + pageDiff, PROT_READ | PROT_EXEC);
}

void MakePatch(const char* dll, std::string_view sig, std::string_view patch) {
	u32 addr = FindSig(dll, sig);

	if (addr != 0) MakePatch(addr, (u8*)patch.data(), patch.length());
}