#pragma once

// Cross-platform compatibility shims for macOS / Linux
// Include this header after any platform-specific headers

#ifdef __APPLE__
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <ifaddrs.h>
#include <dispatch/dispatch.h>
#include <mach-o/dyld.h>
#include <CommonCrypto/CommonDigest.h>
#include <fnmatch.h>
#include <strings.h> // strcasecmp

// ---- Windows type shims ----
#ifndef MAX_PATH
#define MAX_PATH 1024
#endif

#ifndef SOCKET
using SOCKET = int;
#endif
#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif
#define closesocket close
#define SD_BOTH SHUT_RDWR

#ifndef WSAEWOULDBLOCK
#define WSAEWOULDBLOCK EWOULDBLOCK
#endif
#ifndef WSAETIMEDOUT
#define WSAETIMEDOUT ETIMEDOUT
#endif
#ifndef WSAGetLastError
inline int WSAGetLastError() { return errno; }
#endif

// Minimal stubs for Windows types used in guarded code
using DWORD = uint32_t;
using HANDLE = void*;
using HCRYPTPROV = uintptr_t;
using HCRYPTHASH = uintptr_t;
using BYTE = unsigned char;
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

// ---- localtime_s compat ----
inline int localtime_s(struct tm* out, const time_t* t) {
    struct tm* r = localtime_r(t, out);
    return r ? 0 : -1;
}

// ---- _stricmp -> strcasecmp ----
#ifndef _stricmp
#define _stricmp strcasecmp
#endif
#ifndef _wcsicmp
#define _wcsicmp wcscasecmp
#endif

// ---- PathMatchSpecW shim using fnmatch on UTF-8 ----
inline bool PathMatchSpecW_compat(const std::wstring& name, const std::wstring& filter) {
    // Convert wide to utf8 for fnmatch (approximation, mac paths are utf8)
    auto w2u = [](const std::wstring& w) -> std::string {
        std::string s;
        s.reserve(w.size());
        for (wchar_t c : w) {
            if (c < 0x80) s.push_back((char)c);
            else if (c < 0x800) { s.push_back((char)(0xC0 | (c >> 6))); s.push_back((char)(0x80 | (c & 0x3F))); }
            else { s.push_back((char)(0xE0 | (c >> 12))); s.push_back((char)(0x80 | ((c >> 6) & 0x3F))); s.push_back((char)(0x80 | (c & 0x3F))); }
        }
        return s;
    };
    std::string n = w2u(name);
    std::string f = w2u(filter);
    return fnmatch(f.c_str(), n.c_str(), FNM_CASEFOLD) == 0;
}
#define PathMatchSpecW(a,b) (PathMatchSpecW_compat(a,b) ? TRUE : FALSE)

// ---- Config / data directories on macOS ----
inline std::string macGetConfigDir() {
    const char* home = getenv("HOME");
    if (!home) home = "/tmp";
    std::string dir = std::string(home) + "/Library/Application Support/FastEnough";
    // Ensure exists
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return dir;
}

inline std::string macGetTempDir() {
    const char* tmp = getenv("TMPDIR");
    if (tmp && tmp[0]) return std::string(tmp);
    return "/tmp";
}

inline std::string macGetExeDir() {
    char path[4096] = {};
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0) {
        std::string s(path);
        auto slash = s.find_last_of('/');
        if (slash != std::string::npos) return s.substr(0, slash);
        return s;
    }
    return ".";
}

inline std::string macGetExePath() {
    char path[4096] = {};
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0) return std::string(path);
    return "";
}

// ---- SHA256 via CommonCrypto ----
inline bool macComputeSha256(const std::string& path, std::string& outHex, std::string& detail) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) { detail = "Could not open local file"; return false; }
    CC_SHA256_CTX ctx;
    CC_SHA256_Init(&ctx);
    const size_t bufSize = 4*1024*1024;
    std::vector<char> buf(bufSize);
    size_t n;
    while ((n = fread(buf.data(), 1, bufSize, f)) > 0) {
        CC_SHA256_Update(&ctx, buf.data(), (CC_LONG)n);
    }
    if (ferror(f)) { fclose(f); detail = "Read error"; return false; }
    fclose(f);
    unsigned char digest[CC_SHA256_DIGEST_LENGTH];
    CC_SHA256_Final(digest, &ctx);
    static const char* hex = "0123456789abcdef";
    outHex.clear();
    outHex.reserve(64);
    for (int i=0;i<32;i++) { outHex.push_back(hex[digest[i]>>4]); outHex.push_back(hex[digest[i]&0x0F]); }
    return true;
}

// ---- Socket helpers ----
inline bool macSetNonBlocking(SOCKET s, bool nonBlocking) {
    int flags = fcntl(s, F_GETFL, 0);
    if (flags < 0) return false;
    if (nonBlocking) flags |= O_NONBLOCK;
    else flags &= ~O_NONBLOCK;
    return fcntl(s, F_SETFL, flags) == 0;
}

#endif // __APPLE__

#ifdef _WIN32
// On Windows, keep original behavior - no shims needed
#endif
