// ============================================================
//  gshim.h —— GDI+ 兼容层（只在 macOS 编译时生效，HSW_MAC）
//
//  目的：让源码里 123 处 Graphics、109 处 GraphicsPath、183 处 SolidBrush、
//        104 处 Pen、70 处 Bitmap…… 这些 GDI+ 调用原封不动地编译通过。
//
//  做法：类名、方法签名全部照抄 GDI+，实现换成 CoreGraphics / CoreText。
//  内部状态用 void*（PIMPL），所以这个头文件在 Windows 上也能编译 —— 
//  于是可以用 `g++ -DHSW_MAC -fsyntax-only` 在 Windows 上做签名自检，
//  不用等到 CI 才发现少了个方法。
// ============================================================
#ifndef HSW_GSHIM_H
#define HSW_GSHIM_H

#if defined(HSW_MAC)

#include <cstdint>
#include <cstring>
#include <cwchar>

typedef float REAL;

// 见 winshim.h 里的说明
#if !defined(__OBJC__)
typedef int BOOL;
#endif
typedef unsigned int UINT32;
typedef unsigned long long UINT64;
class GraphicsPath;
typedef unsigned char BYTE;
typedef int   INT;
typedef float GraphicsState;     // GDI+ 里是 GraphicsState，这里用 int 编号代替

// ---------------- 枚举（取值照抄 GDI+，方便直接对照文档） ----------------
enum Status { Ok = 0, GenericError = 1, InvalidParameter = 2, OutOfMemory = 3,
              ObjectBusy = 4, InsufficientBuffer = 5, NotImplemented = 6,
              Win32Error = 7, WrongState = 8, Aborted = 9, FileNotFound = 10,
              ValueOverflow = 11, AccessDenied = 12, UnknownImageFormat = 13,
              FontFamilyNotFound = 14, FontStyleNotFound = 15, NotTrueTypeFont = 16,
              UnsupportedGdiplusVersion = 17, GdiplusNotInitialized = 18,
              PropertyNotFound = 19, PropertyNotSupported = 20 };

enum SmoothingMode { SmoothingModeInvalid = -1, SmoothingModeDefault = 0,
                     SmoothingModeHighSpeed = 1, SmoothingModeHighQuality = 2,
                     SmoothingModeNone = 3, SmoothingModeAntiAlias = 4 };
enum InterpolationMode { InterpolationModeInvalid = -1, InterpolationModeDefault = 0,
                     InterpolationModeLowQuality = 1, InterpolationModeHighQuality = 2,
                     InterpolationModeBilinear = 3, InterpolationModeBicubic = 4,
                     InterpolationModeNearestNeighbor = 5,
                     InterpolationModeHighQualityBilinear = 6,
                     InterpolationModeHighQualityBicubic = 7 };
enum PixelOffsetMode { PixelOffsetModeInvalid = -1, PixelOffsetModeDefault = 0,
                     PixelOffsetModeHighSpeed = 1, PixelOffsetModeHighQuality = 2,
                     PixelOffsetModeNone = 3, PixelOffsetModeHalf = 4 };
enum CompositingMode { CompositingModeSourceOver = 0, CompositingModeSourceCopy = 1 };
enum CompositingQuality { CompositingQualityInvalid = -1, CompositingQualityDefault = 0,
                     CompositingQualityHighSpeed = 1, CompositingQualityHighQuality = 2,
                     CompositingQualityGammaCorrected = 3, CompositingQualityAssumeLinear = 4 };
enum TextRenderingHint { TextRenderingHintSystemDefault = 0,
                     TextRenderingHintSingleBitPerPixelGridFit = 1,
                     TextRenderingHintSingleBitPerPixel = 2,
                     TextRenderingHintAntiAliasGridFit = 3,
                     TextRenderingHintAntiAlias = 4,
                     TextRenderingHintClearTypeGridFit = 5 };
enum Unit { UnitWorld = 0, UnitDisplay = 1, UnitPixel = 2, UnitPoint = 3,
            UnitInch = 4, UnitDocument = 5, UnitMillimeter = 6 };
enum PixelFormat { PixelFormatIndexed = 0x00010000, PixelFormatGDI = 0x00020000,
            PixelFormatAlpha = 0x00040000, PixelFormatPAlpha = 0x00080000,
            PixelFormatExtended = 0x00100000, PixelFormatCanonical = 0x00200000,
            PixelFormatUndefined = 0, PixelFormatDontCare = 0,
            PixelFormat1bppIndexed = 196865, PixelFormat4bppIndexed = 197634,
            PixelFormat8bppIndexed = 198659, PixelFormat16bppGrayScale = 65793,
            PixelFormat16bppRGB555 = 135173, PixelFormat16bppRGB565 = 135174,
            PixelFormat16bppARGB1555 = 397319, PixelFormat24bppRGB = 137224,
            PixelFormat32bppRGB = 139273, PixelFormat32bppARGB = 2498570,
            PixelFormat32bppPARGB = 925707, PixelFormat48bppRGB = 1060876,
            PixelFormat64bppARGB = 3424269, PixelFormat64bppPARGB = 1851406 };
enum StringAlignment { StringAlignmentNear = 0, StringAlignmentCenter = 1, StringAlignmentFar = 2 };
enum CombineMode { CombineModeReplace = 0, CombineModeIntersect = 1, CombineModeUnion = 2,
                   CombineModeXor = 3, CombineModeExclude = 4, CombineModeComplement = 5 };
enum FillMode { FillModeAlternate = 0, FillModeWinding = 1 };
enum LineJoin { LineJoinMiter = 0, LineJoinBevel = 1, LineJoinRound = 2,
                LineJoinMiterClipped = 3 };
enum LineCap { LineCapFlat = 0, LineCapSquare = 1, LineCapRound = 2,
               LineCapTriangle = 3, LineCapNoAnchor = 0x10,
               LineCapSquareAnchor = 0x11, LineCapRoundAnchor = 0x12,
               LineCapDiamondAnchor = 0x13, LineCapArrowAnchor = 0x14,
               LineCapCustom = 0xff };
enum DashStyle { DashStyleSolid = 0, DashStyleDash = 1, DashStyleDot = 2,
                 DashStyleDashDot = 3, DashStyleDashDotDot = 4, DashStyleCustom = 5 };
enum FontStyle { FontStyleRegular = 0, FontStyleBold = 1, FontStyleItalic = 2,
                 FontStyleBoldItalic = 3, FontStyleUnderline = 4, FontStyleStrikeout = 8 };
enum LinearGradientMode { LinearGradientModeHorizontal = 0,
                          LinearGradientModeVertical = 1,
                          LinearGradientModeForwardDiagonal = 2,
                          LinearGradientModeBackwardDiagonal = 3 };
enum WarpMode { WarpModePerspective = 0, WarpModeBilinear = 1 };
enum MatrixOrder { MatrixOrderPrepend = 0, MatrixOrderAppend = 1 };
enum ColorAdjustType { ColorAdjustTypeDefault = 0, ColorAdjustTypeBitmap = 1,
                       ColorAdjustTypeBrush = 2, ColorAdjustTypePen = 3,
                       ColorAdjustTypeText = 4 };
enum ColorMatrixFlags { ColorMatrixFlagsDefault = 0, ColorMatrixFlagsSkipGrays = 1,
                        ColorMatrixFlagsAltGray = 2 };
enum StringFormatFlags { StringFormatFlagsDirectionRightToLeft = 0x00000001,
                         StringFormatFlagsDirectionVertical = 0x00000002,
                         StringFormatFlagsNoFitBlackBox = 0x00000004,
                         StringFormatFlagsDisplayFormatControl = 0x00000020,
                         StringFormatFlagsNoFontFallback = 0x00000400,
                         StringFormatFlagsMeasureTrailingSpaces = 0x00000800,
                         StringFormatFlagsNoWrap = 0x00001000,
                         StringFormatFlagsLineLimit = 0x00002000,
                         StringFormatFlagsNoClip = 0x00004000 };
enum BrushType { BrushTypeSolidColor = 0, BrushTypeHatchFill = 1,
                 BrushTypeTextureFill = 2, BrushTypePathGradient = 3,
                 BrushTypeLinearGradient = 4 };
enum ImageFlags { ImageFlagsNone = 0 };

enum ImageLockMode { ImageLockModeRead = 1, ImageLockModeWrite = 2,
                     ImageLockModeUserInputBuf = 4 };
enum DashCap { DashCapFlat = 0, DashCapRound = 2, DashCapTriangle = 3 };

// ---------------- 基础值类型 ----------------
struct Color {
    unsigned int Value;                      // ARGB，和 GDI+ 一样是 0xAARRGGBB
    Color() : Value(0xFF000000u) {}
    Color(unsigned int argb) : Value(argb) {}
    Color(unsigned char a, unsigned char r, unsigned char g, unsigned char b)
        : Value(((unsigned)a << 24) | ((unsigned)r << 16) | ((unsigned)g << 8) | (unsigned)b) {}
    Color(unsigned char r, unsigned char g, unsigned char b)
        : Value(0xFF000000u | ((unsigned)r << 16) | ((unsigned)g << 8) | (unsigned)b) {}
    unsigned char GetA() const { return (unsigned char)((Value >> 24) & 0xFF); }
    unsigned char GetR() const { return (unsigned char)((Value >> 16) & 0xFF); }
    unsigned char GetG() const { return (unsigned char)((Value >> 8) & 0xFF); }
    unsigned char GetB() const { return (unsigned char)(Value & 0xFF); }
    unsigned int  GetValue() const { return Value; }
    void SetValue(unsigned int v) { Value = v; }
    static Color MakeARGB(unsigned char a, unsigned char r, unsigned char g, unsigned char b)
        { return Color(a, r, g, b); }
};

struct HswPoint {
    int X, Y;
    HswPoint() : X(0), Y(0) {}
    HswPoint(int x, int y) : X(x), Y(y) {}
};
struct PointF {
    REAL X, Y;
    PointF() : X(0), Y(0) {}
    PointF(REAL x, REAL y) : X(x), Y(y) {}
};
struct SizeF {
    REAL Width, Height;
    SizeF() : Width(0), Height(0) {}
    SizeF(REAL w, REAL h) : Width(w), Height(h) {}
};
struct RectF {
    REAL X, Y, Width, Height;
    RectF() : X(0), Y(0), Width(0), Height(0) {}
    RectF(REAL x, REAL y, REAL w, REAL h) : X(x), Y(y), Width(w), Height(h) {}
    REAL GetLeft()   const { return X; }
    REAL GetTop()    const { return Y; }
    REAL GetRight()  const { return X + Width; }
    REAL GetBottom() const { return Y + Height; }
    BOOL IsEmptyArea() const { return (Width <= 0) || (Height <= 0); }
    BOOL Contains(REAL x, REAL y) const
        { return x >= X && x < X + Width && y >= Y && y < Y + Height; }
};
struct HswRect {
    int X, Y, Width, Height;
    HswRect() : X(0), Y(0), Width(0), Height(0) {}
    HswRect(int x, int y, int w, int h) : X(x), Y(y), Width(w), Height(h) {}
    int GetLeft()   const { return X; }
    int GetTop()    const { return Y; }
    int GetRight()  const { return X + Width; }
    int GetBottom() const { return Y + Height; }
};

struct ColorMatrix {
    REAL m[5][5];
};

struct BitmapData {
    unsigned int Width;
    unsigned int Height;
    int          Stride;
    int          PixelFormat_;     // GDI+ 里这个成员叫 PixelFormat，会和枚举撞名，这里改名
    void*        Scan0;
    unsigned long long Reserved;
};

// ---------------- 画刷 / 画笔 ----------------
class Brush {
public:
    void* impl;
    Brush() : impl(nullptr) {}
    virtual ~Brush() {}
};

class SolidBrush : public Brush {
public:
    Color col;
    SolidBrush();
    SolidBrush(const Color& c);
    ~SolidBrush();
    Status GetColor(Color* c) const;
    Status SetColor(const Color& c);
};

class LinearGradientBrush : public Brush {
public:
    LinearGradientBrush();
    LinearGradientBrush(const PointF& p0, const PointF& p1, const Color& c0, const Color& c1);
    LinearGradientBrush(const HswPoint& p0, const HswPoint& p1, const Color& c0, const Color& c1);
    LinearGradientBrush(const RectF& r, const Color& c0, const Color& c1, LinearGradientMode mode);
    LinearGradientBrush(const RectF& r, const Color& c0, const Color& c1, REAL angle, BOOL isAngleScalable);
    ~LinearGradientBrush();
    Status SetWrapMode(int wrap);
    Status SetBlend(const REAL* factors, const REAL* positions, int count);
    Status SetGammaCorrection(BOOL on);
};

// 径向渐变（GDI+ 的 PathGradientBrush）：中心色 + 边缘一圈色
class PathGradientBrush : public Brush {
public:
    PathGradientBrush();
    PathGradientBrush(const PointF* pts, int n);
    PathGradientBrush(const GraphicsPath* p);
    ~PathGradientBrush();
    Status SetCenterColor(const Color& c);
    Status SetSurroundColors(const Color* cols, int* count);
    Status SetCenterPoint(const PointF& p);
    Status SetFocusScales(REAL x, REAL y);
};

class Pen {
public:
    void* impl;
    Color  col;
    REAL   width;
    Pen();
    Pen(const Color& c, REAL w);
    ~Pen();
    Status SetColor(const Color& c);
    Status SetWidth(REAL w);
    Status SetLineJoin(LineJoin j);
    Status SetStartCap(LineCap c);
    Status SetEndCap(LineCap c);
    Status SetLineCap(LineCap start, LineCap end, DashCap dash);
    Status SetDashStyle(DashStyle s);
    Status SetAlignment(int a);
};

// ---------------- 字体 ----------------
class FontFamily {
public:
    void* impl;
    std::wstring name;
    FontFamily();
    FontFamily(const wchar_t* name);
    ~FontFamily();
    BOOL IsAvailable() const;
    Status GetFamilyName(wchar_t* buf, int n) const;
};

class Font {
public:
    void* impl;
    Font();
    Font(const FontFamily* fam, REAL size, int style, Unit unit);
    Font(const wchar_t* famName, REAL size, int style, Unit unit);
    ~Font();
    REAL GetSize() const;
    int  GetStyle() const;
};

class StringFormat {
public:
    void* impl;
    StringFormat();
    StringFormat(int formatFlags);
    ~StringFormat();
    Status SetAlignment(StringAlignment a);
    Status SetLineAlignment(StringAlignment a);
    Status SetFormatFlags(int flags);
};

class Matrix {
public:
    void* impl;
    Matrix();
    Matrix(REAL m11, REAL m12, REAL m21, REAL m22, REAL dx, REAL dy);
    ~Matrix();
    Status Shear(REAL sx, REAL sy);
    Status Translate(REAL dx, REAL dy);
    Status Rotate(REAL angle);
    Status Scale(REAL sx, REAL sy);
    Status Reset();
};

class ImageAttributes {
public:
    void* impl;
    ImageAttributes();
    ~ImageAttributes();
    Status SetColorMatrix(const ColorMatrix* cm);
    Status SetColorMatrix(const ColorMatrix* cm, ColorMatrixFlags flags, ColorAdjustType type);
    Status SetWrapMode(int wrap);
};

// ---------------- 图像 ----------------
class Image {
public:
    void* impl;
    Image() : impl(nullptr) {}
    virtual ~Image() {}
    virtual unsigned int GetWidth() const;
    virtual unsigned int GetHeight() const;
};

class Bitmap : public Image {
public:
    int  w, h;
    int  pf;
    void* bits;                    // LockBits 时直接指向像素缓冲
    Status lastStatus;
    Bitmap(int width, int height, PixelFormat format);
    Bitmap(int width, int height);
    Bitmap(const wchar_t* file);
    ~Bitmap();
    static Bitmap* FromFile(const wchar_t* file);
    Status GetLastStatus() const;
    Status LockBits(const HswRect* rc, unsigned int flags, PixelFormat format, BitmapData* data);
    Status UnlockBits(BitmapData* data);
    Bitmap* Clone(const RectF& rc, PixelFormat format);
    unsigned int GetWidth() const;
    unsigned int GetHeight() const;
};

// ---------------- 路径 ----------------
class GraphicsPath {
public:
    void* impl;
    GraphicsPath();
    GraphicsPath(FillMode mode);
    ~GraphicsPath();
    Status AddLine(REAL x0, REAL y0, REAL x1, REAL y1);
    Status AddLine(const PointF& a, const PointF& b);
    Status AddLines(const PointF* pts, int n);
    Status AddRectangle(const RectF& r);
    Status AddEllipse(REAL x, REAL y, REAL w, REAL h);
    Status AddEllipse(const RectF& r);
    Status AddArc(REAL x, REAL y, REAL w, REAL h, REAL startAngle, REAL sweepAngle);
    Status AddBezier(REAL x0, REAL y0, REAL x1, REAL y1,
                     REAL x2, REAL y2, REAL x3, REAL y3);
    Status AddBezier(const PointF& a, const PointF& b, const PointF& c, const PointF& d);
    Status AddCurve(const PointF* pts, int n);
    Status AddCurve(const PointF* pts, int n, REAL tension);
    Status AddPolygon(const PointF* pts, int n);
    Status AddString(const wchar_t* s, int len, const FontFamily* fam, int style,
                     REAL size, const PointF& origin, const StringFormat* fmt);
    Status Transform(const Matrix* m);
    Status StartFigure();
    Status CloseFigure();
    Status Reset();
    Status GetBounds(RectF* r) const;
    Status SetFillMode(FillMode m);
    int    GetPointCount() const;
};

// ---------------- Graphics ----------------
class Graphics {
public:
    void* impl;
    Graphics(void* image);                 // 从 Bitmap / Image 建（游戏里主要是这个）
    Graphics(void* hdc, bool unused);      // 从 HDC 建
    ~Graphics();

    Status Clear(const Color& c);
    GraphicsState Save();
    Status Restore(GraphicsState st);

    Status FillPath(const Brush* b, const GraphicsPath* p);
    Status DrawPath(const Pen* p, const GraphicsPath* path);
    Status FillRectangle(const Brush* b, const RectF& r);
    Status FillRectangle(const Brush* b, REAL x, REAL y, REAL w, REAL h);
    Status DrawRectangle(const Pen* p, const RectF& r);
    Status DrawRectangle(const Pen* p, REAL x, REAL y, REAL w, REAL h);
    Status FillEllipse(const Brush* b, const RectF& r);
    Status FillEllipse(const Brush* b, REAL x, REAL y, REAL w, REAL h);
    Status DrawEllipse(const Pen* p, const RectF& r);
    Status DrawEllipse(const Pen* p, REAL x, REAL y, REAL w, REAL h);
    Status DrawLine(const Pen* p, REAL x0, REAL y0, REAL x1, REAL y1);
    Status DrawLine(const Pen* p, const PointF& a, const PointF& b);
    Status DrawLines(const Pen* p, const PointF* pts, int n);
    Status DrawPolygon(const Pen* p, const PointF* pts, int n);
    Status FillPolygon(const Brush* b, const PointF* pts, int n);
    Status DrawArc(const Pen* p, const RectF& r, REAL start, REAL sweep);
    Status DrawArc(const Pen* p, REAL x, REAL y, REAL w, REAL h, REAL start, REAL sweep);
    Status FillClosedCurve(const Brush* b, const PointF* pts, int n);

    Status DrawImage(Image* img, const RectF& dest);
    Status DrawImage(Image* img, REAL x, REAL y, REAL w, REAL h);
    Status DrawImage(Image* img, const RectF& dest, REAL sx, REAL sy, REAL sw, REAL sh,
                     Unit unit, const ImageAttributes* attr);
    Status DrawImage(Image* img, const RectF& dest, REAL sx, REAL sy, REAL sw, REAL sh, Unit unit);

    Status MeasureString(const wchar_t* s, int len, const Font* f, const SizeF& layout,
                         StringFormat* fmt, RectF* out) const;
    Status MeasureString(const wchar_t* s, int len, const Font* f, const RectF& layout,
                         StringFormat* fmt, RectF* out, int* fitted, int* lines) const;
    Status MeasureString(const wchar_t* s, int len, const Font* f, const PointF& origin,
                         StringFormat* fmt, RectF* out) const;
    Status MeasureString(const wchar_t* s, int len, const Font* f, const PointF& origin, RectF* out) const;
    Status DrawString(const wchar_t* s, int len, const Font* f, const PointF& origin,
                      const StringFormat* fmt, const Brush* b);
    Status DrawString(const wchar_t* s, int len, const Font* f, const RectF& rc,
                      const StringFormat* fmt, const Brush* b);

    Status SetClip(const RectF& r);
    Status SetClip(const RectF& r, CombineMode mode);
    Status SetClip(const GraphicsPath* p);
    Status SetClip(const GraphicsPath* p, CombineMode mode);
    Status ResetClip();

    Status TranslateTransform(REAL dx, REAL dy);
    Status ScaleTransform(REAL sx, REAL sy);
    Status RotateTransform(REAL angle);
    Status MultiplyTransform(const Matrix* m);
    Status MultiplyTransform(const Matrix* m, MatrixOrder order);
    Status SetTransform(const Matrix* m);
    Status GetTransform(Matrix* m) const;
    Status ResetTransform();

    Status SetSmoothingMode(SmoothingMode m);
    Status SetInterpolationMode(InterpolationMode m);
    Status SetPixelOffsetMode(PixelOffsetMode m);
    Status SetCompositingMode(CompositingMode m);
    Status SetCompositingQuality(CompositingQuality q);
    Status SetTextRenderingHint(TextRenderingHint h);
    Status SetPageUnit(Unit u);
    Status SetPageScale(REAL s);

    void* GetHDC();
    void  ReleaseHDC(void* hdc);
};



// ---------------- 启动 / 关闭 ----------------
struct GdiplusStartupInput {
    unsigned int GdiplusVersion;
    void*  DebugEventCallback;
    BOOL   SuppressBackgroundThread;
    BOOL   SuppressExternalCodecs;
    GdiplusStartupInput() : GdiplusVersion(1), DebugEventCallback(nullptr),
                            SuppressBackgroundThread(FALSE), SuppressExternalCodecs(FALSE) {}
};
struct GdiplusStartupOutput { void* NotificationHook; void* NotificationUnhook; };

Status GdiplusStartup(unsigned long long* token, const GdiplusStartupInput* in,
                      GdiplusStartupOutput* out);
void   GdiplusShutdown(unsigned long long token);

// 编码器枚举（导出预览图用）
struct ImageCodecInfo {
    char  Clsid[16];
    char  FormatID[16];
    const wchar_t* CodecName;
    const wchar_t* DllName;
    const wchar_t* FormatDescription;
    const wchar_t* FilenameExtension;
    const wchar_t* MimeType;
    unsigned int Flags;
    unsigned int Version;
    unsigned int SigCount;
    unsigned int SigSize;
    void* SigPattern;
    void* SigMask;
};
Status GetImageEncodersSize(unsigned int* num, unsigned int* size);
Status GetImageEncoders(unsigned int num, unsigned int size, ImageCodecInfo* info);

// 字体名映射放在这里声明（Windows 字体名 → macOS / 随包字体），实现在 shim_mac.mm
const char* HswMapFontUtf8(const char* winName);

// Point / Rect 和 macOS 的 MacTypes 同名结构冲突，用宏别名绕开；
// 这两个宏只在 HSW_MAC 下生效，CGRect / NSPoint 是独立 token，不受影响。
#define Point HswPoint
#define Rect  HswRect

#endif // HSW_MAC
#endif // HSW_GSHIM_H
