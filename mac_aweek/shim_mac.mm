// ==========================================================================
//  shim_mac.mm —— Win32 / GDI+ 兼容层的 macOS 实现
//
//  编译：clang++ -std=c++17 -fobjc-arc -x objective-c++ shim_mac.mm
//
//  设计要点：
//   1. 先 import Cocoa / CoreText / AVFoundation，再 include winshim.h / gshim.h。
//      因为 winshim.h 里有 `#define BOOL int`，不能污染 Objective-C 头文件。
//   2. 位图缓冲统一用 BGRA（byte order 32Little + PremultipliedFirst），
//      和 Windows GDI+ 的 PixelFormat32bppARGB 内存序（B,G,R,A）完全一致，
//      LockBits 那段泛洪抠图代码可以原封不动跑。
//   3. 位图上下文在建立时统一加一次翻转：TranslateCTM(0,H) + ScaleCTM(1,-1)，
//      于是所有 GDI+ 的 y 向下坐标都能直接用，而缓冲本身仍是 CG 原生朝向
//      （row0 = 底部），CGBitmapContextCreateImage 出来是正的。
//      代价：每次 CGContextDrawImage 都要在目标矩形上再翻一次。
// ==========================================================================
#import <Cocoa/Cocoa.h>
#import <CoreGraphics/CoreGraphics.h>
#import <CoreText/CoreText.h>
#import <ImageIO/ImageIO.h>
#import <AVFoundation/AVFoundation.h>
#import <QuartzCore/QuartzCore.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <cmath>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_set>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <mach/mach_time.h>

#include "winshim.h"
#include "gshim.h"

// ==========================================================================
//  第 0 部分：给 Objective-C 侧用的转发声明（在 include winshim 之前）
// ==========================================================================
extern "C" {
    void HswPushMsg(unsigned int msg, long long w, long long l);
    void HswSetQuit(void);
    void HswViewSize(int* w, int* h);
    void HswPresentBackbuffer(void);   // 把背缓冲贴到窗口
}

// ==========================================================================
//  第 1 部分：Cocoa 窗口 / 视图
// ==========================================================================
// 键盘事件转发（定义在下面的 extern "C" 段里，这里先声明，免得 Objective-C 段找不到）
extern "C" void HswPostKey(unsigned int msg, NSEvent* ev);

@interface HswView : NSView
@end

@interface HswDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate>
@property (strong) NSWindow* win;
@property (strong) HswView*  view;
@end

static HswDelegate* g_delegate = nil;
static NSWindow*    g_window   = nil;
static HswView*     g_view     = nil;
static NSApplication* g_app    = nil;

// 背缓冲（屏幕像素）：EndPaint 之后一次性贴到窗口
static unsigned char* g_backBits = nullptr;
static int  g_backW = 0, g_backH = 0, g_backStride = 0;
static CGContextRef g_backCtx = nullptr;
static CGImageRef  g_backImage = nullptr;
static bool g_needDisplay = false;

// 逻辑坐标 <-> 屏幕坐标：游戏是 1280x720 逻辑分辨率等比缩放居中
static int g_clientW = 1280, g_clientH = 720;

@implementation HswView

- (BOOL)isOpaque { return YES; }
- (BOOL)acceptsFirstResponder { return YES; }

// 临时诊断日志（CI 上定位卡点用）
static int g_dbgN = 0;
#define DBG(...) do { if (g_dbgN < 3000) { fprintf(stderr, "[shim] " __VA_ARGS__); fputc('\n', stderr); fflush(stderr); } } while(0)

// Cocoa 原点在左下，翻成 GDI 的 y 向下
static int HswFlipY(CGFloat yInView)
{
    CGFloat h = g_view ? g_view.bounds.size.height : (CGFloat)g_clientH;
    return (int)round(h - yInView);
}

static void HswPostMouse(unsigned int msg, NSEvent* ev, int clicks)
{
    NSPoint loc = [ev locationInWindow];
    if (g_view) loc = [g_view convertPoint:loc fromView:nil];
    int x = (int)round(loc.x);
    int y = HswFlipY(loc.y);
    long long lp = (long long)(unsigned short)x | (((long long)(unsigned short)y) << 16);
    (void)clicks;
    HswPushMsg(msg, (long long)([ev clickCount] > 1 ? 2 : 1), lp);
}

- (void)mouseDown:(NSEvent*)ev       { HswPostMouse(WM_LBUTTONDOWN, ev, 1); }
- (void)mouseUp:(NSEvent*)ev         { HswPostMouse(WM_LBUTTONUP,   ev, 1); }
- (void)rightMouseDown:(NSEvent*)ev  { HswPostMouse(WM_RBUTTONDOWN, ev, 1); }
- (void)rightMouseUp:(NSEvent*)ev    { HswPostMouse(WM_RBUTTONUP,   ev, 1); }
- (void)mouseMoved:(NSEvent*)ev      { HswPostMouse(WM_MOUSEMOVE,   ev, 0); }
- (void)mouseDragged:(NSEvent*)ev    { HswPostMouse(WM_MOUSEMOVE,   ev, 0); }
- (void)rightMouseDragged:(NSEvent*)ev { HswPostMouse(WM_MOUSEMOVE, ev, 0); }

- (void)scrollWheel:(NSEvent*)ev
{
    // 向上滚 deltaY > 0，和 Windows 一致
    int d = (int)round([ev deltaY] * 120.0);
    if (d == 0) d = ([ev deltaY] > 0 ? 120 : -120);
    NSPoint loc = [ev locationInWindow];
    loc = [self convertPoint:loc fromView:nil];
    int x = (int)round(loc.x);
    int y = HswFlipY(loc.y);
    long long lp = (long long)(unsigned short)x | (((long long)(unsigned short)y) << 16);
    HswPushMsg(WM_MOUSEWHEEL, ((long long)d) << 16, lp);
}

- (void)keyDown:(NSEvent*)ev   { HswPostKey(WM_KEYDOWN, ev); }
- (void)keyUp:(NSEvent*)ev     { HswPostKey(WM_KEYUP,   ev); }

- (void)drawRect:(NSRect)dirty
{
    (void)dirty;
    CGContextRef cg = (CGContextRef)[[NSGraphicsContext currentContext] CGContext];
    if (!cg) return;
    [[NSColor blackColor] setFill];
    NSRectFill(self.bounds);
    if (g_backImage) {
        CGContextSaveGState(cg);
        // NSView 默认 y 向上，缓冲是 CG 原生朝向，直接画就是正的
        CGContextDrawImage(cg, CGRectMake(0, 0, g_backW, g_backH), g_backImage);
        CGContextRestoreGState(cg);
    }
}

@end

@implementation HswDelegate

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender
{
    (void)sender;
    return YES;
}

- (void)windowWillClose:(NSNotification*)note
{
    (void)note;
    HswSetQuit();
}

@end

// ==========================================================================
//  第 2 部分：音频（AVAudioPlayer）
// ==========================================================================
@interface HswAudio : NSObject
@property (strong) AVAudioPlayer* music;
@property (strong) NSMutableArray* sfx;
@end

@implementation HswAudio
- (instancetype)init { self = [super init]; self.sfx = [NSMutableArray array]; return self; }
@end

static HswAudio* g_audio = nil;
static double g_audioDur = 0.0;

// ==========================================================================
//  第 3 部分：UTF 转换 / 路径
// ==========================================================================
static std::string W2U(const std::wstring& ws)
{
    std::string out;
    for (wchar_t c : ws) {
        unsigned int cp = (unsigned int)c;
        if (cp < 0x80) out += (char)cp;
        else if (cp < 0x800) { out += (char)(0xC0 | (cp >> 6)); out += (char)(0x80 | (cp & 0x3F)); }
        else if (cp < 0x10000) {
            out += (char)(0xE0 | (cp >> 12));
            out += (char)(0x80 | ((cp >> 6) & 0x3F));
            out += (char)(0x80 | (cp & 0x3F));
        } else {
            out += (char)(0xF0 | (cp >> 18));
            out += (char)(0x80 | ((cp >> 12) & 0x3F));
            out += (char)(0x80 | ((cp >> 6) & 0x3F));
            out += (char)(0x80 | (cp & 0x3F));
        }
    }
    return out;
}

static std::wstring U2W(const std::string& s)
{
    std::wstring out;
    size_t i = 0;
    while (i < s.size()) {
        unsigned char c = (unsigned char)s[i];
        unsigned int cp = 0; int n = 0;
        if (c < 0x80) { cp = c; n = 1; }
        else if ((c & 0xE0) == 0xC0) { cp = c & 0x1F; n = 2; }
        else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F; n = 3; }
        else if ((c & 0xF8) == 0xF0) { cp = c & 0x07; n = 4; }
        else { ++i; continue; }
        for (int k = 1; k < n; ++k) cp = (cp << 6) | ((unsigned char)s[i + k] & 0x3F);
        out += (wchar_t)cp;
        i += (size_t)n;
    }
    return out;
}

static std::string W2U(const wchar_t* ws) { return ws ? W2U(std::wstring(ws)) : std::string(); }

// 游戏里到处用 L"%ls\\%ls" 拼路径，反斜杠在 macOS 上不是分隔符 —— 统一换成 '/'
static std::string HswPath(const wchar_t* ws)
{
    std::string p = W2U(ws);
    for (size_t i = 0; i < p.size(); ++i) if (p[i] == '\\') p[i] = '/';
    return p;
}

extern "C" {

// ---------------- 时间 ----------------
static double g_timeBase = 0.0;
static double HswNowSec(void)
{
    static mach_timebase_info_data_t tb = { 0, 0 };
    if (tb.denom == 0) mach_timebase_info(&tb);
    unsigned long long t = mach_absolute_time();
    return (double)t * (double)tb.numer / (double)tb.denom / 1e9;
}

DWORD GetTickCount(void) { return (DWORD)(HswNowSec() * 1000.0); }
ULONGLONG GetTickCount64(void) { return (ULONGLONG)(HswNowSec() * 1000.0); }
void Sleep(DWORD ms) { usleep((useconds_t)ms * 1000); }
BOOL SetProcessDPIAware(void) { DBG("SetProcessDPIAware"); return TRUE; }
DWORD GetLastError(void) { return 0; }

BOOL QueryPerformanceCounter(LARGE_INTEGER* v)
{ if (!v) return FALSE; v->QuadPart = (long long)(HswNowSec() * 1e9); return TRUE; }
BOOL QueryPerformanceFrequency(LARGE_INTEGER* v)
{ if (!v) return FALSE; v->QuadPart = 1000000000LL; return TRUE; }

// ---------------- 消息 ----------------
struct ShimTimer { UINT_PTR id; UINT ms; double next; };

static std::vector<MSG>     g_msgQ;
static std::vector<ShimTimer> g_timers;
static WNDPROC              g_wndProc = nullptr;
static bool                 g_quitFlag = false;
static HWND                 g_mainWnd = (HWND)1;
static bool                 g_inited = false;

void HswSetQuit(void) { g_quitFlag = true; }

void HswPushMsg(unsigned int msg, long long w, long long l)
{
    MSG m;
    memset(&m, 0, sizeof(m));
    m.hwnd = g_mainWnd;
    m.message = msg;
    m.wParam = (WPARAM)w;
    m.lParam = (LPARAM)l;
    m.pt.x = GET_X_LPARAM(l);
    m.pt.y = GET_Y_LPARAM(l);
    g_msgQ.push_back(m);
}

void HswViewSize(int* w, int* h) { if (w) *w = g_clientW; if (h) *h = g_clientH; }

// 虚拟键映射
static int HswVkFromKey(unsigned short code)
{
    switch (code) {
        case 0x24: return VK_RETURN;
        case 0x35: return VK_ESCAPE;
        case 0x33: return VK_DELETE;
        case 0x30: return VK_TAB;
        case 0x31: return VK_SPACE;
        case 0x7B: return VK_LEFT;
        case 0x7C: return VK_RIGHT;
        case 0x7D: return VK_DOWN;
        case 0x7E: return VK_UP;
        case 0x73: return VK_HOME;
        case 0x77: return VK_END;
        case 0x74: return VK_PRIOR;
        case 0x79: return VK_NEXT;
        case 0x72: return VK_INSERT;
        case 0x32: return '`';
        default: break;
    }
    if (code >= 0x12 && code <= 0x1B) return '1' + (int)(code - 0x12);
    if (code >= 0x1E && code <= 0x27) return '2' + (int)(code - 0x1E);  // 2..9,0
    return 0;
}

void HswPostKey(unsigned int msg, NSEvent* ev)
{
    int vk = HswVkFromKey([ev keyCode]);
    NSString* ch = [ev characters];
    if (vk == 0 && [ch length] > 0) {
        unichar u = [ch characterAtIndex:0];
        if (u >= 'a' && u <= 'z') vk = u - 'a' + 'A';
        else if (u < 128) vk = (int)u;
    }
    if (vk == 0) return;
    HswPushMsg(msg, (long long)vk, 0);
    if (msg == WM_KEYDOWN && vk >= 32 && vk < 128)
        HswPushMsg((unsigned int)WM_CHAR, (long long)vk, 0);
}

// ---------------- 位图 / HDC ----------------
// Graphics(void*) 可能收到 Bitmap*（gshim 的类）也可能收到 HDC（我们的 ShimDC*），
// 两者都是 void*，靠一张「活着的 DC 表」区分，比猜内存布局可靠。
static std::unordered_set<void*> g_dcSet;

// magic 用来区分 Graphics(void*) 收到的是 Bitmap 还是 HDC
#define HSW_MAGIC_IMG 0x484D4931u   // "HMI1"
#define HSW_MAGIC_DC  0x484D4431u   // "HMD1"

struct ShimBuf {
    unsigned int magic = HSW_MAGIC_IMG;
    unsigned char* bits = nullptr;
    int w = 0, h = 0, stride = 0;
    CGContextRef ctx = nullptr;
    CGImageRef   img = nullptr;     // 缓存，dirty 时重建
    bool owned = false;
    bool dirty = true;
};

struct ShimDC {
    unsigned int magic = HSW_MAGIC_DC;
    bool   isWindow = false;
    ShimBuf* bmp = nullptr;
    CGContextRef ctx = nullptr;
    int w = 0, h = 0;
};

static CGColorSpaceRef g_cs = nullptr;

static ShimBuf* HswBufCreate(int w, int h)
{
    if (w <= 0 || h <= 0) return nullptr;
    if (!g_cs) g_cs = CGColorSpaceCreateDeviceRGB();
    ShimBuf* b = new ShimBuf();
    b->w = w; b->h = h;
    b->stride = w * 4;
    b->bits = (unsigned char*)calloc((size_t)b->stride * (size_t)h, 1);
    b->owned = true;
    b->ctx = CGBitmapContextCreate(b->bits, w, h, 8, b->stride, g_cs,
                                   kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little);
    if (!b->ctx) { free(b->bits); delete b; return nullptr; }
    // y 向下：和 GDI+ 一致
    CGContextTranslateCTM(b->ctx, 0, h);
    CGContextScaleCTM(b->ctx, 1, -1);
    CGContextSetInterpolationQuality(b->ctx, kCGInterpolationHigh);
    CGContextSetAllowsAntialiasing(b->ctx, true);
    CGContextSetShouldAntialias(b->ctx, true);
    b->owned = true;
    b->owned = true;
    return b;
}

static CGImageRef HswBufImage(ShimBuf* b)
{
    if (!b) return nullptr;
    // 注意：这里必须看 dirty，否则第一帧的空白图会被一直缓存（画面全黑）
    if (!b->img || b->dirty) {
        if (b->img) { CGImageRelease(b->img); b->img = nullptr; }
        b->img = CGBitmapContextCreateImage(b->ctx);
        b->dirty = false;
    }
    return b->img;
}

static void HswBufDirty(ShimBuf* b)
{
    if (!b) return;
    if (b->img) { CGImageRelease(b->img); b->img = nullptr; }
    b->dirty = true;
}

// ---- 屏幕背缓冲 ----
static void HswBackResize(int w, int h)
{
    if (g_backCtx) { CGContextRelease(g_backCtx); g_backCtx = nullptr; }
    if (g_backImage) { CGImageRelease(g_backImage); g_backImage = nullptr; }
    free(g_backBits); g_backBits = nullptr;
    g_backW = w; g_backH = h; g_backStride = w * 4;
    if (w <= 0 || h <= 0) return;
    if (!g_cs) g_cs = CGColorSpaceCreateDeviceRGB();
    g_backBits = (unsigned char*)calloc((size_t)g_backStride * (size_t)h, 1);
    g_backCtx = CGBitmapContextCreate(g_backBits, w, h, 8, g_backStride, g_cs,
                                      kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little);
    if (!g_backCtx) return;
    CGContextTranslateCTM(g_backCtx, 0, h);
    CGContextScaleCTM(g_backCtx, 1, -1);
}


// ---- 离屏截图（CI 自检用：--shot out.png --shot-at 30,120,300）----
static const char*    g_shotPath = nullptr;
static std::vector<int> g_shotAt;
static int            g_paintCount = 0;
static bool           g_shotQuit = false;

static void HswSavePng(const char* path)
{
    if (!g_backCtx || !path) return;
    CGImageRef img = CGBitmapContextCreateImage(g_backCtx);
    if (!img) return;
    NSString* p = [NSString stringWithUTF8String:path];
    NSURL* u = [NSURL fileURLWithPath:p];
    CGImageDestinationRef dst = CGImageDestinationCreateWithURL((__bridge CFURLRef)u, CFSTR("public.png"), 1, nullptr);
    if (dst) {
        CGImageDestinationAddImage(dst, img, nullptr);
        CGImageDestinationFinalize(dst);
        CFRelease(dst);
        fprintf(stderr, "[shot] %s (%dx%d)\n", path, g_backW, g_backH);
    }
    CGImageRelease(img);
}

void HswPresentBackbuffer(void)
{
    if (!g_backCtx) return;
    if (g_backImage) { CGImageRelease(g_backImage); g_backImage = nullptr; }
    g_backImage = CGBitmapContextCreateImage(g_backCtx);
    if (g_view) [g_view setNeedsDisplay:YES];

    ++g_paintCount;
    // 诊断：前 120 帧每 10 帧自动存一张，用来定位「第几帧开始变黑」
    if (g_shotPath && g_paintCount <= 120 && (g_paintCount % 10) == 0) {
        char ab[1024];
        snprintf(ab, sizeof(ab), "%s_auto%03d.png", g_shotPath, g_paintCount);
        HswSavePng(ab);
    }
    if (g_shotPath && !g_shotAt.empty()) {
        for (size_t i = 0; i < g_shotAt.size(); ++i) {
            if (g_shotAt[i] == g_paintCount) {
                char buf[1024];
                snprintf(buf, sizeof(buf), "%s_%d.png", g_shotPath, g_shotAt[i]);
                HswSavePng(buf);
                g_shotAt[i] = -1;
            }
        }
        bool done = true;
        for (size_t i = 0; i < g_shotAt.size(); ++i) if (g_shotAt[i] > 0) done = false;
        if (done && g_shotQuit) { HswSetQuit(); }
    }
}

// ---------------- 窗口 ----------------
ATOM RegisterClassExW(const WNDCLASSW* wc)
{
    if (wc) g_wndProc = wc->lpfnWndProc;
    return 1;
}

HWND CreateWindowExW(DWORD exStyle, const wchar_t* cls, const wchar_t* title,
                     DWORD style, int x, int y, int w, int h,
                     HWND parent, HMENU menu, HINSTANCE inst, void* param)
{
    (void)exStyle; (void)cls; (void)style; (void)parent; (void)menu; (void)inst; (void)param;
    DBG("CreateWindowExW in %dx%d", w, h);
    if (!g_inited) {
        DBG("NSApplication sharedApplication ...");
        g_app = [NSApplication sharedApplication];
        g_delegate = [[HswDelegate alloc] init];
        [g_app setDelegate:(id<NSApplicationDelegate>)g_delegate];
        [g_app setActivationPolicy:NSApplicationActivationPolicyRegular];
        [g_app finishLaunching];
        DBG("NSApplication ready");
    }
    g_inited = true;

    NSRect frame = NSMakeRect(0, 0, (CGFloat)w, (CGFloat)h);
    NSUInteger mask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                      NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;
    g_window = [[NSWindow alloc] initWithContentRect:frame
                                          styleMask:mask
                                            backing:NSBackingStoreBuffered
                                              defer:NO];
    NSString* t = [NSString stringWithUTF8String:W2U(title ? title : L"高中的一周").c_str()];
    [g_window setTitle:t];
    [g_window setAcceptsMouseMovedEvents:YES];
    [g_window setDelegate:(id<NSWindowDelegate>)g_delegate];
    g_view = [[HswView alloc] initWithFrame:frame];
    [g_window setContentView:g_view];
    [g_window makeFirstResponder:g_view];
    [g_window center];

    g_clientW = w; g_clientH = h;
    HswBackResize(w, h);

    if (x != (int)CW_USEDEFAULT || y != (int)CW_USEDEFAULT) {
        NSScreen* sc = [NSScreen mainScreen];
        CGFloat sy = sc ? sc.frame.size.height : 800;
        [g_window setFrameOrigin:NSMakePoint((CGFloat)x, sy - (CGFloat)y - (CGFloat)h)];
    }
    g_delegate.win = g_window;
    g_delegate.view = g_view;
    DBG("CreateWindow %dx%d", w, h);
    return g_mainWnd;
}

BOOL ShowWindow(HWND hwnd, int cmd)
{
    (void)hwnd;
    if (!g_window) return FALSE;
    if (cmd == SW_HIDE) { [g_window orderOut:nil]; return TRUE; }
    DBG("ShowWindow cmd=%d", cmd);
    [g_window makeKeyAndOrderFront:nil];
    [g_app activateIgnoringOtherApps:YES];
    if (cmd == SW_MAXIMIZE) [g_window zoom:nil];
    return TRUE;
}

BOOL UpdateWindow(HWND hwnd) { (void)hwnd; if (g_view) [g_view setNeedsDisplay:YES]; return TRUE; }
BOOL SetWindowTextW(HWND hwnd, const wchar_t* text)
{
    (void)hwnd;
    if (!g_window || !text) return FALSE;
    [g_window setTitle:[NSString stringWithUTF8String:W2U(text).c_str()]];
    return TRUE;
}

BOOL SetWindowPos(HWND hwnd, HWND after, int x, int y, int w, int h, UINT flags)
{
    (void)hwnd; (void)after;
    if (!g_window) return FALSE;
    NSRect f = g_window.frame;
    if (!(flags & SWP_NOMOVE)) {
        NSScreen* sc = [NSScreen mainScreen];
        CGFloat sy = sc ? sc.frame.size.height : 900;
        f.origin = NSMakePoint((CGFloat)x, sy - (CGFloat)y - f.size.height);
    }
    if (!(flags & SWP_NOSIZE)) {
        CGFloat oldTop = f.origin.y + f.size.height;
        f.size = NSMakeSize((CGFloat)w, (CGFloat)h);
        f.origin.y = oldTop - (CGFloat)h;
    }
    [g_window setFrame:f display:YES];
    return TRUE;
}

BOOL GetWindowRect(HWND hwnd, RECT* rc)
{
    (void)hwnd;
    if (!rc) return FALSE;
    if (!g_window) { rc->left = rc->top = 0; rc->right = g_clientW; rc->bottom = g_clientH; return TRUE; }
    NSRect f = g_window.frame;
    NSScreen* sc = [NSScreen mainScreen];
    CGFloat sy = sc ? sc.frame.size.height : 900;
    rc->left = (LONG)f.origin.x;
    rc->top  = (LONG)(sy - f.origin.y - f.size.height);
    rc->right = rc->left + (LONG)f.size.width;
    rc->bottom = rc->top + (LONG)f.size.height;
    return TRUE;
}

BOOL GetClientRect(HWND hwnd, RECT* rc)
{
    (void)hwnd;
    if (!rc) return FALSE;
    if (g_view) {
        NSRect b = g_view.bounds;
        g_clientW = (int)round(b.size.width);
        g_clientH = (int)round(b.size.height);
    }
    rc->left = 0; rc->top = 0; rc->right = g_clientW; rc->bottom = g_clientH;
    return TRUE;
}

BOOL MoveWindow(HWND hwnd, int x, int y, int w, int h, BOOL repaint)
{
    (void)repaint;
    return SetWindowPos(hwnd, nullptr, x, y, w, h, 0);
}

LONG SetWindowLongW(HWND hwnd, int idx, LONG val) { (void)hwnd; (void)idx; return val; }
LONG GetWindowLongW(HWND hwnd, int idx) { (void)hwnd; (void)idx; return 0; }
LONG_PTR SetWindowLongPtrW(HWND hwnd, int idx, LONG_PTR val) { (void)hwnd; (void)idx; return val; }
LONG_PTR GetWindowLongPtrW(HWND hwnd, int idx) { (void)hwnd; (void)idx; return 0; }
BOOL SetForegroundWindow(HWND hwnd) { (void)hwnd; [g_app activateIgnoringOtherApps:YES]; return TRUE; }
BOOL SetFocus2(void) { return TRUE; }

// ---------------- 消息泵 ----------------
static void HswPump(double timeoutSec)
{
    if (!g_app) return;
    @autoreleasepool {
        NSDate* until = [NSDate dateWithTimeIntervalSinceNow:timeoutSec];
        NSEvent* ev = [g_app nextEventMatchingMask:NSEventMaskAny
                                          untilDate:until
                                             inMode:NSDefaultRunLoopMode
                                            dequeue:YES];
        if (ev) [g_app sendEvent:ev];
    }
}

BOOL PeekMessageW(MSG* msg, HWND hwnd, UINT min, UINT max, UINT remove)
{
    (void)hwnd; (void)min; (void)max;
    if (!msg) return FALSE;
    if (g_msgQ.empty()) {
        // 定时器
        double now = HswNowSec() * 1000.0;
        for (size_t i = 0; i < g_timers.size(); ++i) {
            if (now >= g_timers[i].next) {
                g_timers[i].next = now + g_timers[i].ms;
                HswPushMsg(WM_TIMER, (long long)g_timers[i].id, 0);
            }
        }
        HswPump(0.0);
    }
    if (g_msgQ.empty()) return FALSE;
    *msg = g_msgQ.front();
    if (remove) g_msgQ.erase(g_msgQ.begin());
    return TRUE;
}

BOOL GetMessageW(MSG* msg, HWND hwnd, UINT min, UINT max)
{
    (void)hwnd; (void)min; (void)max;
    if (!msg) return FALSE;
    for (;;) {
        if (!g_msgQ.empty()) {
            MSG m = g_msgQ.front();
            g_msgQ.erase(g_msgQ.begin());
            *msg = m;
            if (m.message == WM_QUIT) return FALSE;
            return TRUE;
        }
        if (g_quitFlag) {
            g_quitFlag = false;
            HswPushMsg(WM_QUIT, 0, 0);
            continue;
        }
        // 定时器到期就塞一条 WM_TIMER
        double now = HswNowSec() * 1000.0;
        bool fired = false;
        for (size_t i = 0; i < g_timers.size(); ++i) {
            if (now >= g_timers[i].next) {
                g_timers[i].next = now + g_timers[i].ms;
                HswPushMsg(WM_TIMER, (long long)g_timers[i].id, 0);
                fired = true;
            }
        }
        if (fired) continue;
        HswPump(0.002);
    }
}

BOOL TranslateMessage(const MSG* msg) { (void)msg; return TRUE; }

LRESULT DispatchMessageW(const MSG* msg)
{
    static int dbgMsg = 0;
    if (!msg || !g_wndProc) return 0;
    if (dbgMsg < 40) { fprintf(stderr, "[shim] dispatch msg=0x%X\n", msg->message); fflush(stderr); ++dbgMsg; }
    return g_wndProc(msg->hwnd, msg->message, msg->wParam, msg->lParam);
}

LRESULT DefWindowProcW(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{ (void)hwnd; (void)msg; (void)wp; (void)lp; return 0; }
LRESULT SendMessageW(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{ (void)hwnd; return DefWindowProcW(hwnd, msg, wp, lp); }
BOOL PostMessageW(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{ (void)hwnd; HswPushMsg(msg, (long long)wp, (long long)lp); return TRUE; }
void PostQuitMessage(int code) { (void)code; HswPushMsg(WM_QUIT, 0, 0); }

// ---------------- 绘制 ----------------
HDC BeginPaint(HWND hwnd, PAINTSTRUCT* ps)
{
    (void)hwnd;
    if (ps) { memset(ps, 0, sizeof(*ps)); GetClientRect(hwnd, &ps->rcPaint); }
    if (g_clientW != g_backW || g_clientH != g_backH) HswBackResize(g_clientW, g_clientH);
    ShimDC* dc = new ShimDC();
    dc->isWindow = true;
    dc->ctx = g_backCtx;
    dc->w = g_backW; dc->h = g_backH;
    g_dcSet.insert((void*)dc);
    if (ps) ps->hdc = dc;
    ++g_dbgN;
    static int paintN = 0;
    ++paintN;
    if (paintN % 300 == 0) {
        fprintf(stderr, "[shim] paint #%d t=%.2fs\n", paintN, HswNowSec() - g_timeBase);
        fflush(stderr);
    }
    return dc;
}

BOOL EndPaint(HWND hwnd, const PAINTSTRUCT* ps)
{
    (void)hwnd;
    if (ps && ps->hdc) { g_dcSet.erase((void*)ps->hdc); delete (ShimDC*)ps->hdc; }
    HswPresentBackbuffer();
    return TRUE;
}

HDC GetDC(HWND hwnd)
{
    (void)hwnd;
    if (g_clientW != g_backW || g_clientH != g_backH) HswBackResize(g_clientW, g_clientH);
    ShimDC* dc = new ShimDC();
    dc->isWindow = true; dc->ctx = g_backCtx; dc->w = g_backW; dc->h = g_backH;
    g_dcSet.insert((void*)dc);
    return dc;
}

int ReleaseDC(HWND hwnd, HDC hdc)
{
    (void)hwnd;
    g_dcSet.erase((void*)hdc);
    delete (ShimDC*)hdc;
    HswPresentBackbuffer();
    return 1;
}

BOOL InvalidateRect(HWND hwnd, const RECT* rc, BOOL erase)
{
    (void)hwnd; (void)rc; (void)erase;
    HswPushMsg(WM_PAINT, 0, 0);
    return TRUE;
}

HDC CreateCompatibleDC(HDC hdc)
{
    (void)hdc;
    ShimDC* dc = new ShimDC();
    dc->isWindow = false;
    g_dcSet.insert((void*)dc);
    return dc;
}

BOOL DeleteDC(HDC hdc) { g_dcSet.erase((void*)hdc); delete (ShimDC*)hdc; return TRUE; }

HBITMAP CreateCompatibleBitmap(HDC hdc, int w, int h)
{
    (void)hdc;
    return (HBITMAP)HswBufCreate(w, h);
}

HBITMAP CreateDIBSection(HDC hdc, const BITMAPINFO* bmi, UINT usage,
                         void** bits, HANDLE section, DWORD offset)
{
    (void)hdc; (void)usage; (void)section; (void)offset;
    if (!bmi) return nullptr;
    int w = bmi->bmiHeader.biWidth;
    int h = bmi->bmiHeader.biHeight;
    if (h < 0) h = -h;
    ShimBuf* b = HswBufCreate(w, h);
    if (!b) return nullptr;
    if (bits) *bits = b->bits;
    return (HBITMAP)b;
}

HGDIOBJ SelectObject(HDC hdc, HGDIOBJ obj)
{
    ShimDC* dc = (ShimDC*)hdc;
    if (!dc) return nullptr;
    ShimBuf* old = dc->bmp;
    // obj 可能是 ShimBuf*，也可能是别的东西；只认我们发出来的
    dc->bmp = (ShimBuf*)obj;
    dc->ctx = dc->bmp ? dc->bmp->ctx : g_backCtx;
    if (dc->bmp) { dc->w = dc->bmp->w; dc->h = dc->bmp->h; }
    return (HGDIOBJ)old;
}

BOOL DeleteObject(HGDIOBJ obj)
{
    ShimBuf* b = (ShimBuf*)obj;
    if (!b) return FALSE;
    HswBufDirty(b);
    if (b->ctx) CGContextRelease(b->ctx);
    if (b->img) CGImageRelease(b->img);
    if (b->owned && b->bits) free(b->bits);
    delete b;
    return TRUE;
}

BOOL BitBlt(HDC dst, int dx, int dy, int w, int h, HDC src, int sx, int sy, DWORD rop)
{
    (void)rop;
    ShimDC* d = (ShimDC*)dst;
    ShimDC* s = (ShimDC*)src;
    if (!d || !s) return FALSE;
    CGImageRef img = nullptr;
    if (s->bmp) img = HswBufImage(s->bmp);
    else if (s->isWindow && s->ctx) img = HswBufImage(nullptr);
    if (!img && s->isWindow) {
        if (g_backImage) CGImageRelease(g_backImage);
        g_backImage = CGBitmapContextCreateImage(g_backCtx);
        img = g_backImage;
    }
    if (!img) return FALSE;
    CGContextRef c = d->bmp ? d->bmp->ctx : (d->isWindow ? g_backCtx : d->ctx);
    if (!c) return FALSE;
    CGContextSaveGState(c);
    CGContextTranslateCTM(c, dx, dy + h);
    CGContextScaleCTM(c, 1, -1);
    CGRect sr = CGRectMake((CGFloat)sx, (CGFloat)sy, (CGFloat)w, (CGFloat)h);
    CGImageRef sub = CGImageCreateWithImageInRect(img, sr);
    if (sub) {
        CGContextDrawImage(c, CGRectMake(0, 0, (CGFloat)w, (CGFloat)h), sub);
        CGImageRelease(sub);
    }
    CGContextRestoreGState(c);
    if (d->bmp) HswBufDirty(d->bmp);
    return TRUE;
}

BOOL StretchBlt(HDC dst, int dx, int dy, int dw, int dh,
                HDC src, int sx, int sy, int sw, int sh, DWORD rop)
{
    (void)rop;
    ShimDC* d = (ShimDC*)dst;
    ShimDC* s = (ShimDC*)src;
    if (!d || !s) return FALSE;
    CGImageRef img = s->bmp ? HswBufImage(s->bmp) : nullptr;
    if (!img && s->isWindow) {
        if (g_backImage) CGImageRelease(g_backImage);
        g_backImage = CGBitmapContextCreateImage(g_backCtx);
        img = g_backImage;
    }
    if (!img) return FALSE;
    CGContextRef c = d->bmp ? d->bmp->ctx : (d->isWindow ? g_backCtx : d->ctx);
    if (!c) return FALSE;
    CGContextSaveGState(c);
    CGContextTranslateCTM(c, dx, dy + dh);
    CGContextScaleCTM(c, 1, -1);
    CGImageRef sub = CGImageCreateWithImageInRect(img, CGRectMake(sx, sy, sw, sh));
    if (sub) {
        CGContextDrawImage(c, CGRectMake(0, 0, dw, dh), sub);
        CGImageRelease(sub);
    }
    CGContextRestoreGState(c);
    if (d->bmp) HswBufDirty(d->bmp);
    return TRUE;
}

UINT_PTR SetTimer(HWND hwnd, UINT_PTR id, UINT ms, void* proc)
{
    (void)hwnd; (void)proc;
    DBG("SetTimer id=%llu ms=%u", (unsigned long long)id, (unsigned)ms);
    for (size_t i = 0; i < g_timers.size(); ++i)
        if (g_timers[i].id == id) { g_timers[i].ms = ms; g_timers[i].next = HswNowSec() * 1000.0 + ms; return id; }
    ShimTimer t; t.id = id; t.ms = ms ? ms : 16; t.next = HswNowSec() * 1000.0 + t.ms;
    g_timers.push_back(t);
    return id;
}

BOOL KillTimer(HWND hwnd, UINT_PTR id)
{
    (void)hwnd;
    for (size_t i = 0; i < g_timers.size(); ++i)
        if (g_timers[i].id == id) { g_timers.erase(g_timers.begin() + i); return TRUE; }
    return FALSE;
}

int GetSystemMetrics(int idx)
{
    NSScreen* sc = [NSScreen mainScreen];
    NSRect f = sc ? sc.frame : NSMakeRect(0, 0, 1440, 900);
    switch (idx) {
        case SM_CXSCREEN: return (int)f.size.width;
        case SM_CYSCREEN: return (int)f.size.height;
        case SM_CYCAPTION: return 28;
        case SM_CXSIZEFRAME: return 4;
        case SM_CYSIZEFRAME: return 4;
        default: return 0;
    }
}

SHORT GetKeyState(int vk) { (void)vk; return 0; }
SHORT GetAsyncKeyState(int vk) { (void)vk; return 0; }

BOOL GetCursorPos(POINT* pt)
{
    if (!pt) return FALSE;
    NSPoint p = [NSEvent mouseLocation];
    pt->x = (LONG)p.x;
    NSScreen* sc = [NSScreen mainScreen];
    CGFloat sy = sc ? sc.frame.size.height : 900;
    pt->y = (LONG)(sy - p.y);
    return TRUE;
}

BOOL ScreenToClient(HWND hwnd, POINT* pt)
{
    (void)hwnd;
    if (!pt) return FALSE;
    if (!g_window) return TRUE;
    NSRect f = g_window.frame;
    NSScreen* sc = [NSScreen mainScreen];
    CGFloat sy = sc ? sc.frame.size.height : 900;
    pt->x -= (LONG)f.origin.x;
    pt->y -= (LONG)(sy - f.origin.y - f.size.height);
    return TRUE;
}

BOOL ClientToScreen(HWND hwnd, POINT* pt)
{
    (void)hwnd;
    if (!pt) return FALSE;
    if (!g_window) return TRUE;
    NSRect f = g_window.frame;
    NSScreen* sc = [NSScreen mainScreen];
    CGFloat sy = sc ? sc.frame.size.height : 900;
    pt->x += (LONG)f.origin.x;
    pt->y += (LONG)(sy - f.origin.y - f.size.height);
    return TRUE;
}

HCURSOR LoadCursorW(HINSTANCE inst, const wchar_t* name) { (void)inst; (void)name; return nullptr; }
HCURSOR SetCursor(HCURSOR c) { (void)c; return nullptr; }
HGDIOBJ GetStockObject(int idx) { (void)idx; return nullptr; }
BOOL TrackMouseEvent(TRACKMOUSEEVENT* tme) { (void)tme; return TRUE; }

int MultiByteToWideChar(UINT cp, DWORD flags, const char* src, int srcLen,
                        wchar_t* dst, int dstLen)
{
    (void)cp; (void)flags;
    if (!src) return 0;
    std::string s(src, srcLen > 0 ? (size_t)srcLen : strlen(src));
    std::wstring w = U2W(s);
    if (dst && dstLen > 0) {
        int n = (int)w.size() < dstLen ? (int)w.size() : dstLen;
        memcpy(dst, w.c_str(), (size_t)n * sizeof(wchar_t));
        if (n < dstLen) dst[n] = 0;
        return n;
    }
    return (int)w.size() + 1;
}

int WideCharToMultiByte(UINT cp, DWORD flags, const wchar_t* src, int srcLen,
                        char* dst, int dstLen, const char* def, BOOL* used)
{
    (void)cp; (void)flags; (void)def; (void)used;
    if (!src) return 0;
    std::wstring w(src, srcLen > 0 ? (size_t)srcLen : wcslen(src));
    std::string s = W2U(w);
    if (dst && dstLen > 0) {
        int n = (int)s.size() < dstLen ? (int)s.size() : dstLen;
        memcpy(dst, s.c_str(), (size_t)n);
        if (n < dstLen) dst[n] = 0;
        return n;
    }
    return (int)s.size() + 1;
}

// ---------------- 路径 / 文件 ----------------
static std::string g_resDir;   // .app/Contents/Resources（或 exe 同目录）
static std::string g_appData;  // ~/Library/Application Support/高中的一周

static void HswInitDirs(void)
{
    if (!g_resDir.empty()) return;
    NSString* rp = [[NSBundle mainBundle] resourcePath];
    if (rp) g_resDir = [rp UTF8String];
    if (g_resDir.empty()) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd))) g_resDir = cwd;
    }
    NSString* home = NSHomeDirectory();
    g_appData = std::string([home UTF8String]) + "/Library/Application Support/高中的一周";
    mkdir(g_appData.c_str(), 0755);
}

DWORD GetModuleFileNameW(HMODULE mod, wchar_t* buf, DWORD cap)
{
    (void)mod;
    DBG("GetModuleFileNameW");
    HswInitDirs();
    std::wstring p = U2W(g_resDir + "/高中的一周");
    if (buf && cap > 0) {
        DWORD n = (DWORD)p.size() < cap - 1 ? (DWORD)p.size() : cap - 1;
        memcpy(buf, p.c_str(), (size_t)n * sizeof(wchar_t));
        buf[n] = 0;
        return n;
    }
    return (DWORD)p.size();
}

HRESULT SHGetFolderPathW(HWND hwnd, int csidl, HANDLE token, DWORD flags, wchar_t* path)
{
    (void)hwnd; (void)csidl; (void)token; (void)flags;
    DBG("SHGetFolderPathW");
    HswInitDirs();
    std::wstring p = U2W(g_appData);
    if (path) {
        size_t n = p.size();
        memcpy(path, p.c_str(), (n + 1) * sizeof(wchar_t));
    }
    return S_OK;
}

FILE* _wfopen(const wchar_t* path, const wchar_t* mode)
{
    DBG("_wfopen %s", mode ? W2U(mode).c_str() : "?");
    if (!path) return nullptr;
    std::string p = HswPath(path);
    std::string m = mode ? W2U(mode) : "rb";
    if (m.find('b') == std::string::npos) m += "b";
    DBG("  open [%s] mode=%s", p.c_str(), m.c_str());
    FILE* fp = fopen(p.c_str(), m.c_str());
    DBG("  -> %s", fp ? "ok" : "null");
    return fp;
}

int _fseeki64(FILE* f, long long off, int whence) { return fseeko(f, (off_t)off, whence); }
long long _ftelli64(FILE* f) { return (long long)ftello(f); }

int _wcsicmp(const wchar_t* a, const wchar_t* b)
{
    if (a == b) return 0;
    if (!a) return -1;
    if (!b) return 1;
    while (*a && *b) {
        wchar_t ca = *a, cb = *b;
        if (ca >= 'A' && ca <= 'Z') ca += 32;
        if (cb >= 'A' && cb <= 'Z') cb += 32;
        if (ca != cb) return (ca < cb) ? -1 : 1;
        ++a; ++b;
    }
    return (*a == *b) ? 0 : ((*a < *b) ? -1 : 1);
}

BOOL CreateDirectoryW(const wchar_t* path, void* sa)
{
    (void)sa;
    DBG("CreateDirectoryW");
    if (!path) return FALSE;
    std::string p = HswPath(path);
    mkdir(p.c_str(), 0755);
    struct stat st;
    return (stat(p.c_str(), &st) == 0) ? TRUE : FALSE;
}

int SHCreateDirectoryExW(HWND hwnd, const wchar_t* path, void* sec)
{
    (void)hwnd; (void)sec;
    return CreateDirectoryW(path, nullptr) ? 0 : 1;
}

DWORD GetFileAttributesW(const wchar_t* path)
{
    DBG("GetFileAttributesW");
    if (!path) return 0xFFFFFFFFu;
    struct stat st;
    if (stat(HswPath(path).c_str(), &st) != 0) return 0xFFFFFFFFu;
    return S_ISDIR(st.st_mode) ? FILE_ATTRIBUTE_DIRECTORY : 0x00000020u;
}

BOOL SetFileAttributesW(const wchar_t* path, DWORD attr)
{ (void)path; (void)attr; return TRUE; }

BOOL CopyFileW(const wchar_t* src, const wchar_t* dst, BOOL failIfExists)
{
    DBG("CopyFileW");
    if (!src || !dst) return FALSE;
    std::string s = HswPath(src), d = HswPath(dst);
    if (failIfExists) { struct stat st; if (stat(d.c_str(), &st) == 0) return FALSE; }
    FILE* fi = fopen(s.c_str(), "rb");
    if (!fi) return FALSE;
    FILE* fo = fopen(d.c_str(), "wb");
    if (!fo) { fclose(fi); return FALSE; }
    char buf[65536];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), fi)) > 0) fwrite(buf, 1, n, fo);
    fclose(fi); fclose(fo);
    return TRUE;
}

BOOL DeleteFileW(const wchar_t* path)
{
    if (!path) return FALSE;
    return unlink(HswPath(path).c_str()) == 0 ? TRUE : FALSE;
}

BOOL PathFileExistsW(const wchar_t* path)
{
    DBG("PathFileExistsW");
    if (!path) return FALSE;
    struct stat st;
    return stat(HswPath(path).c_str(), &st) == 0 ? TRUE : FALSE;
}

// ---- 文件遍历 ----
struct ShimFind {
    std::string dir;
    std::string ext;
    std::vector<std::string> names;
    size_t idx;
};

HANDLE FindFirstFileW(const wchar_t* pattern, WIN32_FIND_DATAW* fd)
{
    DBG("FindFirstFileW");
    if (!pattern || !fd) return INVALID_HANDLE_VALUE;
    std::string p = HswPath(pattern);
    size_t slash = p.find_last_of('/');
    std::string dir = (slash == std::string::npos) ? "." : p.substr(0, slash);
    std::string name = (slash == std::string::npos) ? p : p.substr(slash + 1);
    ShimFind* f = new ShimFind();
    f->dir = dir;
    f->ext = (name.size() > 1 && name[0] == '*') ? name.substr(1) : "";
    f->idx = 0;
    DIR* d = opendir(dir.c_str());
    if (d) {
        struct dirent* e;
        while ((e = readdir(d)) != nullptr) {
            std::string fn = e->d_name;
            if (fn == "." || fn == "..") continue;
            if (!f->ext.empty()) {
                if (fn.size() < f->ext.size()) continue;
                if (fn.compare(fn.size() - f->ext.size(), f->ext.size(), f->ext) != 0) continue;
            }
            f->names.push_back(fn);
        }
        closedir(d);
    }
    if (f->names.empty()) { delete f; return INVALID_HANDLE_VALUE; }
    std::wstring wn = U2W(f->names[0]);
    memset(fd, 0, sizeof(*fd));
    size_t n = wn.size() < MAX_PATH - 1 ? wn.size() : MAX_PATH - 1;
    memcpy(fd->cFileName, wn.c_str(), n * sizeof(wchar_t));
    fd->cFileName[n] = 0;
    f->idx = 1;
    return (HANDLE)f;
}

BOOL FindNextFileW(HANDLE h, WIN32_FIND_DATAW* fd)
{
    ShimFind* f = (ShimFind*)h;
    if (!f || !fd || f->idx >= f->names.size()) return FALSE;
    std::wstring wn = U2W(f->names[f->idx++]);
    memset(fd, 0, sizeof(*fd));
    size_t n = wn.size() < MAX_PATH - 1 ? wn.size() : MAX_PATH - 1;
    memcpy(fd->cFileName, wn.c_str(), n * sizeof(wchar_t));
    fd->cFileName[n] = 0;
    return TRUE;
}

BOOL FindClose(HANDLE h) { delete (ShimFind*)h; return TRUE; }

// ---- 资源（macOS 没有内嵌资源）----
HRSRC   HswFindResource(HMODULE mod, const wchar_t* name, const wchar_t* type)
{ (void)mod; (void)name; (void)type; return nullptr; }
HGLOBAL HswLoadResource(HMODULE mod, HRSRC res) { (void)mod; (void)res; return nullptr; }
void*   HswLockResource(HGLOBAL g) { (void)g; return nullptr; }
DWORD   HswSizeofResource(HMODULE mod, HRSRC res) { (void)mod; (void)res; return 0; }

long HswMFStartup(unsigned long version, unsigned long flags) { (void)version; (void)flags; return 0; }
long HswMFShutdown(void) { return 0; }

HMODULE LoadLibraryW(const wchar_t* name) { (void)name; return nullptr; }
void*   GetProcAddress(HMODULE mod, const char* name) { (void)mod; (void)name; return nullptr; }
BOOL    FreeLibrary(HMODULE mod) { (void)mod; return TRUE; }

HMONITOR MonitorFromWindow(HWND hwnd, DWORD flags) { (void)hwnd; (void)flags; return nullptr; }
BOOL GetMonitorInfoW(HMONITOR mon, MONITORINFO* mi)
{
    (void)mon;
    if (!mi) return FALSE;
    NSScreen* sc = [NSScreen mainScreen];
    NSRect f = sc ? sc.frame : NSMakeRect(0, 0, 1440, 900);
    NSRect v = sc ? sc.visibleFrame : f;
    CGFloat sy = f.size.height;
    mi->rcMonitor = RECT{ 0, 0, (LONG)f.size.width, (LONG)f.size.height };
    mi->rcWork = RECT{ (LONG)v.origin.x, (LONG)(sy - v.origin.y - v.size.height),
                       (LONG)(v.origin.x + v.size.width), (LONG)(sy - v.origin.y) };
    mi->dwFlags = 0;
    return TRUE;
}

BOOL AdjustWindowRect(RECT* rc, DWORD style, BOOL menu)
{
    (void)style; (void)menu;
    if (!rc) return FALSE;
    rc->left -= 0; rc->right += 0; rc->top -= 28; rc->bottom += 0;
    return TRUE;
}

BOOL GetWindowPlacement(HWND hwnd, void* wp)
{
    (void)hwnd;
    if (!wp) return FALSE;
    memset(wp, 0, 128);
    return TRUE;
}
BOOL SetWindowPlacement(HWND hwnd, const void* wp) { (void)hwnd; (void)wp; return TRUE; }
BOOL SetLayeredWindowAttributes(HWND hwnd, DWORD key, BYTE alpha, DWORD flags)
{ (void)hwnd; (void)key; (void)alpha; (void)flags; return TRUE; }

void HswFatal(const char* msg)
{
    if (msg) fprintf(stderr, "HswFatal: %s\n", msg);
    exit(1);
}

// ---------------- 音频 ----------------
static void HswAudEnsure(void)
{
    if (!g_audio) g_audio = [[HswAudio alloc] init];
}

bool HswAudLoadFile(const wchar_t* path, double maxSec)
{
    DBG("HswAudLoadFile");
    (void)maxSec;
    HswAudEnsure();
    g_audioDur = 0.0;
    if (!path) return false;
    NSString* p = [NSString stringWithUTF8String:W2U(path).c_str()];
    NSURL* u = [NSURL fileURLWithPath:p];
    NSError* err = nil;
    AVAudioPlayer* pl = [[AVAudioPlayer alloc] initWithContentsOfURL:u error:&err];
    if (!pl) return false;
    [pl prepareToPlay];
    g_audio.music = pl;
    g_audioDur = pl.duration;
    return true;
}

bool HswAudLoadMem(const void* data, unsigned long long bytes, double maxSec)
{
    (void)maxSec;
    HswAudEnsure();
    g_audioDur = 0.0;
    if (!data || bytes == 0) return false;
    NSData* d = [NSData dataWithBytes:data length:(NSUInteger)bytes];
    NSError* err = nil;
    AVAudioPlayer* pl = [[AVAudioPlayer alloc] initWithData:d error:&err];
    if (!pl) return false;
    [pl prepareToPlay];
    g_audio.music = pl;
    g_audioDur = pl.duration;
    return true;
}

void HswAudPlay(void)   { if (g_audio && g_audio.music) [g_audio.music play]; }
void HswAudPause(void)  { if (g_audio && g_audio.music) [g_audio.music pause]; }
void HswAudStop(void)   { if (g_audio && g_audio.music) { [g_audio.music stop]; g_audio.music.currentTime = 0; } }
double HswAudPos(void)  { return (g_audio && g_audio.music) ? g_audio.music.currentTime : -1.0; }
double HswAudDur(void)  { return g_audioDur; }

BOOL PlaySoundA(const char* mem, HMODULE mod, DWORD flags)
{
    (void)mod;
    if (!mem || !(flags & SND_MEMORY)) return FALSE;
    // 内存 WAV：前 44 字节是 WAV 头，整块交给 AVAudioPlayer
    // 这里假定调用方传的是完整 WAV 内存块，长度用头里的字段推不出来，
    // 所以用一个足够大的上界（调用方只用来放很短的音效）。
    HswAudEnsure();
    const unsigned char* p = (const unsigned char*)mem;
    unsigned long long riff = 0;
    if (p[0] == 'R' && p[1] == 'I' && p[2] == 'F' && p[3] == 'F') {
        riff = (unsigned long long)p[4] | ((unsigned long long)p[5] << 8) |
               ((unsigned long long)p[6] << 16) | ((unsigned long long)p[7] << 24);
    }
    unsigned long long total = riff ? riff + 8 : (1u << 20);
    NSData* d = [NSData dataWithBytes:mem length:(NSUInteger)total];
    NSError* err = nil;
    AVAudioPlayer* pl = [[AVAudioPlayer alloc] initWithData:d error:&err];
    if (!pl) return FALSE;
    [pl setVolume:0.6f];
    [pl play];
    [g_audio.sfx addObject:pl];
    if (g_audio.sfx.count > 12) [g_audio.sfx removeObjectAtIndex:0];
    return TRUE;
}

BOOL PlaySoundW(const wchar_t* file, HMODULE mod, DWORD flags)
{
    (void)mod;
    if (!file || !(flags & SND_FILENAME)) return FALSE;
    HswAudEnsure();
    NSString* p = [NSString stringWithUTF8String:W2U(file).c_str()];
    NSURL* u = [NSURL fileURLWithPath:p];
    NSError* err = nil;
    AVAudioPlayer* pl = [[AVAudioPlayer alloc] initWithContentsOfURL:u error:&err];
    if (!pl) return FALSE;
    [pl play];
    [g_audio.sfx addObject:pl];
    if (g_audio.sfx.count > 12) [g_audio.sfx removeObjectAtIndex:0];
    return TRUE;
}

} // extern "C"

// ==========================================================================
//  第 4 部分：GDI+ → CoreGraphics
// ==========================================================================

// ---- 字体名映射 ----
static std::string g_kaitiName;   // 行楷（随包 or 系统）
static std::string g_heiName;     // 黑体

const char* HswMapFontUtf8(const char* winName)
{
    if (!winName) return nullptr;
    static std::string out;
    std::string n = winName;
    auto has = [&](const char* k) { return n.find(k) != std::string::npos; };

    if (has("YaHei") || has("雅黑") || has("SimHei") || has("黑体") ||
        has("SimSun") || has("宋体") || has("PingFang") || has("苹方")) {
        out = g_heiName.empty() ? std::string("PingFang SC") : g_heiName;
        return out.c_str();
    }
    if (has("行楷") || has("Xingkai") || has("KaiTi") || has("楷体") ||
        has("STKaiti") || has("WenKai") || has("霞鹜")) {
        out = g_kaitiName.empty() ? std::string("Songti SC") : g_kaitiName;
        return out.c_str();
    }
    if (has("Emoji") || has("emoji")) { out = "Apple Color Emoji"; return out.c_str(); }
    out = n;
    return out.c_str();
}

// macOS 的 CTFontCreateWithName 在字体不存在时会去问字体服务（离线机器上能卡死几十秒），
// 所以先拿一份「系统装了哪些字体家族」的清单，按清单挑，挑不到就用苹方兜底。
static CFArrayRef g_famNames = nullptr;

static bool HswFamExists(const std::string& nm)
{
    if (!g_famNames) g_famNames = CTFontManagerCopyAvailableFontFamilyNames();
    if (!g_famNames) return false;
    NSString* want = [NSString stringWithUTF8String:nm.c_str()];
    if (!want) return false;
    CFIndex n = CFArrayGetCount(g_famNames);
    for (CFIndex i = 0; i < n; ++i) {
        NSString* s = (__bridge NSString*)CFArrayGetValueAtIndex(g_famNames, i);
        if ([s caseInsensitiveCompare:want] == NSOrderedSame) return true;
    }
    return false;
}

static std::string HswPickFont(const std::vector<std::string>& cands, const char* dflt)
{
    for (size_t i = 0; i < cands.size(); ++i)
        if (!cands[i].empty() && HswFamExists(cands[i])) return cands[i];
    return std::string(dflt);
}

static void HswRegisterBundledFonts(void)
{
    HswInitDirs();
    NSString* rp = [NSString stringWithUTF8String:g_resDir.c_str()];
    NSString* fd = [rp stringByAppendingPathComponent:@"fonts"];
    NSFileManager* fm = [NSFileManager defaultManager];
    NSArray* items = [fm contentsOfDirectoryAtPath:fd error:nil];
    if (!items) return;
    for (NSString* it in items) {
        NSString* low = [it lowercaseString];
        if (!([low hasSuffix:@".ttf"] || [low hasSuffix:@".otf"] || [low hasSuffix:@".ttc"])) continue;
        NSURL* u = [NSURL fileURLWithPath:[fd stringByAppendingPathComponent:it]];
        CFErrorRef err = nullptr;
        bool ok = CTFontManagerRegisterFontsForURL((__bridge CFURLRef)u, kCTFontManagerScopeProcess, &err);
        if (ok) {
            if ([low hasSuffix:@".ttf"] || [low hasSuffix:@".otf"]) {
                CGDataProviderRef dp = CGDataProviderCreateWithURL((__bridge CFURLRef)u);
                CGFontRef cgf = CGFontCreateWithDataProvider(dp);
                if (cgf) {
                    NSString* full = (__bridge_transfer NSString*)CGFontCopyFullName(cgf);
                    std::string s = full ? [full UTF8String] : "";
                    if (!s.empty()) {
                        if (s.find("WenKai") != std::string::npos ||
                            s.find("文楷") != std::string::npos ||
                            s.find("LXGW") != std::string::npos) g_kaitiName = s;
                        else if (s.find("Source Han Sans") != std::string::npos ||
                                 s.find("Noto Sans") != std::string::npos ||
                                 s.find("思源") != std::string::npos) g_heiName = s;
                    }
                    CGFontRelease(cgf);
                }
                if (dp) CGDataProviderRelease(dp);
            }
        }
        if (err) CFRelease(err);
    }
    if (!g_kaitiName.empty() && !HswFamExists(g_kaitiName)) g_kaitiName.clear();
    if (!g_heiName.empty() && !HswFamExists(g_heiName)) g_heiName.clear();
    g_kaitiName = HswPickFont({ g_kaitiName, "LXGW WenKai", "Kaiti SC", "STKaiti",
                                "Songti SC", "STSong", "PingFang SC" }, "PingFang SC");
    g_heiName   = HswPickFont({ g_heiName, "PingFang SC", "Hiragino Sans GB",
                                "Heiti SC", "Helvetica" }, "Helvetica");
    fprintf(stderr, "[shim] fonts: kaiti=%s hei=%s\n", g_kaitiName.c_str(), g_heiName.c_str());
    fflush(stderr);
}

// ---- 内部结构体 ----
struct ShimPath {
    CGMutablePathRef p = nullptr;
    FillMode mode = FillModeAlternate;
};

struct ShimGfx {
    CGContextRef ctx = nullptr;
    void* buf = nullptr;       // ShimImg*（Bitmap）或 ShimBuf*（DIB）
    bool  bufIsImg = false;
    int   bw = 0, bh = 0;      // 目标尺寸（ResetTransform / Clear 用）
    int depth = 0;
};

struct ShimImg {
    unsigned int magic = HSW_MAGIC_IMG;
    CGImageRef   cg = nullptr;      // 加载的图 / 由 ctx 生成
    CGContextRef ctx = nullptr;     // 可写的 BGRA 上下文
    unsigned char* bits = nullptr;
    int w = 0, h = 0, stride = 0;
    bool owned = false;
    bool dirty = true;              // ctx 被改过 → cg 要重建
};

// 画完之后打脏标记（否则缓存的 CGImage 不会更新）
static void HswTouch(ShimGfx* g)
{
    if (!g || !g->buf) return;
    if (g->bufIsImg) ((ShimImg*)g->buf)->dirty = true;
    else             ((ShimBuf*)g->buf)->dirty = true;
}

struct ShimGrad {
    CGGradientRef g = nullptr;
    CGPoint p0, p1;
    bool radial = false;
    PointF center;
    REAL radius = 0;
    Color c0, c1;
    bool valid = false;
};

static CGContextRef ImgCtx(ShimImg* im)
{
    if (!im) return nullptr;
    if (!im->ctx) {
        if (im->w <= 0 || im->h <= 0) return nullptr;
        if (!g_cs) g_cs = CGColorSpaceCreateDeviceRGB();
        im->stride = im->w * 4;
        im->bits = (unsigned char*)calloc((size_t)im->stride * (size_t)im->h, 1);
        im->owned = true;
        im->ctx = CGBitmapContextCreate(im->bits, im->w, im->h, 8, im->stride, g_cs,
                                        kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little);
        if (!im->ctx) return nullptr;
        CGContextTranslateCTM(im->ctx, 0, im->h);
        CGContextScaleCTM(im->ctx, 1, -1);
        if (im->cg) {
            CGContextSaveGState(im->ctx);
            CGContextTranslateCTM(im->ctx, 0, im->h);
            CGContextScaleCTM(im->ctx, 1, -1);
            CGContextDrawImage(im->ctx, CGRectMake(0, 0, im->w, im->h), im->cg);
            CGContextRestoreGState(im->ctx);
        }
        im->dirty = false;      // 缓冲和 cg 此时是一致的
    }
    return im->ctx;
}

static void ImgInvalidate(ShimImg* im)
{
    if (!im) return;
    if (im->cg) { CGImageRelease(im->cg); im->cg = nullptr; }
    im->dirty = true;
}

static CGImageRef ImgCG(ShimImg* im)
{
    if (!im) return nullptr;
    // 只有「自己带像素缓冲」的位图才需要按 dirty 重建；
    // 从文件加载的图只有 cg，没有 ctx，绝不能因为 dirty 把它扔掉。
    if (im->ctx) {
        if (!im->cg || im->dirty) {
            if (im->cg) { CGImageRelease(im->cg); im->cg = nullptr; }
            im->cg = CGBitmapContextCreateImage(im->ctx);
            im->dirty = false;
        }
    }
    return im->cg;
}

// ---- Brush ----（构造 / 析构已在 gshim.h 内联给出）

// SolidBrush：impl 直接存 Color 的堆拷贝
SolidBrush::SolidBrush() { impl = new Color(255, 255, 255, 255); col = Color(255, 255, 255, 255); }
SolidBrush::SolidBrush(const Color& c) { impl = new Color(c); col = c; }
SolidBrush::~SolidBrush() { delete (Color*)impl; impl = nullptr; }
Status SolidBrush::GetColor(Color* c) const { if (c) *c = *(Color*)impl; return Ok; }
Status SolidBrush::SetColor(const Color& c) { *(Color*)impl = c; col = c; return Ok; }

static CGGradientRef MakeGrad(const Color& c0, const Color& c1)
{
    if (!g_cs) g_cs = CGColorSpaceCreateDeviceRGB();
    CGFloat comps[8] = {
        c0.GetR() / 255.0f, c0.GetG() / 255.0f, c0.GetB() / 255.0f, c0.GetA() / 255.0f,
        c1.GetR() / 255.0f, c1.GetG() / 255.0f, c1.GetB() / 255.0f, c1.GetA() / 255.0f
    };
    CGFloat locs[2] = { 0.0f, 1.0f };
    return CGGradientCreateWithColorComponents(g_cs, comps, locs, 2);
}

LinearGradientBrush::LinearGradientBrush()
{
    impl = new ShimGrad();
    ShimGrad* s = (ShimGrad*)impl;
    s->p0 = CGPointMake(0, 0); s->p1 = CGPointMake(0, 1);
    s->c0 = Color(255, 255, 255, 255); s->c1 = Color(255, 0, 0, 0);
    s->g = MakeGrad(s->c0, s->c1); s->valid = true;
}
LinearGradientBrush::LinearGradientBrush(const PointF& p0, const PointF& p1,
                                         const Color& c0, const Color& c1)
{
    impl = new ShimGrad();
    ShimGrad* s = (ShimGrad*)impl;
    s->p0 = CGPointMake(p0.X, p0.Y); s->p1 = CGPointMake(p1.X, p1.Y);
    s->c0 = c0; s->c1 = c1;
    s->g = MakeGrad(c0, c1); s->valid = true;
}
LinearGradientBrush::LinearGradientBrush(const HswPoint& p0, const HswPoint& p1,
                                         const Color& c0, const Color& c1)
{
    impl = new ShimGrad();
    ShimGrad* s = (ShimGrad*)impl;
    s->p0 = CGPointMake(p0.X, p0.Y); s->p1 = CGPointMake(p1.X, p1.Y);
    s->c0 = c0; s->c1 = c1;
    s->g = MakeGrad(c0, c1); s->valid = true;
}
LinearGradientBrush::LinearGradientBrush(const RectF& r, const Color& c0, const Color& c1,
                                         LinearGradientMode mode)
{
    impl = new ShimGrad();
    ShimGrad* s = (ShimGrad*)impl;
    switch (mode) {
        case LinearGradientModeHorizontal:
            s->p0 = CGPointMake(r.X, r.Y); s->p1 = CGPointMake(r.X + r.Width, r.Y); break;
        case LinearGradientModeForwardDiagonal:
            s->p0 = CGPointMake(r.X, r.Y); s->p1 = CGPointMake(r.X + r.Width, r.Y + r.Height); break;
        case LinearGradientModeBackwardDiagonal:
            s->p0 = CGPointMake(r.X + r.Width, r.Y); s->p1 = CGPointMake(r.X, r.Y + r.Height); break;
        default:
            s->p0 = CGPointMake(r.X, r.Y); s->p1 = CGPointMake(r.X, r.Y + r.Height); break;
    }
    s->c0 = c0; s->c1 = c1;
    s->g = MakeGrad(c0, c1); s->valid = true;
}
LinearGradientBrush::LinearGradientBrush(const RectF& r, const Color& c0, const Color& c1,
                                         REAL angle, BOOL isAngleScalable)
{
    (void)isAngleScalable;
    impl = new ShimGrad();
    ShimGrad* s = (ShimGrad*)impl;
    double a = angle * M_PI / 180.0;
    double cx = r.X + r.Width * 0.5, cy = r.Y + r.Height * 0.5;
    double L = r.Width * 0.5 * fabs(cos(a)) + r.Height * 0.5 * fabs(sin(a));
    s->p0 = CGPointMake(cx - cos(a) * L, cy - sin(a) * L);
    s->p1 = CGPointMake(cx + cos(a) * L, cy + sin(a) * L);
    s->c0 = c0; s->c1 = c1;
    s->g = MakeGrad(c0, c1); s->valid = true;
}
LinearGradientBrush::~LinearGradientBrush()
{
    ShimGrad* s = (ShimGrad*)impl;
    if (s) { if (s->g) CGGradientRelease(s->g); delete s; }
    impl = nullptr;
}
Status LinearGradientBrush::SetWrapMode(int wrap) { (void)wrap; return Ok; }
Status LinearGradientBrush::SetBlend(const REAL* f, const REAL* p, int n) { (void)f; (void)p; (void)n; return Ok; }
Status LinearGradientBrush::SetGammaCorrection(BOOL on) { (void)on; return Ok; }

PathGradientBrush::PathGradientBrush() { impl = new ShimGrad(); }
PathGradientBrush::PathGradientBrush(const PointF* pts, int n)
{
    impl = new ShimGrad();
    ShimGrad* s = (ShimGrad*)impl;
    double minx = 1e18, miny = 1e18, maxx = -1e18, maxy = -1e18;
    for (int i = 0; i < n; ++i) {
        minx = std::min<double>(minx, pts[i].X); maxx = std::max<double>(maxx, pts[i].X);
        miny = std::min<double>(miny, pts[i].Y); maxy = std::max<double>(maxy, pts[i].Y);
    }
    s->radial = true;
    s->center = PointF((REAL)((minx + maxx) * 0.5), (REAL)((miny + maxy) * 0.5));
    s->radius = (REAL)std::max<double>(maxx - minx, maxy - miny) * 0.5f;
    if (s->radius <= 0) s->radius = 1;
    s->c0 = Color(255, 255, 255, 255); s->c1 = Color(0, 0, 0, 0);
    s->g = MakeGrad(s->c0, s->c1); s->valid = true;
}
PathGradientBrush::PathGradientBrush(const GraphicsPath* p)
{
    impl = new ShimGrad();
    ShimGrad* s = (ShimGrad*)impl;
    RectF b; b.X = 0; b.Y = 0; b.Width = 0; b.Height = 0;
    if (p) p->GetBounds(&b);
    s->radial = true;
    s->center = PointF(b.X + b.Width * 0.5f, b.Y + b.Height * 0.5f);
    s->radius = std::max(b.Width, b.Height) * 0.5f;
    if (s->radius <= 0) s->radius = 1;
    s->c0 = Color(255, 255, 255, 255); s->c1 = Color(0, 0, 0, 0);
    s->g = MakeGrad(s->c0, s->c1); s->valid = true;
}
PathGradientBrush::~PathGradientBrush()
{
    ShimGrad* s = (ShimGrad*)impl;
    if (s) { if (s->g) CGGradientRelease(s->g); delete s; }
    impl = nullptr;
}
Status PathGradientBrush::SetCenterColor(const Color& c)
{
    ShimGrad* s = (ShimGrad*)impl;
    if (!s) return InvalidParameter;
    s->c0 = c;
    if (s->g) CGGradientRelease(s->g);
    s->g = MakeGrad(s->c0, s->c1);
    return Ok;
}
Status PathGradientBrush::SetSurroundColors(const Color* cols, int* count)
{
    ShimGrad* s = (ShimGrad*)impl;
    if (!s || !cols || !count || *count < 1) return InvalidParameter;
    s->c1 = cols[0];
    if (s->g) CGGradientRelease(s->g);
    s->g = MakeGrad(s->c0, s->c1);
    return Ok;
}
Status PathGradientBrush::SetCenterPoint(const PointF& p)
{
    ShimGrad* s = (ShimGrad*)impl;
    if (s) s->center = p;
    return Ok;
}
Status PathGradientBrush::SetFocusScales(REAL x, REAL y) { (void)x; (void)y; return Ok; }

// ---- Pen ----
struct ShimPen {
    Color col{ 255, 0, 0, 0 };
    REAL  width = 1.0f;
    LineJoin join = LineJoinMiter;
    LineCap  start = LineCapFlat, end = LineCapFlat;
    DashCap  dash = DashCapFlat;
    DashStyle style = DashStyleSolid;
    int  align = 0;
};

Pen::Pen() { impl = new ShimPen(); col = Color(255, 0, 0, 0); width = 1.0f; }
Pen::Pen(const Color& c, REAL w)
{
    impl = new ShimPen();
    ShimPen* s = (ShimPen*)impl;
    s->col = c; s->width = w;
    col = c; width = w;
}
Pen::~Pen() { delete (ShimPen*)impl; impl = nullptr; }
Status Pen::SetColor(const Color& c) { ShimPen* s = (ShimPen*)impl; if (s) s->col = c; col = c; return Ok; }
Status Pen::SetWidth(REAL w) { ShimPen* s = (ShimPen*)impl; if (s) s->width = w; width = w; return Ok; }
Status Pen::SetLineJoin(LineJoin j) { ShimPen* s = (ShimPen*)impl; if (s) s->join = j; return Ok; }
Status Pen::SetStartCap(LineCap c) { ShimPen* s = (ShimPen*)impl; if (s) s->start = c; return Ok; }
Status Pen::SetEndCap(LineCap c) { ShimPen* s = (ShimPen*)impl; if (s) s->end = c; return Ok; }
Status Pen::SetLineCap(LineCap a, LineCap b, DashCap d)
{
    ShimPen* s = (ShimPen*)impl;
    if (s) { s->start = a; s->end = b; s->dash = d; }
    return Ok;
}
Status Pen::SetDashStyle(DashStyle d) { ShimPen* s = (ShimPen*)impl; if (s) s->style = d; return Ok; }
Status Pen::SetAlignment(int a) { ShimPen* s = (ShimPen*)impl; if (s) s->align = a; return Ok; }

// ---- FontFamily / Font ----
struct ShimFam { std::string name; CTFontDescriptorRef desc = nullptr; };

FontFamily::FontFamily() { impl = new ShimFam(); name = L"PingFang SC"; }
FontFamily::FontFamily(const wchar_t* nm)
{
    impl = new ShimFam();
    name = nm ? nm : L"PingFang SC";
    const char* mapped = HswMapFontUtf8(W2U(name).c_str());
    ((ShimFam*)impl)->name = mapped ? mapped : "PingFang SC";
    DBG("FontFamily ctor -> %s", ((ShimFam*)impl)->name.c_str());
}
FontFamily::~FontFamily() { delete (ShimFam*)impl; impl = nullptr; }
BOOL FontFamily::IsAvailable() const
{
    ShimFam* f = (ShimFam*)impl;
    if (!f) return FALSE;
    bool ok = HswFamExists(f->name);
    DBG("IsAvailable [%s] -> %d", f->name.c_str(), ok ? 1 : 0);
    return ok ? TRUE : FALSE;
}
Status FontFamily::GetFamilyName(wchar_t* buf, int n) const
{
    if (!buf || n <= 0) return InvalidParameter;
    std::wstring w = U2W(((ShimFam*)impl)->name);
    int k = (int)w.size() < n - 1 ? (int)w.size() : n - 1;
    memcpy(buf, w.c_str(), (size_t)k * sizeof(wchar_t));
    buf[k] = 0;
    return Ok;
}

struct ShimFont {
    std::string fam;
    REAL size = 12;
    int  style = FontStyleRegular;
    CTFontRef ct = nullptr;
};

static CTFontRef MakeCTFont(const std::string& fam, REAL size, int style)
{
    std::string pick = HswPickFont({ fam, "PingFang SC", "Helvetica" }, "Helvetica");
    NSString* nm = [NSString stringWithUTF8String:pick.c_str()];
    CTFontRef base = CTFontCreateWithName((CFStringRef)nm, (CGFloat)size, nullptr);
    if (!base) return nullptr;
    CTFontSymbolicTraits tr = 0;
    if (style & FontStyleBold) tr |= kCTFontBoldTrait;
    if (style & FontStyleItalic) tr |= kCTFontItalicTrait;
    if (tr == 0) return base;
    CTFontRef d = CTFontCreateCopyWithSymbolicTraits(base, (CGFloat)size, nullptr, tr, tr);
    if (d) { CFRelease(base); return d; }
    return base;
}

Font::Font() { impl = new ShimFont(); }
Font::Font(const FontFamily* fam, REAL size, int style, Unit unit)
{
    (void)unit;
    impl = new ShimFont();
    ShimFont* f = (ShimFont*)impl;
    f->fam = fam ? ((ShimFam*)fam->impl)->name : std::string("PingFang SC");
    f->size = size; f->style = style;
    f->ct = MakeCTFont(f->fam, size, style);
}
Font::Font(const wchar_t* famName, REAL size, int style, Unit unit)
{
    (void)unit;
    impl = new ShimFont();
    ShimFont* f = (ShimFont*)impl;
    const char* mapped = HswMapFontUtf8(W2U(famName ? famName : L"PingFang SC").c_str());
    f->fam = mapped ? mapped : "PingFang SC";
    f->size = size; f->style = style;
    f->ct = MakeCTFont(f->fam, size, style);
}
Font::~Font() { ShimFont* f = (ShimFont*)impl; if (f && f->ct) CFRelease(f->ct); delete (ShimFont*)impl; impl = nullptr; }
REAL Font::GetSize() const { ShimFont* f = (ShimFont*)impl; return f ? f->size : 12; }
int  Font::GetStyle() const { ShimFont* f = (ShimFont*)impl; return f ? f->style : FontStyleRegular; }

// ---- StringFormat ----
struct ShimFmt { StringAlignment align = StringAlignmentNear; StringAlignment line = StringAlignmentNear; int flags = 0; };
StringFormat::StringFormat() { impl = new ShimFmt(); }
StringFormat::StringFormat(int f) { impl = new ShimFmt(); ((ShimFmt*)impl)->flags = f; }
StringFormat::~StringFormat() { delete (ShimFmt*)impl; impl = nullptr; }
Status StringFormat::SetAlignment(StringAlignment a) { ShimFmt* f = (ShimFmt*)impl; if (f) f->align = a; return Ok; }
Status StringFormat::SetLineAlignment(StringAlignment a) { ShimFmt* f = (ShimFmt*)impl; if (f) f->line = a; return Ok; }
Status StringFormat::SetFormatFlags(int fl) { ShimFmt* f = (ShimFmt*)impl; if (f) f->flags = fl; return Ok; }

// ---- Matrix ----
struct ShimMat { CGAffineTransform m = CGAffineTransformIdentity; };
Matrix::Matrix() { impl = new ShimMat(); }
Matrix::Matrix(REAL a, REAL b, REAL c, REAL d, REAL dx, REAL dy)
{ impl = new ShimMat(); ((ShimMat*)impl)->m = CGAffineTransformMake(a, b, c, d, dx, dy); }
Matrix::~Matrix() { delete (ShimMat*)impl; impl = nullptr; }
Status Matrix::Shear(REAL sx, REAL sy)
{
    ShimMat* s = (ShimMat*)impl; if (!s) return InvalidParameter;
    CGAffineTransform sh = CGAffineTransformMake(1, sy, sx, 1, 0, 0);
    s->m = CGAffineTransformConcat(sh, s->m);
    return Ok;
}
Status Matrix::Translate(REAL dx, REAL dy)
{
    ShimMat* s = (ShimMat*)impl; if (!s) return InvalidParameter;
    s->m = CGAffineTransformTranslate(s->m, dx, dy);
    return Ok;
}
Status Matrix::Rotate(REAL angle)
{
    ShimMat* s = (ShimMat*)impl; if (!s) return InvalidParameter;
    s->m = CGAffineTransformRotate(s->m, angle * M_PI / 180.0);
    return Ok;
}
Status Matrix::Scale(REAL sx, REAL sy)
{
    ShimMat* s = (ShimMat*)impl; if (!s) return InvalidParameter;
    s->m = CGAffineTransformScale(s->m, sx, sy);
    return Ok;
}
Status Matrix::Reset() { ShimMat* s = (ShimMat*)impl; if (s) s->m = CGAffineTransformIdentity; return Ok; }

// ---- ImageAttributes ----
struct ShimAttr { ColorMatrix cm; bool hasCm = false; };
ImageAttributes::ImageAttributes() { impl = new ShimAttr(); }
ImageAttributes::~ImageAttributes() { delete (ShimAttr*)impl; impl = nullptr; }
Status ImageAttributes::SetColorMatrix(const ColorMatrix* cm)
{ ShimAttr* a = (ShimAttr*)impl; if (a && cm) { a->cm = *cm; a->hasCm = true; } return Ok; }
Status ImageAttributes::SetColorMatrix(const ColorMatrix* cm, ColorMatrixFlags f, ColorAdjustType t)
{ (void)f; (void)t; return SetColorMatrix(cm); }
Status ImageAttributes::SetWrapMode(int w) { (void)w; return Ok; }

// ---- Image / Bitmap ----
unsigned int Image::GetWidth() const { return 0; }
unsigned int Image::GetHeight() const { return 0; }

Bitmap::Bitmap(int width, int height, PixelFormat format)
{
    (void)format;
    impl = new ShimImg();
    ShimImg* im = (ShimImg*)impl;
    im->w = width; im->h = height;
    w = width; h = height; pf = PixelFormat32bppARGB; bits = nullptr;
    lastStatus = Ok;
    ImgCtx(im);
}
Bitmap::Bitmap(int width, int height) : Bitmap(width, height, PixelFormat32bppARGB) {}

static ShimImg* HswLoadImageFile(const wchar_t* file)
{
    if (!file) return nullptr;
    std::string p = HswPath(file);
    DBG("LoadImage [%s]", p.c_str());
    NSString* np = [NSString stringWithUTF8String:p.c_str()];
    NSURL* u = [NSURL fileURLWithPath:np];
    CGImageSourceRef src = CGImageSourceCreateWithURL((__bridge CFURLRef)u, nullptr);
    if (!src) {
        // 兜底：先读文件到内存
        FILE* f = fopen(p.c_str(), "rb");
        if (!f) return nullptr;
        fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
        if (sz <= 0) { fclose(f); return nullptr; }
        unsigned char* buf = (unsigned char*)malloc((size_t)sz);
        size_t got = fread(buf, 1, (size_t)sz, f);
        fclose(f);
        CFDataRef d = CFDataCreate(nullptr, buf, (CFIndex)got);
        free(buf);
        src = CGImageSourceCreateWithData(d, nullptr);
        CFRelease(d);
        if (!src) return nullptr;
    }
    CGImageRef cg = CGImageSourceCreateImageAtIndex(src, 0, nullptr);
    CFRelease(src);
    if (!cg) { DBG("  -> 解不出图"); return nullptr; }
    ShimImg* im = new ShimImg();
    im->cg = cg;
    im->w = (int)CGImageGetWidth(cg);
    im->h = (int)CGImageGetHeight(cg);
    im->dirty = false;          // 刚加载，cg 就是权威数据
    DBG("  -> %dx%d", im->w, im->h);
    return im;
}

Bitmap::Bitmap(const wchar_t* file)
{
    impl = HswLoadImageFile(file);
    ShimImg* im = (ShimImg*)impl;
    lastStatus = im ? Ok : FileNotFound;
    w = im ? im->w : 0; h = im ? im->h : 0; pf = PixelFormat32bppARGB; bits = nullptr;
}
Bitmap::~Bitmap()
{
    ShimImg* im = (ShimImg*)impl;
    if (im) {
        if (im->cg) CGImageRelease(im->cg);
        if (im->ctx) CGContextRelease(im->ctx);
        if (im->owned && im->bits) free(im->bits);
        delete im;
    }
    impl = nullptr;
}
Bitmap* Bitmap::FromFile(const wchar_t* file) { return new Bitmap(file); }
Status Bitmap::GetLastStatus() const { return lastStatus; }

Status Bitmap::LockBits(const HswRect* rc, unsigned int flags, PixelFormat format, BitmapData* data)
{
    (void)flags; (void)format;
    ShimImg* im = (ShimImg*)impl;
    if (!im || !data) return InvalidParameter;
    CGContextRef c = ImgCtx(im);
    if (!c) return InvalidParameter;
    int x = 0, y = 0, ww = im->w, hh = im->h;
    if (rc) { x = rc->X; y = rc->Y; ww = rc->Width; hh = rc->Height; }
    if (ww <= 0 || hh <= 0) return InvalidParameter;
    // 缓冲是 BGRA；GDI+ 的 PixelFormat32bppARGB 内存序也是 B,G,R,A，直接用。
    // 注意：我们的缓冲第 0 行是「视觉底部」（CG 原生朝向），
    // 但游戏只做逐像素处理 / 对称采样，方向不影响结果。
    unsigned char* p = im->bits + (size_t)y * (size_t)im->stride + (size_t)x * 4;
    data->Width = (unsigned int)ww;
    data->Height = (unsigned int)hh;
    data->Stride = im->stride;
    data->PixelFormat_ = PixelFormat32bppARGB;
    data->Scan0 = p;
    data->Reserved = 0;
    return Ok;
}

Status Bitmap::UnlockBits(BitmapData* data)
{
    (void)data;
    ImgInvalidate((ShimImg*)impl);
    return Ok;
}

Bitmap* Bitmap::Clone(const RectF& rc, PixelFormat format)
{
    (void)format;
    ShimImg* im = (ShimImg*)impl;
    if (!im) return nullptr;
    CGImageRef cg = ImgCG(im);
    if (!cg) return nullptr;
    CGRect r = CGRectMake(rc.X, rc.Y, rc.Width, rc.Height);
    CGImageRef sub = CGImageCreateWithImageInRect(cg, r);
    if (!sub) return nullptr;
    Bitmap* b = new Bitmap((int)rc.Width, (int)rc.Height, PixelFormat32bppARGB);
    ShimImg* out = (ShimImg*)b->impl;
    CGContextRef c = out->ctx;
    if (c) {
        CGContextSaveGState(c);
        CGContextTranslateCTM(c, 0, rc.Height);
        CGContextScaleCTM(c, 1, -1);
        CGContextDrawImage(c, CGRectMake(0, 0, rc.Width, rc.Height), sub);
        CGContextRestoreGState(c);
    }
    CGImageRelease(sub);
    return b;
}
unsigned int Bitmap::GetWidth() const { ShimImg* im = (ShimImg*)impl; return im ? (unsigned int)im->w : 0; }
unsigned int Bitmap::GetHeight() const { ShimImg* im = (ShimImg*)impl; return im ? (unsigned int)im->h : 0; }

// ---- GraphicsPath ----
GraphicsPath::GraphicsPath()
{
    impl = new ShimPath();
    ((ShimPath*)impl)->p = CGPathCreateMutable();
}
GraphicsPath::GraphicsPath(FillMode m)
{
    impl = new ShimPath();
    ShimPath* s = (ShimPath*)impl;
    s->p = CGPathCreateMutable();
    s->mode = m;
}
GraphicsPath::~GraphicsPath()
{
    ShimPath* s = (ShimPath*)impl;
    if (s) { if (s->p) CGPathRelease(s->p); delete s; }
    impl = nullptr;
}
Status GraphicsPath::AddLine(REAL x0, REAL y0, REAL x1, REAL y1)
{
    ShimPath* s = (ShimPath*)impl; if (!s || !s->p) return InvalidParameter;
    if (CGPathIsEmpty(s->p)) CGPathMoveToPoint(s->p, nullptr, x0, y0);
    CGPathAddLineToPoint(s->p, nullptr, x1, y1);
    return Ok;
}
Status GraphicsPath::AddLine(const PointF& a, const PointF& b) { return AddLine(a.X, a.Y, b.X, b.Y); }
Status GraphicsPath::AddLines(const PointF* pts, int n)
{
    for (int i = 0; i + 1 < n; ++i) AddLine(pts[i], pts[i + 1]);
    return Ok;
}
Status GraphicsPath::AddRectangle(const RectF& r)
{
    ShimPath* s = (ShimPath*)impl; if (!s || !s->p) return InvalidParameter;
    CGPathAddRect(s->p, nullptr, CGRectMake(r.X, r.Y, r.Width, r.Height));
    return Ok;
}
Status GraphicsPath::AddEllipse(const RectF& r) { return AddEllipse(r.X, r.Y, r.Width, r.Height); }
Status GraphicsPath::AddEllipse(REAL x, REAL y, REAL w, REAL h)
{
    ShimPath* s = (ShimPath*)impl; if (!s || !s->p) return InvalidParameter;
    CGPathAddEllipseInRect(s->p, nullptr, CGRectMake(x, y, w, h));
    return Ok;
}
Status GraphicsPath::AddArc(REAL x, REAL y, REAL w, REAL h, REAL startAngle, REAL sweepAngle)
{
    ShimPath* s = (ShimPath*)impl; if (!s || !s->p) return InvalidParameter;
    CGFloat cx = x + w * 0.5f, cy = y + h * 0.5f, rx = w * 0.5f, ry = h * 0.5f;
    CGFloat a0 = -startAngle * M_PI / 180.0;         // GDI+ 顺时针为正，CG 逆时针为正
    CGFloat a1 = -(startAngle + sweepAngle) * M_PI / 180.0;
    CGAffineTransform t = CGAffineTransformMakeTranslation(cx, cy);
    t = CGAffineTransformScale(t, rx, ry);
    CGPathAddArc(s->p, &t, 0, 0, 1.0, a0, a1, (a1 < a0));
    return Ok;
}
Status GraphicsPath::AddBezier(REAL x0, REAL y0, REAL x1, REAL y1, REAL x2, REAL y2, REAL x3, REAL y3)
{
    ShimPath* s = (ShimPath*)impl; if (!s || !s->p) return InvalidParameter;
    if (CGPathIsEmpty(s->p)) CGPathMoveToPoint(s->p, nullptr, x0, y0);
    CGPathAddCurveToPoint(s->p, nullptr, x1, y1, x2, y2, x3, y3);
    return Ok;
}
Status GraphicsPath::AddBezier(const PointF& a, const PointF& b, const PointF& c, const PointF& d)
{ return AddBezier(a.X, a.Y, b.X, b.Y, c.X, c.Y, d.X, d.Y); }

// 简易 Catmull-Rom（够画手绘曲线）
static void HswAddCurveToPath(CGMutablePathRef path, const PointF* pts, int n, REAL tension)
{
    if (!path || n < 2) return;
    if (CGPathIsEmpty(path)) CGPathMoveToPoint(path, nullptr, pts[0].X, pts[0].Y);
    REAL t = (tension <= 0) ? 0.5f : tension;
    for (int i = 0; i + 1 < n; ++i) {
        PointF p0 = (i > 0) ? pts[i - 1] : pts[i];
        PointF p1 = pts[i];
        PointF p2 = pts[i + 1];
        PointF p3 = (i + 2 < n) ? pts[i + 2] : pts[i + 1];
        REAL k = t / 0.5f / 6.0f;
        CGPathAddCurveToPoint(path, nullptr,
                              p1.X + (p2.X - p0.X) * k, p1.Y + (p2.Y - p0.Y) * k,
                              p2.X - (p3.X - p1.X) * k, p2.Y - (p3.Y - p1.Y) * k,
                              p2.X, p2.Y);
    }
}

static void HswAddCurve(ShimPath* s, const PointF* pts, int n, REAL tension)
{
    if (!s || !s->p || n < 2) return;
    HswAddCurveToPath(s->p, pts, n, tension);
}
Status GraphicsPath::AddCurve(const PointF* pts, int n) { HswAddCurve((ShimPath*)impl, pts, n, 0.5f); return Ok; }
Status GraphicsPath::AddCurve(const PointF* pts, int n, REAL tension) { HswAddCurve((ShimPath*)impl, pts, n, tension); return Ok; }
Status GraphicsPath::AddPolygon(const PointF* pts, int n)
{
    ShimPath* s = (ShimPath*)impl; if (!s || !s->p || n < 2) return InvalidParameter;
    CGPathMoveToPoint(s->p, nullptr, pts[0].X, pts[0].Y);
    for (int i = 1; i < n; ++i) CGPathAddLineToPoint(s->p, nullptr, pts[i].X, pts[i].Y);
    CGPathCloseSubpath(s->p);
    return Ok;
}
Status GraphicsPath::Transform(const Matrix* m)
{
    ShimPath* s = (ShimPath*)impl;
    if (!s || !s->p || !m) return InvalidParameter;
    CGAffineTransform t = ((ShimMat*)m->impl)->m;
    CGMutablePathRef np = CGPathCreateMutable();
    CGPathAddPath(np, &t, s->p);
    CGPathRelease(s->p);
    s->p = np;
    return Ok;
}
Status GraphicsPath::AddString(const wchar_t* str, int len, const FontFamily* fam, int style,
                               REAL size, const PointF& origin, const StringFormat* fmt)
{
    (void)fmt;
    ShimPath* s = (ShimPath*)impl;
    if (!s || !s->p || !str) return InvalidParameter;
    int n = (len < 0) ? (int)wcslen(str) : len;
    if (n <= 0) return Ok;
    std::string fname = fam ? ((ShimFam*)fam->impl)->name : std::string("PingFang SC");
    CTFontRef font = MakeCTFont(fname, size, style);
    if (!font) return InvalidParameter;
    CGFloat ascent = CTFontGetAscent(font);
    CGFloat descent = CTFontGetDescent(font);
    int upem = (int)CTFontGetUnitsPerEm(font);
    if (upem <= 0) upem = 1000;

    // UTF-32 → UTF-16
    std::vector<UniChar> u16;
    for (int i = 0; i < n; ++i) {
        unsigned int cp = (unsigned int)str[i];
        if (cp >= 0x10000) {
            cp -= 0x10000;
            u16.push_back((UniChar)(0xD800 + (cp >> 10)));
            u16.push_back((UniChar)(0xDC00 + (cp & 0x3FF)));
        } else u16.push_back((UniChar)cp);
    }
    std::vector<CGGlyph> glyphs(u16.size(), 0);
    bool ok = CTFontGetGlyphsForCharacters(font, u16.data(), glyphs.data(), (CFIndex)u16.size());
    if (!ok && u16.size()) {
        // 回退到苹方
        CFRelease(font);
        font = MakeCTFont("PingFang SC", size, style);
        if (!font) return InvalidParameter;
        ascent = CTFontGetAscent(font);
        CTFontGetGlyphsForCharacters(font, u16.data(), glyphs.data(), (CFIndex)u16.size());
    }
    std::vector<CGSize> adv(u16.size());
    CTFontGetAdvancesForGlyphs(font, kCTFontOrientationHorizontal, glyphs.data(), adv.data(), (CFIndex)u16.size());

    REAL penX = origin.X;
    REAL baseY = origin.Y + ascent;      // GDI+ 的 origin 是文字格子左上角
    for (size_t i = 0; i < glyphs.size(); ++i) {
        CGPathRef g = CTFontCreatePathForGlyph(font, glyphs[i], nullptr);
        if (g) {
            CGRect bb = CGPathGetBoundingBox(g);
            REAL sc = (REAL)size / (REAL)upem;
            if (bb.size.height > 0 && bb.size.height > (CGFloat)(size * 2.0)) {
                // 路径是字体设计单位
            } else {
                sc = 1.0f;   // 已经是点单位
            }
            CGAffineTransform t = CGAffineTransformMakeTranslation(penX, baseY);
            t = CGAffineTransformScale(t, sc, -sc);
            CGPathAddPath(s->p, &t, g);
            CGPathRelease(g);
        }
        penX += (REAL)adv[i].width;
    }
    CFRelease(font);
    (void)descent;
    return Ok;
}
Status GraphicsPath::StartFigure()
{
    ShimPath* s = (ShimPath*)impl;
    if (!s || !s->p) return InvalidParameter;
    CGPathMoveToPoint(s->p, nullptr, 0, 0);
    return Ok;
}
Status GraphicsPath::CloseFigure()
{
    ShimPath* s = (ShimPath*)impl;
    if (s && s->p) CGPathCloseSubpath(s->p);
    return Ok;
}
Status GraphicsPath::Reset()
{
    ShimPath* s = (ShimPath*)impl;
    if (s) { if (s->p) CGPathRelease(s->p); s->p = CGPathCreateMutable(); }
    return Ok;
}
Status GraphicsPath::GetBounds(RectF* r) const
{
    ShimPath* s = (ShimPath*)impl;
    if (!r) return InvalidParameter;
    if (!s || !s->p) { *r = RectF(0, 0, 0, 0); return Ok; }
    CGRect b = CGPathGetBoundingBox(s->p);
    if (b.size.width < 0) b.size.width = 0;
    if (b.size.height < 0) b.size.height = 0;
    if (b.size.width < 1e-6f) {
        // 空路径：给个 0 尺寸，调用方会自己兜底
        *r = RectF((REAL)b.origin.x, (REAL)b.origin.y, 0, 0);
        return Ok;
    }
    r->X = (REAL)b.origin.x; r->Y = (REAL)b.origin.y;
    r->Width = (REAL)b.size.width; r->Height = (REAL)b.size.height;
    return Ok;
}
Status GraphicsPath::SetFillMode(FillMode m) { ShimPath* s = (ShimPath*)impl; if (s) s->mode = m; return Ok; }
int GraphicsPath::GetPointCount() const { return 0; }

// CoreGraphics 没有 CGContextSetCTM，用 先乘逆矩阵再乘目标矩阵 来模拟
static void HswSetCTM(CGContextRef c, CGAffineTransform t)
{
    if (!c) return;
    CGAffineTransform cur = CGContextGetCTM(c);
    CGContextConcatCTM(c, CGAffineTransformInvert(cur));
    CGContextConcatCTM(c, t);
}

// ---- Graphics ----
static void HswGfxInit(ShimGfx* g, void* src)
{
    if (!g || !src) return;
    if (g_dcSet.count(src)) {
        ShimDC* dc = (ShimDC*)src;
        g->buf = dc->bmp;
        g->bufIsImg = false;
        g->ctx = dc->bmp ? dc->bmp->ctx : dc->ctx;
        if (dc->bmp) { g->bw = dc->bmp->w; g->bh = dc->bmp->h; }
        else { g->bw = g_backW; g->bh = g_backH; }
    } else {
        // 这个指针是 gshim 的 Bitmap / Image 对象，真正的内部结构在 impl 里
        Image* img = (Image*)src;
        ShimImg* im = (ShimImg*)img->impl;
        g->buf = im;
        g->bufIsImg = true;
        g->ctx = im ? ImgCtx(im) : nullptr;
        g->bw = im ? im->w : 0;
        g->bh = im ? im->h : 0;
    }
    if (!g->ctx) { g->ctx = g_backCtx; g->bw = g_backW; g->bh = g_backH; }
}

Graphics::Graphics(void* image)
{
    impl = new ShimGfx();
    HswGfxInit((ShimGfx*)impl, image);
}
Graphics::Graphics(void* hdc, bool unused)
{
    (void)unused;
    impl = new ShimGfx();
    HswGfxInit((ShimGfx*)impl, hdc);
}
Graphics::~Graphics() { delete (ShimGfx*)impl; impl = nullptr; }

Status Graphics::Clear(const Color& c)
{
    ShimGfx* g = (ShimGfx*)impl;
    if (!g || !g->ctx) return InvalidParameter;
    CGFloat w = (CGFloat)(g->bw ? g->bw : g_backW);
    CGFloat h = (CGFloat)(g->bh ? g->bh : g_backH);
    CGContextSaveGState(g->ctx);
    CGContextSetBlendMode(g->ctx, kCGBlendModeCopy);
    CGContextSetFillColorWithColor(g->ctx,
        [NSColor colorWithCalibratedRed:c.GetR()/255.0 green:c.GetG()/255.0 blue:c.GetB()/255.0 alpha:c.GetA()/255.0].CGColor);
    CGContextFillRect(g->ctx, CGRectMake(-w, -h, w * 4, h * 4));
    CGContextRestoreGState(g->ctx);
    HswTouch(g);
    HswTouch(g);
    return Ok;
}
GraphicsState Graphics::Save()
{
    ShimGfx* g = (ShimGfx*)impl;
    if (!g || !g->ctx) return 0;
    CGContextSaveGState(g->ctx);
    return (GraphicsState)(++g->depth);
}
Status Graphics::Restore(GraphicsState st)
{
    ShimGfx* g = (ShimGfx*)impl;
    if (!g || !g->ctx) return InvalidParameter;
    int target = (int)st;
    while (g->depth > target) { CGContextRestoreGState(g->ctx); --g->depth; }
    HswTouch(g);
    return Ok;
}

static void HswApplyBrush(CGContextRef c, const Brush* b)
{
    if (!c || !b) return;
    // SolidBrush
    if (dynamic_cast<const SolidBrush*>(b)) {
        Color col = ((SolidBrush*)b)->col;
        CGContextSetFillColorWithColor(c,
            [NSColor colorWithCalibratedRed:col.GetR()/255.0 green:col.GetG()/255.0 blue:col.GetB()/255.0 alpha:col.GetA()/255.0].CGColor);
    }
}

static bool HswFillGrad(CGContextRef c, const Brush* b, CGPathRef path, FillMode mode)
{
    ShimGrad* s = (ShimGrad*)b->impl;
    if (!s || !s->valid || !s->g) return false;
    CGRect bb = CGPathGetBoundingBox(path);
    if (bb.size.width <= 0 || bb.size.height <= 0) return false;
    CGContextSaveGState(c);
    CGContextAddPath(c, path);
    if (mode == FillModeAlternate) CGContextEOClip(c); else CGContextClip(c);
    if (s->radial) {
        CGPoint ctr = CGPointMake(s->center.X, s->center.Y);
        CGContextDrawRadialGradient(c, s->g, ctr, 0, ctr, s->radius,
                                    kCGGradientDrawsBeforeStartLocation | kCGGradientDrawsAfterEndLocation);
    } else {
        CGContextDrawLinearGradient(c, s->g, s->p0, s->p1,
                                    kCGGradientDrawsBeforeStartLocation | kCGGradientDrawsAfterEndLocation);
    }
    CGContextRestoreGState(c);
    return true;
}

Status Graphics::FillPath(const Brush* b, const GraphicsPath* p)
{
    ShimGfx* g = (ShimGfx*)impl;
    if (!g || !g->ctx || !b || !p) return InvalidParameter;
    ShimPath* sp = (ShimPath*)p->impl;
    if (!sp || !sp->p) return Ok;
    if (dynamic_cast<const SolidBrush*>(b)) {
        HswApplyBrush(g->ctx, b);
        CGContextAddPath(g->ctx, sp->p);
        if (sp->mode == FillModeAlternate) CGContextEOFillPath(g->ctx);
        else CGContextFillPath(g->ctx);
    } else {
        if (!HswFillGrad(g->ctx, b, sp->p, sp->mode)) return GenericError;
    }
    HswTouch(g);
    HswTouch(g);
    return Ok;
}

static void HswSetPen(CGContextRef c, const Pen* pen)
{
    ShimPen* s = (ShimPen*)pen->impl;
    if (!s) return;
    Color col = s->col;
    CGContextSetStrokeColorWithColor(c,
        [NSColor colorWithCalibratedRed:col.GetR()/255.0 green:col.GetG()/255.0 blue:col.GetB()/255.0 alpha:col.GetA()/255.0].CGColor);
    CGContextSetLineWidth(c, s->width <= 0 ? 1 : s->width);
    switch (s->join) {
        case LineJoinRound: CGContextSetLineJoin(c, kCGLineJoinRound); break;
        case LineJoinBevel: CGContextSetLineJoin(c, kCGLineJoinBevel); break;
        default: CGContextSetLineJoin(c, kCGLineJoinMiter); break;
    }
    auto cap = [](LineCap c) -> CGLineCap {
        if (c == LineCapRound || c == LineCapRoundAnchor) return kCGLineCapRound;
        if (c == LineCapSquare || c == LineCapSquareAnchor) return kCGLineCapSquare;
        return kCGLineCapButt;
    };
    CGContextSetLineCap(c, cap(s->end));
    switch (s->style) {
        case DashStyleDash: { CGFloat d[2] = { 3, 3 }; CGContextSetLineDash(c, 0, d, 2); break; }
        case DashStyleDot:  { CGFloat d[2] = { 1, 2 }; CGContextSetLineDash(c, 0, d, 2); break; }
        case DashStyleDashDot: { CGFloat d[4] = { 3, 2, 1, 2 }; CGContextSetLineDash(c, 0, d, 4); break; }
        case DashStyleDashDotDot: { CGFloat d[6] = { 3, 2, 1, 2, 1, 2 }; CGContextSetLineDash(c, 0, d, 6); break; }
        default: CGContextSetLineDash(c, 0, nullptr, 0); break;
    }
}

Status Graphics::DrawPath(const Pen* pen, const GraphicsPath* path)
{
    ShimGfx* g = (ShimGfx*)impl;
    if (!g || !g->ctx || !pen || !path) return InvalidParameter;
    ShimPath* sp = (ShimPath*)path->impl;
    if (!sp || !sp->p) return Ok;
    CGContextSaveGState(g->ctx);
    HswSetPen(g->ctx, pen);
    CGContextAddPath(g->ctx, sp->p);
    CGContextStrokePath(g->ctx);
    CGContextRestoreGState(g->ctx);
    HswTouch(g);
    return Ok;
}

Status Graphics::FillRectangle(const Brush* b, const RectF& r)
{
    ShimGfx* g = (ShimGfx*)impl;
    if (!g || !g->ctx) return InvalidParameter;
    if (dynamic_cast<const SolidBrush*>(b)) {
        HswApplyBrush(g->ctx, b);
        CGContextFillRect(g->ctx, CGRectMake(r.X, r.Y, r.Width, r.Height));
    } else {
        CGMutablePathRef pp = CGPathCreateMutable();
        CGPathAddRect(pp, nullptr, CGRectMake(r.X, r.Y, r.Width, r.Height));
        HswFillGrad(g->ctx, b, pp, FillModeAlternate);
        CGPathRelease(pp);
    }
    HswTouch(g);
    return Ok;
}
Status Graphics::FillRectangle(const Brush* b, REAL x, REAL y, REAL w, REAL h)
{ return FillRectangle(b, RectF(x, y, w, h)); }

Status Graphics::DrawRectangle(const Pen* p, const RectF& r)
{
    ShimGfx* g = (ShimGfx*)impl;
    if (!g || !g->ctx || !p) return InvalidParameter;
    CGContextSaveGState(g->ctx);
    HswSetPen(g->ctx, p);
    CGContextStrokeRect(g->ctx, CGRectMake(r.X, r.Y, r.Width, r.Height));
    CGContextRestoreGState(g->ctx);
    HswTouch(g);
    return Ok;
}
Status Graphics::DrawRectangle(const Pen* p, REAL x, REAL y, REAL w, REAL h)
{ return DrawRectangle(p, RectF(x, y, w, h)); }

Status Graphics::FillEllipse(const Brush* b, REAL x, REAL y, REAL w, REAL h)
{
    ShimGfx* g = (ShimGfx*)impl;
    if (!g || !g->ctx) return InvalidParameter;
    if (dynamic_cast<const SolidBrush*>(b)) {
        HswApplyBrush(g->ctx, b);
        CGContextFillEllipseInRect(g->ctx, CGRectMake(x, y, w, h));
    } else {
        CGMutablePathRef pp = CGPathCreateMutable();
        CGPathAddEllipseInRect(pp, nullptr, CGRectMake(x, y, w, h));
        HswFillGrad(g->ctx, b, pp, FillModeAlternate);
        CGPathRelease(pp);
    }
    HswTouch(g);
    return Ok;
}
Status Graphics::FillEllipse(const Brush* b, const RectF& r) { return FillEllipse(b, r.X, r.Y, r.Width, r.Height); }
Status Graphics::DrawEllipse(const Pen* p, REAL x, REAL y, REAL w, REAL h)
{
    ShimGfx* g = (ShimGfx*)impl;
    if (!g || !g->ctx || !p) return InvalidParameter;
    CGContextSaveGState(g->ctx);
    HswSetPen(g->ctx, p);
    CGContextStrokeEllipseInRect(g->ctx, CGRectMake(x, y, w, h));
    CGContextRestoreGState(g->ctx);
    HswTouch(g);
    return Ok;
}
Status Graphics::DrawEllipse(const Pen* p, const RectF& r) { return DrawEllipse(p, r.X, r.Y, r.Width, r.Height); }

Status Graphics::DrawLine(const Pen* p, REAL x0, REAL y0, REAL x1, REAL y1)
{
    ShimGfx* g = (ShimGfx*)impl;
    if (!g || !g->ctx || !p) return InvalidParameter;
    CGContextSaveGState(g->ctx);
    HswSetPen(g->ctx, p);
    CGContextBeginPath(g->ctx);
    CGContextMoveToPoint(g->ctx, x0, y0);
    CGContextAddLineToPoint(g->ctx, x1, y1);
    CGContextStrokePath(g->ctx);
    CGContextRestoreGState(g->ctx);
    HswTouch(g);
    return Ok;
}
Status Graphics::DrawLine(const Pen* p, const PointF& a, const PointF& b) { return DrawLine(p, a.X, a.Y, b.X, b.Y); }
Status Graphics::DrawLines(const Pen* p, const PointF* pts, int n)
{
    ShimGfx* g = (ShimGfx*)impl;
    if (!g || !g->ctx || !p || n < 2) return InvalidParameter;
    CGContextSaveGState(g->ctx);
    HswSetPen(g->ctx, p);
    CGContextBeginPath(g->ctx);
    CGContextMoveToPoint(g->ctx, pts[0].X, pts[0].Y);
    for (int i = 1; i < n; ++i) CGContextAddLineToPoint(g->ctx, pts[i].X, pts[i].Y);
    CGContextStrokePath(g->ctx);
    CGContextRestoreGState(g->ctx);
    HswTouch(g);
    return Ok;
}
Status Graphics::DrawPolygon(const Pen* p, const PointF* pts, int n)
{
    ShimGfx* g = (ShimGfx*)impl;
    if (!g || !g->ctx || !p || n < 2) return InvalidParameter;
    CGContextSaveGState(g->ctx);
    HswSetPen(g->ctx, p);
    CGContextBeginPath(g->ctx);
    CGContextMoveToPoint(g->ctx, pts[0].X, pts[0].Y);
    for (int i = 1; i < n; ++i) CGContextAddLineToPoint(g->ctx, pts[i].X, pts[i].Y);
    CGContextClosePath(g->ctx);
    CGContextStrokePath(g->ctx);
    CGContextRestoreGState(g->ctx);
    HswTouch(g);
    return Ok;
}
Status Graphics::FillPolygon(const Brush* b, const PointF* pts, int n)
{
    ShimGfx* g = (ShimGfx*)impl;
    if (!g || !g->ctx || !b || n < 2) return InvalidParameter;
    if (dynamic_cast<const SolidBrush*>(b)) {
        HswApplyBrush(g->ctx, b);
        CGContextBeginPath(g->ctx);
        CGContextMoveToPoint(g->ctx, pts[0].X, pts[0].Y);
        for (int i = 1; i < n; ++i) CGContextAddLineToPoint(g->ctx, pts[i].X, pts[i].Y);
        CGContextClosePath(g->ctx);
        CGContextFillPath(g->ctx);
    } else {
        CGMutablePathRef pp = CGPathCreateMutable();
        CGPathMoveToPoint(pp, nullptr, pts[0].X, pts[0].Y);
        for (int i = 1; i < n; ++i) CGPathAddLineToPoint(pp, nullptr, pts[i].X, pts[i].Y);
        CGPathCloseSubpath(pp);
        HswFillGrad(g->ctx, b, pp, FillModeAlternate);
        CGPathRelease(pp);
    }
    HswTouch(g);
    return Ok;
}
Status Graphics::DrawArc(const Pen* p, REAL x, REAL y, REAL w, REAL h, REAL start, REAL sweep)
{
    ShimGfx* g = (ShimGfx*)impl;
    if (!g || !g->ctx || !p) return InvalidParameter;
    CGContextSaveGState(g->ctx);
    HswSetPen(g->ctx, p);
    CGFloat cx = x + w * 0.5f, cy = y + h * 0.5f;
    CGFloat a0 = -start * M_PI / 180.0, a1 = -(start + sweep) * M_PI / 180.0;
    CGContextBeginPath(g->ctx);
    CGContextAddArc(g->ctx, cx, cy, w * 0.5f, a0, a1, a1 < a0);
    CGContextStrokePath(g->ctx);
    CGContextRestoreGState(g->ctx);
    HswTouch(g);
    return Ok;
}
Status Graphics::DrawArc(const Pen* p, const RectF& r, REAL start, REAL sweep)
{ return DrawArc(p, r.X, r.Y, r.Width, r.Height, start, sweep); }

Status Graphics::FillClosedCurve(const Brush* b, const PointF* pts, int n)
{
    ShimGfx* g = (ShimGfx*)impl;
    if (!g || !g->ctx || !b || n < 3) return InvalidParameter;
    CGMutablePathRef pp = CGPathCreateMutable();
    HswAddCurveToPath(pp, pts, n, 0.5f);
    CGPathCloseSubpath(pp);
    if (dynamic_cast<const SolidBrush*>(b)) {
        HswApplyBrush(g->ctx, b);
        CGContextAddPath(g->ctx, pp);
        CGContextFillPath(g->ctx);
    } else HswFillGrad(g->ctx, b, pp, FillModeAlternate);
    CGPathRelease(pp);
    HswTouch(g);
    return Ok;
}

Status Graphics::DrawImage(Image* img, const RectF& dest)
{
    ShimGfx* g = (ShimGfx*)impl;
    if (!g || !g->ctx || !img) return InvalidParameter;
    ShimImg* im = (ShimImg*)img->impl;
    CGImageRef cg = ImgCG(im);
    if (!cg) return Ok;
    CGContextSaveGState(g->ctx);
    CGContextTranslateCTM(g->ctx, dest.X, dest.Y + dest.Height);
    CGContextScaleCTM(g->ctx, 1, -1);
    CGContextDrawImage(g->ctx, CGRectMake(0, 0, dest.Width, dest.Height), cg);
    CGContextRestoreGState(g->ctx);
    HswTouch(g);
    return Ok;
}
Status Graphics::DrawImage(Image* img, REAL x, REAL y, REAL w, REAL h)
{ return DrawImage(img, RectF(x, y, w, h)); }
Status Graphics::DrawImage(Image* img, const RectF& dest, REAL sx, REAL sy, REAL sw, REAL sh, Unit unit)
{ return DrawImage(img, dest, sx, sy, sw, sh, unit, nullptr); }
Status Graphics::DrawImage(Image* img, const RectF& dest, REAL sx, REAL sy, REAL sw, REAL sh,
                           Unit unit, const ImageAttributes* attr)
{
    (void)unit;
    ShimGfx* g = (ShimGfx*)impl;
    if (!g || !g->ctx || !img) return InvalidParameter;
    ShimImg* im = (ShimImg*)img->impl;
    CGImageRef cg = ImgCG(im);
    if (!cg) return Ok;
    CGImageRef sub = CGImageCreateWithImageInRect(cg, CGRectMake(sx, sy, sw, sh));
    if (!sub) sub = CGImageCreateWithImageInRect(cg, CGRectMake(sx, sy, sw, sh));
    if (!sub) return Ok;
    CGContextSaveGState(g->ctx);
    CGContextTranslateCTM(g->ctx, dest.X, dest.Y + dest.Height);
    CGContextScaleCTM(g->ctx, 1, -1);
    if (attr) {
        ShimAttr* a = (ShimAttr*)attr->impl;
        if (a && a->hasCm) {
            // 只支持整体透明度（ColorMatrix 第 4 行第 4 列）
            REAL mul = a->cm.m[3][3];
            CGContextSetAlpha(g->ctx, (CGFloat)(mul < 0 ? 0 : (mul > 1 ? 1 : mul)));
        }
    }
    CGContextDrawImage(g->ctx, CGRectMake(0, 0, dest.Width, dest.Height), sub);
    CGContextRestoreGState(g->ctx);
    CGImageRelease(sub);
    HswTouch(g);
    return Ok;
}

// ---- 文字 ----
struct ShimTextRun {
    CTLineRef line = nullptr;
    CGFloat ascent = 0, descent = 0, leading = 0;
    double width = 0;
};

static ShimTextRun HswMakeLine(const wchar_t* s, int len, const Font* f, bool bold)
{
    ShimTextRun r;
    ShimFont* sf = (ShimFont*)f->impl;
    if (!sf) return r;
    CTFontRef ct = sf->ct;
    if (!ct) ct = MakeCTFont(sf->fam, sf->size, sf->style);
    if (!ct) return r;
    int n = (len < 0) ? (int)wcslen(s) : len;
    if (n <= 0) return r;
    std::vector<UniChar> u16;
    for (int i = 0; i < n; ++i) {
        unsigned int cp = (unsigned int)s[i];
        if (cp == 0) break;
        if (cp >= 0x10000) {
            cp -= 0x10000;
            u16.push_back((UniChar)(0xD800 + (cp >> 10)));
            u16.push_back((UniChar)(0xDC00 + (cp & 0x3FF)));
        } else u16.push_back((UniChar)cp);
    }
    if (u16.empty()) return r;

    CTFontRef useFont = ct;
    if (bold && !(sf->style & FontStyleBold)) {
        useFont = MakeCTFont(sf->fam, sf->size, sf->style | FontStyleBold);
        if (!useFont) useFont = ct;
    }
    CFStringRef str = CFStringCreateWithCharacters(nullptr, u16.data(), (CFIndex)u16.size());
    CFMutableDictionaryRef attr = CFDictionaryCreateMutable(nullptr, 0,
        &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFDictionarySetValue(attr, kCTFontAttributeName, useFont);
    CFAttributedStringRef as = CFAttributedStringCreate(nullptr, str, attr);
    CTLineRef ln = CTLineCreateWithAttributedString(as);
    r.line = ln;
    r.width = ln ? CTLineGetTypographicBounds(ln, &r.ascent, &r.descent, &r.leading) : 0;
    if (r.width <= 0 && ln) r.width = CTLineGetImageBounds(ln, nullptr).size.width;
    CFRelease(as); CFRelease(attr); CFRelease(str);
    if (useFont != ct) CFRelease(useFont);
    return r;
}

static void HswDrawRun(CGContextRef c, ShimTextRun& r, CGFloat x, CGFloat baselineY, const Color& col,
                       bool strokeOnly, REAL strokeW, const Color& strokeCol)
{
    if (!c || !r.line) return;
    CGContextSaveGState(c);
    CGContextSetTextMatrix(c, CGAffineTransformIdentity);
    CGContextSetTextPosition(c, x, baselineY);
    if (strokeOnly) {
        CGContextSetTextDrawingMode(c, kCGTextStroke);
        CGContextSetLineWidth(c, strokeW <= 0 ? 1 : strokeW);
        CGContextSetStrokeColorWithColor(c,
            [NSColor colorWithCalibratedRed:strokeCol.GetR()/255.0 green:strokeCol.GetG()/255.0
                                      blue:strokeCol.GetB()/255.0 alpha:strokeCol.GetA()/255.0].CGColor);
    } else {
        CGContextSetTextDrawingMode(c, kCGTextFill);
        CGContextSetFillColorWithColor(c,
            [NSColor colorWithCalibratedRed:col.GetR()/255.0 green:col.GetG()/255.0
                                      blue:col.GetB()/255.0 alpha:col.GetA()/255.0].CGColor);
    }
    CTLineDraw(r.line, c);
    CGContextRestoreGState(c);
    CFRelease(r.line);
    r.line = nullptr;
}

Status Graphics::DrawString(const wchar_t* s, int len, const Font* f, const PointF& origin,
                            const StringFormat* fmt, const Brush* b)
{
    ShimGfx* g = (ShimGfx*)impl;
    if (!g || !g->ctx || !s || !f || !b) return InvalidParameter;
    Color col(255, 0, 0, 0);
    if (dynamic_cast<const SolidBrush*>(b)) col = ((SolidBrush*)b)->col;
    else if (b->impl) { ShimGrad* sg = (ShimGrad*)b->impl; col = sg->c0; }
    (void)fmt;
    ShimTextRun r = HswMakeLine(s, len, f, false);
    if (!r.line) return Ok;
    // GDI+ 里 origin 是文字格左上角；翻转之后基线在下方
    HswDrawRun(g->ctx, r, origin.X, origin.Y + r.ascent, col, false, 0, Color());
    HswTouch(g);
    return Ok;
}

Status Graphics::DrawString(const wchar_t* s, int len, const Font* f, const RectF& rc,
                            const StringFormat* fmt, const Brush* b)
{
    ShimGfx* g = (ShimGfx*)impl;
    if (!g || !g->ctx || !s || !f || !b) return InvalidParameter;
    Color col(255, 0, 0, 0);
    if (dynamic_cast<const SolidBrush*>(b)) col = ((SolidBrush*)b)->col;
    else if (b->impl) { ShimGrad* sg = (ShimGrad*)b->impl; col = sg->c0; }
    ShimFmt* sf = fmt ? (ShimFmt*)fmt->impl : nullptr;
    StringAlignment ha = sf ? sf->align : StringAlignmentNear;
    StringAlignment va = sf ? sf->line : StringAlignmentNear;
    ShimTextRun r = HswMakeLine(s, len, f, false);
    if (!r.line) return Ok;
    CGFloat tw = (CGFloat)r.width;
    CGFloat th = r.ascent + r.descent;
    CGFloat x = rc.X;
    if (ha == StringAlignmentCenter) x = rc.X + (rc.Width - tw) * 0.5f;
    else if (ha == StringAlignmentFar) x = rc.X + rc.Width - tw;
    CGFloat baseline;
    if (va == StringAlignmentCenter) baseline = rc.Y + (rc.Height - th) * 0.5f + r.ascent;
    else if (va == StringAlignmentFar) baseline = rc.Y + rc.Height - r.descent;
    else baseline = rc.Y + r.ascent;
    HswDrawRun(g->ctx, r, x, baseline, col, false, 0, Color());
    HswTouch(g);
    return Ok;
}

Status Graphics::MeasureString(const wchar_t* s, int len, const Font* f, const SizeF& layout,
                               StringFormat* fmt, RectF* out) const
{
    (void)layout; (void)fmt;
    if (!out) return InvalidParameter;
    ShimTextRun r = HswMakeLine(s, len, f, false);
    out->X = 0; out->Y = 0;
    out->Width = (REAL)r.width;
    out->Height = (REAL)(r.ascent + r.descent);
    if (r.line) CFRelease(r.line);
    return Ok;
}
Status Graphics::MeasureString(const wchar_t* s, int len, const Font* f, const RectF& layout,
                               StringFormat* fmt, RectF* out, int* fitted, int* lines) const
{
    (void)fmt;
    if (!out) return InvalidParameter;
    ShimTextRun r = HswMakeLine(s, len, f, false);
    int n = (len < 0) ? (int)wcslen(s) : len;
    out->X = 0; out->Y = 0;
    out->Width = (REAL)std::min<double>(r.width, layout.Width);
    out->Height = (REAL)(r.ascent + r.descent);
    if (fitted) *fitted = (r.width <= layout.Width) ? n : n;
    if (lines) *lines = 1;
    if (r.line) CFRelease(r.line);
    return Ok;
}
Status Graphics::MeasureString(const wchar_t* s, int len, const Font* f, const PointF& origin,
                               StringFormat* fmt, RectF* out) const
{
    (void)fmt;
    if (!out) return InvalidParameter;
    ShimTextRun r = HswMakeLine(s, len, f, false);
    out->X = origin.X; out->Y = origin.Y;
    out->Width = (REAL)r.width;
    out->Height = (REAL)(r.ascent + r.descent);
    if (r.line) CFRelease(r.line);
    return Ok;
}
Status Graphics::MeasureString(const wchar_t* s, int len, const Font* f, const PointF& origin, RectF* out) const
{
    return MeasureString(s, len, f, origin, nullptr, out);
}

// ---- 裁剪 / 变换 / 状态 ----
Status Graphics::SetClip(const RectF& r) { return SetClip(r, CombineModeReplace); }
Status Graphics::SetClip(const RectF& r, CombineMode mode)
{
    ShimGfx* g = (ShimGfx*)impl;
    if (!g || !g->ctx) return InvalidParameter;
    if (mode == CombineModeReplace) CGContextResetClip(g->ctx);
    CGContextClipToRect(g->ctx, CGRectMake(r.X, r.Y, r.Width, r.Height));
    HswTouch(g);
    return Ok;
}
Status Graphics::SetClip(const GraphicsPath* p) { return SetClip(p, CombineModeReplace); }
Status Graphics::SetClip(const GraphicsPath* p, CombineMode mode)
{
    ShimGfx* g = (ShimGfx*)impl;
    if (!g || !g->ctx || !p) return InvalidParameter;
    ShimPath* sp = (ShimPath*)p->impl;
    if (!sp || !sp->p) return Ok;
    if (mode == CombineModeReplace) CGContextResetClip(g->ctx);
    CGContextAddPath(g->ctx, sp->p);
    if (sp->mode == FillModeAlternate) CGContextEOClip(g->ctx); else CGContextClip(g->ctx);
    HswTouch(g);
    return Ok;
}
Status Graphics::ResetClip()
{
    ShimGfx* g = (ShimGfx*)impl;
    if (g && g->ctx) CGContextResetClip(g->ctx);
    HswTouch(g);
    return Ok;
}
Status Graphics::TranslateTransform(REAL dx, REAL dy)
{ ShimGfx* g = (ShimGfx*)impl; if (g && g->ctx) CGContextTranslateCTM(g->ctx, dx, dy); return Ok; }
Status Graphics::ScaleTransform(REAL sx, REAL sy)
{ ShimGfx* g = (ShimGfx*)impl; if (g && g->ctx) CGContextScaleCTM(g->ctx, sx, sy); return Ok; }
Status Graphics::RotateTransform(REAL angle)
{ ShimGfx* g = (ShimGfx*)impl; if (g && g->ctx) CGContextRotateCTM(g->ctx, angle * M_PI / 180.0); return Ok; }
Status Graphics::MultiplyTransform(const Matrix* m) { return MultiplyTransform(m, MatrixOrderPrepend); }
Status Graphics::MultiplyTransform(const Matrix* m, MatrixOrder order)
{
    ShimGfx* g = (ShimGfx*)impl;
    if (!g || !g->ctx || !m) return InvalidParameter;
    CGAffineTransform t = ((ShimMat*)m->impl)->m;
    if (order == MatrixOrderAppend) CGContextConcatCTM(g->ctx, t);
    else HswSetCTM(g->ctx, CGAffineTransformConcat(t, CGContextGetCTM(g->ctx)));
    HswTouch(g);
    return Ok;
}
Status Graphics::SetTransform(const Matrix* m)
{
    ShimGfx* g = (ShimGfx*)impl;
    if (!g || !g->ctx || !m) return InvalidParameter;
    HswSetCTM(g->ctx, ((ShimMat*)m->impl)->m);
    HswTouch(g);
    return Ok;
}
Status Graphics::GetTransform(Matrix* m) const
{
    ShimGfx* g = (ShimGfx*)impl;
    if (!g || !g->ctx || !m) return InvalidParameter;
    ((ShimMat*)m->impl)->m = CGContextGetCTM(g->ctx);
    HswTouch(g);
    return Ok;
}
Status Graphics::ResetTransform()
{
    ShimGfx* g = (ShimGfx*)impl;
    if (!g || !g->ctx) return InvalidParameter;
    // 恢复成「y 向下」的初始状态
    CGSize sz = CGSizeMake(g->bw ? g->bw : g_backW, g->bh ? g->bh : g_backH);
    HswSetCTM(g->ctx, CGAffineTransformIdentity);
    CGContextTranslateCTM(g->ctx, 0, sz.height);
    CGContextScaleCTM(g->ctx, 1, -1);
    HswTouch(g);
    return Ok;
}
Status Graphics::SetSmoothingMode(SmoothingMode m)
{ ShimGfx* g = (ShimGfx*)impl; if (g && g->ctx) CGContextSetShouldAntialias(g->ctx, m != SmoothingModeNone); return Ok; }
Status Graphics::SetInterpolationMode(InterpolationMode m)
{
    ShimGfx* g = (ShimGfx*)impl;
    if (!g || !g->ctx) return InvalidParameter;
    CGInterpolationQuality q = kCGInterpolationDefault;
    if (m == InterpolationModeHighQuality || m == InterpolationModeHighQualityBicubic) q = kCGInterpolationHigh;
    else if (m == InterpolationModeNearestNeighbor || m == InterpolationModeLowQuality) q = kCGInterpolationNone;
    else q = kCGInterpolationMedium;
    CGContextSetInterpolationQuality(g->ctx, q);
    HswTouch(g);
    return Ok;
}
Status Graphics::SetPixelOffsetMode(PixelOffsetMode m) { (void)m; return Ok; }
Status Graphics::SetCompositingMode(CompositingMode m)
{
    ShimGfx* g = (ShimGfx*)impl;
    if (g && g->ctx) CGContextSetBlendMode(g->ctx, (m == CompositingModeSourceCopy) ? kCGBlendModeCopy : kCGBlendModeNormal);
    HswTouch(g);
    return Ok;
}
Status Graphics::SetCompositingQuality(CompositingQuality q) { (void)q; return Ok; }
Status Graphics::SetTextRenderingHint(TextRenderingHint h)
{ ShimGfx* g = (ShimGfx*)impl; if (g && g->ctx) CGContextSetShouldSmoothFonts(g->ctx, h != TextRenderingHintSingleBitPerPixel); return Ok; }
Status Graphics::SetPageUnit(Unit u) { (void)u; return Ok; }
Status Graphics::SetPageScale(REAL s) { (void)s; return Ok; }

void* Graphics::GetHDC() { return nullptr; }
void  Graphics::ReleaseHDC(void* hdc) { (void)hdc; }

// ---- 启动 / 编码器 ----
Status GdiplusStartup(unsigned long long* token, const GdiplusStartupInput* in, GdiplusStartupOutput* out)
{
    (void)in; (void)out;
    DBG("GdiplusStartup in");
    if (token) *token = 1;
    if (!g_cs) g_cs = CGColorSpaceCreateDeviceRGB();
    DBG("colorspace ok");
    HswInitDirs();
    DBG("dirs ok res=%s", g_resDir.c_str());
    HswRegisterBundledFonts();
    DBG("GdiplusStartup out");
    return Ok;
}
void GdiplusShutdown(unsigned long long token) { (void)token; }

Status GetImageEncodersSize(unsigned int* num, unsigned int* size)
{ if (num) *num = 0; if (size) *size = 0; return Ok; }
Status GetImageEncoders(unsigned int num, unsigned int size, ImageCodecInfo* info)
{ (void)num; (void)size; (void)info; return Ok; }

// ==========================================================================
//  第 5 部分：入口 main() → WinMain()
// ==========================================================================
// 注意：不能用 extern "C" —— 游戏里的 WinMain 是 C++ 链接，加了就成了 _WinMain 找不到
int WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmd, int show);

int main(int argc, char** argv)
{
    // CI 自检参数：--shot 目录/前缀  --shot-at 帧号,帧号   --shot-quit
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--shot") && i + 1 < argc) g_shotPath = argv[++i];
        else if (!strcmp(argv[i], "--shot-at") && i + 1 < argc) {
            std::string csv = argv[++i];
            size_t pos = 0;
            while (pos < csv.size()) {
                size_t c = csv.find(',', pos);
                if (c == std::string::npos) c = csv.size();
                std::string one = csv.substr(pos, c - pos);
                if (!one.empty()) g_shotAt.push_back(atoi(one.c_str()));
                pos = c + 1;
            }
        }
        else if (!strcmp(argv[i], "--shot-quit")) g_shotQuit = true;
    }
    g_timeBase = HswNowSec();
    if (g_shotPath) fprintf(stderr, "[shot] path=%s frames=%zu\n", g_shotPath, g_shotAt.size());

    @autoreleasepool {
        return WinMain(nullptr, nullptr, nullptr, 0);
    }
}
