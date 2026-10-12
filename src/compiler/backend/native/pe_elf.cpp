#include "native/image.hpp"
#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace nexus::native {
namespace {

constexpr std::uint32_t kFileAlign = 0x200;
constexpr std::uint32_t kSectionAlign = 0x1000;
constexpr std::uint64_t kImageBase = 0x140000000ull;
constexpr std::uint32_t kTextRva = 0x1000;

std::uint32_t alignUp(std::uint32_t v, std::uint32_t a) {
    return (v + a - 1) / a * a;
}

struct Writer {
    std::vector<std::uint8_t> out;
    void b(std::uint8_t v) { out.push_back(v); }
    void w16(std::uint16_t v) {
        out.push_back(static_cast<std::uint8_t>(v & 0xFF));
        out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
    }
    void w32(std::uint32_t v) {
        for (int i = 0; i < 4; ++i)
            out.push_back(static_cast<std::uint8_t>((v >> (8 * i)) & 0xFF));
    }
    void w64(std::uint64_t v) {
        for (int i = 0; i < 8; ++i)
            out.push_back(static_cast<std::uint8_t>((v >> (8 * i)) & 0xFF));
    }
    void bytes(const std::uint8_t* p, std::size_t n) {
        out.insert(out.end(), p, p + n);
    }
    void text(const char* s) {
        while (*s) out.push_back(static_cast<std::uint8_t>(*s++));
    }
    void pad(std::size_t n) { out.insert(out.end(), n, 0); }
    void patch32(std::size_t at, std::uint32_t v) {
        for (int i = 0; i < 4; ++i)
            out[at + static_cast<std::size_t>(i)] = static_cast<std::uint8_t>((v >> (8 * i)) & 0xFF);
    }
};

} // namespace

std::vector<std::uint8_t> writeWindowsPE(const CodeImage& image) {
    if (!image.labels.contains("_start"))
        throw std::runtime_error("builtin native v0: missing _start label");
    if (image.imports.empty())
        throw std::runtime_error("builtin native v0: Windows image needs kernel32 imports");

    const std::uint32_t codeSize = static_cast<std::uint32_t>(image.bytes.size());
    const std::uint32_t textFileSize = alignUp(codeSize, kFileAlign);
    const std::uint32_t rdataRva = kTextRva + alignUp(codeSize, kSectionAlign);

    // --- lay out .rdata: strings, then import tables ---
    std::map<std::string, std::uint32_t> dataAddr; // symbol -> RVA
    std::vector<std::uint8_t> rdata;
    for (std::size_t i = 0; i < image.strings.size(); ++i) {
        dataAddr["str" + std::to_string(i)] = rdataRva + static_cast<std::uint32_t>(rdata.size());
        const std::string& s = image.strings[i];
        rdata.insert(rdata.end(), s.begin(), s.end());
        rdata.push_back(0); // NUL terminator: runtime strings are C strings
    }
    while (rdata.size() % 8 != 0) rdata.push_back(0);

    const std::size_t nImp = image.imports.size();
    const std::uint32_t idtOff = static_cast<std::uint32_t>(rdata.size());
    const std::uint32_t idtSize = 40; // 2 descriptors
    const std::uint32_t iltOff = idtOff + idtSize;
    const std::uint32_t tableSize = static_cast<std::uint32_t>((nImp + 1) * 8);
    const std::uint32_t iatOff = iltOff + tableSize;
    std::uint32_t namesOff = iatOff + tableSize;
    std::vector<std::uint32_t> hintNameRva;
    for (const auto& name : image.imports) {
        hintNameRva.push_back(rdataRva + namesOff);
        namesOff += static_cast<std::uint32_t>(2 + name.size() + 1);
    }
    const std::uint32_t dllNameOff = namesOff;
    const char kDll[] = "kernel32.dll";
    namesOff += static_cast<std::uint32_t>(sizeof(kDll)); // includes NUL
    rdata.resize(namesOff, 0);

    auto w32at = [&](std::size_t at, std::uint32_t v) {
        for (int i = 0; i < 4; ++i)
            rdata[at + static_cast<std::size_t>(i)] = static_cast<std::uint8_t>((v >> (8 * i)) & 0xFF);
    };
    auto w64at = [&](std::size_t at, std::uint64_t v) {
        for (int i = 0; i < 8; ++i)
            rdata[at + static_cast<std::size_t>(i)] = static_cast<std::uint8_t>((v >> (8 * i)) & 0xFF);
    };
    // Import directory table: one entry + terminator.
    w32at(idtOff + 0, rdataRva + iltOff); // OriginalFirstThunk
    w32at(idtOff + 12, rdataRva + dllNameOff); // Name
    w32at(idtOff + 16, rdataRva + iatOff); // FirstThunk
    for (std::size_t i = 0; i < nImp; ++i) {
        w64at(iltOff + i * 8, hintNameRva[i]);
        w64at(iatOff + i * 8, hintNameRva[i]); // loader overwrites with addresses
        dataAddr["__iat_" + image.imports[i]] = rdataRva + iatOff + static_cast<std::uint32_t>(i * 8);
    }
    std::size_t hn = idtOff + idtSize + tableSize * 2;
    for (const auto& name : image.imports) {
        rdata[hn] = 0;
        rdata[hn + 1] = 0;
        for (std::size_t i = 0; i < name.size(); ++i)
            rdata[hn + 2 + i] = static_cast<std::uint8_t>(name[i]);
        rdata[hn + 2 + name.size()] = 0;
        hn += 2 + name.size() + 1;
    }
    for (std::size_t i = 0; kDll[i]; ++i)
        rdata[dllNameOff + i] = static_cast<std::uint8_t>(kDll[i]);

    const std::uint32_t rdataFileSize = alignUp(static_cast<std::uint32_t>(rdata.size()), kFileAlign);
    // Writable arena TOC: __nx_arena[0] = base, __nx_arena[1] = pos.
    const std::uint32_t dataRva = rdataRva + alignUp(static_cast<std::uint32_t>(rdata.size()), kSectionAlign);
    const std::uint32_t dataFileSize = kFileAlign;
    dataAddr["__nx_arena"] = dataRva;
    const std::uint32_t textFileOff = alignUp(64 + 4 + 20 + 240 + 3 * 40, kFileAlign);
    const std::uint32_t rdataFileOff = textFileOff + textFileSize;
    const std::uint32_t dataFileOff = rdataFileOff + rdataFileSize;
    const std::uint32_t sizeOfImage = dataRva + kSectionAlign;
    const std::uint32_t entryRva = kTextRva + static_cast<std::uint32_t>(image.labels.at("_start"));

    // --- patch code fixups ---
    std::vector<std::uint8_t> code = image.bytes;
    auto symAddr = [&](const std::string& sym) -> std::uint32_t {
        const auto cl = image.labels.find(sym);
        if (cl != image.labels.end())
            return kTextRva + static_cast<std::uint32_t>(cl->second);
        const auto da = dataAddr.find(sym);
        if (da != dataAddr.end()) return da->second;
        throw std::runtime_error("builtin native v0: unknown symbol '" + sym + "'");
    };
    for (const auto& fx : image.fixups) {
        const std::uint32_t target = symAddr(fx.symbol);
        const std::uint64_t here = static_cast<std::uint64_t>(kTextRva + fx.at + 4);
        const std::int64_t value = static_cast<std::int64_t>(target) -
            static_cast<std::int64_t>(here) + fx.addend;
        if (value < -0x80000000ll || value > 0x7FFFFFFFll)
            throw std::runtime_error("builtin native v0: fixup out of range for '" + fx.symbol + "'");
        const std::uint32_t v = static_cast<std::uint32_t>(static_cast<std::int32_t>(value));
        for (int i = 0; i < 4; ++i)
            code[fx.at + static_cast<std::size_t>(i)] = static_cast<std::uint8_t>((v >> (8 * i)) & 0xFF);
    }

    // --- headers ---
    Writer w;
    // DOS header (64 bytes)
    w.w16(0x5A4D); // MZ
    for (int i = 0; i < 29; ++i) w.w16(0);
    w.w32(64); // e_lfanew: PE header right after the DOS header
    // PE signature + COFF header
    w.w32(0x00004550); // PE\0\0
    w.w16(0x8664); // AMD64
    w.w16(3); // sections
    w.w32(0); // TimeDateStamp: fixed for reproducible builds
    w.w32(0); // PointerToSymbolTable
    w.w32(0); // NumberOfSymbols
    w.w16(240); // SizeOfOptionalHeader
    w.w16(0x0022); // EXECUTABLE_IMAGE | LARGE_ADDRESS_AWARE
    // Optional header (PE32+, 240 bytes)
    w.w16(0x20B); // magic
    w.b(0); w.b(0); // linker version
    w.w32(textFileSize); // SizeOfCode
    w.w32(rdataFileSize + dataFileSize); // SizeOfInitializedData
    w.w32(0); // SizeOfUninitializedData
    w.w32(entryRva); // AddressOfEntryPoint
    w.w32(kTextRva); // BaseOfCode
    w.w64(kImageBase);
    w.w32(kSectionAlign);
    w.w32(kFileAlign);
    w.w16(6); w.w16(0); // OS version
    w.w16(0); w.w16(0); // image version
    w.w16(6); w.w16(0); // subsystem version
    w.w32(0); // Win32VersionValue
    w.w32(sizeOfImage);
    w.w32(textFileOff); // SizeOfHeaders
    w.w32(0); // CheckSum
    w.w16(3); // CONSOLE subsystem
    w.w16(0x0100); // NX_COMPAT (no ASLR: image has no relocations)
    w.w64(0x100000); // SizeOfStackReserve
    w.w64(0x1000); // SizeOfStackCommit
    w.w64(0x100000); // SizeOfHeapReserve
    w.w64(0x1000); // SizeOfHeapCommit
    w.w32(0); // LoaderFlags
    w.w32(16); // NumberOfRvaAndSizes
    w.w32(0); w.w32(0); // export
    w.w32(rdataRva + idtOff); w.w32(idtSize); // import
    for (int i = 2; i < 16; ++i) {
        w.w32(0); w.w32(0);
    }
    // Section headers
    w.text(".text");
    w.pad(8 - 5);
    w.w32(alignUp(codeSize, kSectionAlign)); // VirtualSize
    w.w32(kTextRva); // VirtualAddress
    w.w32(textFileSize); // SizeOfRawData
    w.w32(textFileOff); // PointerToRawData
    w.w32(0); // PointerToRelocations
    w.w32(0); // PointerToLinenumbers
    w.w16(0); // NumberOfRelocations
    w.w16(0); // NumberOfLinenumbers
    w.w32(0x60000020); // CODE | EXECUTE | READ
    w.text(".rdata");
    w.pad(8 - 6);
    w.w32(static_cast<std::uint32_t>(rdata.size())); // VirtualSize
    w.w32(rdataRva); // VirtualAddress
    w.w32(rdataFileSize); // SizeOfRawData
    w.w32(rdataFileOff); // PointerToRawData
    w.w32(0); // PointerToRelocations
    w.w32(0); // PointerToLinenumbers
    w.w16(0); // NumberOfRelocations
    w.w16(0); // NumberOfLinenumbers
    w.w32(0x40000040); // INITIALIZED_DATA | READ
    w.text(".data");
    w.pad(8 - 5);
    w.w32(16); // VirtualSize: __nx_arena base + pos
    w.w32(dataRva); // VirtualAddress
    w.w32(dataFileSize); // SizeOfRawData
    w.w32(dataFileOff); // PointerToRawData
    w.w32(0); // PointerToRelocations
    w.w32(0); // PointerToLinenumbers
    w.w16(0); // NumberOfRelocations
    w.w16(0); // NumberOfLinenumbers
    w.w32(0xC0000040); // INITIALIZED_DATA | READ | WRITE
    if (w.out.size() > textFileOff)
        throw std::runtime_error("builtin native v0: PE header size mismatch");
    w.pad(textFileOff - w.out.size());
    w.bytes(code.data(), code.size());
    w.pad(textFileSize - codeSize);
    w.bytes(rdata.data(), rdata.size());
    w.pad(rdataFileSize - rdata.size());
    w.pad(dataFileSize); // zeroed arena TOC
    return w.out;
}

std::vector<std::uint8_t> writeLinuxELF(const CodeImage& image) {
    if (!image.labels.contains("_start"))
        throw std::runtime_error("builtin native v0: missing _start label");
    if (!image.imports.empty())
        throw std::runtime_error("builtin native v0: ELF image takes no imports");

    constexpr std::uint64_t kBase = 0x400000;
    constexpr std::uint64_t kHdrSize = 64 + 3 * 56; // ehdr + 3 phdrs
    constexpr std::uint64_t kPageSize = 0x1000;
    const auto alignPage = [](std::uint64_t value) {
        return (value + kPageSize - 1) & ~(kPageSize - 1);
    };

    // ELF loaders apply permissions by memory page. Keep code, read-only
    // literals, and writable arena metadata on separate pages, otherwise
    // short PT_LOAD segments can make _start non-executable or the arena
    // metadata read-only.
    const std::uint64_t codeOff = kPageSize;
    const std::uint64_t codeAddr = kBase + codeOff;
    const std::uint64_t dataOff = alignPage(codeOff + image.bytes.size());
    const std::uint64_t dataAddr = kBase + dataOff;

    std::map<std::string, std::uint64_t> sym;
    for (const auto& [name, off] : image.labels)
        sym[name] = codeAddr + off;
    std::uint64_t strOff = dataOff;
    std::vector<std::uint8_t> data;
    for (std::size_t i = 0; i < image.strings.size(); ++i) {
        sym["str" + std::to_string(i)] = kBase + strOff;
        const std::string& s = image.strings[i];
        data.insert(data.end(), s.begin(), s.end());
        data.push_back(0); // NUL terminator: runtime strings are C strings
        strOff += s.size() + 1;
    }
    const std::uint64_t writableOff = alignPage(dataOff + data.size());
    const std::uint64_t writableAddr = kBase + writableOff;
    sym["__nx_arena"] = writableAddr;

    std::vector<std::uint8_t> code = image.bytes;
    for (const auto& fx : image.fixups) {
        const auto it = sym.find(fx.symbol);
        if (it == sym.end())
            throw std::runtime_error("builtin native v0: unknown symbol '" + fx.symbol + "'");
        const std::int64_t here = static_cast<std::int64_t>(codeAddr + fx.at + 4);
        const std::int64_t value = static_cast<std::int64_t>(it->second) - here + fx.addend;
        if (value < -0x80000000ll || value > 0x7FFFFFFFll)
            throw std::runtime_error("builtin native v0: fixup out of range for '" + fx.symbol + "'");
        const std::uint32_t v = static_cast<std::uint32_t>(static_cast<std::int32_t>(value));
        for (int i = 0; i < 4; ++i)
            code[fx.at + static_cast<std::size_t>(i)] = static_cast<std::uint8_t>((v >> (8 * i)) & 0xFF);
    }

    Writer w;
    // ELF header
    w.b(0x7F); w.text("ELF");
    w.b(2); // 64-bit
    w.b(1); // little-endian
    w.b(1); // version
    w.b(0); // System V ABI
    w.pad(8);
    w.w16(2); // EXEC
    w.w16(62); // x86-64
    w.w32(1); // version
    w.w64(codeAddr + image.labels.at("_start")); // entry
    w.w64(64); // phoff
    w.w64(0); // shoff
    w.w32(0); // flags
    w.w16(64); // ehsize
    w.w16(56); // phentsize
    w.w16(3); // phnum
    w.w16(0); w.w16(0); w.w16(0); // shentsize/shnum/shstrndx
    // PHDR 0: code (R+E)
    w.w32(1); // PT_LOAD
    w.w32(5); // R+E
    w.w64(codeOff);
    w.w64(codeAddr);
    w.w64(codeAddr);
    w.w64(code.size());
    w.w64(code.size());
    w.w64(0x1000);
    // PHDR 1: rodata (R)
    w.w32(1); // PT_LOAD
    w.w32(4); // R
    w.w64(dataOff);
    w.w64(dataAddr);
    w.w64(dataAddr);
    w.w64(data.size());
    w.w64(data.size());
    w.w64(0x1000);
    // PHDR 2: arena TOC (RW)
    w.w32(1); // PT_LOAD
    w.w32(6); // R+W
    w.w64(writableOff);
    w.w64(writableAddr);
    w.w64(writableAddr);
    w.w64(16);
    w.w64(16);
    w.w64(0x1000);
    if (w.out.size() != kHdrSize)
        throw std::runtime_error("builtin native v0: ELF header size mismatch");
    w.pad(static_cast<std::size_t>(codeOff - w.out.size()));
    w.bytes(code.data(), code.size());
    w.pad(static_cast<std::size_t>(dataOff - w.out.size()));
    w.bytes(data.data(), data.size());
    w.pad(static_cast<std::size_t>(writableOff - w.out.size()));
    w.pad(16); // zeroed __nx_arena
    return w.out;
}

} // namespace nexus::native
