// ============================================================
//  winshim.h —— Win32 兼容层（只在 macOS 编译时生效，HSW_MAC）
//
//  目的：让《高中的一周》这份 9500 行的 Windows 源码几乎不改就能在 Mac 上编译。
//  这里只放「类型 + 常量 + 函数声明」，实现全部在 shim_mac.mm（Cocoa / CoreGraphics）。
//
//  约定：
//    · 所有句柄（HWND / HDC / HBITMAP…）都是不透明指针，头文件里不出现任何 Cocoa 类型，
//      这样在 Windows 上用 -DHSW_MAC 也能做「只编译不链接」的语法 / 签名自检。
//    · 消息循环不是真的 Win32 消息循环：shim 在 GetMessageW() 里泵 Cocoa 事件，
//      把鼠标 / 键盘 / 滚轮 / 定时器 / 重绘翻译成 WM_* 塞进队列，DispatchMessageW 再回调 WndProc。
// ============================================================
#ifndef HSW_WINSHIM_H
#define HSW_WINSHIM_H

#if defined(HSW_MAC)

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdio>
#include <cwchar>
#include <cstdlib>
#include <string>

// ---------------- 基本类型 ----------------
typedef void*       HWND;
typedef void*       HINSTANCE;
typedef void*       HDC;
typedef void*       HGDIOBJ;
typedef void*       HBITMAP;
typedef void*       HBRUSH;
typedef void*       HPEN;
typedef void*       HFONT;
typedef void*       HICON;
typedef void*       HCURSOR;
typedef void*       HMENU;
typedef void*       HMODULE;
typedef void*       HANDLE;
typedef void*       HRGN;

typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned int   UINT;
typedef unsigned long  DWORD;
typedef int            INT;
typedef long           LONG;
typedef unsigned long  ULONG;
typedef long long      LONGLONG;
typedef unsigned long long ULONGLONG;
typedef unsigned long long ULONG_PTR;
typedef long long      LONG64;
typedef unsigned int   ATOM;
typedef wchar_t        WCHAR;
typedef short          SHORT;
typedef unsigned short USHORT;

typedef long long      WPARAM;
typedef long long      LPARAM;
typedef long long      LRESULT;
typedef char*          LPSTR;
typedef const char*    LPCSTR;
typedef wchar_t*       LPWSTR;
typedef const wchar_t* LPCWSTR;
typedef long long      LONG_PTR;
typedef long           HRESULT;
typedef unsigned long long UINT_PTR;
typedef unsigned long long DWORD_PTR;

#define TRUE  1
#define FALSE 0
#define NULL  nullptr
#define WINAPI
#define CALLBACK
#define APIENTRY
#define FAR
#define CONST const
#define IN
#define OUT

#define MAX_PATH 260
#define S_OK     0L
#define S_FALSE  1L
#define E_FAIL   0x80004005L

// BOOL：Objective-C 编译时用系统自带的（signed char / bool），
// 纯 C++ 编译（游戏主体 高中的一周.cpp）时自己给一个 int 版本。
#if !defined(__OBJC__)
typedef int BOOL;
#endif

// ---------------- 结构体 ----------------
struct RECT  { LONG left, top, right, bottom; };
struct POINT { LONG x, y; };
struct SIZE  { LONG cx, cy; };

struct PAINTSTRUCT {
    HDC   hdc;
    BOOL  fErase;
    RECT  rcPaint;
    BOOL  fRestore;
    BOOL  fIncUpdate;
    char  rgbReserved[32];
};

struct MSG {
    HWND   hwnd;
    UINT   message;
    WPARAM wParam;
    LPARAM lParam;
    DWORD  time;
    POINT  pt;
};

typedef LRESULT (CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);

struct WNDCLASSW {
    UINT        cbSize;
    UINT        style;
    WNDPROC     lpfnWndProc;
    int         cbClsExtra;
    int         cbWndExtra;
    HINSTANCE   hInstance;
    HICON       hIcon;
    HCURSOR     hCursor;
    HBRUSH      hbrBackground;
    const wchar_t* lpszMenuName;
    const wchar_t* lpszClassName;
};
typedef WNDCLASSW WNDCLASSEXW;

struct BITMAPINFOHEADER {
    DWORD biSize;
    LONG  biWidth;
    LONG  biHeight;
    WORD  biPlanes;
    WORD  biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG  biXPelsPerMeter;
    LONG  biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
};
struct LARGE_INTEGER { long long QuadPart; };
struct RGBQUAD { BYTE rgbBlue, rgbGreen, rgbRed, rgbReserved; };
struct BITMAPINFO {
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD          bmiColors[1];
};

// ---------------- 消息 ----------------
#define WM_NULL          0x0000
#define WM_DESTROY       0x0002
#define WM_MOVE          0x0003
#define WM_SIZE          0x0005
#define WM_ACTIVATE      0x0006
#define WM_SETFOCUS      0x0007
#define WM_KILLFOCUS     0x0008
#define WM_PAINT         0x000F
#define WM_CLOSE         0x0010
#define WM_QUIT          0x0012
#define WM_ERASEBKGND    0x0014
#define WM_SHOWWINDOW    0x0018
#define WM_SETCURSOR     0x0020
#define WM_KEYDOWN       0x0100
#define WM_KEYUP         0x0101
#define WM_CHAR          0x0102
#define WM_SYSKEYDOWN    0x0104
#define WM_SYSCOMMAND    0x0112
#define WM_TIMER         0x0113
#define WM_MOUSEMOVE     0x0200
#define WM_LBUTTONDOWN   0x0201
#define WM_LBUTTONUP     0x0202
#define WM_LBUTTONDBLCLK 0x0203
#define WM_RBUTTONDOWN   0x0204
#define WM_RBUTTONUP     0x0205
#define WM_MOUSEWHEEL    0x020A
#define WM_MOUSELEAVE    0x02A3
#define WM_SIZING        0x0214
#define WM_ENTERSIZEMOVE 0x0231
#define WM_EXITSIZEMOVE  0x0232
#define WM_NCCALCSIZE    0x0083
#define WM_NCHITTEST     0x0084
#define WM_GETMINMAXINFO 0x0024

// WM_SIZING 的 wParam
#define WMSZ_LEFT   1
#define WMSZ_RIGHT  2
#define WMSZ_TOP    3
#define WMSZ_TOPLEFT 4
#define WMSZ_TOPRIGHT 5
#define WMSZ_BOTTOM 6
#define WMSZ_BOTTOMLEFT 7
#define WMSZ_BOTTOMRIGHT 8

// ---------------- 虚拟键 ----------------
#define VK_BACK      0x08
#define VK_TAB       0x09
#define VK_RETURN    0x0D
#define VK_SHIFT     0x10
#define VK_CONTROL   0x11
#define VK_MENU      0x12
#define VK_PAUSE     0x13
#define VK_CAPITAL   0x14
#define VK_ESCAPE    0x1B
#define VK_SPACE     0x20
#define VK_PRIOR     0x21
#define VK_NEXT      0x22
#define VK_END       0x23
#define VK_HOME      0x24
#define VK_LEFT      0x25
#define VK_UP        0x26
#define VK_RIGHT     0x27
#define VK_DOWN      0x28
#define VK_SNAPSHOT  0x2C
#define VK_INSERT    0x2D
#define VK_DELETE    0x2E
#define VK_LWIN      0x5B
#define VK_RWIN      0x5C
#define VK_NUMPAD0   0x60
#define VK_NUMPAD1   0x61
#define VK_NUMPAD2   0x62
#define VK_NUMPAD3   0x63
#define VK_NUMPAD4   0x64
#define VK_NUMPAD5   0x65
#define VK_NUMPAD6   0x66
#define VK_NUMPAD7   0x67
#define VK_NUMPAD8   0x68
#define VK_NUMPAD9   0x69
#define VK_F1        0x70
#define VK_F11       0x7A
#define VK_F12       0x7B

// ---------------- 窗口样式 / 命令 ----------------
#define WS_OVERLAPPED     0x00000000L
#define WS_POPUP          0x80000000L
#define WS_CAPTION        0x00C00000L
#define WS_SYSMENU        0x00080000L
#define WS_THICKFRAME     0x00040000L
#define WS_MINIMIZEBOX    0x00020000L
#define WS_MAXIMIZEBOX    0x00010000L
#define WS_VISIBLE        0x10000000L
#define WS_CLIPSIBLINGS   0x04000000L
#define WS_CLIPCHILDREN   0x02000000L
#define WS_EX_APPWINDOW   0x00040000L
#define WS_EX_WINDOWEDGE  0x00000100L
#define WS_EX_CLIENTEDGE  0x00000200L
#define CW_USEDEFAULT     ((int)0x80000000)
#define SW_SHOW           5
#define SW_HIDE           0
#define SW_RESTORE        9
#define SW_MAXIMIZE       3
#define SWP_NOZORDER      0x0004
#define SWP_NOMOVE        0x0002
#define SWP_NOSIZE        0x0001
#define SWP_FRAMECHANGED  0x0020
#define SWP_SHOWWINDOW    0x0040
#define SM_CXSCREEN       0
#define SM_CYSCREEN       1
#define SM_CXSIZEFRAME    32
#define SM_CYSIZEFRAME    33
#define SM_CYCAPTION      4
#define IDC_ARROW         ((const wchar_t*)32512)
#define IDI_APPLICATION   ((const wchar_t*)32512)
#define COLOR_WINDOW      5
#define WHITE_BRUSH       0
#define BLACK_BRUSH       4
#define NULL_BRUSH        5
#define GRAY_BRUSH        2
#define DC_BRUSH          18
#define GWL_STYLE         (-16)
#define GWL_EXSTYLE       (-20)
#define CS_HREDRAW        0x0002
#define CS_VREDRAW        0x0001
#define CS_OWNDC          0x0020
#define CS_DBLCLKS        0x0008
#define SRCCOPY           0x00CC0020u
#define DIB_RGB_COLORS    0
#define BI_RGB            0

// ---------------- 常用宏 ----------------
#define LOWORD(l)  ((WORD)(((DWORD_PTR)(l)) & 0xFFFF))
#define HIWORD(l)  ((WORD)((((DWORD_PTR)(l)) >> 16) & 0xFFFF))
#define GET_X_LPARAM(lp)  ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp)  ((int)(short)HIWORD(lp))
#define MAKELONG(a, b)    ((LONG)(((WORD)(a)) | (((DWORD)((WORD)(b))) << 16)))
#define MAKEWORD(a, b)    ((WORD)(((BYTE)(a)) | (((WORD)((BYTE)(b))) << 8)))
#define WHEEL_DELTA       120

// PlaySound 标志（音效走内存 WAV）
#define SND_MEMORY     0x00000004L
#define SND_ASYNC      0x00000001L
#define SND_NODEFAULT  0x00000002L
#define SND_SYNC       0x00000000L
#define SND_LOOP       0x00000008L
#define SND_FILENAME   0x00020000L

#define CSIDL_LOCAL_APPDATA  0x001C
#define CP_UTF8              65001

// ---------------- 函数声明（实现在 shim_mac.mm） ----------------
extern "C" {

BOOL  SetProcessDPIAware(void);
DWORD GetTickCount(void);
ULONGLONG GetTickCount64(void);
void  Sleep(DWORD ms);

ATOM  RegisterClassExW(const WNDCLASSW* wc);
HWND  CreateWindowExW(DWORD exStyle, const wchar_t* cls, const wchar_t* title,
                      DWORD style, int x, int y, int w, int h,
                      HWND parent, HMENU menu, HINSTANCE inst, void* param);
BOOL  ShowWindow(HWND hwnd, int cmd);
BOOL  UpdateWindow(HWND hwnd);
BOOL  SetWindowTextW(HWND hwnd, const wchar_t* text);
BOOL  SetWindowPos(HWND hwnd, HWND after, int x, int y, int w, int h, UINT flags);
BOOL  GetWindowRect(HWND hwnd, RECT* rc);
BOOL  GetClientRect(HWND hwnd, RECT* rc);
BOOL  MoveWindow(HWND hwnd, int x, int y, int w, int h, BOOL repaint);
LONG  SetWindowLongW(HWND hwnd, int idx, LONG val);
LONG  GetWindowLongW(HWND hwnd, int idx);
BOOL  SetForegroundWindow(HWND hwnd);
BOOL  SetFocus2(void);

BOOL  GetMessageW(MSG* msg, HWND hwnd, UINT min, UINT max);
BOOL  PeekMessageW(MSG* msg, HWND hwnd, UINT min, UINT max, UINT remove);
BOOL  TranslateMessage(const MSG* msg);
LRESULT DispatchMessageW(const MSG* msg);
LRESULT DefWindowProcW(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT SendMessageW(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
BOOL  PostMessageW(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
void  PostQuitMessage(int code);

HDC   BeginPaint(HWND hwnd, PAINTSTRUCT* ps);
BOOL  EndPaint(HWND hwnd, const PAINTSTRUCT* ps);
HDC   GetDC(HWND hwnd);
int   ReleaseDC(HWND hwnd, HDC hdc);
BOOL  InvalidateRect(HWND hwnd, const RECT* rc, BOOL erase);

HDC   CreateCompatibleDC(HDC hdc);
BOOL  DeleteDC(HDC hdc);
HBITMAP CreateCompatibleBitmap(HDC hdc, int w, int h);
HBITMAP CreateDIBSection(HDC hdc, const BITMAPINFO* bmi, UINT usage,
                         void** bits, HANDLE section, DWORD offset);
HGDIOBJ SelectObject(HDC hdc, HGDIOBJ obj);
BOOL  DeleteObject(HGDIOBJ obj);
BOOL  BitBlt(HDC dst, int dx, int dy, int w, int h, HDC src, int sx, int sy, DWORD rop);
BOOL  StretchBlt(HDC dst, int dx, int dy, int dw, int dh,
                 HDC src, int sx, int sy, int sw, int sh, DWORD rop);
DWORD GetLastError(void);

UINT_PTR SetTimer(HWND hwnd, UINT_PTR id, UINT ms, void* proc);
BOOL  KillTimer(HWND hwnd, UINT_PTR id);

int   GetSystemMetrics(int idx);
SHORT GetKeyState(int vk);
BOOL  GetCursorPos(POINT* pt);
BOOL  ScreenToClient(HWND hwnd, POINT* pt);
BOOL  ClientToScreen(HWND hwnd, POINT* pt);
HCURSOR LoadCursorW(HINSTANCE inst, const wchar_t* name);
HGDIOBJ GetStockObject(int idx);
int    MultiByteToWideChar(UINT cp, DWORD flags, const char* src, int srcLen,
                           wchar_t* dst, int dstLen);
int    WideCharToMultiByte(UINT cp, DWORD flags, const wchar_t* src, int srcLen,
                           char* dst, int dstLen, const char* def, BOOL* used);

// 路径 / 文件：macOS 下 GetModuleFileNameW 返回「资源目录」里的一个伪 exe 路径，
// 这样 ExeDir() 算出来的目录正好是 .app/Contents/Resources，图片 / mp3 都在那儿。
DWORD GetModuleFileNameW(HMODULE mod, wchar_t* buf, DWORD cap);
HRESULT SHGetFolderPathW(HWND hwnd, int csidl, HANDLE token, DWORD flags, wchar_t* path);

// 音频（mp3 用 AVAudioPlayer；内存 WAV 音效用一个临时播放器）
BOOL  PlaySoundA(const char* mem, HMODULE mod, DWORD flags);
BOOL  PlaySoundW(const wchar_t* file, HMODULE mod, DWORD flags);

// MSVC 风格的文件函数（MinGW 也有，macOS 没有）
FILE* _wfopen(const wchar_t* path, const wchar_t* mode);
int   _fseeki64(FILE* f, long long off, int whence);
long long _ftelli64(FILE* f);
int   _wcsicmp(const wchar_t* a, const wchar_t* b);

BOOL  QueryPerformanceCounter(LARGE_INTEGER* v);
BOOL  QueryPerformanceFrequency(LARGE_INTEGER* v);
BOOL  CreateDirectoryW(const wchar_t* path, void* sa);
DWORD GetFileAttributesW(const wchar_t* path);
BOOL  SetFileAttributesW(const wchar_t* path, DWORD attr);
BOOL  CopyFileW(const wchar_t* src, const wchar_t* dst, BOOL failIfExists);
BOOL  DeleteFileW(const wchar_t* path);
BOOL  PathFileExistsW(const wchar_t* path);

// 文件遍历（PtScanAudio 扫 mp3 用）
struct WIN32_FIND_DATAW {
    DWORD dwFileAttributes;
    wchar_t cFileName[MAX_PATH];
    wchar_t cAlternateFileName[14];
};
#define INVALID_HANDLE_VALUE ((HANDLE)(long long)-1)
#define FILE_ATTRIBUTE_DIRECTORY 0x00000010
HANDLE FindFirstFileW(const wchar_t* pattern, WIN32_FIND_DATAW* fd);
BOOL   FindNextFileW(HANDLE h, WIN32_FIND_DATAW* fd);
BOOL   FindClose(HANDLE h);

// 资源（macOS 上没有内嵌 mp3，一律返回空）
typedef void* HRSRC;
typedef void* HGLOBAL;
#define MAKEINTRESOURCEW(i) ((const wchar_t*)(unsigned long long)(unsigned short)(i))
#define MAKEINTRESOURCEA(i) ((const char*)(unsigned long long)(unsigned short)(i))
#define RT_RCDATA ((const wchar_t*)10)
HRSRC   HswFindResource(HMODULE mod, const wchar_t* name, const wchar_t* type);
HGLOBAL HswLoadResource(HMODULE mod, HRSRC res);
void*   HswLockResource(HGLOBAL g);
DWORD   HswSizeofResource(HMODULE mod, HRSRC res);
#define FindResourceW  HswFindResource
#define LoadResource   HswLoadResource
#define LockResource   HswLockResource
#define SizeofResource HswSizeofResource

// Media Foundation：macOS 不需要，做成空实现，游戏里的 MFStartup 照call不误
#define MF_VERSION 0x00020070
long HswMFStartup(unsigned long version, unsigned long flags);
long HswMFShutdown(void);
#define MFStartup  HswMFStartup
#define MFShutdown HswMFShutdown

double HswAudDur(void);          // 当前音频总时长（秒）

// 杂项：光标 / 鼠标离开跟踪 / 窗口样式 / 多显示器（全屏切换那一段会用到）
#define ZeroMemory(p, n) memset((p), 0, (n))
#define GWLP_STYLE      (-16)
#define GWLP_EXSTYLE    (-20)
#define HWND_TOP        ((HWND)0)
#define HWND_NOTOPMOST  ((HWND)-2)
#define HWND_TOPMOST    ((HWND)-1)
#define MONITOR_DEFAULTTONEAREST 2
#define MONITOR_DEFAULTTOPRIMARY 1
#define TME_LEAVE       0x00000002
#define IDC_HAND        ((const wchar_t*)32649)
#define IDC_IBEAM       ((const wchar_t*)32513)
#define TME_HOVER       0x00000001

struct MONITORINFO {
    DWORD cbSize;
    RECT  rcMonitor;
    RECT  rcWork;
    DWORD dwFlags;
};
struct TRACKMOUSEEVENT {
    DWORD cbSize;
    DWORD dwFlags;
    HWND  hwndTrack;
    DWORD dwHoverTime;
};
typedef void* HMONITOR;

SHORT GetAsyncKeyState(int vk);
BOOL  TrackMouseEvent(TRACKMOUSEEVENT* tme);
HCURSOR SetCursor(HCURSOR c);
LONG_PTR GetWindowLongPtrW(HWND hwnd, int idx);
LONG_PTR SetWindowLongPtrW(HWND hwnd, int idx, LONG_PTR val);
HMONITOR MonitorFromWindow(HWND hwnd, DWORD flags);
BOOL  GetMonitorInfoW(HMONITOR mon, MONITORINFO* mi);
BOOL  AdjustWindowRect(RECT* rc, DWORD style, BOOL menu);
BOOL  GetWindowPlacement(HWND hwnd, void* wp);
BOOL  SetWindowPlacement(HWND hwnd, const void* wp);
int   SHCreateDirectoryExW(HWND hwnd, const wchar_t* path, void* sec);
BOOL  SetLayeredWindowAttributes(HWND hwnd, DWORD key, BYTE alpha, DWORD flags);

typedef const void* LPCVOID;
typedef void*       LPVOID;
typedef long long (*FARPROC_T)(void);
HMODULE LoadLibraryW(const wchar_t* name);
void*   GetProcAddress(HMODULE mod, const char* name);
BOOL    FreeLibrary(HMODULE mod);

void  HswFatal(const char* msg);

// 音频（macOS 版 PtAudio 用）：AVAudioPlayer，实现在 shim_mac.mm
bool  HswAudLoadFile(const wchar_t* path, double maxSec);
bool  HswAudLoadMem(const void* data, unsigned long long bytes, double maxSec);
void  HswAudPlay(void);
void  HswAudPause(void);
void  HswAudStop(void);
double HswAudPos(void);          // 播放位置（秒），不可用返回 -1

} // extern "C"

#define WNDCLASSEX WNDCLASSEXW
#define DefWindowProc DefWindowProcW
#define DispatchMessage DispatchMessageW
#define GetMessage GetMessageW
#define RegisterClassEx RegisterClassExW
#define CreateWindowEx CreateWindowExW
#define SetWindowText SetWindowTextW
#define SetWindowLong SetWindowLongW
#define GetWindowLong GetWindowLongW
#define LoadCursorA LoadCursorW
#define LoadCursor LoadCursorW
#define SetWindowLongPtr SetWindowLongPtrW
#define GetWindowLongPtr GetWindowLongPtrW
#define PostMessage PostMessageW
#define SendMessage SendMessageW
#define GetMonitorInfo GetMonitorInfoW
#define CreateDirectory CreateDirectoryW
#define SetWindowTextA SetWindowTextW
#define PlaySound PlaySoundA

// macOS 下 wchar_t 是 4 字节 UTF-32，CoreText 要 UTF-16，所以在 shim 里统一转一次。
// 游戏里凡是要交给 GDI+ 的宽字符串都会经过 gshim 的转换，不需要改业务代码。
#define HSW_WCHAR_IS_UTF32 1

#endif // HSW_MAC
#endif // HSW_WINSHIM_H
