// ==========================================================================
//  钢琴键游戏 Piano Tiles  ——  macOS 版
//
//  编译：见同目录 build_mac.sh（在一台 Mac 上执行一条命令即可打出 .app）
//  依赖：只用到系统自带框架（Cocoa / CoreGraphics / CoreText / CoreVideo /
//        QuartzCore / AVFoundation / ImageIO），不需要任何第三方库。
//
//  操作：1~7 击键      ↑↓ 选曲      Enter 开始
//        Space 暂停    Esc 返回      R 重来
//        Cmd+F 全屏    Cmd+D 帧率
// ==========================================================================

#import <Cocoa/Cocoa.h>
#import <AVFoundation/AVFoundation.h>
#import <CoreVideo/CoreVideo.h>
#import <QuartzCore/QuartzCore.h>
#import <CoreText/CoreText.h>
#import <ImageIO/ImageIO.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>
#include <algorithm>
#include <random>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================ 常量 ============================
static const int    LANES        = 7;      // 7 条道路
static const double HIT_WINDOW   = 0.175;  // 击打判定窗口（秒，正负各 0.175s）
static const double MISS_AFTER   = 0.16;   // 超过这个时间没打就判定漏接
static const float  TRAVEL       = 2.85f;  // 琴键从最远处飞到判定线所需时间（秒）
static const float  GAMMA_P      = 1.90f;  // 透视曲线指数，越大越"后面加速"
static const float  TILE_T       = 0.17f;  // 一个琴键在时间上占的长度（秒）
static const double MAX_PLAY_SEC = 151.0;  // 一局最长秒数（想打完整首歌就调大）

// ============================ 数据结构 ============================
struct Note {                 // 一个音符（一个琴键）
    int    lane;              // 所在车道 0..6
    double t;                 // 应该被击打的时间点（秒）
    int    state;             // 0=待击打 1=已击中 2=漏接
    double hitT;              // 击中时刻（用于绿色闪光）
    double redT;              // 被误判染红的时刻
};

struct Song {
    const char* title;        // 歌名（UTF-8）
    const char* artist;       // 演唱者
    const char* key;          // 匹配音频文件名用的关键字
    double      bpm;          // 速度
    unsigned    seed;         // 谱面随机种子（固定后每次谱面一致）
    double      len;          // 谱面时长（秒）
};

struct FX {                   // 命中特效（扩散光环）
    int    lane;
    double age;
    double life;
    int    type;              // 0=绿色命中 1=红色错误
};

struct Particle {             // 火花粒子
    float x, y, vx, vy;
    float age, life, size;
    float r, g, b;
};

struct Dust {                 // 背景微尘
    float x, y, vy, size, alpha;
};

// 透视视图参数
struct View {
    float W, H;
    float cx;          // 灭点（水平中心）
    float horizonY;    // 地平线 y
    float hitY;        // 判定线 y
    float nearHalf;    // 判定线处道路半宽
    float lw;          // 判定线处单条车道宽度
    float kFar;        // 最远处的缩放系数
    float S;           // 界面缩放系数（以 1280x812 为基准 1.0）

    void Setup(float w, float h) {
        W = w; H = h;
        cx        = w * 0.5f;
        horizonY  = h * 0.135f;
        hitY      = h * 0.815f;
        nearHalf  = w * 0.47f;
        lw        = nearHalf * 2.0f / LANES;
        kFar      = 0.055f;
        S         = std::min(w / 1280.0f, h / 812.0f);   // 宽高都考虑，窄窗口也不会挤变形
        if (S < 0.55f) S = 0.55f;
        if (S > 2.60f) S = 2.60f;
    }
    inline float KY(float k) const { return horizonY + (hitY - horizonY) * k; }
    inline float PX(float xn, float k) const { return cx + (xn - cx) * k; }
    inline float XB(int i) const { return cx + (i - 3.5f) * lw; }
    // 进程 p（0=最远, 1=判定线）→ 深度 k。用幂曲线代替严格 1/Z，
    // 既保留"越近越快"的透视感，又不会在终点突然飞出去。
    inline float KofP(float p) const {
        if (p < 0.0f)  p = 0.0f;
        if (p > 1.40f) p = 1.40f;
        return kFar + (1.0f - kFar) * powf(p, GAMMA_P);
    }
    inline float PofT(double remain) const { return 1.0f - (float)(remain / (double)TRAVEL); }
};

// ============================ 全局状态 ============================
enum State { ST_MENU, ST_LOADING, ST_COUNTDOWN, ST_PLAY, ST_PAUSE, ST_RESULT };
enum { TA_NEAR = 0, TA_CENTER = 1, TA_FAR = 2 };

static const Song gSongs[4] = {
    { "The Phantom of the Opera", "Andrew Lloyd Webber · 歌剧魅影", "phantom",        124.0, 1001u, 112.0 },
    { "City of Stars",            "La La Land · 爱乐之城",           "city of stars",   92.0, 2002u, 104.0 },
    { "My Shot",                  "Hamilton · 汉密尔顿",             "my shot",        132.0, 3003u, 110.0 },
    { "Look Down",                "Les Misérables · 悲惨世界",       "look down",       96.0, 4004u, 106.0 },
};

static View    gView;
static State   gState   = ST_MENU;
static int     gSel     = 0;
static double  gT       = 0.0;      // 谱面时间（秒）
static double  gSongLen = 100.0;    // 当前曲目实际可玩时长（跟随音频真实长度）
static std::vector<Note> gNotes;

static int  gScore = 0, gCombo = 0, gMaxCombo = 0;
static int  gHits = 0, gMiss = 0, gWrong = 0, gTotal = 0;
static double gWrongFlash[LANES] = { 0, 0, 0, 0, 0, 0, 0 };

static std::vector<FX>       gFX;
static std::vector<Particle> gParts;
static std::vector<Dust>     gDust;

static bool   gHasMusic = false;
static bool   gAudioFound[4] = { false, false, false, false };
static bool   gShowFps  = false;
static double gFrameMs  = 16.7;
static double gFpsAvg   = 60.0;
static double gAnimT    = 0.0;

static CGColorSpaceRef gCS = nullptr;

// ============================ 颜色 / 绘图小工具 ============================
// 参数顺序和 GDI+ 的 Color(A,R,G,B) 保持一致，移植时不容易写错
struct Col { float r, g, b, a; };
static inline Col COL(int a, int r, int g, int b) {
    Col c; c.r = r / 255.0f; c.g = g / 255.0f; c.b = b / 255.0f; c.a = a / 255.0f; return c;
}
static inline void SetFill(CGContextRef g, Col c)   { CGContextSetRGBFillColor(g, c.r, c.g, c.b, c.a); }
static inline void SetStroke(CGContextRef g, Col c) { CGContextSetRGBStrokeColor(g, c.r, c.g, c.b, c.a); }

static void FillRect(CGContextRef g, float x, float y, float w, float h, Col c) {
    SetFill(g, c);
    CGContextFillRect(g, CGRectMake(x, y, w, h));
}
static void PolyPath(CGContextRef g, const CGPoint* p, int n) {
    CGContextBeginPath(g);
    CGContextMoveToPoint(g, p[0].x, p[0].y);
    for (int i = 1; i < n; i++) CGContextAddLineToPoint(g, p[i].x, p[i].y);
    CGContextClosePath(g);
}
static void FillPoly(CGContextRef g, const CGPoint* p, int n, Col c) {
    PolyPath(g, p, n); SetFill(g, c); CGContextFillPath(g);
}
static void FillPolyGrad(CGContextRef g, const CGPoint* p, int n,
                         float x0, float y0, float x1, float y1, Col c0, Col c1) {
    PolyPath(g, p, n);
    CGContextSaveGState(g);
    CGContextClip(g);
    CGFloat comps[8] = { c0.r, c0.g, c0.b, c0.a, c1.r, c1.g, c1.b, c1.a };
    CGGradientRef gr = CGGradientCreateWithColorComponents(gCS, comps, nullptr, 2);
    if (gr) {
        CGContextDrawLinearGradient(g, gr, CGPointMake(x0, y0), CGPointMake(x1, y1), 0);
        CGGradientRelease(gr);
    }
    CGContextRestoreGState(g);
}
static void Line(CGContextRef g, float x0, float y0, float x1, float y1, Col c, float w) {
    CGContextBeginPath(g);
    CGContextMoveToPoint(g, x0, y0);
    CGContextAddLineToPoint(g, x1, y1);
    CGContextSetLineWidth(g, w);
    SetStroke(g, c);
    CGContextStrokePath(g);
}
// 椭圆形的柔光（用来代替 GDI+ 的 PathGradientBrush）
static void RadialGlow(CGContextRef g, float cx, float cy, float rx, float ry, Col c) {
    CGContextSaveGState(g);
    CGContextTranslateCTM(g, cx, cy);
    CGContextScaleCTM(g, rx, ry);
    CGFloat comps[8] = { c.r, c.g, c.b, c.a, c.r, c.g, c.b, 0.0f };
    CGGradientRef gr = CGGradientCreateWithColorComponents(gCS, comps, nullptr, 2);
    if (gr) {
        CGContextDrawRadialGradient(g, gr, CGPointMake(0, 0), 0.0,
                                    CGPointMake(0, 0), 1.0, kCGGradientDrawsAfterEndLocation);
        CGGradientRelease(gr);
    }
    CGContextRestoreGState(g);
}
// 外发光：用 CoreGraphics 的阴影实现，一次填充就带出光晕（比叠绘多层便宜）
static void GlowPoly(CGContextRef g, const CGPoint* p, int n, Col c) {
    CGContextSaveGState(g);
    CGFloat comps[4] = { c.r, c.g, c.b, c.a };
    CGColorRef col = CGColorCreate(gCS, comps);
    if (col) { CGContextSetShadowWithColor(g, CGSizeMake(0, 0), 11.0, col); CGColorRelease(col); }
    FillPoly(g, p, n, c);
    CGContextRestoreGState(g);
}
static void FillRoundRect(CGContextRef g, CGRect r, float rad, Col fill, Col border, float bw) {
    CGPathRef p = CGPathCreateWithRoundedRect(r, rad, rad, nullptr);
    if (!p) return;
    CGContextBeginPath(g); CGContextAddPath(g, p);
    SetFill(g, fill); CGContextFillPath(g);
    if (bw > 0) {
        CGContextBeginPath(g); CGContextAddPath(g, p);
        CGContextSetLineWidth(g, bw);
        SetStroke(g, border);
        CGContextStrokePath(g);
    }
    CGPathRelease(p);
}

// ============================ 文字 ============================
static NSFont* FontFor(int fam, float size, bool bold) {
    NSFont* f = nil;
    if (fam == 0) f = [NSFont fontWithName:@"PingFang SC" size:size];      // 中文
    else          f = [NSFont fontWithName:@"Helvetica Neue" size:size];   // 拉丁
    if (f && bold) {
        NSFont* b = [[NSFontManager sharedFontManager] convertFont:f toHaveTrait:NSFontBoldTrait];
        if (b) f = b;
    }
    if (!f) f = bold ? [NSFont boldSystemFontOfSize:size] : [NSFont systemFontOfSize:size];
    return f;
}

static void Text_(CGContextRef g, const std::string& s, CGRect rc, float size, int fam,
                  bool bold, Col c, int ha = TA_CENTER, bool glow = false) {
    if (s.empty() || size <= 0) return;
    NSString* str = [NSString stringWithUTF8String:s.c_str()];
    if (!str || str.length == 0) return;

    NSFont* f = FontFor(fam, size, bold);
    CTFontRef ct = (__bridge CTFontRef)f;
    CGFloat comps[4] = { c.r, c.g, c.b, c.a };
    CGColorRef col = CGColorCreate(gCS, comps);
    if (!col) {
        CGFloat white[4] = { 1, 1, 1, 1 };
        col = CGColorCreate(gCS, white);
    }

    NSDictionary* attrs = @{
        (__bridge id)kCTFontAttributeName            : (__bridge id)ct,
        (__bridge id)kCTForegroundColorAttributeName : (__bridge id)col
    };
    NSAttributedString* as = [[NSAttributedString alloc] initWithString:str attributes:attrs];
    CTLineRef line = CTLineCreateWithAttributedString((__bridge CFAttributedStringRef)as);
    if (!line) { CGColorRelease(col); return; }

    CGFloat lw   = (CGFloat)CTLineGetTypographicBounds(line, nullptr, nullptr, nullptr);
    CGFloat asc  = CTFontGetAscent(ct);
    CGFloat desc = CTFontGetDescent(ct);

    CGFloat x = rc.origin.x;
    if (ha == TA_CENTER) x = rc.origin.x + (rc.size.width - lw) * 0.5f;
    else if (ha == TA_FAR) x = rc.origin.x + rc.size.width - lw;
    CGFloat baseline = rc.origin.y + (rc.size.height - (asc + desc)) * 0.5f + asc;

    CGContextSaveGState(g);
    if (glow) CGContextSetShadowWithColor(g, CGSizeMake(0, 0), 7.0, col);
    // 我们的坐标系是 y 向下，CoreText 是 y 向上，靠文字矩阵把方向扳回来
    CGContextSetTextMatrix(g, CGAffineTransformMake(1, 0, 0, -1, x, baseline));
    CTLineDraw(line, g);
    CGContextRestoreGState(g);

    CFRelease(line);
    CGColorRelease(col);
}

// 把字号缩到能塞进 maxW 的最大值（长歌名在小窗口里不会顶到计分器）
static float FitSize(const std::string& s, float size, int fam, bool bold, float maxW) {
    if (maxW <= 20 || s.empty()) return size * 0.5f;
    NSString* str = [NSString stringWithUTF8String:s.c_str()];
    if (!str) return size;
    NSFont* f = FontFor(fam, size, bold);
    CTFontRef ct = (__bridge CTFontRef)f;
    NSDictionary* attrs = @{ (__bridge id)kCTFontAttributeName : (__bridge id)ct };
    NSAttributedString* as = [[NSAttributedString alloc] initWithString:str attributes:attrs];
    CTLineRef line = CTLineCreateWithAttributedString((__bridge CFAttributedStringRef)as);
    if (!line) return size;
    CGFloat lw = (CGFloat)CTLineGetTypographicBounds(line, nullptr, nullptr, nullptr);
    CFRelease(line);
    if (lw <= 0 || lw <= maxW) return size;
    float k = (float)(maxW / lw);
    if (k < 0.42f) k = 0.42f;
    return size * k;
}

// ============================ 音频（AVFoundation） ============================
@interface AudioBox : NSObject {
@public
    AVAudioPlayer* music;
    NSMutableArray<AVAudioPlayer*>* sfx;   // 7 个车道各一个音高的音效
}
@end
@implementation AudioBox
- (instancetype)init {
    self = [super init];
    if (self) sfx = [NSMutableArray array];
    return self;
}
@end
static AudioBox* gAudio = nil;

static std::vector<unsigned char> MakeWav(double freq, double dur, double amp) {
    const int SR = 44100;
    int n = (int)(SR * dur);
    int dataBytes = n * 2;
    std::vector<unsigned char> b(44 + dataBytes, 0);
    auto put32 = [&](int off, unsigned v)          { memcpy(&b[off], &v, 4); };
    auto put16 = [&](int off, unsigned short v)    { memcpy(&b[off], &v, 2); };
    memcpy(&b[0], "RIFF", 4);
    put32(4, 36 + dataBytes);
    memcpy(&b[8], "WAVEfmt ", 8);
    put32(16, 16);
    put16(20, 1);            // PCM
    put16(22, 1);            // 单声道
    put32(24, SR);
    put32(28, SR * 2);
    put16(32, 2);
    put16(34, 16);
    memcpy(&b[36], "data", 4);
    put32(40, dataBytes);

    short* s = (short*)(&b[44]);
    for (int i = 0; i < n; i++) {
        double t = (double)i / SR;
        double env = exp(-t * 14.0);                 // 指数衰减，像敲击琴弦
        double w = sin(2 * M_PI * freq * t) * 0.7
                 + sin(2 * M_PI * freq * 2 * t) * 0.22
                 + sin(2 * M_PI * freq * 3 * t) * 0.08;
        double v = w * env * amp;
        if (v > 1.0) v = 1.0; if (v < -1.0) v = -1.0;
        s[i] = (short)(v * 32000);
    }
    return b;
}

static void InitSfx() {
    static const double f[LANES] = { 523.25, 587.33, 659.26, 783.99, 880.00, 1046.50, 1174.66 };
    [gAudio->sfx removeAllObjects];
    for (int i = 0; i < LANES; i++) {
        std::vector<unsigned char> w = MakeWav(f[i], 0.26, 0.55);
        NSData* d = [NSData dataWithBytes:w.data() length:w.size()];
        NSError* err = nil;
        AVAudioPlayer* p = [[AVAudioPlayer alloc] initWithData:d error:&err];
        if (p) { p.volume = 0.5f; [p prepareToPlay]; [gAudio->sfx addObject:p]; }
    }
}
static void PlayTone(int lane) {
    if (!gAudio || !gAudio->sfx) return;
    if (lane < 0 || lane >= LANES) return;
    if ((NSUInteger)lane >= gAudio->sfx.count) return;
    AVAudioPlayer* p = [gAudio->sfx objectAtIndex:(NSUInteger)lane];
    if (!p) return;
    p.currentTime = 0;
    [p play];
}

// 曲目音频的路径：优先用 .app 里内置的资源（aud0~aud3.mp3），
// 其次看可执行文件同目录下有没有带关键字的音频（方便临时换歌，不改代码）
static NSString* AudioPathForSong(int idx) {
    NSBundle* mb = [NSBundle mainBundle];
    NSString* name = [NSString stringWithFormat:@"aud%d", idx];
    NSString* p = [mb pathForResource:name ofType:@"mp3"];
    if (!p) p = [mb pathForResource:name ofType:@"m4a"];
    if (!p) p = [mb pathForResource:name ofType:@"wav"];
    if (p) return p;

    NSString* exe = [mb executablePath];
    if (!exe) return nil;
    NSString* exeDir = [exe stringByDeletingLastPathComponent];
    NSArray* files = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:exeDir error:nil];
    NSString* key = [NSString stringWithUTF8String:gSongs[idx].key];
    for (NSString* f in files) {
        NSString* lf = [f lowercaseString];
        if ([lf rangeOfString:key].location == NSNotFound) continue;
        if (!([lf hasSuffix:@".mp3"] || [lf hasSuffix:@".m4a"] || [lf hasSuffix:@".wav"])) continue;
        return [exeDir stringByAppendingPathComponent:f];
    }
    return nil;
}

static void ScanAudio() {
    // 只查路径是否存在，不把音频读进内存（回菜单时会重新调用一次）
    for (int i = 0; i < 4; i++) gAudioFound[i] = (AudioPathForSong(i) != nil);
}

static void AudioCloseMusic() {
    if (gAudio && gAudio->music) { [gAudio->music stop]; gAudio->music = nil; }
}
static bool LoadSongAudio(int idx) {
    AudioCloseMusic();
    NSString* path = AudioPathForSong(idx);
    if (!path) return false;
    NSData* d = [NSData dataWithContentsOfFile:path];
    if (!d) return false;
    NSError* err = nil;
    AVAudioPlayer* p = [[AVAudioPlayer alloc] initWithData:d error:&err];
    if (!p) return false;
    p.volume = 0.75f;
    [p prepareToPlay];
    gAudio->music = p;
    return true;
}
static double MusicDuration() { return gAudio->music ? gAudio->music.duration : 0.0; }
static double MusicPos()      { return gAudio->music ? gAudio->music.currentTime : -1.0; }
static void MusicPlay()       { if (gAudio->music) [gAudio->music play]; }
static void MusicPause()      { if (gAudio->music) [gAudio->music pause]; }
static void MusicStop()       { if (gAudio->music) { [gAudio->music stop]; gAudio->music.currentTime = 0; } }

// ============================ 生成谱面 ============================
static void BuildChart(const Song& s, double len, std::vector<Note>& out) {
    out.clear();
    std::mt19937 rng(s.seed);
    std::uniform_real_distribution<double> ur(0.0, 1.0);

    double step = 60.0 / s.bpm / 2.0;          // 八分音符为最小格
    int total = (int)((len - 4.0) / step);
    int lastLane = -1;

    for (int i = 0; i < total; i++) {
        double t = 2.0 + i * step;

        // 用 16 步为一段做"强弱起伏"，避免整首歌节奏一模一样
        int sec = (i / 16) % 4;
        double dens = (sec == 0) ? 0.32 : (sec == 1 ? 0.58 : (sec == 2 ? 0.80 : 0.46));
        if (ur(rng) > dens) continue;

        int lane;
        int guard = 0;
        do { lane = (int)(ur(rng) * LANES); } while (lane == lastLane && ur(rng) < 0.75 && ++guard < 8);
        if (lane >= LANES) lane = LANES - 1;

        Note n; n.lane = lane; n.t = t; n.state = 0; n.hitT = -1; n.redT = -1;
        out.push_back(n);
        lastLane = lane;

        // 偶尔来一个双键和弦
        if (ur(rng) < 0.09) {
            int l2; int g2 = 0;
            do { l2 = (int)(ur(rng) * LANES); } while (l2 == lane && ++g2 < 8);
            if (l2 < LANES && l2 != lane) {
                Note m; m.lane = l2; m.t = t; m.state = 0; m.hitT = -1; m.redT = -1;
                out.push_back(m);
            }
        }
    }
    std::stable_sort(out.begin(), out.end(),
                     [](const Note& a, const Note& b) { return a.t < b.t; });
}

// ============================ 开始 / 复位 ============================
static void RenderNow();

static void StartSong(int idx) {
    gSel = idx;
    gScore = 0; gCombo = 0; gMaxCombo = 0;
    gHits = 0; gMiss = 0; gWrong = 0;
    gFX.clear(); gParts.clear();
    for (int i = 0; i < LANES; i++) gWrongFlash[i] = 0;

    // 先画一帧"解码中"，再同步解码
    gHasMusic = false;
    AudioCloseMusic();
    gState = ST_LOADING;
    RenderNow();

    gHasMusic = LoadSongAudio(gSel);

    // 谱面长度跟着音频真实时长走：音乐放完，这一局也就结束了
    double dur = gSongs[gSel].len;
    if (gHasMusic) {
        double real = MusicDuration();
        if (real > 0) dur = real;
    }
    double playLen = (dur > MAX_PLAY_SEC) ? MAX_PLAY_SEC : (dur - 1.2);
    if (playLen < 30.0) playLen = dur;

    gSongLen = playLen;
    BuildChart(gSongs[gSel], gSongLen, gNotes);
    gTotal = (int)gNotes.size();

    gT = -3.0;                 // 3 秒倒计时，期间琴键由远及近
    gState = ST_COUNTDOWN;
}
static void BackToMenu() {
    AudioCloseMusic();
    ScanAudio();
    gHasMusic = false;
    gState = ST_MENU;
    gNotes.clear();
    gFX.clear(); gParts.clear();
}

// ============================ 击打判定 ============================
static void Press(int lane) {
    if (lane < 0 || lane >= LANES) return;
    if (gState != ST_PLAY) return;

    Note* best = nullptr;
    double bestD = 1e9;
    for (auto& n : gNotes) {
        if (n.state != 0 || n.lane != lane) continue;
        double d = fabs(n.t - gT);
        if (d < bestD) { bestD = d; best = &n; }
    }

    if (best && bestD <= HIT_WINDOW) {
        // ---------- 打对：加 1 分，琴键变绿 ----------
        best->state = 1;
        best->hitT  = gT;
        gScore++; gHits++; gCombo++;
        if (gCombo > gMaxCombo) gMaxCombo = gCombo;

        FX f; f.lane = lane; f.age = 0; f.life = 0.42; f.type = 0;
        gFX.push_back(f);

        float cxLane = (gView.XB(lane) + gView.XB(lane + 1)) * 0.5f;
        for (int i = 0; i < 16; i++) {
            Particle p;
            float ang = (float)(-M_PI * 0.5 + ((rand() % 1000) / 1000.0 - 0.5) * 2.4);
            float sp  = 140.0f + (rand() % 260);
            p.x = cxLane + (float)((rand() % 100) - 50) * 0.8f;
            p.y = gView.hitY;
            p.vx = cosf(ang) * sp;
            p.vy = sinf(ang) * sp;
            p.age = 0;
            p.life = 0.35f + (rand() % 40) / 100.0f;
            p.size = 1.6f + (rand() % 3);
            p.r = 130 / 255.0f; p.g = (220 + rand() % 35) / 255.0f; p.b = 200 / 255.0f;
            gParts.push_back(p);
        }
        PlayTone(lane);
    } else {
        // ---------- 打错：扣 1 分（不低于 0），车道闪红 ----------
        gScore = (gScore > 0) ? gScore - 1 : 0;
        gCombo = 0;
        gWrong++;
        gWrongFlash[lane] = 0.34;

        FX f; f.lane = lane; f.age = 0; f.life = 0.34; f.type = 1;
        gFX.push_back(f);

        Note* nNear = nullptr; double nd = 1e9;
        for (auto& n : gNotes) {
            if (n.state != 0 || n.lane != lane) continue;
            double d = n.t - gT;
            if (d > 0 && d < 0.55 && d < nd) { nd = d; nNear = &n; }
        }
        if (nNear) nNear->redT = gT;
    }
}

// ============================ 更新 ============================
static void Update(double dt) {
    gAnimT += dt;

    for (auto& d : gDust) { d.y += d.vy * dt; if (d.y > 1.05f) d.y -= 1.1f; }
    for (auto& f : gFX) f.age += dt;
    gFX.erase(std::remove_if(gFX.begin(), gFX.end(),
              [](const FX& f) { return f.age > f.life; }), gFX.end());
    for (auto& p : gParts) {
        p.vy += 980.0f * (float)dt;
        p.x += p.vx * (float)dt;
        p.y += p.vy * (float)dt;
        p.age += (float)dt;
    }
    gParts.erase(std::remove_if(gParts.begin(), gParts.end(),
                 [](const Particle& p) { return p.age > p.life; }), gParts.end());
    for (int i = 0; i < LANES; i++) if (gWrongFlash[i] > 0) gWrongFlash[i] -= dt;

    if (gState == ST_COUNTDOWN) {
        gT += dt;
        if (gT >= 0.0) {
            gT = 0.0;
            gState = ST_PLAY;
            if (gHasMusic) MusicPlay();
        }
        return;
    }
    if (gState != ST_PLAY) return;

    gT += dt;

    // 让谱面时间缓缓对齐音乐实际播放位置。
    // 加了死区：设备报位置本身是分块更新的（有几毫秒抖动），
    // 80ms 以内的差当作噪声不管，免得把抖动带进琴键的移动里。
    if (gHasMusic) {
        double mp = MusicPos();
        if (mp > 0.10) {
            double diff = gT - mp;
            if (fabs(diff) > 1.0)        gT -= diff;
            else if (fabs(diff) > 0.08)  gT -= diff * 0.035;
        }
    }

    // 漏接判定
    for (auto& n : gNotes) {
        if (n.state == 0 && gT > n.t + MISS_AFTER) {
            n.state = 2;
            gCombo = 0;
            gMiss++;
        }
    }

    if (gT > gSongLen - 0.35) {
        gState = ST_RESULT;
        MusicStop();
    }
}

// ============================ 场景绘制 ============================
static void DrawBackdrop(CGContextRef g, float W, float H) {
    // 垂直渐变底
    CGFloat comps[8] = { 3 / 255.0f, 5 / 255.0f, 15 / 255.0f, 1.0f,
                         0.0f,       0.0f,       1 / 255.0f,  1.0f };
    CGGradientRef gr = CGGradientCreateWithColorComponents(gCS, comps, nullptr, 2);
    if (gr) {
        CGContextDrawLinearGradient(g, gr, CGPointMake(0, 0), CGPointMake(0, H), 0);
        CGGradientRelease(gr);
    }
    // 地平线上的辉光
    RadialGlow(g, W * 0.5f, gView.horizonY + H * 0.02f,
               W * 0.65f, H * 0.18f, COL(120, 24, 78, 210));
}

static void DrawDust(CGContextRef g, float W, float H, float dim) {
    for (auto& d : gDust) {
        SetFill(g, COL((int)(d.alpha * 255 * dim), 120, 190, 255));
        float r = d.size;
        CGContextFillEllipseInRect(g, CGRectMake(d.x * W - r, d.y * H - r, r * 2, r * 2));
    }
}

static void DrawRoad(CGContextRef g) {
    const View& V = gView;

    // 路面：从最远处一直延伸出屏幕底边
    float kEnd = 2.4f;
    CGPoint road[4] = {
        CGPointMake(V.PX(V.XB(0), V.kFar),      V.KY(V.kFar)),
        CGPointMake(V.PX(V.XB(LANES), V.kFar),  V.KY(V.kFar)),
        CGPointMake(V.PX(V.XB(LANES), kEnd),    V.KY(kEnd)),
        CGPointMake(V.PX(V.XB(0), kEnd),        V.KY(kEnd))
    };
    FillPolyGrad(g, road, 4, 0, V.KY(V.kFar), 0, V.KY(kEnd),
                 COL(255, 8, 14, 34), COL(255, 3, 6, 18));

    // 远处横向网格（增强纵深感）
    for (int i = 1; i <= 9; i++) {
        float z = 1.0f + i * 1.9f;
        float k = 1.0f / z;
        if (k < V.kFar) break;
        Line(g, V.PX(V.XB(0), k), V.KY(k), V.PX(V.XB(LANES), k), V.KY(k),
             COL(20, 70, 140, 235), 1.0f);
    }

    // 7 条车道之间的分隔线（严格交于灭点）
    for (int i = 0; i <= LANES; i++) {
        int a = (i == 0 || i == LANES) ? 130 : 84;
        Line(g, V.PX(V.XB(i), V.kFar), V.KY(V.kFar), V.PX(V.XB(i), kEnd), V.KY(kEnd),
             COL(a, 70, 160, 255), (i == 0 || i == LANES) ? 2.0f : 1.2f);
    }

    // 车道底色（冷暖相间，视觉上更有层次）
    for (int i = 0; i < LANES; i++) {
        if (i % 2 == 0) continue;
        CGPoint q[4] = {
            CGPointMake(V.PX(V.XB(i), V.kFar),     V.KY(V.kFar)),
            CGPointMake(V.PX(V.XB(i + 1), V.kFar), V.KY(V.kFar)),
            CGPointMake(V.PX(V.XB(i + 1), kEnd),   V.KY(kEnd)),
            CGPointMake(V.PX(V.XB(i), kEnd),       V.KY(kEnd))
        };
        FillPoly(g, q, 4, COL(22, 34, 84, 190));
    }
}

// ============================ 画一个琴键 ============================
// pFront: 琴键前沿的进度（0=最远, 1=判定线）
// mode:   0 正常蓝  1 命中绿  2 误按红 ；alpha 用于淡出
static void DrawTile(CGContextRef g, int lane, float pFront, int mode, float alpha) {
    float kf = gView.KofP(pFront);
    float kb = gView.KofP(pFront - TILE_T / TRAVEL);

    float m = gView.lw * 0.055f;                 // 左右留缝，形成琴键之间的间隙
    float xn0 = gView.XB(lane) + m;
    float xn1 = gView.XB(lane + 1) - m;

    float yb  = gView.KY(kb),  yf  = gView.KY(kf);
    float xb0 = gView.PX(xn0, kb), xb1 = gView.PX(xn1, kb);
    float xf0 = gView.PX(xn0, kf), xf1 = gView.PX(xn1, kf);
    if (yf - yb < 0.8f) yb = yf - 0.8f;          // 极远处至少留 1 像素

    CGPoint q[4] = { CGPointMake(xb0, yb), CGPointMake(xb1, yb),
                     CGPointMake(xf1, yf), CGPointMake(xf0, yf) };

    Col cTop, cBot, cGlow, cEdge;
    if (mode == 1) {          // 命中 → 绿色
        cTop  = COL((int)(235 * alpha),   6, 140,  62);
        cBot  = COL((int)(255 * alpha), 120, 255, 150);
        cGlow = COL((int)(175 * alpha),  90, 255, 150);
        cEdge = COL((int)(255 * alpha), 215, 255, 225);
    } else if (mode == 2) {   // 误按 → 红色
        cTop  = COL((int)(235 * alpha), 110,   6,  12);
        cBot  = COL((int)(255 * alpha), 255,  90,  90);
        cGlow = COL((int)(175 * alpha), 255,  70,  70);
        cEdge = COL((int)(255 * alpha), 255, 210, 210);
    } else {                  // 正常 → 炫酷蓝
        cTop  = COL((int)(238 * alpha),   8,  30, 128);
        cBot  = COL((int)(255 * alpha),  70, 190, 255);
        cGlow = COL((int)(190 * alpha),  30, 130, 255);
        cEdge = COL((int)(255 * alpha), 190, 250, 255);
    }

    // 外发光
    GlowPoly(g, q, 4, cGlow);

    // 主体渐变
    float midX = (xb0 + xb1) * 0.5f;
    FillPolyGrad(g, q, 4, midX, yb, midX, yf, cTop, cBot);

    // 键面中缝高光（让键看起来是"立体的"）
    float inset = 0.30f;
    float cyh = (yb + yf) * 0.5f;
    float cx4 = (xb0 + xb1 + xf0 + xf1) * 0.25f;
    CGPoint hi[4];
    for (int i = 0; i < 4; i++) {
        hi[i].x = cx4 + (q[i].x - cx4) * (1.0f - inset * 0.4f);
        hi[i].y = cyh + (q[i].y - cyh) * (1.0f - inset * 2.2f);
    }
    FillPoly(g, hi, 4, COL((int)(40 * alpha), 255, 255, 255));

    // 前沿亮边 + 两侧描边
    float penW = 1.2f + 2.4f * (kf > 1.5f ? 1.0f : kf / 1.5f);
    Line(g, q[3].x, q[3].y, q[2].x, q[2].y, cEdge, penW);
    Line(g, q[0].x, q[0].y, q[3].x, q[3].y, COL((int)(150 * alpha), 140, 215, 255), 1.1f);
    Line(g, q[1].x, q[1].y, q[2].x, q[2].y, COL((int)(150 * alpha), 140, 215, 255), 1.1f);
}

// ============================ 静态层缓存 ============================
// 背景渐变、辉光、道路网格、车道线每帧都一样，烘焙成图片，每帧只贴一次。
struct Layer {
    CGContextRef ctx = nullptr;
    CGImageRef   img = nullptr;
    void*        bits = nullptr;
    int          wpx = 0, hpx = 0;
    double       scale = 0;

    void Free() {
        if (img) { CGImageRelease(img); img = nullptr; }
        if (ctx) { CGContextRelease(ctx); ctx = nullptr; }
        if (bits) { free(bits); bits = nullptr; }
        wpx = hpx = 0; scale = 0;
    }
    // 建一个「y 向下、单位为点」的位图上下文，像素尺寸按 backingScaleFactor 放大
    bool Build(float w, float h, double sc) {
        Free();
        int ww = (int)ceilf(w * (float)sc), hh = (int)ceilf(h * (float)sc);
        if (ww < 1 || hh < 1) return false;
        size_t stride = (size_t)ww * 4;
        bits = calloc((size_t)ww * (size_t)hh, 4);
        if (!bits) return false;
        ctx = CGBitmapContextCreate(bits, ww, hh, 8, stride, gCS,
                                    kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host);
        if (!ctx) { free(bits); bits = nullptr; return false; }
        CGContextTranslateCTM(ctx, 0, hh);
        CGContextScaleCTM(ctx, (CGFloat)sc, -(CGFloat)sc);
        CGContextSetInterpolationQuality(ctx, kCGInterpolationHigh);
        wpx = ww; hpx = hh; scale = sc;
        return true;
    }
    void Snapshot() { if (ctx) { if (img) CGImageRelease(img); img = CGBitmapContextCreateImage(ctx); } }
};
static Layer gLStat, gLMenu;
static float  gLayerW = 0, gLayerH = 0;
static double gLayerScale = 0;

static void DrawHitLine(CGContextRef g);
static void DrawPads(CGContextRef g);

static void BuildStaticLayer(float w, float h, double scale) {
    gView.Setup(w, h);

    if (!gLStat.Build(w, h, scale)) return;
    DrawBackdrop(gLStat.ctx, w, h);
    DrawRoad(gLStat.ctx);
    gLStat.Snapshot();

    // 再烘一张"压暗版"给菜单用，省掉每帧一次全屏半透明填充
    if (gLMenu.Build(w, h, scale) && gLStat.img) {
        CGContextRef g2 = gLMenu.ctx;
        // 把静态层贴进来（两层都是 y 向下，这里要做一次翻转抵消）
        CGContextSaveGState(g2);
        CGContextTranslateCTM(g2, 0, h);
        CGContextScaleCTM(g2, 1, -1);
        CGContextDrawImage(g2, CGRectMake(0, 0, w, h), gLStat.img);
        CGContextRestoreGState(g2);
        // 判定线和键帽在菜单里永远是"暗着"的状态，一起烘进来
        DrawHitLine(g2);
        DrawPads(g2);
        FillRect(g2, 0, 0, w, h, COL(205, 0, 0, 0));
        gLMenu.Snapshot();
    }

    gLayerW = w; gLayerH = h; gLayerScale = scale;
}

static void DrawHitLine(CGContextRef g) {
    const View& V = gView;
    float pulse = 0.5f + 0.5f * sinf((float)gAnimT * 4.2f);

    for (int i = 3; i >= 1; i--) {
        Line(g, V.XB(0), V.hitY, V.XB(LANES), V.hitY,
             COL((int)(28 * i * 0.6f + 20), 60, 200, 255), 2.0f + i * 3.0f);
    }
    Line(g, V.XB(0), V.hitY, V.XB(LANES), V.hitY,
         COL(255, (int)(180 + 60 * pulse), 245, 255), 2.6f);
}

static void DrawPads(CGContextRef g) {
    const View& V = gView;
    float top = V.hitY + 7.0f;
    float hgt = V.H - top - 8.0f;
    if (hgt < 20) return;

    for (int i = 0; i < LANES; i++) {
        CGRect r = CGRectMake(V.XB(i) + 4.0f, top, V.lw - 8.0f, hgt);
        bool wrong = gWrongFlash[i] > 0;

        float lit = 0;
        for (auto& f : gFX) {
            if (f.lane != i) continue;
            float a = 1.0f - (float)(f.age / f.life);
            if (f.type == 0) lit = std::max(lit, a);
        }

        Col fill, border;
        if (wrong) {
            fill   = COL(150, 120, 12, 20);
            border = COL(235, 255, 80, 80);
        } else if (lit > 0) {
            fill   = COL((int)(50 + 70 * lit), 18, (int)(64 + 70 * lit), 58);
            border = COL(235, 120, 255, 170);
        } else {
            fill   = COL(56, 16, 44, 96);
            border = COL(150, 60, 140, 235);
        }

        FillRoundRect(g, r, 8.0f, fill, border, 1.8f);

        char num[4];
        snprintf(num, sizeof(num), "%d", i + 1);
        float fs = V.lw * 0.40f;
        if (fs > 30) fs = 30;
        if (fs < 12) fs = 12;
        Col tc = wrong ? COL(255, 255, 170, 170)
                       : (lit > 0 ? COL(255, 235, 255, 245) : COL(215, 175, 215, 255));
        // 键帽数字不加辉光：7 个数字 ×4 次叠绘，省下来很划算，肉眼几乎看不出差别
        Text_(g, num, r, fs, 1, true, tc, TA_CENTER, false);
    }
}

static void DrawTiles(CGContextRef g) {
    for (int idx = (int)gNotes.size() - 1; idx >= 0; idx--) {
        const Note& n = gNotes[idx];
        if (n.state == 2) continue;                     // 漏接的不再画

        if (n.state == 1) {                             // 已命中：定格在判定线，绿色闪光
            double age = gT - n.hitT;
            if (age > 0.30) continue;
            float alpha = (float)(1.0 - age / 0.30);
            DrawTile(g, n.lane, 1.0f, 1, alpha * 0.95f + 0.05f);
            continue;
        }

        float p = gView.PofT(n.t - gT);
        if (p < 0.0f)  continue;                        // 还在极远处
        if (p > 1.10f) continue;                        // 已经滑过判定线

        bool red = (n.redT >= 0 && gT - n.redT < 0.32);
        DrawTile(g, n.lane, p, red ? 2 : 0, 1.0f);
    }
}

static void DrawFX(CGContextRef g) {
    const View& V = gView;
    for (auto& f : gFX) {
        float a = 1.0f - (float)(f.age / f.life);
        if (a <= 0) continue;
        float cxL = (V.XB(f.lane) + V.XB(f.lane + 1)) * 0.5f;

        if (f.type == 0) {   // 绿色扩散环
            float r = 12.0f + 150.0f * (float)(f.age / f.life);
            CGContextBeginPath(g);
            CGContextSetLineWidth(g, 3.0f * a + 0.6f);
            SetStroke(g, COL((int)(190 * a), 120, 255, 160));
            CGContextStrokeEllipseInRect(g, CGRectMake(cxL - r, V.hitY - r * 0.42f, r * 2, r * 0.84f));

            // 车道上竖直的光柱
            FillRect(g, cxL - V.lw * 0.42f, V.hitY - 8 - 220 * (1 - a),
                     V.lw * 0.84f, 220 * (1 - a) + 8, COL((int)(70 * a), 120, 255, 190));
        } else {             // 红色警示环
            float r = 10.0f + 90.0f * (float)(f.age / f.life);
            CGContextBeginPath(g);
            CGContextSetLineWidth(g, 2.6f * a + 0.4f);
            SetStroke(g, COL((int)(200 * a), 255, 80, 80));
            CGContextStrokeEllipseInRect(g, CGRectMake(cxL - r, V.hitY - r * 0.4f, r * 2, r * 0.8f));
        }
    }

    // 火花
    for (auto& p : gParts) {
        float a = 1.0f - p.age / p.life;
        if (a <= 0) continue;
        SetFill(g, COL((int)(255 * a), (int)(p.r * 255), (int)(p.g * 255), (int)(p.b * 255)));
        CGContextFillEllipseInRect(g, CGRectMake(p.x - p.size, p.y - p.size, p.size * 2, p.size * 2));
    }
}

// ============================ HUD ============================
static void DrawHUD(CGContextRef g) {
    const View& V = gView;
    float W = V.W, H = V.H, S = V.S;

    float panelW = std::min(230.0f * S, W * 0.30f);

    // ---- 中上方：歌名 ----
    {
        float tsz = FitSize(gSongs[gSel].title, 34.0f * S, 0, true,
                            W - 2.0f * (panelW + 30.0f * S) - 20.0f * S);
        Text_(g, gSongs[gSel].title, CGRectMake(0, H * 0.020f, W, 48 * S),
              tsz, 0, true, COL(255, 205, 242, 255), TA_CENTER, true);

        Text_(g, gSongs[gSel].artist, CGRectMake(0, H * 0.020f + 44 * S, W, 26 * S),
              15.0f * S, 0, false, COL(185, 130, 185, 245), TA_CENTER, false);
    }

    // ---- 右上角：计分器 ----
    {
        float ph = 124.0f * S;
        CGRect panel = CGRectMake(W - panelW - 26 * S, 20 * S, panelW, ph);
        FillRoundRect(g, panel, 12.0f * S, COL(120, 6, 16, 40), COL(150, 60, 170, 255), 1.6f);

        Text_(g, "得  分", CGRectMake(panel.origin.x, panel.origin.y + 6 * S, panel.size.width, 18 * S),
              13.0f * S, 0, true, COL(190, 110, 190, 255), TA_CENTER, false);

        char buf[32];
        snprintf(buf, sizeof(buf), "%d", gScore);
        Text_(g, buf, CGRectMake(panel.origin.x, panel.origin.y + 20 * S, panel.size.width, 64 * S),
              52.0f * S, 1, true, COL(255, 130, 235, 255), TA_CENTER, true);

        char cb[64];
        if (gCombo >= 1) snprintf(cb, sizeof(cb), "COMBO  %d", gCombo);
        else             snprintf(cb, sizeof(cb), "COMBO  -");
        Col cc = (gCombo >= 10) ? COL(255, 255, 220, 120) : COL(200, 150, 210, 255);
        Text_(g, cb, CGRectMake(panel.origin.x, panel.origin.y + 86 * S, panel.size.width, 22 * S),
              15.0f * S, 1, true, cc, TA_CENTER, gCombo >= 10);
    }

    // ---- 左上角：进度 / 提示 ----
    {
        double cur = std::max(0.0, gT), tot = gSongLen;
        int cm = (int)(cur / 60), cs = (int)(cur) % 60;
        int tm = (int)(tot / 60), ts = (int)(tot) % 60;
        char tb[64];
        snprintf(tb, sizeof(tb), "%d:%02d / %d:%02d", cm, cs, tm, ts);
        Text_(g, tb, CGRectMake(24 * S, 22 * S, 220 * S, 22 * S),
              15.0f * S, 1, true, COL(200, 140, 200, 255), TA_NEAR, false);

        Text_(g, "1~7 击键   SPACE 暂停   Cmd+F 全屏   ESC 菜单",
              CGRectMake(24 * S, 46 * S, 460 * S, 20 * S),
              13.0f * S, 0, false, COL(130, 110, 150, 200), TA_NEAR, false);

        if (!gHasMusic) {
            Text_(g, "♪ 该曲目无音频，静音跑谱面",
                  CGRectMake(24 * S, 68 * S, 420 * S, 20 * S),
                  13.0f * S, 0, false, COL(150, 220, 170, 120), TA_NEAR, false);
        }
    }

    // ---- 顶部进度条 ----
    {
        float prog = (float)(std::max(0.0, gT) / gSongLen);
        if (prog > 1) prog = 1;
        FillRect(g, 0, 0, W, 3 * S, COL(90, 40, 90, 160));
        FillRect(g, 0, 0, W * prog, 3 * S, COL(220, 90, 210, 255));
    }

    // ---- 倒计时 ----
    if (gState == ST_COUNTDOWN) {
        int n = (int)ceil(-gT);
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", n > 0 ? n : 0);
        float p = (float)(-gT) - floorf((float)(-gT));
        int a = (int)(120 + 135 * p);
        Text_(g, buf, CGRectMake(0, H * 0.30f, W, 150 * S),
              128.0f * S, 1, true, COL(a, 150, 230, 255), TA_CENTER, true);
    }
}

// ============================ 菜单 / 暂停 / 结算 ============================
static void DrawMenu(CGContextRef g) {
    const View& V = gView;
    float W = V.W, H = V.H, S = V.S;

    Text_(g, "PIANO  TILES", CGRectMake(0, H * 0.085f, W, 78 * S),
          std::min(64.0f * S, H * 0.10f), 1, true, COL(255, 150, 235, 255), TA_CENTER, true);

    Text_(g, "钢 琴 键 · 七 键 节 奏", CGRectMake(0, H * 0.085f + 72 * S, W, 28 * S),
          17.0f * S, 0, false, COL(180, 120, 180, 240), TA_CENTER, false);

    float rw   = std::min(640.0f * S, W * 0.86f);
    float rowH = 60.0f * S;
    float listT = H * 0.30f;
    float x0 = W * 0.5f - rw * 0.5f;

    for (int i = 0; i < 4; i++) {
        CGRect row = CGRectMake(x0, listT + i * rowH, rw, rowH - 10 * S);
        bool sel = (i == gSel);

        if (sel) FillRoundRect(g, row, 10.0f * S, COL(190, 14, 60, 130), COL(240, 90, 210, 255), 2.0f);

        char idx[8];
        snprintf(idx, sizeof(idx), "%d", i + 1);
        Text_(g, idx, CGRectMake(row.origin.x + 12 * S, row.origin.y, 30 * S, row.size.height),
              20.0f * S, 1, true, sel ? COL(255, 150, 230, 255) : COL(140, 110, 140, 200),
              TA_CENTER, false);

        Text_(g, gSongs[i].title,
              CGRectMake(row.origin.x + 50 * S, row.origin.y + 3 * S, rw * 0.62f, 30 * S),
              21.0f * S, 0, true, sel ? COL(255, 225, 250, 255) : COL(175, 150, 175, 230),
              TA_NEAR, false);

        Text_(g, gSongs[i].artist,
              CGRectMake(row.origin.x + 50 * S, row.origin.y + 30 * S, rw * 0.66f, 22 * S),
              13.0f * S, 0, false, COL(150, 120, 160, 230), TA_NEAR, false);

        Text_(g, gAudioFound[i] ? "♪ 音频就绪" : "— 无音频 —",
              CGRectMake(row.origin.x + row.size.width - 138 * S, row.origin.y, 128 * S, row.size.height),
              12.0f * S, 0, false,
              gAudioFound[i] ? COL(200, 120, 220, 160) : COL(150, 210, 130, 130),
              TA_FAR, false);
    }

    Text_(g, "↑ ↓ 选择曲目      ENTER 开始      Cmd+F 全屏      ESC 退出",
          CGRectMake(0, listT + 4 * rowH + 16 * S, W, 26 * S),
          15.0f * S, 0, false, COL(160, 150, 190, 235), TA_CENTER, false);
}

static void DrawPause(CGContextRef g) {
    const View& V = gView;
    float S = V.S;
    FillRect(g, 0, 0, V.W, V.H, COL(170, 0, 0, 0));
    Text_(g, "PAUSED", CGRectMake(0, V.H * 0.40f, V.W, 70 * S),
          56.0f * S, 1, true, COL(255, 140, 220, 255), TA_CENTER, true);
    Text_(g, "SPACE 继续      ESC 返回菜单      R 重新开始",
          CGRectMake(0, V.H * 0.40f + 78 * S, V.W, 28 * S),
          17.0f * S, 0, false, COL(190, 170, 200, 235), TA_CENTER, false);
}

static void DrawResult(CGContextRef g) {
    const View& V = gView;
    float W = V.W, H = V.H, S = V.S;
    FillRect(g, 0, 0, W, H, COL(190, 0, 0, 0));

    Text_(g, "COMPLETE", CGRectMake(0, H * 0.14f, W, 66 * S),
          52.0f * S, 1, true, COL(255, 150, 240, 255), TA_CENTER, true);

    float tsz = FitSize(gSongs[gSel].title, 20.0f * S, 0, false, W * 0.90f);
    Text_(g, gSongs[gSel].title, CGRectMake(0, H * 0.14f + 64 * S, W, 30 * S),
          tsz, 0, false, COL(200, 140, 200, 245), TA_CENTER, false);

    char buf[64];
    snprintf(buf, sizeof(buf), "%d", gScore);
    Text_(g, buf, CGRectMake(0, H * 0.28f, W, 108 * S),
          92.0f * S, 1, true, COL(255, 120, 230, 255), TA_CENTER, true);

    double acc = gTotal > 0 ? (100.0 * gHits / gTotal) : 0.0;

    char b1[32], b3[32], b4[32];
    snprintf(b1, sizeof(b1), "%d", gHits);
    snprintf(b3, sizeof(b3), "%.1f%%", acc);
    snprintf(b4, sizeof(b4), "%d", gWrong);

    struct Item { const char* label; std::string val; };
    char c2[32]; snprintf(c2, sizeof(c2), "%d", gMaxCombo);
    Item items[4] = {
        { "命中 / 总数", std::string(b1) + " / " + std::to_string(gTotal) },
        { "最高连击",   std::string(c2) },
        { "准确率",     std::string(b3) },
        { "误按次数",   std::string(b4) },
    };

    float bw = 168.0f * S, gap = 16.0f * S;
    if (bw * 4 + gap * 3 > W * 0.92f) {
        bw = W * 0.92f / 4 * 0.94f;
        gap = W * 0.92f / 4 * 0.06f;
    }
    float startX = W * 0.5f - (bw * 4 + gap * 3) * 0.5f;
    float by = H * 0.50f;
    for (int i = 0; i < 4; i++) {
        CGRect r = CGRectMake(startX + i * (bw + gap), by, bw, 84 * S);
        FillRoundRect(g, r, 10.0f * S, COL(150, 10, 26, 58), COL(140, 70, 170, 255), 1.3f);

        Text_(g, items[i].val, CGRectMake(r.origin.x, r.origin.y + 10 * S, r.size.width, 36 * S),
              26.0f * S, 1, true, COL(255, 200, 245, 255), TA_CENTER, false);
        Text_(g, items[i].label, CGRectMake(r.origin.x, r.origin.y + 50 * S, r.size.width, 24 * S),
              13.0f * S, 0, false, COL(180, 150, 185, 235), TA_CENTER, false);
    }

    Text_(g, "ENTER / R 再来一次      ESC 返回菜单",
          CGRectMake(0, by + 108 * S, W, 28 * S),
          17.0f * S, 0, false, COL(190, 170, 200, 240), TA_CENTER, false);
}

// ============================ 渲染一帧 ============================
static void RenderScene(CGContextRef g, float w, float h, double scale) {
    CGContextSetAllowsAntialiasing(g, true);
    CGContextSetShouldAntialias(g, true);

    gView.Setup(w, h);

    bool menu = (gState == ST_MENU);
    if (!gLStat.img || !gLMenu.img ||
        (int)w != (int)gLayerW || (int)h != (int)gLayerH || scale != gLayerScale) {
        BuildStaticLayer(w, h, scale);
    }

    // 背景 + 道路整层直接贴缓存：两次翻转互相抵消，得到 1:1 的像素拷贝
    CGImageRef base = menu ? gLMenu.img : gLStat.img;
    if (base) {
        CGContextSaveGState(g);
        CGContextTranslateCTM(g, 0, h);
        CGContextScaleCTM(g, 1, -1);
        CGContextSetBlendMode(g, kCGBlendModeCopy);
        CGContextDrawImage(g, CGRectMake(0, 0, w, h), base);
        CGContextRestoreGState(g);
        CGContextSetBlendMode(g, kCGBlendModeNormal);
    } else {
        FillRect(g, 0, 0, w, h, COL(255, 0, 0, 0));
    }

    DrawDust(g, w, h, menu ? 0.18f : 1.0f);
    if (!menu) {                 // 菜单层里已经烘好了判定线和键帽
        DrawTiles(g);
        DrawHitLine(g);
        DrawPads(g);
        DrawFX(g);
    }

    if (gShowFps) {
        char fb[64];
        snprintf(fb, sizeof(fb), "%.0f FPS   %.2f ms", gFpsAvg, gFrameMs);
        Text_(g, fb, CGRectMake(14 * gView.S, h - 26 * gView.S, 260 * gView.S, 20 * gView.S),
              13.0f * gView.S, 1, true, COL(220, 130, 255, 170), TA_NEAR, false);
    }

    if (gState == ST_MENU) {
        DrawMenu(g);
    } else if (gState == ST_LOADING) {
        FillRect(g, 0, 0, w, h, COL(235, 0, 0, 0));
        Text_(g, "正在解码音乐…", CGRectMake(0, h * 0.44f, w, 40),
              26.0f, 0, true, COL(255, 150, 230, 255), TA_CENTER, true);
        Text_(g, gSongs[gSel].title, CGRectMake(0, h * 0.44f + 44, w, 26),
              14.0f, 0, false, COL(170, 130, 175, 235), TA_CENTER, false);
    } else if (gState == ST_RESULT) {
        DrawResult(g);
    } else {
        DrawHUD(g);
        if (gState == ST_PAUSE) DrawPause(g);
    }
}

// ============================ 视图 / 窗口 ============================
static CVDisplayLinkRef gLink = nullptr;
static double gLastT = 0;

@class GameView;
static NSWindow* gNSWindow = nil;
static GameView* gNSView   = nil;

enum VKey { K_NONE = 0, K_ENTER, K_SPACE, K_ESC, K_UP, K_DOWN, K_LEFT, K_RIGHT, K_R, K_FULL, K_FPS };

static void HandleKey(VKey k) {
    if (k == K_FULL) {
        if (gNSWindow) [gNSWindow toggleFullScreen:nil];
        return;
    }
    if (k == K_FPS) { gShowFps = !gShowFps; return; }

    if (gState == ST_MENU) {
        if (k == K_UP)   { gSel = (gSel + 3) % 4; }
        if (k == K_DOWN) { gSel = (gSel + 1) % 4; }
        if (k == K_LEFT) { gSel = (gSel + 3) % 4; }
        if (k == K_RIGHT){ gSel = (gSel + 1) % 4; }
        if (k == K_ENTER) StartSong(gSel);
        if (k == K_ESC)   [NSApp terminate:nil];
        return;
    }
    if (gState == ST_RESULT) {
        if (k == K_ENTER || k == K_R) StartSong(gSel);
        else if (k == K_ESC) BackToMenu();
        return;
    }
    if (gState == ST_PAUSE) {
        if (k == K_SPACE) { gState = ST_PLAY; if (gHasMusic) MusicPlay(); }
        else if (k == K_ESC) BackToMenu();
        else if (k == K_R) StartSong(gSel);
        return;
    }
    if (k == K_ESC)   { BackToMenu(); return; }
    if (k == K_SPACE) { gState = ST_PAUSE; if (gHasMusic) MusicPause(); return; }
    if (k == K_R && gState != ST_COUNTDOWN) { StartSong(gSel); return; }
}

static void HandleLane(int lane) {
    if (gState == ST_PLAY) Press(lane);
}

@interface GameView : NSView
@end

@implementation GameView
- (BOOL)isFlipped { return NO; }        // 我们自己用 CTM 翻转，方便复用 Windows 版的坐标
- (BOOL)isOpaque  { return YES; }
- (BOOL)acceptsFirstResponder { return YES; }

- (void)drawRect:(NSRect)dirtyRect {
    CGContextRef ctx = [[NSGraphicsContext currentContext] CGContext];
    if (!ctx) return;
    NSRect b = self.bounds;
    float w = (float)b.size.width, h = (float)b.size.height;
    if (w <= 0 || h <= 0) return;
    double scale = self.window ? self.window.backingScaleFactor : 1.0;
    if (scale < 1.0) scale = 1.0;

    CGContextSaveGState(ctx);
    // 翻成 y 向下，和 Windows 版完全一致
    CGContextTranslateCTM(ctx, 0, h);
    CGContextScaleCTM(ctx, 1, -1);
    RenderScene(ctx, w, h, scale);
    CGContextRestoreGState(ctx);
}

- (void)keyDown:(NSEvent*)e {
    if (e.isARepeat) return;                      // 忽略按住不放的自动重复
    unsigned short kc = e.keyCode;
    BOOL cmd = (e.modifierFlags & NSEventModifierFlagCommand) != 0;

    NSString* ch = e.characters;
    if (ch.length >= 1) {
        unichar c = [ch characterAtIndex:0];
        if (c >= '1' && c <= '7') { HandleLane(c - '1'); return; }
    }
    if (kc >= 83 && kc <= 89) { HandleLane((int)(kc - 83)); return; }   // 小键盘 1~7

    switch (kc) {
        case 36: case 76: HandleKey(K_ENTER); return;   // Return / 小键盘回车
        case 49:          HandleKey(K_SPACE); return;
        case 53:          HandleKey(K_ESC);   return;
        case 126:         HandleKey(K_UP);    return;
        case 125:         HandleKey(K_DOWN);  return;
        case 123:         HandleKey(K_LEFT);  return;
        case 124:         HandleKey(K_RIGHT); return;
        case 15:          HandleKey(K_R);     return;   // R
        case 103:         HandleKey(K_FULL);  return;   // F11
        case 99:          HandleKey(K_FPS);   return;   // F3
        case 3:  if (cmd) { HandleKey(K_FULL); return; } break;   // Cmd+F
        case 2:  if (cmd) { HandleKey(K_FPS);  return; } break;   // Cmd+D
        default: break;
    }
    [super keyDown:e];
}
@end

static void RenderNow() {
    if (gNSView) [gNSView display];
}

static void GameTick() {
    double now = CACurrentMediaTime();
    double dt = now - gLastT;
    gLastT = now;
    if (dt > 0.08) dt = 0.08;
    if (dt < 0)    dt = 0;

    if (dt > 0) {                      // 平滑统计，避免数字乱跳
        double ms = dt * 1000.0;
        gFrameMs += (ms - gFrameMs) * 0.08;
        gFpsAvg  += (1000.0 / ms - gFpsAvg) * 0.08;
    }
    Update(dt);
    RenderNow();
}

static CVReturn LinkCallback(CVDisplayLinkRef link, const CVTimeStamp* now,
                             const CVTimeStamp* out, CVOptionFlags flagsIn,
                             CVOptionFlags* flagsOut, void* user) {
    dispatch_async(dispatch_get_main_queue(), ^{ @autoreleasepool { GameTick(); } });
    return kCVReturnSuccess;
}

// ============================ 调试：离屏导出 PNG ============================
static void WritePNG(CGImageRef img, const char* path) {
    NSString* p = [NSString stringWithUTF8String:path];
    if (!p) return;
    NSURL* url = [NSURL fileURLWithPath:p];
    CGImageDestinationRef dest = CGImageDestinationCreateWithURL((__bridge CFURLRef)url,
                                                                 CFSTR("public.png"), 1, nullptr);
    if (!dest) return;
    CGImageDestinationAddImage(dest, img, nullptr);
    CGImageDestinationFinalize(dest);
    CFRelease(dest);
}

static void RunShotMode() {
    [[NSFileManager defaultManager] createDirectoryAtPath:@"shots"
                              withIntermediateDirectories:YES attributes:nil error:nil];
    struct Sz { int w, h; };
    Sz sizes[3] = { { 1280, 812 }, { 900, 600 }, { 1600, 900 } };
    const char* names[3] = { "shots/1_1280x812.png", "shots/2_900x600.png", "shots/3_1600x900.png" };

    for (int k = 0; k < 3; k++) {
        int w = sizes[k].w, h = sizes[k].h;
        void* bits = calloc((size_t)w * (size_t)h, 4);
        if (!bits) continue;
        CGContextRef ctx = CGBitmapContextCreate(bits, w, h, 8, (size_t)w * 4, gCS,
                                kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host);
        if (!ctx) { free(bits); continue; }
        CGContextTranslateCTM(ctx, 0, h);
        CGContextScaleCTM(ctx, 1, -1);

        if (k == 0) { gState = ST_MENU; }
        else {
            if (gState != ST_PLAY && gSel == 0 && gNotes.empty()) StartSong(0);
            gState = ST_PLAY;
            gT = 8.0;
            Press(0); Press(3);
            Update(0.016);
        }
        RenderScene(ctx, (float)w, (float)h, 1.0);
        CGImageRef img = CGBitmapContextCreateImage(ctx);
        if (img) { WritePNG(img, names[k]); CGImageRelease(img); }
        CGContextRelease(ctx);
        free(bits);
    }
    printf("shots written\n");
}

// ============================ 应用入口 ============================
@interface AppDelegate : NSObject <NSApplicationDelegate>
@end
static AppDelegate* gDelegate = nil;

static void BuildMenuBar() {
    NSMenu* main = [[NSMenu alloc] init];
    NSMenuItem* appItem = [[NSMenuItem alloc] init];
    NSMenu* appMenu = [[NSMenu alloc] init];
    NSString* title = @"钢琴键游戏  Piano Tiles";
    [appMenu addItemWithTitle:[NSString stringWithFormat:@"关于 %@", title]
                       action:@selector(orderFrontStandardAboutPanel:) keyEquivalent:@""];
    [appMenu addItem:[NSMenuItem separatorItem]];
    NSMenuItem* quit = [appMenu addItemWithTitle:[NSString stringWithFormat:@"退出 %@", title]
                                          action:@selector(terminate:) keyEquivalent:@"q"];
    quit.keyEquivalentModifierMask = NSEventModifierFlagCommand;
    appItem.submenu = appMenu;
    [main addItem:appItem];
    [NSApp setMainMenu:main];
}

@implementation AppDelegate
- (void)applicationDidFinishLaunching:(NSNotification*)note {
    gCS = CGColorSpaceCreateDeviceRGB();
    srand((unsigned)time(nullptr));

    gAudio = [[AudioBox alloc] init];
    InitSfx();
    ScanAudio();

    // 背景微尘
    for (int i = 0; i < 60; i++) {
        Dust d;
        d.x = (rand() % 10000) / 10000.0f;
        d.y = (rand() % 10000) / 10000.0f;
        d.vy = 0.008f + (rand() % 100) / 10000.0f;
        d.size = 0.6f + (rand() % 18) / 10.0f;
        d.alpha = 0.10f + (rand() % 25) / 100.0f;
        gDust.push_back(d);
    }

    NSRect frame = NSMakeRect(0, 0, 1280, 812);
    NSUInteger style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                       NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;
    gNSWindow = [[NSWindow alloc] initWithContentRect:frame
                                          styleMask:style
                                            backing:NSBackingStoreBuffered
                                              defer:NO];
    [gNSWindow setTitle:@"钢琴键游戏  Piano Tiles"];
    [gNSWindow setContentMinSize:NSMakeSize(880, 580)];
    [gNSWindow setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
    [gNSWindow center];

    gNSView = [[GameView alloc] initWithFrame:frame];
    [gNSView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    [gNSWindow setContentView:gNSView];
    [gNSWindow makeFirstResponder:gNSView];
    [gNSWindow makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];

    // 调试参数
    NSArray* args = [[NSProcessInfo processInfo] arguments];
    BOOL shotMode = [args containsObject:@"--shot"];
    BOOL playMode = [args containsObject:@"--play"];
    if (shotMode) gShowFps = NO;

    if (playMode) StartSong(0);

    gLastT = CACurrentMediaTime();

    if (shotMode) {
        RunShotMode();
        [NSApp terminate:nil];
        return;
    }

    // 用 CVDisplayLink 贴着显示器刷新率跑（不会撕裂也不会跳帧）
    if (CVDisplayLinkCreateWithActiveCGDisplays(&gLink) == kCVReturnSuccess && gLink) {
        CVDisplayLinkSetOutputCallback(gLink, LinkCallback, nullptr);
        CVDisplayLinkStart(gLink);
    } else {
        [NSTimer scheduledTimerWithTimeInterval:1.0 / 60.0
                                         target:self
                                       selector:@selector(tick:)
                                       userInfo:nil
                                        repeats:YES];
    }
}
- (void)tick:(NSTimer*)t { GameTick(); }

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)app { return YES; }

- (void)applicationWillTerminate:(NSNotification*)note {
    if (gLink) { CVDisplayLinkStop(gLink); CVDisplayLinkRelease(gLink); gLink = nullptr; }
    AudioCloseMusic();
    gLStat.Free();
    gLMenu.Free();
    if (gCS) { CGColorSpaceRelease(gCS); gCS = nullptr; }
}
@end

int main(int argc, const char* argv[]) {
    @autoreleasepool {
        NSApplication* app = [NSApplication sharedApplication];
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];
        gDelegate = [[AppDelegate alloc] init];
        [app setDelegate:gDelegate];
        BuildMenuBar();
        [app run];
    }
    return 0;
}
