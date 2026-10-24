// ============================================================
//  高中的一周
//  技术：Win32 + GDI+（C++17，MinGW-W64 静态编译）
//
//  第一步：纯黑底白字免责声明，展示 3 秒
//  第二步：3 秒后自动进入菜单：
//          · 背景「高中的一周菜单界面.jpg」（16:9 动漫版，整屏铺满；
//            换成 3.76:1 的校园门口.jpg 会自动变成模糊底 + 清晰横带）
//          · 右侧五个手绘漫画风按钮：开始游戏 / 继续游戏 / 游戏说明 /
//            成就查看 / 退出游戏
//          · 点击「退出游戏」结束程序
//  第三步：点「游戏说明」进入泛黄纸张说明页，右上角红叉点一下返回主菜单
//  第四步：点「开始游戏」进入角色选择页：
//          · 标题「请选择你的英雄」
//          · 两张长方形卡片 = Q 版立绘 + 角色名（芋头 / 继科）
//          · 点卡片弹出角色介绍 +「是否确认选择该英雄 / 是 / 返回」
//          · 返回 = 关掉介绍回到选角，是 = 确认并高亮该角色
//  第五步：游戏内 UI（选「是」后进入）：
//          · 左上：角色头像 + 名字 + 五条属性（健康/饱食/精神/快乐/知识，
//            前四项满值 100，知识不设上限；初始 70/70/70/70/0）
//          · 右上：天数大圆盘（1~7day），圆的左下角是当前时间段
//          · 正下方：九格物品栏，第一格显示金钱（初始 200）
//  第六步：表情变化（严格按开发说明的判定条件，封装为 FaceResolver 类）：
//          · 精神 < 40        -> 疲惫头像
//          · 精神 >= 40 时：
//              快乐 <  40     -> 难过头像
//              快乐 40 ~ 80   -> 平静头像
//              快乐 >  80     -> 开心头像
//          由 FaceState 状态机托管，每次玩家做出选择都会调用 Refresh 检测，
//          发生切换时约 0.2s 交叉淡化；游戏界面按 1~4 可强制预览，5 恢复自动
//  第七步：教学楼界面：选好角色后的见面场景，背景换成「教学楼界面.jpg」
//          （图比 16:9 更高，按整幅清晰图居中 + 两侧虚化补边摆放）
//  第八步：地图页：
//          · 每次玩家进入下一个时间节点时，先进入地图页
//          · 背景 = 「地图.jpg」（2124x1280，按高度铺满居中，逻辑坐标 1280x720）
//          · 9 个建筑热区，用「有方向的矩形」贴合各建筑朝向（坐标见 kMapSpots）：
//              1 教学楼   2 实验楼   3 操场   4 体育馆   5 球场
//              6 行政楼与图书馆   7 食堂和小卖部   8 农田   9 宿舍
//          · 鼠标停留在建筑上：显示该建筑名称 + 橙色手绘矩形框 + 编号
//          · 点击「教学楼」跳转到教学楼界面；其余场地留待后续开发
//  第九步：上课场景（属于教学楼界面）：
//          · 左上角角色头像略微缩小（AV_S 118 -> 100）
//          · 物理 / 语文 / 数学 / 英语 四节课，背景都用「教学楼界面.jpg」
//          · 老师站在讲台左侧，下半身被最近处课桌略微遮挡
//          · 左黑板写对应学科的高中知识，右黑板写这是哪门课 + 老师名字
//          · 当前上哪节课由课表决定；day1 清晨 = 物理课
//  第十步：对话框 / day1 清晨剧情 / 事务安排：
//          · 对话框在屏幕下方，左上角说话人名字，右下角「>继续」，文字逐字浮现
//          · 支持选项（长方形上下排列、对齐对话框，此时不显示「>继续」）
//          · day1 清晨班会三句（日日老师），说完第三句物品栏加入「事务安排」
//          · 「事务安排」= 蓝色日记本，左键点开：七页白纸横线，
//            右上角红叉关闭（不改时间），左下「←」右下「→」翻页
//          · 课表按「事务安排」原文（Day1~Day3），到点在教学楼界面换对应老师
//  第十一步：状态变化 + 上课功能：
//          · 每过一个时间段：饱食 -5、精神 -3、快乐 -1（独立于其它活动）
//          · 有课的时段进教学楼先问「这节课该如何度过」三选一，每节课都要重选
//              认真听课 知识+20 精神-10 快乐-10
//              随便听课 知识+10 精神-5  快乐-5
//              直接摸鱼 快乐+5
//          · 选完播对应老师的台词（部分台词会带角色名），播完直接跳下个时间段
//            （不经过地图）
//  第十二步：食堂和小卖部：
//          · 地图点 7 号进入，背景「食堂和小卖部界面.jpg」，保留游戏内 UI
//          · 对话框右边两个选项：购买 / 离开（离开回地图）
//          · 购买 → 屏幕中间「食堂商店」：健康饭菜/小卖部饼干/不那么健康食品/压缩饼干
//          · 钱不够正中提示「预算不足」1 秒；背包满了提示「背包已满」1 秒
//          · 小卖部饼干、压缩饼干、咖啡进背包（左键使用）；
//            健康饭菜、不那么健康食品当场吃掉并跳过一个时段（介绍栏里写明）
//          · 食堂只在饭点开放（清晨/中午下课/中午吃饭/中午午时/晚饭前/晚饭），
//            不在饭点点它会提示「当前食堂还未开放」；吃到时间跳出饭点则回地图并提示「食堂已经关闭」
//  第十三步：宿舍界面：
//          · 地图点 9 号进入；到「晚寝时间」会自动把人拉回宿舍，正中提示「该休息了」1 秒
//          · 背景「宿舍界面.jpg」，保留游戏内 UI，对话框用缩小的那种
//          · 白天（非晚自习下课/晚寝/凌晨）：小睡一会（跳一个时段 + 精神20 + 快乐5）/ 离开
//          · 晚上（晚自习下课/晚寝/凌晨）：睡觉（精神回满）/ 熬夜学习（精神-40 健康-20 知识+100）
//            / 熬夜玩（精神-40 健康-20 快乐回满）；芋头熬夜只扣精神和健康各 10
//            —— 三个都跳到第二天清晨
//  第十四步：行政楼与图书馆（书屋）：
//          · 地图点 6 号先弹确认框「去行政楼与图书馆会消耗时间，是否前往」，是才去
//          · 背景「书屋界面.jpg」，对话框右边「购买」「离开」（离开回地图并跳一个时段）
//          · 「书屋」商店卖五本教辅，全进背包；钱不够/背包满各弹一次正中提示
//          · 背包里对应科目的书 → 该科目课堂知识 +50%；课外教辅 → 非课堂知识 +30%（小数向上取整）
//          · 右键点背包里的书 → 弹出「是否确认扔掉这本书」
//  第十五步：农田界面：
//          · 地图点 8 号进入，保留 UI
//          · 进入先播台词：我"现在的玉米还没有成熟" >继续 / 我"要随便看看吗" → 选项「要 / 离开」
//          · 选「要」：回复15点快乐 5点精神，接旁白"在农田转了转" >继续，
//            播完回地图并跳过一个时间段；选「离开」直接回地图（不消耗时间）
//          · 玉米事件完成后（g_cornDone）背景换成「农田结束界面.jpg」，
//            台词变短、属性只回 5 点快乐（玉米事件本身后续开发）
//  第十六步：操场界面：
//          · 地图点 3 号进入；到「体育课」时段会自动把人拉去操场，正中提示「现在是体育课时间」1 秒
//          · 体育课：我"体育老师又不知道去哪里了" >继续 / 我"体育课该干些什么呢" → 跑步吧 / 跑路了兄弟们
//          · 非体育课：我"要做些什么吗" → 跑步吧 / 离开
//          · 跑步吧 → 屏幕播 5 秒 Q 版小人跑步：体育课 健康+20（继科额外+10）、
//            非体育课 健康+10（继科额外+5），精神-10，然后跳到下一个时间段
//          · 跑路了兄弟们 / 离开 → 回地图

//  第十七步：篮球场 + 投篮小游戏：
//          · 地图点 5 号进入，背景「篮球场界面.jpg」，保留游戏内 UI
//          · 对话「练习投篮？」→ 来 / 溜了；「溜了」直接回地图
//          · 「来」打开投篮小游戏（移植自 投篮小游戏\投篮游戏.cpp）：
//            第一人称持球，长按蓄力松手投出，3 次机会（静止 / 左右移动 / 全屏快速移动篮筐）
//          · 结算：精神-10，健康+10*得分、快乐+10*得分（继科额外+15 健康），然后回地图并跳一个时段
//  第十八步：属性变化提示：
//          · 「下一时间段」按钮改名「自修一会」：保留原本的时段属性变化，额外 +5 知识
//          · 除了时段导致的自然衰减，每次属性变化都在屏幕正中列出来（加=绿、减=红）
//          · 面板下方有「确定」，点了才继续切界面 / 推时段（ESC 等同确定）
//  第十九步：音乐楼 + 钢琴键小游戏：
//          · 地图新增第 10 号区域「音乐楼」（原地图上直接加，热区互不重叠）
//          · 背景「音乐楼界面.jpg」，保留游戏内 UI；对话「要弹奏一曲吗」→ 展示我的音乐才华 / 保留择偶权
//          · 「展示我的音乐才华」→ 打开钢琴键小游戏（移植自 钢琴键游戏\main.cpp）：
//            黑底 7 条透视道路、蓝色琴键由远及近、1~7 击打、按对变绿按错变红，音频走 MF + waveOut
//          · 结算页按 ENTER 或点击 → 精神-10、快乐 +积分/5，推一个时段回地图（数值待确认）
//  第二十步：动作动画 + 事件：
//          · 吃饭 / 小睡 / 睡觉 / 熬夜学习 / 熬夜玩 / 农田闲逛 / 上课自习 都加 5 秒动画
//            （和跑步同一套：整屏压暗 + 标题 + 进度条 + 小人做动作，播完再继续原流程）
//          · 事件1 课外实践活动：day2 上午第 5 节强制进入「课外事件活动1.jpg」，
//            三句台词后让玩家点物品栏里的一样东西：
//            饼干/压缩饼干/咖啡 → 正常使用；物理白皮 → 铁锅炖白皮（快乐回满、知识+50、换第二张图）；
//            其它物品不消失。结尾「好在后来家长送来了食物」饱食+30，回地图并跳到中午午时
//          · 事件2 北京名师讲高考：day2 下午第一节强制进「讲高考.jpg」（第二十三步补全，
//            见文件末尾「第二十三步 · 新增4」）
//  第二十一步：动作动画统一 2 秒 + 先播动画再显示属性变化：
//          · 顺序改成「动作 → 2 秒动画 → 属性变化面板 → 确定 → 继续原流程」
//  · 事件3 考试：day3 下午第一节强制进「考试.jpg」，正中提示「考试了」1 秒
//          下午一节物理 / 二节语文 / 三节英语 / 四节数学，每门都是
//          广播"这是浙南联盟X考试" → 我"要合理分配精力" → 认真考（精神-10）/ 轻松考（精神-5），
//          考完跳到下一节且不切到地图
//          数学考完再播：旁白"紧张数学考试中" → 广播"同学们，农田的玉米熟了快去摘玉米"
//          → 我"该怎么做呢" → 去摘玉米（开摘玉米游戏）/ 去你*的（跳到下午五节回地图）
//          成绩 = ((1+知识/300-精神/100)^(知识/300))*(100/e) ± 10；物理 ≤100，
//          其余三门 *1.5 且 ≤150，数学选摘玉米再 *0.9；成绩只保存不展示
//  · 事件4 摘玉米：移植「摘玉米游戏\摘玉米游戏.cpp」，每周目只能玩一次。
//          按住 W 前进、点击近处的玉米采摘，限时 30 秒；
//          每 10 积分换 1 个玉米进物品栏（使用 = 饱食+50 健康+20），背包满提示「背包满了」，
//          摘玉米积分 > 80 解锁「赫鲁晓夫」成就
//  第二十二步：部分结局与成就（v0.1）
//  · 成就（菜单「成就查看」进入的纸张页可看对应，达成/未达成两种样式）
//      赫鲁晓夫   —— 在考试摘玉米中获得 80 分以上
//      铁锅炖白皮 —— 在课外实践活动中选择「物理白皮」
//      学霸       —— 检测到知识达到 300 触发
//    解锁状态存 %LOCALAPPDATA%\高中的一周\achievements.txt，重开游戏也留着
//  · 结局：任何时候 精神/健康/饱食/快乐 归零 -> 整屏变黑，显示结局名 + 描述，可返回菜单
//      精神 0 -> 精神病院进修结局   健康 0 -> 医院结局
//      饱食 0 -> 饿晕结局           快乐 0 -> 抑郁结局
//  · 时间到 day4 -> 「此为本次测试的全部内容」+ 返回菜单按钮
//  · 全屏 / 小窗适配：窗口可拉伸、拖动锁 16:9、F11（或 Alt+Enter）全屏切换，
//    渲染照旧按 1280x720 等比缩放 + 居中黑边
//  · 单文件打包：打包v0.1.py 把全部图片/音频追加进 exe 尾部，
//    运行时释放到本地缓存目录，生成 高中的一周v0.1.exe（只拷一个文件即可运行）
//  第二十三步：bug 修复与优化
//  · 修「食堂卡死」：商店右上红叉关掉、以及当场吃完（时间仍在饭点）之后，
//    都要把对话框右边的「购买 / 离开」重新显示出来，否则界面没有任何出口
//  · 修「台词 >继续 点不动」：食堂 / 宿舍 / 音乐楼 / 操场这些有独立命中分支的场地，
//    台词阶段点对话框 = 继续；并补齐了体育课 / 农田等入口漏掉的 g_dlgOptN 归零
//  · 成就页现在能同时看「成就」和「达成的结局」，内容超出一屏时用滚轮上下滚
//  · 存档：右上角时间 UI 右侧新增「保存」按钮（正中提示「数据已保存」1 秒），
//    菜单「继续游戏」读档接着玩，「开始游戏」重新选角 = 新开一局
//  · 任何购买行为都弹「已经购买XX」1 秒（食堂 5 件 + 书屋 5 本）
//  第二十三步 · 新增3
//  · 修「事件结束后界面停在事件图」：g_loc 以前只有考试会复位，课外实践 / 讲高考
//    结束后回到教室，背景还挂着事件那张图。现在离开游戏画面（回地图 / 回菜单）统一
//    LeaveEventLoc() 复位到教学楼；事件进行中也不再给「自修一会」，界面老实停在事件里
//  · 钢琴键大屏卡顿：静态背景层以前按 1280x720 烘，大屏时每帧都被 GDI+ 缩放
//    （1080p 实测 154ms/帧）。现在按屏幕像素烘 + 1:1 贴图，并加了细节分级
//    （远处小键不发光、大屏关文字外发光 / 减发光层数 / 火花限量）
//    → 1080p 4~5ms、1440p 8~10ms
//  · 钢琴键和摘玉米一样每周目只能玩一次：弹完结算就退出音乐楼，
//    再想弹（含结算页按 R）会被拦住并回到地图
//  第二十三步 · 新增4（事件2 讲高考补全 —— 修「事件还没结束就退出事件界面」）
//  · 讲高考按开发说明原文做成四节课：下午一节 ~ 下午四节，每节 6/6/6/5 句台词，
//    每听完一节时间推进一节、界面仍停在讲高考，弹「跑路 / 继续听」
//    （跑路 → 直接回地图，时间停在那一节；继续听 → 接着讲）
//  · 最后一段：我「ZZZ」→ 旁白「你睡着了」→ 精神 +30、知识 -50（不低于 0）
//    → 回地图，时间变为晚饭前（time 14）
//  · 台词里的表情符号用「Segoe UI Emoji」分段绘制，不再是一排方框
//  · 事件（课外实践 / 讲高考 / 考试）进行中屏蔽「返回菜单」「自修一会」两个隐形热区，
//    界面不会因为手抖点一下就跳出事件
//  · 新开一局会重置事件进度（以前第二周目 day2 的两个事件不再触发）
//  第二十三步 · 新增5（修「自修一会进事件 → 点确定 → 被踢回地图」）
//  · 根因：自修一会是「推进时段 → 属性链（学习动画 → 属性变化 → 确定 → 回地图）」，
//    时段一推进，WM_TIMER 里的事件触发就先一步把人拉进了事件界面；等玩家点「确定」，
//    属性链最后那一步「回地图」又把事件顶掉了（事件标记已置位，也不会再触发）。
//  · 现在两道保险：
//    ① 属性链进行中（DeltaBusy）不允许插队触发事件 / 体育课 / 晚寝；
//    ② 属性链走完时先 TryForceEvent()：撞上事件就直接进事件界面，不回地图。
//    同一条路也覆盖「食堂吃完 / 跑步结算 / 投篮结算 / 弹琴结算」推时段撞上事件的情况。
//  ESC 随时退出（地图页 ESC = 返回教学楼界面）
//
//  详细的项目进度 / 架构约定 / 已知坑见同目录「开发总结.md」
// ============================================================

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601     // 需要 GetTickCount64 / SetProcessDPIAware
#endif
#ifndef WINVER
#define WINVER 0x0601
#endif

// macOS 移植：定义 HSW_MAC 时改用 mac/ 下的兼容层（Win32 → Cocoa、GDI+ → CoreGraphics），
// 业务代码一行不改；不定义时就是原来的 Windows 版本，行为完全一致。
#if defined(HSW_MAC)
#include "winshim.h"
#include "gshim.h"
#else
#include <windows.h>
#include <windowsx.h>
#include <gdiplus.h>
#endif
#include <cmath>
#include <cstring>
#include <cwchar>
#include <string>
#include <vector>
#include <random>
#include <algorithm>
#if !defined(HSW_MAC)
#include <mmsystem.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <shlwapi.h>
#include <shlobj.h>
#endif
#include <cstdio>
#include <cstdlib>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#if !defined(HSW_MAC)
using namespace Gdiplus;
#endif

// ---------------- 逻辑分辨率（所有坐标都按 1280x720 设计） ----------------
static const int BASE_W = 1280;
static const int BASE_H = 720;

// 第二十二步新增：6 成就查看页 / 7 结局（黑屏）/ 8 Day4 测试结束页
enum Scene { SCENE_DISCLAIMER = 0, SCENE_MENU = 1, SCENE_HELP = 2, SCENE_PICK = 3,
             SCENE_GAME = 4, SCENE_MAP = 5, SCENE_ACHIEVE = 6,
             SCENE_ENDING = 7, SCENE_TESTEND = 8 };

static const ULONGLONG DISCLAIMER_MS = 3000;   // 免责界面停留 3 秒

// 菜单背景图：16:9 动漫版（自带标题，可整屏铺满）
// 想换回原始照片「校园门口.jpg」（3.76:1 超宽全景，会自动变成模糊底 + 中间相框）改这一行即可
static const wchar_t* kMenuBg = L"高中的一周菜单界面.jpg";

// 第七步：游戏内场景（选定角色后的见面场景）背景 = 教学楼界面
static const wchar_t* kGameBg = L"教学楼界面.jpg";

// 第八步：地图页背景
static const wchar_t* kMapBg = L"地图.jpg";

static HWND      g_hwnd = nullptr;
static ULONGLONG g_t0 = 0;                     // 当前场景开始的时刻
static int       g_scene = SCENE_DISCLAIMER;
static const UINT_PTR ID_TIMER = 1;

static FontFamily* g_famTitle = nullptr;        // 标题：粗黑体
static FontFamily* g_famBody = nullptr;         // 正文：楷体
static FontFamily* g_famUI = nullptr;           // 按钮：黑体
static FontFamily* g_famChalk = nullptr;        // 板书：手写体（华文行楷，缺失时退回楷体）

static Bitmap* g_menuImg = nullptr;             // 校园门口.jpg（原图）
static Bitmap* g_bgCache = nullptr;             // 拼好的背景（按窗口尺寸缓存）
static int     g_bgW = 0, g_bgH = 0;
static Bitmap* g_paper = nullptr;               // 程序生成的泛黄纸（1280x720）

// 说明界面右上角的红叉
static const float XCX = 1178.0f, XCY = 98.0f, XARM = 26.0f, XHIT = 42.0f;
static float g_xAnim = 0.0f;                    // 红叉悬停插值
static bool  g_xHover = false;

static int   g_hover = -1;                      // 当前鼠标悬停的按钮
static int   g_pressed = -1;
static float g_hoverAnim[72] = { 0 };           // 每个可点元素（按钮/卡片/地图热区）的悬停插值 0~1（第二十三步扩到 72，64 = 保存按钮）

static std::wstring g_toastText;
static ULONGLONG    g_toastUntil = 0;

// ---------------- 菜单按钮 ----------------
enum { ACT_START = 0, ACT_CONTINUE, ACT_HELP, ACT_ACHIEVE, ACT_QUIT };

struct MenuItem { const wchar_t* label; int action; };
static const MenuItem kMenu[5] = {
    { L"开始游戏", ACT_START    },
    { L"继续游戏", ACT_CONTINUE },
    { L"游戏说明", ACT_HELP     },
    { L"成就查看", ACT_ACHIEVE  },
    { L"退出游戏", ACT_QUIT     }
};

// 按钮布局（逻辑坐标）
static const float BTN_X = 962.0f, BTN_W = 268.0f, BTN_H = 66.0f, BTN_GAP = 18.0f;
static const float BTN_TOP = 152.0f;

// ---------------- 小工具 ----------------
static float Clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }
static float EaseOut(float t)  { float u = 1.0f - t; return 1.0f - u * u * u; }
static float EaseInOut(float t) { return t < 0.5f ? 4.0f * t * t * t
                                                  : 1.0f - powf(-2.0f * t + 2.0f, 3.0f) * 0.5f; }

// 稳定的伪随机（同一下标每帧结果一致，不会抖动闪烁）
static float Noise1(int i)
{
    float v = std::sin((float)i * 127.1f + 311.7f) * 43758.5453f;
    return (v - std::floor(v)) * 2.0f - 1.0f;
}

static BYTE Alpha(BYTE a, float k)
{
    float v = a * k;
    if (v < 0) v = 0;
    if (v > 255) v = 255;
    return (BYTE)v;
}

// ---------------- 字体度量（用“国”字量基线，标点才不会飘） ----------------
struct FontMetric { float top; float height; };

static FontMetric Metrics(const FontFamily& fam, INT style, float em)
{
    FontMetric m = { 0.0f, em };
    GraphicsPath probe;
    probe.AddString(L"国", 1, &fam, style, em, PointF(0.0f, 0.0f), nullptr);
    RectF b(0, 0, 0, 0);
    probe.GetBounds(&b);
    m.top = b.Y;
    m.height = b.Height;
    return m;
}

// ---------------- 文字绘制：逐字排版 + 轻微手绘抖动 ----------------
static void DrawTextLine(Graphics& g, const wchar_t* text, const FontFamily& fam, INT style,
                         float em, float cx, float topY, float spacing,
                         BYTE alpha, float jitterAmp, int seed)
{
    if (!text || !*text || alpha == 0) return;

    FontMetric fm = Metrics(fam, style, em);

    std::vector<GraphicsPath*> glyphs;
    std::vector<float> adv;
    float total = 0.0f;

    for (const wchar_t* p = text; *p; ++p) {
        GraphicsPath* gp = new GraphicsPath();
        gp->AddString(p, 1, &fam, style, em, PointF(0.0f, 0.0f), nullptr);

        RectF b(0, 0, 0, 0);
        gp->GetBounds(&b);

        Matrix shift;
        shift.Translate(0.0f, -fm.top);    // 按基线归位，保留字体自带留白
        gp->Transform(&shift);
        glyphs.push_back(gp);

        float half = em * 0.5f;
        float a = ((unsigned)*p >= 0x2E80u) ? (em + spacing)
                                           : ((b.Width > half ? b.Width : half) + spacing);
        adv.push_back(a);
        total += a;
    }

    SolidBrush brush(Color(alpha, 255, 255, 255));
    float x = cx - total * 0.5f;

    for (size_t i = 0; i < glyphs.size(); ++i) {
        float jy = jitterAmp * Noise1(seed + (int)i);
        Matrix m;
        m.Translate(x, topY + jy);
        glyphs[i]->Transform(&m);
        g.FillPath(&brush, glyphs[i]);
        delete glyphs[i];
        x += adv[i];
    }
}

// 居中写一行（自动垂直居中）
static void DrawTextCenter(Graphics& g, const wchar_t* text, const FontFamily& fam, INT style,
                           float em, float cx, float cy, float spacing, Color color, float jitter)
{
    FontMetric fm = Metrics(fam, style, em);
    SolidBrush brush(color);
    BYTE a = color.GetA();
    if (a == 0) return;

    // 借用 DrawTextLine 的排版，但颜色由自己控制：这里直接内联一遍
    std::vector<GraphicsPath*> glyphs;
    std::vector<float> adv;
    float total = 0.0f;
    for (const wchar_t* p = text; *p; ++p) {
        GraphicsPath* gp = new GraphicsPath();
        gp->AddString(p, 1, &fam, style, em, PointF(0.0f, 0.0f), nullptr);
        RectF b(0, 0, 0, 0);
        gp->GetBounds(&b);
        Matrix shift;
        shift.Translate(0.0f, -fm.top);
        gp->Transform(&shift);
        glyphs.push_back(gp);
        float half = em * 0.5f;
        float w = ((unsigned)*p >= 0x2E80u) ? (em + spacing)
                                           : ((b.Width > half ? b.Width : half) + spacing);
        adv.push_back(w);
        total += w;
    }
    float x = cx - total * 0.5f;
    float y = cy - fm.height * 0.5f;
    for (size_t i = 0; i < glyphs.size(); ++i) {
        Matrix m;
        m.Translate(x, y + jitter * Noise1((int)i + 11));
        glyphs[i]->Transform(&m);
        g.FillPath(&brush, glyphs[i]);
        delete glyphs[i];
        x += adv[i];
    }
}

// ---------------- 手绘感波浪线 ----------------
static void DrawWobbleLine(Graphics& g, float cx, float y, float halfLen, float amp,
                           float phase, float thick, Color color)
{
    if (halfLen <= 0.5f || color.GetA() == 0) return;

    const int N = 56;
    std::vector<PointF> pts;
    pts.reserve(N + 1);
    for (int i = 0; i <= N; ++i) {
        float t   = (float)i / (float)N;
        float x   = cx - halfLen + 2.0f * halfLen * t;
        float env = std::sin(t * 3.14159265f);
        float yy  = y + amp * env * std::sin(t * 3.14159265f * 2.6f + phase);
        pts.push_back(PointF(x, yy));
    }

    Pen pen(color, thick);
    pen.SetLineCap(LineCapRound, LineCapRound, DashCapRound);
    g.DrawLines(&pen, pts.data(), (INT)pts.size());
}

// ---------------- 圆角矩形路径 ----------------
static void MakeRoundRect(GraphicsPath& p, RectF r, float rad)
{
    float d = rad * 2.0f;
    if (d > r.Width)  d = r.Width;
    if (d > r.Height) d = r.Height;
    p.AddArc(r.X, r.Y, d, d, 180.0f, 90.0f);
    p.AddArc(r.X + r.Width - d, r.Y, d, d, 270.0f, 90.0f);
    p.AddArc(r.X + r.Width - d, r.Y + r.Height - d, d, d, 0.0f, 90.0f);
    p.AddArc(r.X, r.Y + r.Height - d, d, d, 90.0f, 90.0f);
    p.CloseFigure();
}

// ---------------- 图片按比例裁切铺满 ----------------
static void DrawCover(Graphics& g, Image* img, RectF dest)
{
    if (!img) return;
    float iw = (float)img->GetWidth();
    float ih = (float)img->GetHeight();
    if (iw < 1.0f || ih < 1.0f) return;

    float sx = dest.Width / iw, sy = dest.Height / ih;
    float s  = sx > sy ? sx : sy;                 // cover
    float w = iw * s, h = ih * s;
    float x = dest.X + (dest.Width - w) * 0.5f;
    float y = dest.Y + (dest.Height - h) * 0.5f;
    g.DrawImage(img, RectF(x, y, w, h));
}

// ---------------- 拼背景（按窗口尺寸缓存） ----------------
static void BuildBackground(int W, int H)
{
    if (W <= 0 || H <= 0) return;
    if (g_bgCache && g_bgW == W && g_bgH == H) return;

    delete g_bgCache;
    g_bgCache = new Bitmap(W, H, PixelFormat32bppPARGB);
    g_bgW = W;
    g_bgH = H;

    Graphics gb(g_bgCache);
    gb.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    gb.SetPixelOffsetMode(PixelOffsetModeHighQuality);
    gb.Clear(Color(255, 0, 0, 0));

    if (!g_menuImg) {
        // 没有背景图：给个深灰渐变兜底
        LinearGradientBrush fallback(Point(0, 0), Point(0, H),
                                     Color(255, 34, 38, 46), Color(255, 12, 14, 18));
        gb.FillRectangle(&fallback, 0, 0, W, H);
        return;
    }

    float iw = (float)g_menuImg->GetWidth();
    float ih = (float)g_menuImg->GetHeight();
    float aspect = (ih > 0.0f) ? (iw / ih) : 3.7f;

    // 1) 先缩小再放大 = 廉价高斯模糊，铺满整屏
    int sw = W / 26, sh = H / 26;
    if (sw < 8) sw = 8;
    if (sh < 8) sh = 8;
    {
        Bitmap small(sw, sh, PixelFormat32bppPARGB);
        Graphics gs(&small);
        gs.SetInterpolationMode(InterpolationModeHighQualityBicubic);
        gs.Clear(Color(255, 0, 0, 0));
        // 直接拉伸铺满再糊掉：反正糊了看不出变形，色彩却和清晰横带一致
        gs.DrawImage(g_menuImg, RectF(0.0f, 0.0f, (float)sw, (float)sh));

        gb.DrawImage(&small, RectF(0.0f, 0.0f, (float)W, (float)H));
    }

    // 稍微压暗，让右边的按钮更跳出来
    SolidBrush dim(Color(70, 0, 0, 0));
    gb.FillRectangle(&dim, 0, 0, W, H);

    // 2) 清晰画面
    float bandW = (float)W;
    float bandH = bandW / aspect;
    bool fullBleed = (bandH >= (float)H * 0.90f);   // 图够“高”就直接铺满全屏

    if (fullBleed) {
        Bitmap crisp(W, H, PixelFormat32bppPARGB);
        {
            Graphics gc(&crisp);
            gc.SetInterpolationMode(InterpolationModeHighQualityBicubic);
            gc.SetPixelOffsetMode(PixelOffsetModeHighQuality);
            gc.Clear(Color(255, 0, 0, 0));
            DrawCover(gc, g_menuImg, RectF(0.0f, 0.0f, (float)W, (float)H));
        }
        gb.DrawImage(&crisp, 0, 0, W, H);
    } else {
        // 超宽全景：整幅按比例占满宽度，居中，外面套一圈白色相框（像贴在手账上的照片）
        float bandX = ((float)W - bandW) * 0.5f;
        float bandY = ((float)H - bandH) * 0.5f;

        // 硬阴影
        GraphicsPath shadowPath;
        MakeRoundRect(shadowPath, RectF(bandX + 9.0f, bandY + 11.0f, bandW, bandH), 8.0f);
        SolidBrush shadow(Color(115, 0, 0, 0));
        gb.FillPath(&shadow, &shadowPath);

        // 白色相框
        GraphicsPath frame;
        MakeRoundRect(frame, RectF(bandX, bandY, bandW, bandH), 8.0f);
        SolidBrush white(Color(255, 255, 252, 240));
        gb.FillPath(&white, &frame);

        // 照片本体（内缩 7px，露出白边）
        Bitmap crisp(W, H, PixelFormat32bppPARGB);
        {
            Graphics gc(&crisp);
            gc.SetInterpolationMode(InterpolationModeHighQualityBicubic);
            gc.SetPixelOffsetMode(PixelOffsetModeHighQuality);
            gc.Clear(Color(0, 0, 0, 0));
            gc.DrawImage(g_menuImg, RectF(bandX + 7.0f, bandY + 7.0f,
                                          bandW - 14.0f, bandH - 14.0f));
        }
        gb.DrawImage(&crisp, 0, 0, W, H);
    }

    // 3) 右侧压暗，保证按钮文字对比度
    {
        LinearGradientBrush scrim(Point((INT)(W * 0.56f), 0), Point(W, 0),
                                  Color(0, 0, 0, 0), Color(135, 0, 0, 0));
        gb.FillRectangle(&scrim, (INT)(W * 0.56f), 0, W, H);
    }
}

// ---------------- 程序生成的泛黄纸张（第三步用） ----------------
static float Rand01(unsigned& s)
{
    s = s * 1664525u + 1013904223u;
    return (float)((s >> 8) & 0xFFFFu) / 65535.0f;
}

static void BuildPaper()
{
    delete g_paper;
    g_paper = new Bitmap(BASE_W, BASE_H, PixelFormat32bppPARGB);

    // 1) 底色 + 逐像素纸纹（老纸的颗粒感）
    {
        Rect lockRect(0, 0, BASE_W, BASE_H);
        BitmapData bd = {};
        if (g_paper->LockBits(&lockRect, ImageLockModeWrite, PixelFormat32bppPARGB, &bd) == Ok) {
            unsigned seed = 20261021u;
            for (int y = 0; y < BASE_H; ++y) {
                UINT32* row = (UINT32*)((BYTE*)bd.Scan0 + (size_t)y * bd.Stride);
                for (int x = 0; x < BASE_W; ++x) {
                    seed = seed * 1664525u + 1013904223u;
                    int n = (int)((seed >> 16) & 0x3Fu) - 32;
                    n >>= 3;                                    // -4 .. 3
                    int r = 250 + n;
                    int gg = 242 + n;
                    int bb = 214 + n;
                    row[x] = ((UINT32)255 << 24) | ((UINT32)r << 16) |
                             ((UINT32)gg << 8) | (UINT32)bb;
                }
            }
            g_paper->UnlockBits(&bd);
        }
    }

    Graphics g(g_paper);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetPixelOffsetMode(PixelOffsetModeHalf);

    // 2) 中间亮一点，像灯下的纸
    {
        GraphicsPath full;
        MakeRoundRect(full, RectF(0.0f, 0.0f, (float)BASE_W, (float)BASE_H), 4.0f);
        PathGradientBrush pg(&full);
        pg.SetCenterPoint(PointF(BASE_W * 0.46f, BASE_H * 0.44f));
        pg.SetCenterColor(Color(40, 255, 252, 235));
        Color surround[1] = { Color(0, 255, 252, 235) };
        int cnt = 1;
        pg.SetSurroundColors(surround, &cnt);
        g.FillRectangle(&pg, 0, 0, BASE_W, BASE_H);
    }

    // 3) 水渍 / 霉斑
    {
        unsigned s = 99173u;
        for (int i = 0; i < 52; ++i) {
            float x  = Rand01(s) * BASE_W;
            float y  = Rand01(s) * BASE_H;
            float rx = 34.0f + Rand01(s) * 175.0f;
            float ry = rx * (0.45f + Rand01(s) * 0.55f);
            int a = 6 + (int)(Rand01(s) * 13.0f);
            bool brown = Rand01(s) > 0.55f;
            SolidBrush b(brown ? Color(a, 186, 150, 92) : Color(a, 234, 212, 168));
            g.FillEllipse(&b, x - rx, y - ry, rx * 2.0f, ry * 2.0f);
        }
        // 边缘几块重一点的老渍
        for (int i = 0; i < 5; ++i) {
            float x = (i & 1) ? BASE_W - 40.0f - Rand01(s) * 120.0f : 40.0f + Rand01(s) * 120.0f;
            float y = 60.0f + Rand01(s) * (BASE_H - 120.0f);
            float r = 70.0f + Rand01(s) * 90.0f;
            SolidBrush b(Color(10, 178, 140, 84));
            g.FillEllipse(&b, x - r, y - r * 0.62f, r * 2.0f, r * 1.24f);
        }
        // 几道横向纤维丝
        for (int i = 0; i < 14; ++i) {
            float y = Rand01(s) * BASE_H;
            int a = 5 + (int)(Rand01(s) * 7.0f);
            Pen pen(Color(a, 156, 132, 86), 1.1f);
            g.DrawLine(&pen, 0.0f, y, (float)BASE_W, y + (Rand01(s) - 0.5f) * 6.0f);
        }
    }

    // 4) 四边压暗（纸张被岁月晒黄）
    {
        Color edge(72, 200, 168, 108), clear(0, 200, 168, 108);
        LinearGradientBrush l(Point(0, 0), Point(96, 0), edge, clear);
        g.FillRectangle(&l, 0, 0, 96, BASE_H);
        LinearGradientBrush r(Point(BASE_W, 0), Point(BASE_W - 96, 0), edge, clear);
        g.FillRectangle(&r, BASE_W - 96, 0, 96, BASE_H);
        LinearGradientBrush t(Point(0, 0), Point(0, 78), edge, clear);
        g.FillRectangle(&t, 0, 0, BASE_W, 78);
        LinearGradientBrush b2(Point(0, BASE_H), Point(0, BASE_H - 78), edge, clear);
        g.FillRectangle(&b2, 0, BASE_H - 78, BASE_W, 78);
    }

    // 5) 两道手绘感内框
    {
        Pen p1(Color(72, 96, 78, 54), 2.6f);
        p1.SetLineJoin(LineJoinRound);
        GraphicsPath f1;
        MakeRoundRect(f1, RectF(42.0f, 42.0f, BASE_W - 84.0f, BASE_H - 84.0f), 12.0f);
        g.DrawPath(&p1, &f1);

        Pen p2(Color(44, 96, 78, 54), 1.5f);
        p2.SetLineJoin(LineJoinRound);
        GraphicsPath f2;
        MakeRoundRect(f2, RectF(50.0f, 50.0f, BASE_W - 100.0f, BASE_H - 100.0f), 9.0f);
        Matrix m;
        m.Translate(1.4f, 1.8f);
        f2.Transform(&m);
        g.DrawPath(&p2, &f2);
    }
}

// ---------------- 中文折行排版（说明正文用） ----------------
static bool IsClosingPunct(wchar_t c)
{
    return c == L'，' || c == L'。' || c == L'；' || c == L'：' ||
           c == L'！' || c == L'？' || c == L'）' || c == L'、';
}

// 返回下一段该从哪个 y 开始
static float DrawBodyText(Graphics& g, const wchar_t* text, const FontFamily& fam, INT style,
                          float em, float x, float y, float maxW, float lineH,
                          Color color, float alphaK, float indent)
{
    Font font(&fam, em, style, UnitPixel);
    StringFormat fmt;
    fmt.SetAlignment(StringAlignmentNear);
    fmt.SetLineAlignment(StringAlignmentNear);
    fmt.SetFormatFlags(StringFormatFlagsNoWrap | StringFormatFlagsNoClip);

    SolidBrush brush(Color(Alpha(color.GetA(), alphaK), color.GetR(),
                          color.GetG(), color.GetB()));

    std::wstring line;
    float w = 0.0f;
    float cy = y;
    float curX = x + indent;
    float limit = maxW - indent;

    for (const wchar_t* p = text; *p; ++p) {
        wchar_t c = *p;
        float adv = ((unsigned)c >= 0x2E80u) ? em : em * 0.55f;
        if (w + adv > limit && !IsClosingPunct(c) && !line.empty()) {
            g.DrawString(line.c_str(), (INT)line.size(), &font, PointF(curX, cy), &fmt, &brush);
            line.clear();
            w = 0.0f;
            cy += lineH;
            curX = x;                 // 第二行开始不再缩进
        }
        line.push_back(c);
        w += adv;
    }
    if (!line.empty()) {
        g.DrawString(line.c_str(), (INT)line.size(), &font, PointF(curX, cy), &fmt, &brush);
        cy += lineH;
    }
    return cy;
}

// ---------------- 红叉 ----------------
static void DrawRedCross(Graphics& g, float scale, float alphaK)
{
    float arm = XARM * scale;
    BYTE  a   = Alpha(255, alphaK);

    // 悬停时一圈淡淡的红
    if (g_xAnim > 0.01f) {
        SolidBrush halo(Color(Alpha(34, g_xAnim * alphaK), 210, 60, 48));
        float r = 38.0f * scale;
        g.FillEllipse(&halo, XCX - r, XCY - r, r * 2.0f, r * 2.0f);
    }

    Pen pen(Color(a, 205, 52, 42), 9.0f * scale);
    pen.SetLineCap(LineCapRound, LineCapRound, DashCapRound);

    PointF s1[3] = { PointF(XCX - arm, XCY - arm + 3.0f),
                     PointF(XCX - 1.0f,  XCY - 1.0f),
                     PointF(XCX + arm - 1.0f, XCY + arm) };
    PointF s2[3] = { PointF(XCX + arm, XCY - arm + 1.0f),
                     PointF(XCX + 1.0f, XCY + 1.0f),
                     PointF(XCX - arm + 2.0f, XCY + arm - 2.0f) };
    GraphicsPath p1, p2;
    p1.AddCurve(s1, 3, 1.0f);
    p2.AddCurve(s2, 3, 1.0f);
    g.DrawPath(&pen, &p1);
    g.DrawPath(&pen, &p2);
}

// ---------------- 第三步 / 第四步 共用的纸面与标题 ----------------
static void DrawPaperSheet(Graphics& g, float tPaper)
{
    // 从上一个画面（压暗）淡入到纸面
    if (tPaper < 1.0f) {
        if (g_bgCache) g.DrawImage(g_bgCache, RectF(0.0f, 0.0f, (float)BASE_W, (float)BASE_H));
        SolidBrush dim(Color(Alpha(130, 1.0f - tPaper), 0, 0, 0));
        g.FillRectangle(&dim, 0, 0, BASE_W, BASE_H);
    }
    if (!g_paper) return;

    if (tPaper >= 1.0f) {
        g.DrawImage(g_paper, RectF(0.0f, 0.0f, (float)BASE_W, (float)BASE_H));
    } else {
        ImageAttributes ia;
        ColorMatrix cm = { 1, 0, 0, 0, 0,
                           0, 1, 0, 0, 0,
                           0, 0, 1, 0, 0,
                           0, 0, 0, tPaper, 0,
                           0, 0, 0, 0, 1 };
        ia.SetColorMatrix(&cm);
        g.DrawImage(g_paper, RectF(0.0f, 0.0f, (float)BASE_W, (float)BASE_H),
                    0.0f, 0.0f, (float)BASE_W, (float)BASE_H, UnitPixel, &ia);
    }
}

static void DrawPageTitle(Graphics& g, const wchar_t* title, float t)
{
    if (t <= 0.0f) return;

    Font titleFont(g_famTitle, 48.0f, FontStyleBold, UnitPixel);
    StringFormat tf;
    tf.SetAlignment(StringAlignmentNear);
    tf.SetFormatFlags(StringFormatFlagsNoWrap | StringFormatFlagsNoClip);
    SolidBrush tb(Color(Alpha(255, t), 62, 46, 34));
    g.DrawString(title, -1, &titleFont, PointF(116.0f, 92.0f), &tf, &tb);

    float len  = (float)wcslen(title) * 48.0f;
    float half = len * 0.5f * t;
    float cx   = 116.0f + len * 0.5f;
    DrawWobbleLine(g, cx, 174.0f, half, 3.0f, 1.2f, 3.6f, Color(Alpha(150, t), 120, 44, 34));
    DrawWobbleLine(g, cx - 6.0f, 184.0f, half * 0.78f, 2.4f, 2.6f, 1.8f,
                   Color(Alpha(80, t), 120, 44, 34));
}

// ---------------- 第三步：游戏说明界面 ----------------
static void RenderHelp(Graphics& g, float el)
{
    float tPaper = Clamp01(el / 300.0f);
    DrawPaperSheet(g, tPaper);

    Color ink(255, 62, 46, 34);

    // 标题
    float tTitle = EaseOut(Clamp01((el - 140.0f) / 420.0f));
    DrawPageTitle(g, L"游戏说明", tTitle);

    // 正文
    float tBody = Clamp01((el - 330.0f) / 620.0f);
    if (tBody > 0.0f) {
        float em = 35.0f, lineH = 71.0f, bx = 118.0f, bw = 1040.0f;
        float y = DrawBodyText(g,
            L"点击开始游戏后选择角色，每个角色有自己专属的事件成就与结局，每个角色都有五个属性",
            *g_famBody, FontStyleRegular, em, bx, 254.0f, bw, lineH, ink, tBody, em * 2.0f);
        y += 22.0f;
        DrawBodyText(g,
            L"分别为健康，饱食，精神，快乐，知识。当这些数值太低时会发生不太好的事情。"
            L"游戏以时间为节点，每个时间段你可以去做不同的事情，最终达成不同的结局",
            *g_famBody, FontStyleRegular, em, bx, y, bw, lineH, ink, tBody, em * 2.0f);
    }

    // 右上角红叉
    float tX = EaseOut(Clamp01((el - 520.0f) / 300.0f));
    if (tX > 0.0f) {
        float sc = 0.55f + 0.45f * tX + 0.12f * g_xAnim;
        DrawRedCross(g, sc, tX);
    }
}

enum { ACH_HRUSHOV = 0, ACH_TIEGUO, ACH_XUEBA, ACH_COUNT };

static const wchar_t* kAchName[ACH_COUNT] = { L"赫鲁晓夫", L"铁锅炖白皮", L"学霸" };
static const wchar_t* kAchDesc[ACH_COUNT] = {
    L"在考试摘玉米中获得80分以上",
    L"在课外实践活动中选择“物理白皮”",
    L"检测到知识达到300触发"
};
static bool g_ach[ACH_COUNT] = { false, false, false };

// 结局：下标和开发说明的排列顺序一致，方便按顺序判定
enum { END_NONE = 0, END_MENTAL, END_HOSPITAL, END_HUNGRY, END_DEPRESS, END_COUNT };
static const wchar_t* kEndName[END_COUNT] = {
    nullptr, L"精神病院进修结局", L"医院结局", L"饿晕结局", L"抑郁结局"
};
static const wchar_t* kEndDesc[END_COUNT] = {
    nullptr,
    L"由于你的精神过于健康被保送至精神病院大学攻读被研究专业",
    L"由于你的健康状况堪忧被医生认定应该送去治疗",
    L"你的同学在路边发现了饿晕的你并送往了医院",
    L"努力了这么久，你终于考上了抑郁症"
};
static int g_ending = END_NONE;
// 第二十三步：达成过的结局要记下来（成就页要能查看），和成就一起存盘
static bool g_endGot[END_COUNT] = { false, false, false, false, false };

// 第二十三步：成就页内容装不下时用滚轮滚
static float g_achScroll = 0.0f;

// ---------------- 第二十二步：成就徽章（程序化画，不用素材） ----------------
// 达成 = 金色五角星；未达成 = 灰圆 + 问号
static void DrawAchBadge(Graphics& g, float cx, float cy, float r, bool got, float t)
{
    if (!got) {
        SolidBrush gb(Color(Alpha(190, t), 172, 164, 152));
        g.FillEllipse(&gb, cx - r, cy - r, r * 2.0f, r * 2.0f);
        Pen gp(Color(Alpha(220, t), 118, 110, 102), 2.6f);
        g.DrawEllipse(&gp, cx - r, cy - r, r * 2.0f, r * 2.0f);
        DrawTextCenter(g, L"?", *g_famTitle, FontStyleBold, r * 1.2f, cx, cy, 1.0f,
                       Color(Alpha(255, t), 250, 246, 238), 0.0f);
        return;
    }
    PointF pts[10];
    for (int i = 0; i < 10; ++i) {
        float ang = -1.5707963f + (float)i * 0.62831853f;      // 每 36° 一个顶点
        float rr  = (i % 2 == 0) ? r : r * 0.46f;
        pts[i] = PointF(cx + rr * std::cos(ang), cy + rr * std::sin(ang));
    }
    GraphicsPath sp;
    sp.AddPolygon(pts, 10);
    SolidBrush sb(Color(Alpha(255, t), 246, 198, 74));
    g.FillPath(&sb, &sp);
    Pen spen(Color(Alpha(255, t), 176, 122, 32), 3.0f);
    spen.SetLineJoin(LineJoinRound);
    g.DrawPath(&spen, &sp);
}

// ---------------- 第二十二步：成就查看页 ----------------
// 第二十三步：这一页要同时能看「成就」和「达成过的结局」，内容超出一屏 → 滚轮上下滚
static const float ACH_VP_X = 120.0f, ACH_VP_Y = 186.0f;   // 可视区（纸面内的窗口）
static const float ACH_VP_W = 1040.0f, ACH_VP_H = 458.0f;
static const float ACH_ROW_H = 100.0f, ACH_ROW_GAP = 14.0f, ACH_SEC_H = 44.0f;
static float g_achScrollMax = 0.0f;

// 内容总高（成就 + 结局），决定最多能滚多少
static float AchContentH()
{
    return ACH_SEC_H + (float)ACH_COUNT * (ACH_ROW_H + ACH_ROW_GAP)
         + ACH_SEC_H + (float)(END_COUNT - 1) * (ACH_ROW_H + ACH_ROW_GAP);
}

// 画一行（成就 / 结局共用）
static void DrawAchRow(Graphics& g, float rowX, float rowW, float rowH, float y,
                       const wchar_t* name, const wchar_t* desc, bool got, float t)
{
    Color ink(255, 62, 46, 34);
    Color inkDim(255, 148, 138, 126);

    RectF rr(rowX, y, rowW, rowH);
    GraphicsPath p;
    MakeRoundRect(p, rr, 12.0f);
    SolidBrush fb(got ? Color(Alpha(126, t), 248, 224, 148)
                      : Color(Alpha(64, t),  150, 140, 130));
    g.FillPath(&fb, &p);
    Pen ep(Color(Alpha(150, t), 120, 96, 70), 2.2f);
    g.DrawPath(&ep, &p);

    DrawAchBadge(g, rowX + 58.0f, y + rowH * 0.5f, 30.0f, got, t);

    // 名字（DrawBodyText 支持自定义颜色；DrawTextLine 只能画白色，纸上会看不见）
    DrawBodyText(g, name, *g_famTitle, FontStyleBold, 31.0f,
                 rowX + 112.0f, y + 16.0f, rowW - 300.0f, 42.0f,
                 got ? ink : inkDim, t, 0.0f);
    // 描述
    DrawBodyText(g, desc, *g_famBody, FontStyleRegular, 24.0f,
                 rowX + 112.0f, y + 62.0f, rowW - 300.0f, 34.0f,
                 got ? ink : inkDim, t, 0.0f);
    // 右侧状态
    DrawBodyText(g, got ? L"已达成" : L"未达成", *g_famUI, FontStyleBold, 23.0f,
                 rowX + rowW - 112.0f, y + 20.0f, 100.0f, 30.0f,
                 got ? Color(255, 176, 122, 32) : Color(255, 148, 138, 126), t, 0.0f);
}

static void RenderAchieve(Graphics& g, float el)
{
    float tPaper = Clamp01(el / 300.0f);
    DrawPaperSheet(g, tPaper);

    float tTitle = EaseOut(Clamp01((el - 140.0f) / 420.0f));
    DrawPageTitle(g, L"成就查看", tTitle);

    Color ink(255, 62, 46, 34);

    const float rowX = 146.0f, rowW = 988.0f;
    const float rowH = ACH_ROW_H, rowGap = ACH_ROW_GAP, secH = ACH_SEC_H;

    // ---- 先算内容总高（决定最多能滚多少） ----
    float cy = 0.0f;                       // 相对内容顶部的偏移
    float contentH = AchContentH();
    g_achScrollMax = contentH - ACH_VP_H;
    if (g_achScrollMax < 0.0f) g_achScrollMax = 0.0f;
    if (g_achScroll < 0.0f) g_achScroll = 0.0f;
    if (g_achScroll > g_achScrollMax) g_achScroll = g_achScrollMax;

    // ---- 内容区裁剪 + 整体平移（滚轮） ----
    GraphicsState st = g.Save();
    g.SetClip(RectF(ACH_VP_X, ACH_VP_Y, ACH_VP_W, ACH_VP_H), CombineModeReplace);
    g.TranslateTransform(0.0f, -g_achScroll);

    // 小标题：成就
    float tSec1 = EaseOut(Clamp01((el - 300.0f) / 380.0f));
    DrawBodyText(g, L"成就", *g_famTitle, FontStyleBold, 30.0f,
                 rowX, ACH_VP_Y + cy, 400.0f, secH, ink, tSec1, 0.0f);
    cy += secH;

    for (int i = 0; i < ACH_COUNT; ++i) {
        float t = EaseOut(Clamp01((el - (330.0f + i * 130.0f)) / 420.0f));
        DrawAchRow(g, rowX, rowW, rowH, ACH_VP_Y + cy,
                   kAchName[i], kAchDesc[i], g_ach[i], t);
        cy += rowH + rowGap;
    }

    // 小标题：已达成的结局
    float tSec2 = EaseOut(Clamp01((el - 620.0f) / 380.0f));
    DrawBodyText(g, L"达成的结局", *g_famTitle, FontStyleBold, 30.0f,
                 rowX, ACH_VP_Y + cy, 400.0f, secH, ink, tSec2, 0.0f);
    cy += secH;

    for (int e = 1; e < END_COUNT; ++e) {          // 0 = END_NONE 跳过
        float t = EaseOut(Clamp01((el - (650.0f + (e - 1) * 130.0f)) / 420.0f));
        DrawAchRow(g, rowX, rowW, rowH, ACH_VP_Y + cy,
                   kEndName[e], kEndDesc[e], g_endGot[e], t);
        cy += rowH + rowGap;
    }

    g.Restore(st);

    // ---- 上下边缘淡出（让"截断"看起来像翻页，而不是画坏了） ----
    if (g_achScrollMax > 1.0f) {
        const Color paperC(250, 250, 244, 226);
        if (g_achScroll > 1.0f) {
            RectF ft(ACH_VP_X, ACH_VP_Y, ACH_VP_W, 30.0f);
            LinearGradientBrush fb(ft, Color(240, paperC.GetR(), paperC.GetG(), paperC.GetB()),
                                   Color(0, paperC.GetR(), paperC.GetG(), paperC.GetB()),
                                   LinearGradientModeVertical);
            g.FillRectangle(&fb, ft);
        }
        if (g_achScroll < g_achScrollMax - 1.0f) {
            float by = ACH_VP_Y + ACH_VP_H - 30.0f;
            RectF fb2(ACH_VP_X, by, ACH_VP_W, 30.0f);
            LinearGradientBrush fbr(fb2, Color(0, paperC.GetR(), paperC.GetG(), paperC.GetB()),
                                    Color(240, paperC.GetR(), paperC.GetG(), paperC.GetB()),
                                    LinearGradientModeVertical);
            g.FillRectangle(&fbr, fb2);
        }
    }

    // ---- 右侧滚动条 ----
    if (g_achScrollMax > 1.0f) {
        float tx = ACH_VP_X + ACH_VP_W + 8.0f;
        SolidBrush tb(Color(80, 120, 96, 70));
        g.FillRectangle(&tb, tx, ACH_VP_Y, 8.0f, ACH_VP_H);
        float kh = ACH_VP_H * (ACH_VP_H / contentH);
        float ky = ACH_VP_Y + (ACH_VP_H - kh) * (g_achScroll / g_achScrollMax);
        SolidBrush kb(Color(200, 176, 122, 32));
        g.FillRectangle(&kb, tx - 1.0f, ky, 10.0f, kh);
    }

    // 底部提示
    if (g_achScrollMax > 1.0f) {
        DrawTextCenter(g, L"滚轮上下滚动", *g_famUI, FontStyleRegular, 18.0f,
                       640.0f, 660.0f, 1.4f, Color(150, 148, 138, 126), 0.0f);
    }

    // 右上角红叉
    float tX = EaseOut(Clamp01((el - 520.0f) / 300.0f));
    if (tX > 0.0f) {
        float sc = 0.55f + 0.45f * tX + 0.12f * g_xAnim;
        DrawRedCross(g, sc, tX);
    }
}


// 前置声明（按钮画在菜单部分，这里先声明一下）
static void DrawButton(Graphics& g, int index, RectF r, const wchar_t* label,
                       float hover, float appear);

// ================= 第四步：角色选择 =================
static const int HERO_COUNT = 2;

static const wchar_t* kHeroName[HERO_COUNT] = { L"芋头", L"继科" };
static const wchar_t* kHeroFile[HERO_COUNT] = {
    L"芋头_校服立绘_B版_双手插兜.png",
    L"继科_冬季校服立绘_C版_基于夏季B版比例.png"
};
static const wchar_t* kHeroIntro[HERO_COUNT] = {
    L"乐天派芋头，总是感觉到很快乐，具有超乎常人的熬夜能力",
    L"体育超强的继科，拥有超乎常人的健康，而运气似乎具有人鬼二相性？"
};
// 立绘自带的背景色（RGB 无透明通道，卡片里用同色填充，接缝看不出来）
static const int kHeroBG[HERO_COUNT][3] = { { 239, 237, 231 }, { 224, 216, 206 } };

static Bitmap* g_hero[HERO_COUNT] = { nullptr, nullptr };

// 卡片布局（逻辑坐标）
static const float CARD_W = 360.0f, CARD_H = 462.0f, CARD_GAP = 90.0f, CARD_Y = 202.0f;

static int   g_detail = -1;            // 正在看介绍的角色（关闭动画期间保留）
static bool  g_detailOpen = false;
static float g_detailAnim = 0.0f;
static int   g_chosen = -1;            // 已确认的角色

// 介绍面板
static const float PN_X = 190.0f, PN_Y = 126.0f, PN_W = 900.0f, PN_H = 460.0f;
static const float OK_W = 150.0f, OK_H = 58.0f;
static const float OK_X = 566.0f, OK_Y = PN_Y + 370.0f;
static const float BACK_X = OK_X + 172.0f;

static float CardX(int i)
{
    float total = HERO_COUNT * CARD_W + (HERO_COUNT - 1) * CARD_GAP;
    return 640.0f - total * 0.5f + i * (CARD_W + CARD_GAP);
}

// 等比缩放到框内（底对齐，人物脚踩在框底）
static void DrawContain(Graphics& g, Image* img, RectF box, float alphaK)
{
    if (!img || alphaK <= 0.0f) return;
    float iw = (float)img->GetWidth();
    float ih = (float)img->GetHeight();
    if (iw < 1.0f || ih < 1.0f) return;

    float s = box.Width / iw;
    float sy = box.Height / ih;
    if (sy < s) s = sy;
    float w = iw * s, h = ih * s;
    RectF dst(box.X + (box.Width - w) * 0.5f, box.Y + box.Height - h, w, h);

    if (alphaK >= 1.0f) {
        g.DrawImage(img, dst);
    } else {
        ImageAttributes ia;
        ColorMatrix cm = { 1, 0, 0, 0, 0,
                           0, 1, 0, 0, 0,
                           0, 0, 1, 0, 0,
                           0, 0, 0, alphaK, 0,
                           0, 0, 0, 0, 1 };
        ia.SetColorMatrix(&cm);
        g.DrawImage(img, dst, 0.0f, 0.0f, iw, ih, UnitPixel, &ia);
    }
}

static void DrawInkString(Graphics& g, const wchar_t* text, const FontFamily& fam, INT style,
                          float em, float x, float y, Color c)
{
    Font font(&fam, em, style, UnitPixel);
    StringFormat fmt;
    fmt.SetAlignment(StringAlignmentNear);
    fmt.SetFormatFlags(StringFormatFlagsNoWrap | StringFormatFlagsNoClip);
    SolidBrush br(c);
    g.DrawString(text, -1, &font, PointF(x, y), &fmt, &br);
}

// 第二十三步（新增4）：台词里带表情符号时，普通字用 UI 字体、表情用「Segoe UI Emoji」分段画，
//   免得楷体 / 黑体把表情渲染成一片方框（字体取不到就整句照原样画）
static FontFamily* g_famEmoji = nullptr;

static bool IsEmojiUnit(const wchar_t* s, int& len)
{
    wchar_t c = s[0];
    if (c >= 0xD800 && c <= 0xDBFF && s[1] >= 0xDC00 && s[1] <= 0xDFFF) { len = 2; return true; }
    if (c >= 0x2600 && c <= 0x27BF) { len = 1; return true; }   // ❌ ✨ ☝ 之类
    if (c >= 0x2B00 && c <= 0x2BFF) { len = 1; return true; }
    if (c >= 0xFE00 && c <= 0xFE0F) { len = 1; return true; }   // 变体选择符，跟前一个字同段
    len = 1; return false;
}

static void DrawInkStringMixed(Graphics& g, const wchar_t* text, const FontFamily& fam, INT style,
                               float em, float x, float y, Color c)
{
    if (!text || !*text) return;
    if (!g_famEmoji) { DrawInkString(g, text, fam, style, em, x, y, c); return; }

    float cx = x;
    std::wstring seg;
    bool  segEmoji = false;
    auto flush = [&]() {
        if (seg.empty()) return;
        const FontFamily& f = segEmoji ? *g_famEmoji : fam;
        Font font(&f, em, style, UnitPixel);
        StringFormat fmt;
        fmt.SetAlignment(StringAlignmentNear);
        fmt.SetFormatFlags(StringFormatFlagsNoWrap | StringFormatFlagsNoClip);
        SolidBrush br(c);
        g.DrawString(seg.c_str(), (int)seg.size(), &font, PointF(cx, y), &fmt, &br);
        RectF r;
        g.MeasureString(seg.c_str(), (int)seg.size(), &font, PointF(0.0f, 0.0f), &fmt, &r);
        cx += r.Width;
        seg.clear();
    };
    for (const wchar_t* p = text; *p; ) {
        int  len = 1;
        bool em  = IsEmojiUnit(p, len);
        if (!seg.empty() && em != segEmoji) flush();
        segEmoji = em;
        seg.append(p, (size_t)len);
        p += len;
    }
    flush();
}

// 一张角色卡：上面 Q 版立绘，下面名字
static void DrawHeroCard(Graphics& g, int i, float hover, float appear)
{
    RectF r(CardX(i), CARD_Y, CARD_W, CARD_H);
    float sc   = 1.0f + 0.030f * hover;
    float lift = -6.0f * hover;
    float cx   = r.X + r.Width * 0.5f;
    float cy   = r.Y + r.Height * 0.5f;

    RectF rr(cx - r.Width * 0.5f * sc, cy - r.Height * 0.5f * sc + lift,
             r.Width * sc, r.Height * sc);

    // 硬阴影
    {
        GraphicsPath sp;
        MakeRoundRect(sp, RectF(rr.X + 9.0f, rr.Y + 11.0f, rr.Width, rr.Height), 18.0f);
        SolidBrush sb(Color(Alpha(85, appear), 0, 0, 0));
        g.FillPath(&sb, &sp);
    }

    // 卡面
    GraphicsPath card;
    MakeRoundRect(card, rr, 18.0f);
    {
        int cr = 253, cg = 250, cb = 241;
        if (g_chosen == i) { cr = 255; cg = 240; cb = 198; }        // 已选：淡黄
        else if (hover > 0.01f) {
            cr = (int)(253 + (255 - 253) * hover);
            cg = (int)(250 + (240 - 250) * hover);
            cb = (int)(241 + (198 - 241) * hover);
        }
        SolidBrush fill(Color(Alpha(252, appear), (BYTE)cr, (BYTE)cg, (BYTE)cb));
        g.FillPath(&fill, &card);
    }

    // 立绘框
    RectF pr(rr.X + 14.0f, rr.Y + 14.0f, rr.Width - 28.0f, rr.Height - 126.0f);
    {
        GraphicsPath pp;
        MakeRoundRect(pp, pr, 12.0f);
        SolidBrush bg(Color(Alpha(255, appear), (BYTE)kHeroBG[i][0],
                            (BYTE)kHeroBG[i][1], (BYTE)kHeroBG[i][2]));
        g.FillPath(&bg, &pp);
        DrawContain(g, g_hero[i], pr, appear);

        Pen pen(Color(Alpha(110, appear), 60, 48, 36), 1.6f);
        g.DrawPath(&pen, &pp);
    }

    // 名字
    DrawTextCenter(g, kHeroName[i], *g_famTitle, FontStyleBold, 38.0f, cx,
                   rr.Y + rr.Height - 58.0f, 8.0f, Color(Alpha(255, appear), 48, 40, 30), 0.8f);

    // 已选标记
    if (g_chosen == i && appear > 0.9f) {
        DrawInkString(g, L"已选择", *g_famUI, FontStyleBold, 18.0f,
                      rr.X + 26.0f, rr.Y + rr.Height - 56.0f, Color(210, 176, 120, 40));
    }

    // 卡片描边（画两遍，手绘感）
    {
        Pen ink(Color(Alpha(255, appear), 34, 30, 26), 3.2f);
        ink.SetLineJoin(LineJoinRound);
        g.DrawPath(&ink, &card);

        Pen ink2(Color(Alpha(110, appear), 34, 30, 26), 1.4f);
        ink2.SetLineJoin(LineJoinRound);
        Matrix m;
        m.Translate(1.8f, 2.0f);
        card.Transform(&m);
        g.DrawPath(&ink2, &card);
    }
}

// 角色介绍面板
static void DrawDetailPanel(Graphics& g, float t)
{
    if (g_detail < 0 || t <= 0.0f) return;
    int i = g_detail;

    // 压暗背景
    SolidBrush dim(Color(Alpha(130, t), 0, 0, 0));
    g.FillRectangle(&dim, 0, 0, BASE_W, BASE_H);

    float slide = 22.0f * (1.0f - t);
    RectF r(PN_X, PN_Y + slide, PN_W, PN_H);

    // 阴影 + 纸面
    {
        GraphicsPath sp;
        MakeRoundRect(sp, RectF(r.X + 10.0f, r.Y + 13.0f, r.Width, r.Height), 20.0f);
        SolidBrush sb(Color(Alpha(110, t), 0, 0, 0));
        g.FillPath(&sb, &sp);
    }
    GraphicsPath panel;
    MakeRoundRect(panel, r, 20.0f);
    {
        SolidBrush fill(Color(Alpha(253, t), 254, 251, 242));
        g.FillPath(&fill, &panel);
    }

    // 左：立绘
    RectF pr(r.X + 26.0f, r.Y + 26.0f, 320.0f, r.Height - 52.0f);
    {
        GraphicsPath pp;
        MakeRoundRect(pp, pr, 14.0f);
        SolidBrush bg(Color(Alpha(255, t), (BYTE)kHeroBG[i][0],
                            (BYTE)kHeroBG[i][1], (BYTE)kHeroBG[i][2]));
        g.FillPath(&bg, &pp);
        DrawContain(g, g_hero[i], pr, t);
        Pen pen(Color(Alpha(110, t), 60, 48, 36), 1.6f);
        g.DrawPath(&pen, &pp);
    }

    // 右：名字 + 介绍 + 提问 + 按钮
    float tx = r.X + 376.0f;
    float tw = r.X + r.Width - 34.0f - tx;
    Color ink(255, 52, 42, 32);

    DrawInkString(g, kHeroName[i], *g_famTitle, FontStyleBold, 44.0f,
                  tx, r.Y + 44.0f, Color(Alpha(255, t), ink.GetR(), ink.GetG(), ink.GetB()));
    {
        float halfW = 52.0f * (float)wcslen(kHeroName[i]) * 0.5f;
        DrawWobbleLine(g, tx + halfW, r.Y + 116.0f, halfW, 2.6f, 1.5f, 3.0f,
                       Color(Alpha(140, t), 140, 60, 44));
    }

    DrawBodyText(g, kHeroIntro[i], *g_famBody, FontStyleRegular, 29.0f,
                 tx, r.Y + 142.0f, tw, 52.0f, ink, t, 0.0f);

    DrawInkString(g, L"是否确认选择该英雄", *g_famUI, FontStyleRegular, 25.0f,
                  tx, r.Y + 318.0f, Color(Alpha(210, t), 80, 66, 52));

    // 是 / 返回
    DrawButton(g, 10, RectF(OK_X, OK_Y + slide, OK_W, OK_H), L"是", g_hoverAnim[10], t);
    DrawButton(g, 11, RectF(BACK_X, OK_Y + slide, OK_W, OK_H), L"返回", g_hoverAnim[11], t);

    // 面板描边
    {
        Pen inkp(Color(Alpha(255, t), 34, 30, 26), 3.2f);
        inkp.SetLineJoin(LineJoinRound);
        g.DrawPath(&inkp, &panel);
    }
}

// 第四步：角色选择界面
static void RenderPick(Graphics& g, float el)
{
    float tPaper = Clamp01(el / 300.0f);
    DrawPaperSheet(g, tPaper);

    // 标题
    DrawPageTitle(g, L"请选择你的英雄", EaseOut(Clamp01((el - 120.0f) / 420.0f)));

    // 两张角色卡
    for (int i = 0; i < HERO_COUNT; ++i) {
        float t = EaseOut(Clamp01((el - (220.0f + i * 110.0f)) / 450.0f));
        if (t <= 0.0f) continue;
        DrawHeroCard(g, i, (g_detailOpen ? 0.0f : g_hoverAnim[i]), t);
    }

    // 右上角红叉（回主菜单）
    float tX = EaseOut(Clamp01((el - 480.0f) / 300.0f));
    if (tX > 0.0f) DrawRedCross(g, 0.55f + 0.45f * tX + 0.12f * g_xAnim, tX);

    // 介绍面板
    if (g_detail >= 0 && g_detailAnim > 0.004f) DrawDetailPanel(g, g_detailAnim);
}

// ================= 第五步：游戏内 UI =================
static const int TIME_COUNT = 22;
static const wchar_t* kTimeNames[TIME_COUNT] = {
    L"清晨", L"上午1节", L"上午2节", L"上午3节", L"上午4节", L"上午5节",
    L"中午下课", L"中午吃饭时间", L"中午午时", L"午休时间",
    L"下午1节", L"下午2节", L"下午3节", L"下午4节",
    L"晚饭前", L"晚饭时间", L"晚自习一节", L"晚自习二节", L"晚自习三节", L"晚自习下课",
    L"晚寝时间", L"凌晨时间"
};

static const wchar_t* kAttrName[5] = { L"健康", L"饱食", L"精神", L"快乐", L"知识" };
static const int kAttrColor[5][3] = {
    { 226, 88, 82 }, { 238, 158, 62 }, { 84, 148, 214 }, { 240, 196, 74 }, { 116, 178, 122 }
};

// 表情：0 平静 / 1 开心 / 2 疲惫 / 3 难过
enum Face { FACE_CALM = 0, FACE_HAPPY, FACE_TIRED, FACE_SAD, FACE_COUNT };
static const wchar_t* kFaceName[FACE_COUNT] = { L"平静", L"开心", L"疲惫", L"难过" };
static const wchar_t* kFaceFile[HERO_COUNT][FACE_COUNT] = {
    { L"芋头头像_平静.png", L"芋头头像_开心.png", L"芋头头像_疲惫.png", L"芋头头像_难过.png" },
    { L"继科_头像_平静.png", L"继科_头像_开心.png", L"继科_头像_疲惫.png", L"继科_头像_难过.png" }
};

static Bitmap* g_avatar[HERO_COUNT][FACE_COUNT] = { { nullptr } };
static const int kAvatarBG[HERO_COUNT][3] = { { 240, 237, 232 }, { 225, 217, 205 } };

// ===== 第六步：表情判定（严格按开发说明的条件） =====
//   精神 < 40                -> 疲惫
//   否则 快乐 <  40          -> 难过
//        快乐 40 ~ 80        -> 平静
//        快乐 >  80          -> 开心
class FaceResolver
{
public:
    static int Resolve(int spirit, int happy)
    {
        if (spirit < 40) return FACE_TIRED;
        if (happy  < 40) return FACE_SAD;
        if (happy  > 80) return FACE_HAPPY;
        return FACE_CALM;
    }
};

// 表情状态机：记住当前/上一个表情和淡化进度，切换时做交叉淡化
class FaceState
{
public:
    void Reset()
    {
        cur  = FACE_CALM;
        prev = -1;
        fade = 1.0f;
        Refresh(70, 70);          // 用初始数值定一次初始表情
    }

    // 数值一变就调用（玩家每次做选择后都要调）；返回表情是否切换了
    bool Refresh(int spirit, int happy)
    {
        int want = (force >= 0) ? force : FaceResolver::Resolve(spirit, happy);
        if (want == cur) return false;
        prev = cur;
        cur  = want;
        fade = 0.0f;
        return true;
    }

    // 推进淡化，dt 单位秒；一次切换约 0.2 秒
    void Tick(float dt)
    {
        if (fade >= 1.0f) return;
        fade += dt * 5.0f;
        if (fade > 1.0f) fade = 1.0f;
    }

    int   cur  = FACE_CALM;
    int   prev = -1;
    float fade = 1.0f;
    int   force = -1;             // 调试用：>=0 时强制某个表情
};

static FaceState g_faceState;

static Bitmap* g_classBg = nullptr;      // 教学楼界面.jpg（第七步：游戏内场景背景）
static Bitmap* g_gameBg  = nullptr;      // 压暗后的背景
static int     g_gameBgW = 0, g_gameBgH = 0;

// 第十二步：食堂和小卖部的背景（同样先压暗，逻辑坐标下也是整幅居中）
static Bitmap* g_canteenSrc  = nullptr;
static Bitmap* g_canteenBg   = nullptr;
static int     g_canteenBgW = 0, g_canteenBgH = 0;

// 第十三步：宿舍背景
static Bitmap* g_dormSrc  = nullptr;
static Bitmap* g_dormBg   = nullptr;
static int     g_dormBgW = 0, g_dormBgH = 0;

// ================= 第八步：地图页 =================
static Bitmap* g_mapImg    = nullptr;    // 地图.jpg 原图
static Bitmap* g_mapScaled = nullptr;    // 预缩放好的地图（1195x720）
// 10 个场地：热区用「有方向的矩形」贴合建筑朝向（不再用扁椭圆）
// x1,y1,x2,y2 = 矩形（原图像素）；ax,ay = 黑色编号所在位置（原图像素，落在矩形内）
// 矩形之间互不重叠，判定直接用包含关系
struct MapSpot {
    const wchar_t* name;
    int x1, y1, x2, y2;
    int ax, ay;
};
static const MapSpot kMapSpots[10] = {
    { L"教学楼",         410, 680,  945,  960,  600, 800 },   // 1 左下白色建筑群（横向）
    { L"实验楼",         350, 330,  630,  670,  455, 490 },   // 2 左侧高瘦楼群（竖向）
    { L"操场",          1320, 610, 1825,  870, 1570, 735 },   // 3 跑道 + 绿足球场（横向）
    { L"体育馆",        1330, 925, 1870, 1140, 1600, 1030 },  // 4 右下建筑 + 水池（横向）
    { L"球场",          1395, 300, 1800,  520, 1590, 410 },   // 5橙色篮球场（横向）
    { L"行政楼与图书馆",  680, 235, 1115,  455,  890, 380 },   // 6 中上蓝窗大楼（横向）
    { L"食堂和小卖部",  1160,  45, 1590,  300, 1290, 210 },   // 7 上方弧形白楼（横向）
    { L"农田",          1800, 200, 2075,  500, 1880, 320 },   // 8 右上池塘 + 凉亭（方形）
    { L"宿舍",           620,   5, 1140,  215,  860, 150 },   // 9 顶部三栋住宅楼（横向长条）
    { L"音乐楼",        1130, 350, 1395,  525, 1262, 437 }    // 10 蓝玻璃顶的楼（第十九步新增）
};

// 地图在 1280x720 逻辑画布中的摆放：整幅按高度铺满、水平居中
static const float MAP_SCALE = 0.5625f;                       // 720 / 1280
static const float MAP_OFF_X = 42.625f;                       // (1280 - 2124*0.5625)/2
static const float MAP_OFF_Y = 0.0f;
static const float MAP_W     = 1194.75f;
static const float MAP_H     = 720.0f;

static float MapSX(int sx) { return MAP_OFF_X + (float)sx * MAP_SCALE; }
static float MapSY(int sy) { return MAP_OFF_Y + (float)sy * MAP_SCALE; }

// 地图页底部「返回」按钮
static const float MBACK_X = 26.0f, MBACK_Y = 634.0f, MBACK_W = 124.0f, MBACK_H = 50.0f;

static int g_stat[5] = { 70, 70, 70, 70, 0 };   // 健康/饱食/精神/快乐/知识
static int g_day  = 1;
static int g_time = 0;
static int g_money = 200;
static float g_backAnim = 0.0f;

// ===== 第十步：对话框 / 事务安排 =====
static bool  g_hasPlan   = false;   // 物品栏里有没有「事务安排」
static bool  g_planOpen  = false;   // 正在查看事务安排
static float g_planAnim  = 0.0f;    // 开合动画
static int   g_planPage  = 0;       // 0~6 = Day1~Day7
static bool  g_dlgActive = false;   // 对话框是否在显示
static bool  g_storyDone = false;   // day1 清晨剧情是否已播过
static int   g_dlgLine   = 0;       // 当前第几句
static float g_dlgChars  = 0.0f;    // 已浮现的字数
static int   g_dlgHold   = 0;       // 播完这一句后要不要往物品栏塞东西
static int   g_classStamp = -1;     // 第十一步：已经弹过「这节课该如何度过」的时段（day*100+time）

// ===== 第十二步：食堂和小卖部 =====
enum { LOC_CLASS = 0, LOC_CANTEEN = 1, LOC_DORM = 2, LOC_FARM = 3, LOC_BOOK = 4,
       LOC_PLAY = 5,      // 第十六步：操场
       LOC_COURT = 6,     // 第十七步：篮球场
       LOC_MUSIC = 7,     // 第十九步：音乐楼
       LOC_EVENT1 = 8,    // 第二十步：课外实践活动
       LOC_GAOKAO = 9,    // 第二十步：北京名师讲高考
       LOC_EXAM = 10 };   // 第二十一步：考试

// 第二十三步（新增3）：事件场地（课外实践 / 讲高考 / 考试）—— 事件一结束就退回教学楼，
// 免得回到教室后背景还停在事件的图上
static bool IsEventLoc(int loc)
{
    return (loc == LOC_EVENT1 || loc == LOC_GAOKAO || loc == LOC_EXAM);
}
static int    g_loc       = LOC_CLASS;
static void LeaveEventLoc()      // 定义在 g_loc 之后：退出事件场地
{
    if (IsEventLoc(g_loc)) g_loc = LOC_CLASS;
}
static bool   g_shopOpen  = false;            // 商店界面开着
static int    g_shopKind  = 0;                // 0 = 食堂商店，1 = 书屋
static float  g_shopAnim  = 0.0f;
static int    g_inv[9]    = { 0 };            // 物品栏：0 号格放钱，1~8 号格放东西
static const wchar_t* g_centerMsg = nullptr;  // 屏幕正中提示（预算不足 / 背包满了 / 该休息了）
static ULONGLONG      g_centerMsgUntil = 0;
static int    g_dormStamp = -1;               // 第十三步：已经强制拉去宿舍的时段

// ===== 第十四步：行政楼与图书馆（书屋） =====
static Bitmap* g_bookSrc  = nullptr;
static Bitmap* g_bookBg   = nullptr;
static int     g_bookBgW = 0, g_bookBgH = 0;

// 通用确认弹窗（是 / 否）
enum { CONF_NONE = 0, CONF_GOTO_BOOK = 1, CONF_THROW_BOOK = 2 };
static int            g_confirmKind = CONF_NONE;
static const wchar_t* g_confirmText = nullptr;
static int            g_confirmSlot = -1;      // 待扔掉的物品格

// ===== 第十五步：农田 =====
static bool g_cornDone   = false;         // 玉米事件（第二十一步的摘玉米）完成后为 true
static bool g_cornPlayed = false;         // 第二十一步：摘玉米小游戏只能玩一次
static bool g_pianoPlayed = false;        // 第二十三步（新增3）：钢琴键同样只能玩一次

// ===== 第二十一步：考试（状态量提前声明，ResetGameState 里要清） =====
static Bitmap* g_examSrc = nullptr;       // 考试.jpg
static Bitmap* g_examBg  = nullptr;
static int     g_examBgW = 0, g_examBgH = 0;
static bool    g_examDone     = false;    // 考试事件是否已触发过
static int     g_examStamp    = -1;
static int     g_examIdx      = 0;        // 当前第几门 0~3
static int     g_examPhase    = 0;        // 0 = 认真考/轻松考   1 = 去摘玉米/去你*的
static bool    g_examMathCorn = false;    // 数学考试选了「去摘玉米」
static int     g_examScore[4] = { 0, 0, 0, 0 };   // 物理/语文/英语/数学（保存不展示）
static Bitmap* g_farmSrc  = nullptr;      // 初始背景（农田界面.jpg）
static Bitmap* g_farm2Src = nullptr;      // 玉米事件后的背景（农田结束界面.jpg）
static Bitmap* g_farmBg   = nullptr;
static int     g_farmBgW = 0, g_farmBgH = 0;

// ===== 第十六步：操场 =====
static Bitmap* g_playSrc  = nullptr;      // 操场界面.jpg
static Bitmap* g_playBg   = nullptr;
static int     g_playBgW = 0, g_playBgH = 0;
// 第二十步：通用的「5 秒动作动画」，跑步只是其中一种
enum { ANIM_NONE = 0, ANIM_RUN, ANIM_EAT, ANIM_NAP, ANIM_SLEEP,
       ANIM_NIGHT_STUDY, ANIM_NIGHT_GAME, ANIM_FARM_WALK, ANIM_STUDY };
static int            g_animKind  = ANIM_NONE;  // 当前播哪一种
static int            g_animAfter = 0;          // 播完要接着做的 DN_*
static bool           g_runActive = false;    // 动画播放中（沿用旧名）
static ULONGLONG      g_runStart  = 0;
static int            g_playStamp = -1;       // 已经强制拉去操场的时段（体育课）
static bool           g_playPE    = false;    // 这次进操场是不是体育课
static const int      RUN_MS = 2000;          // 第二十一步：每个动作动画统一 2 秒

// 第二十步：两个事件的进度标记
//   放在 ResetGameState 之前 —— 新开一局要把它们一起清掉，否则第二周目事件不再触发
static bool    g_event1Done   = false;     // 课外实践已经做过
static int     g_event1Stamp  = -1;        // 已经触发过的时段戳
static bool    g_event1Second = false;     // 是否换成第二张背景（铁锅炖白皮）
static bool    g_pickItemMode = false;     // 正在让玩家选物品栏里的东西
static bool    g_gaokaoDone   = false;     // 讲高考已经听过
static int     g_gaokaoStamp  = -1;
static int     g_gaokaoSeg    = 0;         // 第二十三步：讲高考讲到第几段（0~3 = 下午一~四节）

// 第十九步：音乐楼背景
static Bitmap* g_musicSrc = nullptr;
static Bitmap* g_musicBg  = nullptr;
static int     g_musicBgW = 0, g_musicBgH = 0;

// 第十七步：篮球场背景 + 投篮游戏开关
static Bitmap* g_courtSrc  = nullptr;
static Bitmap* g_courtBg   = nullptr;
static int     g_courtBgW = 0, g_courtBgH = 0;
static bool    g_basketActive = false;    // 投篮游戏是否开着（开着时接管整屏和输入）
static bool    cnActive       = false;    // 第二十一步：摘玉米游戏是否开着（同样整屏接管）

// 物品栏里的东西（ITEM_NONE 与 ITEM_EMPTY 同义：0 = 空格）
enum { ITEM_NONE = 0, ITEM_EMPTY = 0, ITEM_PLAN = 1, ITEM_BISCUIT = 2, ITEM_BURGER = 3,
       ITEM_CRACKER = 4, ITEM_COFFEE = 5,          // 压缩饼干 / 咖啡
       ITEM_BOOK_PHYS = 6, ITEM_BOOK_CHINESE = 7,  // 第十四步：书屋的书
       ITEM_BOOK_MATH = 8, ITEM_BOOK_ENGLISH = 9, ITEM_BOOK_GUIDE = 10,
       ITEM_CORN = 11 };               // 第二十一步：摘玉米得到的玉米

// 面板布局
// 第九步：左上角头像「略微缩」，给上课场景的老师让出画面
static const float HUD_X = 16.0f, HUD_Y = 14.0f, HUD_W = 462.0f, HUD_H = 184.0f;
static const float AV_X = 30.0f, AV_Y = 26.0f, AV_S = 100.0f;
static const float BAR_X = 168.0f, BAR_W = 170.0f, BAR_H = 18.0f, BAR_TOP = 30.0f, BAR_GAP = 9.0f;
static const float DAY_CX = 1168.0f, DAY_CY = 96.0f, DAY_R = 66.0f;
// 第二十三步：显示时间的 UI 右侧的「保存」按钮（时间标签下方，避开天数圆盘和自修一会）
static const float SAVE_X = 1002.0f, SAVE_Y = 168.0f, SAVE_W = 76.0f, SAVE_H = 40.0f;
static const int   SLOT_N = 9;
static const float SLOT_S = 76.0f, SLOT_GAP = 10.0f, SLOT_Y = 622.0f;
static const float GBACK_X = 26.0f, GBACK_Y = 640.0f, GBACK_W = 104.0f, GBACK_H = 44.0f;

static void ResetGameState()
{
    for (int i = 0; i < 5; ++i) g_stat[i] = 70;
    g_stat[4] = 0;                     // 知识初始 0
    g_day = 1;
    g_time = 0;
    g_money = 200;
    g_faceState.force = -1;
    g_faceState.Reset();
    // 第十步：新开一局时清掉剧情进度和道具
    g_hasPlan   = false;
    g_planOpen  = false;
    g_planAnim  = 0.0f;
    g_planPage  = 0;
    g_dlgActive = false;
    g_storyDone = false;
    g_dlgLine   = 0;
    g_dlgChars  = 0.0f;
    g_dlgHold   = 0;
    g_classStamp = -1;
    // 第十二步：回教学楼、清空背包（钱格保留），商店关掉
    g_loc = LOC_CLASS;
    g_shopOpen = false;
    g_shopKind = 0;
    g_shopAnim = 0.0f;
    g_centerMsg = nullptr;
    g_dormStamp = -1;
    g_confirmKind = CONF_NONE;
    g_confirmSlot = -1;
    g_runActive = false;
    g_animKind  = ANIM_NONE;
    g_animAfter = 0;
    g_basketActive = false;
    g_playStamp = -1;
    g_playPE = false;
    // 第二十一步：考试与摘玉米的进度
    g_cornDone   = false;
    g_cornPlayed = false;
    g_examDone   = false;
    g_examStamp  = -1;
    g_examIdx    = 0;
    g_examPhase  = 0;
    g_examMathCorn = false;
    for (int i = 0; i < 4; ++i) g_examScore[i] = 0;
    g_pianoPlayed = false;                 // 第二十三步（新增3）：钢琴键每周目一次
    // 第二十三步（新增4）：两个事件的进度也要清，否则第二周目 day2 的事件不再触发
    g_event1Done   = false;  g_event1Stamp  = -1;  g_event1Second = false;
    g_pickItemMode = false;
    g_gaokaoDone   = false;  g_gaokaoStamp  = -1;  g_gaokaoSeg    = 0;
    cnActive = false;
    for (int i = 0; i < 9; ++i) g_inv[i] = ITEM_EMPTY;
}

// 第七步：游戏内场景背景 = 教学楼界面.jpg
// 图片是 1850x1279（比 16:9 更"高"），所以按「整幅清晰图 + 两侧虚化补边」的方式摆，
// 保证黑板和课桌都完整露出来；再压暗 + 上下渐深，让第五步的 UI 压得住画面。
// 压暗强度集中在这里调（老师的立绘压暗也读这几个常量，改一处两边同步）
static const int BG_DIM_A = 30;      // 整体压暗（原 72，太闷，调亮）
static const int BG_TOP_A = 118;     // 顶部渐深（原 150）
static const int BG_BOT_A = 132;     // 底部渐深（原 170）

// 通用的「整幅清晰图居中 + 两侧虚化补边 + 压暗」构建（教学楼 / 食堂共用）
static void BuildDimBg(Image* src, Bitmap** cache, int* cw, int* ch, int W, int H)
{
    if (W <= 0 || H <= 0) return;
    if (*cache && *cw == W && *ch == H) return;

    delete *cache;
    *cache = new Bitmap(W, H, PixelFormat32bppPARGB);
    *cw = W;
    *ch = H;

    Graphics gb(*cache);
    gb.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    gb.SetPixelOffsetMode(PixelOffsetModeHighQuality);
    gb.Clear(Color(255, 20, 24, 28));

    if (src) {
        // 1) 虚化底：整屏铺满的模糊色块
        int sw = W / 12, sh = H / 12;
        if (sw < 8) sw = 8;
        if (sh < 8) sh = 8;
        Bitmap small(sw, sh, PixelFormat32bppPARGB);
        {
            Graphics gs(&small);
            gs.SetInterpolationMode(InterpolationModeHighQualityBicubic);
            gs.Clear(Color(255, 0, 0, 0));
            gs.DrawImage(src, RectF(0.0f, 0.0f, (float)sw, (float)sh));
        }
        gb.DrawImage(&small, RectF(0.0f, 0.0f, (float)W, (float)H));

        // 2) 清晰图：按高度铺满、水平居中（左右留边由虚化底补）
        float iw = (float)src->GetWidth();
        float ih = (float)src->GetHeight();
        if (iw > 0.0f && ih > 0.0f) {
            float chh = (float)H;
            float cww = chh * (iw / ih);
            if (cww > (float)W) { cww = (float)W; chh = cww * (ih / iw); }
            RectF crisp((float)W * 0.5f - cww * 0.5f, (float)H * 0.5f - chh * 0.5f, cww, chh);
            gb.DrawImage(src, crisp);
        }
    }

    // 压暗 + 上下渐深，方便 UI 压在画面上
    // 第十步调亮：整体压暗从 72 降到 30，上下渐变也相应减弱，
    // 老师立绘的压暗系数（BuildTeacherSprite 里的 DIM）跟着这个常量走
    SolidBrush dim(Color(BG_DIM_A, 0, 0, 0));
    gb.FillRectangle(&dim, 0, 0, W, H);
    {
        LinearGradientBrush top(Point(0, 0), Point(0, (INT)(H * 0.30f)),
                                Color(BG_TOP_A, 0, 0, 0), Color(0, 0, 0, 0));
        gb.FillRectangle(&top, 0, 0, W, (INT)(H * 0.30f));
        LinearGradientBrush bot(Point(0, (INT)(H * 0.64f)), Point(0, H),
                                Color(0, 0, 0, 0), Color(BG_BOT_A, 0, 0, 0));
        gb.FillRectangle(&bot, 0, (INT)(H * 0.64f), W, H - (INT)(H * 0.64f));
    }
}

static void BuildGameBg(int W, int H)
{
    BuildDimBg(g_classBg, &g_gameBg, &g_gameBgW, &g_gameBgH, W, H);
}

static void BuildCanteenBg(int W, int H)
{
    BuildDimBg(g_canteenSrc, &g_canteenBg, &g_canteenBgW, &g_canteenBgH, W, H);
}

static void BuildDormBg(int W, int H)
{
    BuildDimBg(g_dormSrc, &g_dormBg, &g_dormBgW, &g_dormBgH, W, H);
}

static void BuildBookBg(int W, int H)
{
    BuildDimBg(g_bookSrc, &g_bookBg, &g_bookBgW, &g_bookBgH, W, H);
}

static void BuildFarmBg(int W, int H)
{
    // 玉米事件完成后换用「农田结束界面.jpg」
    Image* src = g_cornDone ? (Image*)g_farm2Src : (Image*)g_farmSrc;
    static Image* lastSrc = nullptr;
    static int    cw = 0, ch = 0;
    if (g_farmBg && cw == W && ch == H && lastSrc == src) return;
    if (lastSrc != src) {                    // 换了图源要强制重建
        delete g_farmBg;                     //（BuildDimBg 只按尺寸判断缓存）
        g_farmBg = nullptr;
    }
    BuildDimBg(src, &g_farmBg, &cw, &ch, W, H);
    lastSrc = src;
}

static void BuildPlayBg(int W, int H)
{
    BuildDimBg(g_playSrc, &g_playBg, &g_playBgW, &g_playBgH, W, H);
}

static void BuildCourtBg(int W, int H)
{
    BuildDimBg(g_courtSrc, &g_courtBg, &g_courtBgW, &g_courtBgH, W, H);
}

static void BuildMusicBg(int W, int H)
{
    BuildDimBg(g_musicSrc, &g_musicBg, &g_musicBgW, &g_musicBgH, W, H);
}

// 头像：按 cover 铺满并支持整体透明度（做表情交叉淡化）
static void DrawCoverAlpha(Graphics& g, Image* img, RectF box, float alphaK)
{
    if (!img || alphaK <= 0.0f) return;
    float iw = (float)img->GetWidth();
    float ih = (float)img->GetHeight();
    if (iw < 1.0f || ih < 1.0f) return;

    float s = box.Width / iw;
    float sy = box.Height / ih;
    if (sy > s) s = sy;                       // cover
    float w = iw * s, h = ih * s;
    RectF dst(box.X + (box.Width - w) * 0.5f, box.Y + (box.Height - h) * 0.5f, w, h);

    if (alphaK >= 1.0f) {
        g.DrawImage(img, dst);
    } else {
        ImageAttributes ia;
        ColorMatrix cm = { 1, 0, 0, 0, 0,
                           0, 1, 0, 0, 0,
                           0, 0, 1, 0, 0,
                           0, 0, 0, alphaK, 0,
                           0, 0, 0, 0, 1 };
        ia.SetColorMatrix(&cm);
        g.DrawImage(img, dst, 0.0f, 0.0f, iw, ih, UnitPixel, &ia);
    }
}

// 一条属性条
static void DrawStatBar(Graphics& g, int idx, float t)
{
    float y = BAR_TOP + idx * (BAR_H + BAR_GAP);
    Color col(255, (BYTE)kAttrColor[idx][0], (BYTE)kAttrColor[idx][1], (BYTE)kAttrColor[idx][2]);

    DrawInkString(g, kAttrName[idx], *g_famUI, FontStyleBold, 19.0f,
                  BAR_X, y - 1.0f, Color(Alpha(255, t), 58, 48, 38));

    RectF track(BAR_X + 46.0f, y, BAR_W, BAR_H);
    GraphicsPath tp;
    MakeRoundRect(tp, track, BAR_H * 0.5f);
    {
        SolidBrush tr(Color(Alpha(255, t), 233, 227, 213));
        g.FillPath(&tr, &tp);
    }

    bool openEnded = (idx == 4);                     // 知识不设上限
    float ratio = (float)g_stat[idx] / 100.0f;
    if (ratio > 1.0f) ratio = 1.0f;
    if (ratio > 0.004f) {
        GraphicsPath fp;
        MakeRoundRect(fp, RectF(track.X + 3.0f, track.Y + 3.0f,
                                (track.Width - 6.0f) * ratio, track.Height - 6.0f),
                      (BAR_H - 6.0f) * 0.5f);
        SolidBrush fb(Color(Alpha(255, t), col.GetR(), col.GetG(), col.GetB()));
        g.FillPath(&fb, &fp);
    }

    Pen ink(Color(Alpha(255, t), 62, 52, 42), 2.2f);
    ink.SetLineJoin(LineJoinRound);
    g.DrawPath(&ink, &tp);

    // 知识：右侧画个 “»” 表示没有上限
    if (openEnded) {
        Pen chev(Color(Alpha(190, t), 120, 104, 82), 2.4f);
        chev.SetLineCap(LineCapRound, LineCapRound, DashCapRound);
        float cx = track.X + track.Width + 5.0f, cy = track.Y + track.Height * 0.5f;
        g.DrawLine(&chev, cx, cy - 5.0f, cx + 6.0f, cy);
        g.DrawLine(&chev, cx + 6.0f, cy, cx, cy + 5.0f);
    }

    wchar_t buf[16];
    swprintf(buf, 16, L"%d", g_stat[idx]);
    DrawInkString(g, buf, *g_famUI, FontStyleBold, 20.0f,
                  track.X + track.Width + (openEnded ? 20.0f : 10.0f), y - 1.0f,
                  Color(Alpha(255, t), 44, 38, 30));
}

// 左上角：头像 + 五条属性
static void DrawHudPanel(Graphics& g, float t)
{
    int hero = (g_chosen >= 0) ? g_chosen : 0;

    // 硬阴影 + 米白面板
    {
        GraphicsPath sp;
        MakeRoundRect(sp, RectF(HUD_X + 8.0f, HUD_Y + 9.0f, HUD_W, HUD_H), 16.0f);
        SolidBrush sb(Color(Alpha(90, t), 0, 0, 0));
        g.FillPath(&sb, &sp);
    }
    GraphicsPath panel;
    MakeRoundRect(panel, RectF(HUD_X, HUD_Y, HUD_W, HUD_H), 16.0f);
    {
        SolidBrush fill(Color(Alpha(244, t), 253, 250, 241));
        g.FillPath(&fill, &panel);
    }

    // 头像框
    {
        RectF fr(AV_X, AV_Y, AV_S, AV_S);
        GraphicsPath fp;
        MakeRoundRect(fp, fr, 14.0f);
        SolidBrush bg(Color(Alpha(255, t), (BYTE)kAvatarBG[hero][0],
                            (BYTE)kAvatarBG[hero][1], (BYTE)kAvatarBG[hero][2]));
        g.FillPath(&bg, &fp);

        // 头像铺满框（cover），表情切换时交叉淡化
        {
            GraphicsState st = g.Save();
            g.SetClip(&fp);
            float f = g_faceState.fade;
            if (f < 0.999f && g_faceState.prev >= 0)
                DrawCoverAlpha(g, g_avatar[hero][g_faceState.prev], fr, 1.0f);
            DrawCoverAlpha(g, g_avatar[hero][g_faceState.cur], fr, (f < 0.999f) ? f : 1.0f);
            g.Restore(st);

            if (t < 0.99f) {
                SolidBrush cover(Color(Alpha(255, 1.0f - t), 253, 250, 241));
                g.FillPath(&cover, &fp);
            }
        }
        Pen ink(Color(Alpha(255, t), 58, 48, 38), 2.6f);
        ink.SetLineJoin(LineJoinRound);
        g.DrawPath(&ink, &fp);
    }

    // 名字
    DrawInkString(g, kHeroName[hero], *g_famUI, FontStyleBold, 22.0f,
                  AV_X + 2.0f, AV_Y + AV_S + 8.0f, Color(Alpha(240, t), 52, 44, 34));

    // 五条属性
    for (int i = 0; i < 5; ++i) DrawStatBar(g, i, t);

    Pen inkp(Color(Alpha(255, t), 34, 30, 26), 3.0f);
    inkp.SetLineJoin(LineJoinRound);
    g.DrawPath(&inkp, &panel);
}

// 右上角：天数圆盘 + 左下角的时间
static void DrawDayBadge(Graphics& g, float t)
{
    {
        GraphicsPath sp;
        sp.AddEllipse(DAY_CX - DAY_R + 8.0f, DAY_CY - DAY_R + 9.0f, DAY_R * 2.0f, DAY_R * 2.0f);
        SolidBrush sb(Color(Alpha(90, t), 0, 0, 0));
        g.FillPath(&sb, &sp);
    }

    GraphicsPath cp;
    cp.AddEllipse(DAY_CX - DAY_R, DAY_CY - DAY_R, DAY_R * 2.0f, DAY_R * 2.0f);
    {
        SolidBrush fill(Color(Alpha(248, t), 253, 250, 241));
        g.FillPath(&fill, &cp);
        Pen ink(Color(Alpha(255, t), 34, 30, 26), 3.4f);
        g.DrawPath(&ink, &cp);
    }

    // 天数
    wchar_t buf[16];
    swprintf(buf, 16, L"%d", g_day);
    DrawTextCenter(g, buf, *g_famTitle, FontStyleBold, 58.0f, DAY_CX, DAY_CY - 8.0f,
                   0.0f, Color(Alpha(255, t), 48, 40, 30), 0.0f);
    DrawTextCenter(g, L"day", *g_famUI, FontStyleBold, 19.0f, DAY_CX, DAY_CY + 32.0f,
                   2.0f, Color(Alpha(220, t), 96, 82, 66), 0.0f);

    // 时间：贴在大圆左下角，右对齐
    const wchar_t* tm = kTimeNames[(g_time >= 0 && g_time < TIME_COUNT) ? g_time : 0];
    float em = 22.0f;
    float tw = (float)wcslen(tm) * (em + 2.0f);
    float rx = DAY_CX - DAY_R - 14.0f;
    float cy = DAY_CY + DAY_R - 22.0f;
    RectF tag(rx - tw - 26.0f, cy - 21.0f, tw + 26.0f, 42.0f);
    {
        GraphicsPath tp;
        MakeRoundRect(tp, tag, 12.0f);
        SolidBrush sb(Color(Alpha(80, t), 0, 0, 0));
        GraphicsPath sp;
        MakeRoundRect(sp, RectF(tag.X + 5.0f, tag.Y + 6.0f, tag.Width, tag.Height), 12.0f);
        g.FillPath(&sb, &sp);
        SolidBrush fill(Color(Alpha(245, t), 253, 250, 241));
        g.FillPath(&fill, &tp);
        Pen ink(Color(Alpha(255, t), 34, 30, 26), 2.6f);
        ink.SetLineJoin(LineJoinRound);
        g.DrawPath(&ink, &tp);
    }
    DrawTextCenter(g, tm, *g_famUI, FontStyleBold, em, tag.X + tag.Width * 0.5f,
                   tag.Y + tag.Height * 0.5f, 2.0f, Color(Alpha(255, t), 52, 44, 34), 0.0f);
}

// 第二十三步：时间 UI 右侧的「保存」按钮（画在天数圆盘 / 时间标签下面，不挡任何东西）
static void DrawSaveButton(Graphics& g, float t)
{
    float hot = g_hoverAnim[64];
    RectF r(SAVE_X, SAVE_Y, SAVE_W, SAVE_H);
    float lift = -2.0f * hot;

    {
        GraphicsPath sp;
        MakeRoundRect(sp, RectF(r.X + 5.0f, r.Y + 6.0f - lift, r.Width, r.Height), 10.0f);
        SolidBrush sb(Color(Alpha(90, t), 0, 0, 0));
        g.FillPath(&sb, &sp);
    }
    GraphicsPath p;
    MakeRoundRect(p, RectF(r.X, r.Y - lift, r.Width, r.Height), 10.0f);
    {
        SolidBrush fb(Color(Alpha((BYTE)(244 + 8 * hot), t), 253, 250, 241));
        g.FillPath(&fb, &p);
        Pen ink(Color(Alpha(255, t), 58, 48, 38), 2.6f);
        ink.SetLineJoin(LineJoinRound);
        g.DrawPath(&ink, &p);
    }
    DrawTextCenter(g, L"保存", *g_famUI, FontStyleBold, 21.0f,
                   r.X + r.Width * 0.5f, r.Y + r.Height * 0.5f - lift,
                   1.6f, Color(Alpha(255, t), 52, 44, 34), 0.0f);
}

// 底部九格物品栏，第一格是金钱
// 第十步的图标要先声明（物品栏在下面用到，实现放在第十步那一段）
static void DrawPlanIcon(Graphics& g, RectF r, float t);
// 第十二步的食物图标同理
static void DrawFoodIcon(Graphics& g, RectF r, int kind, float t);
// 第十四步的书图标同理
static void DrawBookIcon(Graphics& g, RectF r, int kind, float t);
static void DrawCornIcon(Graphics& g, RectF r, float t);     // 第二十一步：玉米

static void DrawItemBar(Graphics& g, float t)
{
    float total = SLOT_N * SLOT_S + (SLOT_N - 1) * SLOT_GAP;
    float x0 = 640.0f - total * 0.5f;

    for (int i = 0; i < SLOT_N; ++i) {
        RectF r(x0 + i * (SLOT_S + SLOT_GAP), SLOT_Y, SLOT_S, SLOT_S);

        GraphicsPath sp;
        MakeRoundRect(sp, RectF(r.X + 5.0f, r.Y + 6.0f, r.Width, r.Height), 12.0f);
        SolidBrush sb(Color(Alpha(80, t), 0, 0, 0));
        g.FillPath(&sb, &sp);

        GraphicsPath cp;
        MakeRoundRect(cp, r, 12.0f);
        SolidBrush fill(Color(Alpha(i == 0 ? 246 : 214, t), 253, 250, 241));
        g.FillPath(&fill, &cp);
        Pen ink(Color(Alpha(255, t), 34, 30, 26), 2.4f);
        ink.SetLineJoin(LineJoinRound);
        g.DrawPath(&ink, &cp);

        if (i == 0) {
            // 金币 + 数额
            float coinX = r.X + r.Width * 0.5f, coinY = r.Y + 26.0f;
            GraphicsPath coin;
            coin.AddEllipse(coinX - 15.0f, coinY - 15.0f, 30.0f, 30.0f);
            SolidBrush gold(Color(Alpha(255, t), 246, 199, 88));
            g.FillPath(&gold, &coin);
            Pen cpen(Color(Alpha(255, t), 120, 92, 40), 2.2f);
            g.DrawPath(&cpen, &coin);
            DrawTextCenter(g, L"¥", *g_famUI, FontStyleBold, 17.0f, coinX, coinY, 0.0f,
                           Color(Alpha(255, t), 120, 88, 36), 0.0f);

            wchar_t buf[16];
            swprintf(buf, 16, L"%d", g_money);
            DrawTextCenter(g, buf, *g_famUI, FontStyleBold, 21.0f, r.X + r.Width * 0.5f,
                           r.Y + 56.0f, 1.0f, Color(Alpha(255, t), 52, 44, 34), 0.0f);
        } else if (g_inv[i] != ITEM_EMPTY) {
            // 第十步：事务安排（蓝色日记本）；第十二步：饼干 / 汉堡；第十四步：书
            float hot = (g_hover == 40 + i) ? g_hoverAnim[40 + i] : 0.0f;
            RectF ir(r.X + 3.0f - 2.0f * hot, r.Y + 3.0f - 2.0f * hot,
                     r.Width - 6.0f + 4.0f * hot, r.Height - 6.0f + 4.0f * hot);
            if (g_inv[i] == ITEM_PLAN)         DrawPlanIcon(g, ir, t);
            else if (g_inv[i] == ITEM_BISCUIT) DrawFoodIcon(g, ir, 1, t);
            else if (g_inv[i] == ITEM_BURGER)  DrawFoodIcon(g, ir, 2, t);
            else if (g_inv[i] == ITEM_CRACKER) DrawFoodIcon(g, ir, 3, t);
            else if (g_inv[i] == ITEM_COFFEE)  DrawFoodIcon(g, ir, 4, t);
            else if (g_inv[i] >= ITEM_BOOK_PHYS && g_inv[i] <= ITEM_BOOK_GUIDE)
                DrawBookIcon(g, ir, g_inv[i] - ITEM_BOOK_PHYS, t);
            else if (g_inv[i] == ITEM_CORN)  DrawCornIcon(g, ir, t);
        }
    }
}

// 左下角返回菜单（开发期方便，不影响 UI 主体）
static void DrawBackBtn(Graphics& g, float t)
{
    RectF r(GBACK_X, GBACK_Y, GBACK_W, GBACK_H);
    float sc = 1.0f + 0.04f * g_backAnim;
    RectF rr(r.X, r.Y - 2.0f * g_backAnim, r.Width * sc, r.Height * sc);

    GraphicsPath sp;
    MakeRoundRect(sp, RectF(rr.X + 5.0f, rr.Y + 6.0f, rr.Width, rr.Height), 12.0f);
    SolidBrush sb(Color(Alpha(85, t), 0, 0, 0));
    g.FillPath(&sb, &sp);

    GraphicsPath cp;
    MakeRoundRect(cp, rr, 12.0f);
    SolidBrush fill(Color(Alpha(232, t), 253, 250, 241));
    g.FillPath(&fill, &cp);
    Pen ink(Color(Alpha(255, t), 34, 30, 26), 2.4f);
    ink.SetLineJoin(LineJoinRound);
    g.DrawPath(&ink, &cp);

    DrawTextCenter(g, L"← 菜单", *g_famUI, FontStyleBold, 20.0f, rr.X + rr.Width * 0.5f,
                   rr.Y + rr.Height * 0.5f, 1.0f, Color(Alpha(240, t), 52, 44, 34), 0.0f);
}

// ================= 第九步：上课场景 =================
//  开发说明：
//    · 「略微缩左上角的 AI」= 左上角角色头像略微缩小（已改 AV_S 118 -> 100）
//    · 分别设计物理 / 语文 / 数学 / 英语四节课的上课界面
//    · 界面背景统一用「教学楼界面.jpg」
//    · 老师站在讲台的左侧，可略微遮挡身体
//    · 黑板上写一些对应学科的高中知识
//    · 所有上课界面都属于教学楼界面，当前上哪节课由课表决定
//    · day1 的清晨为物理课界面

// 四门课（下标即老师/板书数组下标）
enum Subject { SUBJ_NONE = -1, SUBJ_PHYS = 0, SUBJ_CHINESE, SUBJ_MATH, SUBJ_ENGLISH,
               SUBJ_COUNT, SUBJ_PE = SUBJ_COUNT };   // 体育课：不换老师、不进教室

static const wchar_t* kSubjectName[SUBJ_COUNT] = { L"物理", L"语文", L"数学", L"英语" };
static const wchar_t* kTeacherName[SUBJ_COUNT] = {
    L"日日老师", L"玉皇大帝老师", L"吉利老师", L"厄文老师"
};
static const wchar_t* kTeacherFile[SUBJ_COUNT] = {
    L"日日老师_立绘.png", L"玉皇大帝老师_立绘.png",
    L"吉利老师_立绘.png", L"厄文老师_立绘.png"
};

// ---- 课表（第十步：按开发说明里「事务安排」的原文排） ----
// 只排到「下午4节」为止（开发说明的课表也只写到下午），晚上时段暂时没有安排。
// 数值 = 上哪门课（SUBJ_NONE 表示该时段没有已开发的上课界面：
//        班会 / 自由活动 / 自修 / 体育 / 音乐 / 考试 / 讲座 等都属于这种情况，
//        这些只写进「事务安排」里，不换老师）
static const int PLAN_SLOT_N = 14;      // 清晨 ~ 下午4节

static const int kWeekSubj[7][14] = {
    // Day1  清晨班会（日日老师主持）→ 上午2节物理 3节语文 4节数学 5节英语，
    //        下午二节（time 11）是体育课（事务安排原文）
    { SUBJ_PHYS, SUBJ_NONE, SUBJ_PHYS, SUBJ_CHINESE, SUBJ_MATH, SUBJ_ENGLISH,
      SUBJ_NONE, SUBJ_NONE, SUBJ_NONE, SUBJ_NONE,
      SUBJ_NONE, SUBJ_PE, SUBJ_NONE, SUBJ_NONE },
    // Day2  上午1~4节 物理/语文/数学/英语，5节课外实践，下午全是名师讲座
    { SUBJ_NONE, SUBJ_PHYS, SUBJ_CHINESE, SUBJ_MATH, SUBJ_ENGLISH, SUBJ_NONE,
      SUBJ_NONE, SUBJ_NONE, SUBJ_NONE, SUBJ_NONE,
      SUBJ_NONE, SUBJ_NONE, SUBJ_NONE, SUBJ_NONE },
    // Day3  上午1~4节 物理/语文/数学/英语，5节自修，下午四节考试
    { SUBJ_NONE, SUBJ_PHYS, SUBJ_CHINESE, SUBJ_MATH, SUBJ_ENGLISH, SUBJ_NONE,
      SUBJ_NONE, SUBJ_NONE, SUBJ_NONE, SUBJ_NONE,
      SUBJ_NONE, SUBJ_NONE, SUBJ_NONE, SUBJ_NONE },
    // Day4~Day7：待后续开发
    { SUBJ_NONE, SUBJ_NONE, SUBJ_NONE, SUBJ_NONE, SUBJ_NONE, SUBJ_NONE,
      SUBJ_NONE, SUBJ_NONE, SUBJ_NONE, SUBJ_NONE,
      SUBJ_NONE, SUBJ_NONE, SUBJ_NONE, SUBJ_NONE },
    { SUBJ_NONE, SUBJ_NONE, SUBJ_NONE, SUBJ_NONE, SUBJ_NONE, SUBJ_NONE,
      SUBJ_NONE, SUBJ_NONE, SUBJ_NONE, SUBJ_NONE,
      SUBJ_NONE, SUBJ_NONE, SUBJ_NONE, SUBJ_NONE },
    { SUBJ_NONE, SUBJ_NONE, SUBJ_NONE, SUBJ_NONE, SUBJ_NONE, SUBJ_NONE,
      SUBJ_NONE, SUBJ_NONE, SUBJ_NONE, SUBJ_NONE,
      SUBJ_NONE, SUBJ_NONE, SUBJ_NONE, SUBJ_NONE },
    { SUBJ_NONE, SUBJ_NONE, SUBJ_NONE, SUBJ_NONE, SUBJ_NONE, SUBJ_NONE,
      SUBJ_NONE, SUBJ_NONE, SUBJ_NONE, SUBJ_NONE,
      SUBJ_NONE, SUBJ_NONE, SUBJ_NONE, SUBJ_NONE }
};

// 「事务安排」每一页写什么（开发说明给的原文，一行一个时段）
static const wchar_t* kPlanDay1[] = {
    L"清晨　　　　班会",
    L"上午一节　　自由活动",
    L"上午二节　　物理课",
    L"上午三节　　语文课",
    L"上午四节　　数学课",
    L"上午五节　　英语课",
    L"下午一节　　自修课",
    L"下午二节　　体育课",
    L"下午三节　　音乐课",
    L"下午四节　　自修"
};
static const wchar_t* kPlanDay2[] = {
    L"上午1节　　物理课",
    L"上午二节　　语文课",
    L"上午三节　　数学课",
    L"上午四节　　英语课",
    L"上午五节　　课外实践活动",
    L"下午一节至四节　　北京名师讲高考"
};
static const wchar_t* kPlanDay3[] = {
    L"上午1节　　物理课",
    L"上午二节　　语文课",
    L"上午三节　　数学课",
    L"上午四节　　英语课",
    L"上午5节　　自修",
    L"下午一节　　物理考试",
    L"下午二节　　语文考试",
    L"下午三节　　英语考试",
    L"下午四节　　数学考试"
};

struct PlanPage { const wchar_t** lines; int n; };
static const PlanPage kPlanPages[7] = {
    { kPlanDay1, 10 }, { kPlanDay2, 6 }, { kPlanDay3, 9 },
    { nullptr, 0 }, { nullptr, 0 }, { nullptr, 0 }, { nullptr, 0 }
};

// 当前时段上哪节课：返回 SUBJ_NONE 表示没有已开发的上课界面
static int SubjectAt(int day, int time)
{
    int d = day - 1;
    if (d < 0) d = 0;
    if (d > 6) d = 6;
    if (time < 0 || time >= PLAN_SLOT_N) return SUBJ_NONE;
    return kWeekSubj[d][time];
}

// 「教学楼界面.jpg」在 1280x720 逻辑画布里的摆放（与 BuildGameBg 完全一致：
// 整幅按高度铺满、水平居中），用来把原图像素换算成逻辑坐标
static const float CLS_IMG_W = 1850.0f, CLS_IMG_H = 1279.0f;
static const float CLS_SCALE = 720.0f / CLS_IMG_H;                        // ≈0.56294
static float ClsX(float px) { return (BASE_W - CLS_IMG_W * CLS_SCALE) * 0.5f + px * CLS_SCALE; }
static float ClsY(float py) { return py * CLS_SCALE; }

// 黑板的三个区域（原图像素，已用逐行扫描量出来）
static const int BOARD_L_X1 = 310, BOARD_L_Y1 = 536;      // 左板：写板书
static const int BOARD_L_X2 = 644, BOARD_L_Y2 = 890;
static const int BOARD_R_X1 = 1171, BOARD_R_Y1 = 558;     // 右板：写这是哪门课
static const int BOARD_R_X2 = 1430, BOARD_R_Y2 = 880;

// 老师的站位（逻辑坐标）：身体中心 X、脚底 Y、身高
static const float TEACHER_CX     = 556.0f;
static const float TEACHER_FEET_Y = 618.0f;      // 比初版调低 18px，站得更"落地"
static const float TEACHER_H      = 302.0f;

// 「略微遮挡身体」用的前景（最近处那摞书 + 青色书包，原图像素）
// 把这小块背景重画到老师身上，老师看起来就是站在课桌后面
static const int FRONT_X1 = 570, FRONT_Y1 = 952;
static const int FRONT_X2 = 1015, FRONT_Y2 = 1235;

// 抠好背景的老师立绘：img + 人物紧致包围盒
struct Sprite {
    Bitmap* img;
    int x0, y0, x1, y1;
};
static Sprite g_teacher[SUBJ_COUNT] = { { nullptr, 0, 0, 0, 0 }, { nullptr, 0, 0, 0, 0 },
                                        { nullptr, 0, 0, 0, 0 }, { nullptr, 0, 0, 0, 0 } };

// 各学科的板书内容（写在左黑板上，最多 5 行）
// 第一行是这一课的题目，后面几行是课上写的要点
static const wchar_t* kChalkNote[SUBJ_COUNT][5] = {
    { L"牛顿第二定律", L"F = m a", L"a 与 F 成正比", L"单位：牛顿 N", L"" },
    { L"《纪念刘和珍君》", L"不在沉默中爆发", L"就在沉默中灭亡", L"真的猛士", L"—— 鲁迅" },
    { L"《二次曲线》", L"椭圆 / 双曲线", L"抛物线", L"y^2 = 2px", L"e = c / a" },
    { L"《英语应用文教学》", L"书信 / 通知", L"Dear Sir,", L"Yours, Li Hua", L"称呼、正文、落款" }
};

// 把已压暗的背景按逻辑坐标取一小块重画上去（内容与屏幕完全一致，看不出接缝，
// 唯一的效果是把这一块里的老师"擦掉"，形成前景遮挡）
static void DrawBgPatch(Graphics& g, float x, float y, float w, float h)
{
    if (!g_gameBg || g_gameBgW <= 0 || g_gameBgH <= 0) return;
    float kx = (float)g_gameBgW / (float)BASE_W;
    float ky = (float)g_gameBgH / (float)BASE_H;
    g.DrawImage(g_gameBg, RectF(x, y, w, h),
                x * kx, y * ky, w * kx, h * ky, UnitPixel);
}

// 画老师（站在讲台左侧）
static void DrawTeacher(Graphics& g, int idx, float t)
{
    const Sprite& s = g_teacher[idx];
    if (!s.img) return;
    float fw = (float)(s.x1 - s.x0);
    float fh = (float)(s.y1 - s.y0);
    if (fw < 2.0f || fh < 2.0f) return;

    float k  = TEACHER_H / fh;
    float dw = fw * k;
    float bx = TEACHER_CX - dw * 0.5f;
    float by = TEACHER_FEET_Y - TEACHER_H - (1.0f - t) * 14.0f;   // 入场时轻轻落定

    GraphicsState st = g.Save();
    g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    g.SetPixelOffsetMode(PixelOffsetModeHighQuality);
    g.DrawImage(s.img, RectF(bx, by, dw, TEACHER_H),
                (REAL)s.x0, (REAL)s.y0, (REAL)fw, (REAL)fh, UnitPixel);
    g.Restore(st);
}

// 一行粉笔字的宽度（与 DrawChalkLine 的排字规则一致，用来做居中）
static float ChalkWidth(const wchar_t* text, float em, float spacing)
{
    float w = 0.0f;
    for (const wchar_t* p = text; p && *p; ++p)
        w += ((unsigned)*p >= 0x2E80u) ? (em + spacing) : (em * 0.56f);
    return w;
}

// 粉笔字：手写体 + 视觉倾斜（整体右倾约 7°）+ 行级轻微歪斜 + 粉末重影
// 整行一起做剪切，中英文的字距才是字体自己的自然间距
// 黑板在画面里接近正对镜头，所以这个斜度就是"老师站在黑板前写字"的观感
static void DrawChalkLine(Graphics& g, const wchar_t* text, float em,
                          float x, float y, float a, int seed, float spacing = 1.0f)
{
    if (!text || !*text || a <= 0.01f) return;

    Font font(g_famChalk, em, FontStyleRegular, UnitPixel);
    StringFormat fmt;
    fmt.SetAlignment(StringAlignmentNear);
    fmt.SetFormatFlags(StringFormatFlagsNoWrap | StringFormatFlagsNoClip);
    SolidBrush br (Color(Alpha(236, a), 238, 240, 233));
    SolidBrush br2(Color(Alpha(56,  a), 238, 240, 233));

    float wid = ChalkWidth(text, em, spacing);
    float jy  = 1.0f * Noise1(seed * 13 + 5);

    GraphicsState st = g.Save();
    // 绕整行中心剪切，行不会整体跑偏
    g.TranslateTransform(x + wid * 0.5f, y + em * 0.5f + jy);
    {
        Matrix sh;
        sh.Shear(-0.12f, 0.0f);
        g.MultiplyTransform(&sh);
    }
    g.RotateTransform(0.7f * Noise1(seed + 3));          // 整行微微歪一点
    g.TranslateTransform(-wid * 0.5f, -em * 0.5f);

    g.DrawString(text, -1, &font, PointF(0.0f, 0.0f), &fmt, &br);
    g.DrawString(text, -1, &font, PointF(1.0f, 0.9f), &fmt, &br2);   // 粉末重影
    g.Restore(st);
}

// 黑板上的大字（课名）：同样倾斜，绕自身中心剪切
static void DrawChalkBig(Graphics& g, const wchar_t* text, float em,
                         float cx, float cy, float a, int seed)
{
    if (!text || !*text || a <= 0.01f) return;
    GraphicsState st = g.Save();
    g.TranslateTransform(cx, cy);
    {
        Matrix sh;
        sh.Shear(-0.13f, 0.0f);
        g.MultiplyTransform(&sh);
    }
    g.RotateTransform(0.8f * Noise1(seed));
    g.TranslateTransform(-cx, -cy);
    DrawTextCenter(g, text, *g_famChalk, FontStyleRegular, em, cx, cy, 3.0f,
                   Color(Alpha(232, a), 238, 240, 233), 1.4f);
    g.Restore(st);
}

// 黑板板书
static void DrawChalkNotes(Graphics& g, int idx, float t)
{
    // 左板：本科知识，一行一行"写"出来
    const float avail = ClsX((float)BOARD_L_X2) - ClsX((float)BOARD_L_X1) - 20.0f;
    for (int i = 0; i < 5; ++i) {
        const wchar_t* line = kChalkNote[idx][i];
        if (!line || !line[0]) continue;
        float a = EaseOut(Clamp01((t - 0.08f - 0.09f * (float)i) / 0.45f));
        // 课题收紧字距、字号更大，正文松一点；放不下时按比例缩
        float sp = (i == 0) ? 0.0f : 1.0f;
        float em = (i == 0) ? 22.5f : 19.0f;
        float lw = ChalkWidth(line, em, sp);
        if (lw > avail) em *= avail / lw;         // 太长就自动缩小，别写出黑板
        DrawChalkLine(g, line, em, ClsX((float)(BOARD_L_X1 + 8)) + 4.0f,
                      ClsY((float)BOARD_L_Y1) + 24.0f + (float)i * 33.5f,
                      a, idx * 41 + i * 13, sp);
    }

    // 右板：这是哪一门课（大字 + 随手画的下划线 + 老师名字）
    {
        float a = EaseOut(Clamp01((t - 0.10f) / 0.50f));
        if (a > 0.01f) {
            float cx = (ClsX((float)BOARD_R_X1) + ClsX((float)BOARD_R_X2)) * 0.5f;
            float cy = (ClsY((float)BOARD_R_Y1) + ClsY((float)BOARD_R_Y2)) * 0.5f - 42.0f;
            DrawChalkBig(g, kSubjectName[idx], 46.0f, cx, cy, a, idx * 23 + 5);
            DrawWobbleLine(g, cx, cy + 38.0f, 44.0f, 3.0f, 1.3f, 3.0f,
                           Color(Alpha(170, a), 238, 240, 233));
            {
                float nw = ChalkWidth(kTeacherName[idx], 18.0f, 1.0f);
                DrawChalkLine(g, kTeacherName[idx], 18.0f, cx - nw * 0.5f,
                              cy + 50.0f, a * 0.86f, idx * 17 + 3);
            }
        }
    }
}

// 上课场景整层：老师 + 前景遮挡 + 板书
static void RenderClassLayer(Graphics& g, float t)
{
    int subj = SubjectAt(g_day, g_time);
    if (subj < 0 || subj >= SUBJ_COUNT) return;   // 没课（或体育课）就只显示教室

    DrawTeacher(g, subj, t);

    // 老师站在课桌后面：把最近处那摞书的背景盖回下半身，做到「略微遮挡身体」
    DrawBgPatch(g, ClsX((float)FRONT_X1), ClsY((float)FRONT_Y1),
                ClsX((float)FRONT_X2) - ClsX((float)FRONT_X1),
                ClsY((float)FRONT_Y2) - ClsY((float)FRONT_Y1));

    DrawChalkNotes(g, subj, t);
}

// ================= 第十一步：状态随时间变化 =================
//  开发说明：每过一个时间段，饱食 -5、精神 -3、快乐 -1，
//  这段变化独立于其它活动（上课选项自己带来的增减另算）。
// 第十八步：属性变化提示用（声明要放在 AddStat 之前）
static int  g_deltaStat[5] = { 0 };      // 本次动作实际造成的属性变化
static bool g_deltaRecord  = true;       // false 时不记录（时段自然衰减）

static void AddStat(int idx, int d)
{
    if (idx < 0 || idx > 4) return;
    int before = g_stat[idx];
    g_stat[idx] += d;
    if (g_stat[idx] < 0) g_stat[idx] = 0;
    if (idx < 4 && g_stat[idx] > 100) g_stat[idx] = 100;   // 知识不设上限
    // 第十八步：把「实际变化量」攒起来，之后在屏幕正中显示（时段自然衰减不记录）
    if (g_deltaRecord) g_deltaStat[idx] += g_stat[idx] - before;
}

static void AdvanceTime()
{
    g_time++;
    if (g_time >= TIME_COUNT) {          // 一天结束就进入下一天
        g_time = 0;
        g_day++;
        if (g_day > 7) g_day = 1;
    }
    // 时段导致的自然衰减不弹「属性变化」提示
    bool old = g_deltaRecord;
    g_deltaRecord = false;
    AddStat(1, -5);                      // 饱食
    AddStat(2, -3);                      // 精神
    AddStat(3, -1);                      // 快乐
    g_deltaRecord = old;
    g_faceState.Refresh(g_stat[2], g_stat[3]);
}

// ================= 第十八步：属性变化提示 =================
//  开发说明：
//    · 「下一时间段」按钮改成「自修一会」，保留原本的属性变化，额外 +5 知识
//    · 除了时段导致的自然衰减，每次属性变化都要在屏幕中心显示变化的数值
//    · 下方有「确定」按钮，点了才继续切界面 / 推时段
static bool g_deltaActive  = false;      // 提示面板开着
static int  g_deltaNext    = 0;          // 点「确定」之后要做什么（DN_*）

enum {
    DN_NONE = 0,
    DN_MAP,              // 回地图
    DN_CLASS_TALK,       // 接着播老师的课后台词
    DN_FARM_TALK,        // 农田「在农田转了转」旁白
    DN_DORM_OPTIONS,     // 宿舍刷新选项（小睡之后）
    DN_DORM_MORNING,     // 宿舍睡到第二天清晨
    DN_RUN,              // 操场跑步演出
    DN_CANTEEN_EAT,      // 食堂当场吃完 → 推时段 + 检查是否还开着
    DN_BASKET_END,       // 投篮结算 → 推时段 + 回地图
    DN_MUSIC_END,        // 弹琴结算 → 推时段 + 回地图
    DN_SELF_STUDY,       // 自修一会 → 学习动画后回地图
    DN_EVENT1_END,       // 课外实践结束 → 直接跳到中午午时并回地图
    DN_SHOW_DELTA,       // 第二十一步：动作动画播完 → 这时才弹「属性变化」
    DN_EXAM_NEXT,        // 第二十一步：一门考完 → 下一门 / 数学考完播玉米广播
    DN_EXAM_END,         // 第二十一步：考试（或摘玉米）收尾 → 下午五节并回地图
    DN_GAOKAO_END        // 第二十三步：讲高考听完 → 晚饭前并回地图
};

static void RunDeltaNext();              // 定义在后面
static void StartAnim(int kind, int after);   // 第二十步，定义在后面
static int  AnimForNext(int next);            // 第二十一步，定义在 RunDeltaNext 旁边

// 第二十一步：动画播完（或本来就没有动画）之后 → 有变化就弹面板，没变化就继续
static void ShowDeltaOrRun()
{
    bool any = false;
    for (int i = 0; i < 5; ++i) if (g_deltaStat[i] != 0) any = true;
    if (!any) { RunDeltaNext(); return; }
    g_deltaActive = true;
}

// 收尾：先播对应的动作动画，播完再显示属性变化（第二十一步调整了顺序）
static void CommitDeltas(int next)
{
    g_deltaNext = next;
    int anim = AnimForNext(next);
    if (anim != ANIM_NONE) { StartAnim(anim, DN_SHOW_DELTA); return; }
    ShowDeltaOrRun();
}

// 第二十三步（新增5）：属性链进行中 = 已经 CommitDeltas 了，还在等「动作动画播完 →
//   属性变化面板 → 点确定」这一步。这时候 WM_TIMER 里那几个强制跳转（课外实践 / 讲高考 /
//   考试 / 体育课 / 晚寝）不许插队：属性链最后一步多半是「回地图」，插队进来的事件
//   会被那一下直接顶掉，表现就是「进了事件 → 弹属性变化 → 点确定 → 回到地图，事件没了」。
static bool DeltaBusy()
{
    return g_deltaNext != DN_NONE || g_deltaActive;
}

static void ClearDeltas()
{
    for (int i = 0; i < 5; ++i) g_deltaStat[i] = 0;
    g_deltaActive = false;
}

// 画「属性变化」面板（屏幕正中，下方一个确定按钮）
static void DrawDeltaPanel(Graphics& g)
{
    if (!g_deltaActive) return;

    int n = 0;
    for (int i = 0; i < 5; ++i) if (g_deltaStat[i] != 0) ++n;
    if (n <= 0) return;

    float w = 520.0f;
    float h = 96.0f + (float)n * 46.0f + 86.0f;
    float cx = (float)BASE_W * 0.5f;
    RectF r(cx - w * 0.5f, (float)BASE_H * 0.5f - h * 0.5f, w, h);

    // 阴影 + 纸面
    {
        GraphicsPath sp;
        MakeRoundRect(sp, RectF(r.X + 8.0f, r.Y + 10.0f, w, h), 18.0f);
        SolidBrush sb(Color(120, 0, 0, 0));
        g.FillPath(&sb, &sp);
    }
    GraphicsPath pp;
    MakeRoundRect(pp, r, 18.0f);
    {
        SolidBrush fill(Color(250, 253, 250, 241));
        g.FillPath(&fill, &pp);
        Pen ink(Color(255, 34, 30, 26), 3.0f);
        ink.SetLineJoin(LineJoinRound);
        g.DrawPath(&ink, &pp);
    }

    DrawTextCenter(g, L"属性变化", *g_famTitle, FontStyleBold, 30.0f,
                   cx, r.Y + 36.0f, 2.5f, Color(255, 48, 42, 34), 0.8f);

    // 一行一条
    float y = r.Y + 86.0f;
    for (int i = 0; i < 5; ++i) {
        if (g_deltaStat[i] == 0) continue;
        Color lab(kAttrColor[i][0], kAttrColor[i][1], kAttrColor[i][2]);
        DrawTextCenter(g, kAttrName[i], *g_famUI, FontStyleBold, 24.0f,
                       r.X + w * 0.30f, y, 1.0f, lab, 0.0f);

        wchar_t buf[24];
        swprintf(buf, 24, (g_deltaStat[i] > 0) ? L"+%d" : L"%d", g_deltaStat[i]);
        Color num = (g_deltaStat[i] > 0) ? Color(255, 62, 132, 88)      // 加：绿
                                         : Color(255, 190, 76, 64);     // 减：红
        DrawTextCenter(g, buf, *g_famUI, FontStyleBold, 26.0f,
                       r.X + w * 0.66f, y, 1.0f, num, 0.0f);
        y += 46.0f;
    }

    // 确定按钮
    RectF btn(cx - 100.0f, r.Y + h - 78.0f, 200.0f, 56.0f);
    {
        float hot = g_hoverAnim[62];
        GraphicsPath bp;
        MakeRoundRect(bp, btn, 12.0f);
        SolidBrush f(Color(252, (BYTE)(248 - 12 * hot), (BYTE)(243 - 14 * hot),
                           (BYTE)(228 - 12 * hot)));
        g.FillPath(&f, &bp);
        Pen pen(Color(255, 34, 30, 26), 2.8f + 0.8f * hot);
        pen.SetLineJoin(LineJoinRound);
        g.DrawPath(&pen, &bp);
        DrawTextCenter(g, L"确定", *g_famUI, FontStyleBold, 24.0f,
                       btn.X + btn.Width * 0.5f, btn.Y + btn.Height * 0.5f, 1.0f,
                       Color(255, 44, 38, 32), 0.0f);
    }
}
//  每次知识增加时都检测物品栏里的教辅：
//    · 对应科目的书（物理白皮/语文真题卷/数学自编资料/45套）→ 该科目课堂知识 +50%
//    · 课外教辅 → 非课堂上获得的知识 +30%
//  增加量为小数时向上取整
static float KnowledgeMult(int subj)
{
    if (subj >= 0 && subj < SUBJ_COUNT) {
        for (int i = 1; i < SLOT_N; ++i)
            if (g_inv[i] == ITEM_BOOK_PHYS + subj) return 1.5f;
    } else {
        for (int i = 1; i < SLOT_N; ++i)
            if (g_inv[i] == ITEM_BOOK_GUIDE) return 1.3f;
    }
    return 1.0f;
}

// subj >= 0 表示课堂（按科目找书）；subj < 0 表示非课堂来源
static void AddKnowledge(int base, int subj)
{
    float v = (float)base * KnowledgeMult(subj);
    int add = (int)ceilf(v);                   // 小数向上取整
    if (add < 0) add = 0;
    AddStat(4, add);
}

// ================= 第十四步：知识加成（书屋的书） =================
//  每次知识增加时都检测物品栏里的教辅：
//    · 对应科目的书 → 该科目课堂知识 +50%
//    · 课外教辅 → 非课堂上获得的知识 +30%
//  增加量为小数时向上取整
// 书屋商品
struct BookItem {
    const wchar_t* name;
    const wchar_t* effect;
    int price;
    int kind;            // 图标类型
    int itemId;          // 进背包的物品类型
    int subj;            // 对应科目（-1 = 课外教辅）
};

static const BookItem kBooks[5] = {
    { L"物理白皮",     L"物理课时提高 50% 获取的知识", 20, 0, ITEM_BOOK_PHYS,    SUBJ_PHYS },
    { L"语文真题卷",   L"语文课时提高 50% 获取的知识", 20, 1, ITEM_BOOK_CHINESE, SUBJ_CHINESE },
    { L"数学自编资料", L"数学课时提高 50% 获取的知识", 20, 2, ITEM_BOOK_MATH,    SUBJ_MATH },
    { L"45套",         L"英语课时提高 50% 获取的知识", 20, 3, ITEM_BOOK_ENGLISH, SUBJ_ENGLISH },
    { L"课外教辅",     L"非课堂上获取的知识提高 30%",   20, 4, ITEM_BOOK_GUIDE,   -1 }
};

static const int BOOK_N = 5;

// 书图标：kind 0 物理白皮 / 1 语文真题卷 / 2 数学自编资料 / 3 45套 / 4 课外教辅
static void DrawBookIcon(Graphics& g, RectF r, int kind, float t)
{
    float cx = r.X + r.Width * 0.5f, cy = r.Y + r.Height * 0.5f;
    float s  = (r.Width < r.Height ? r.Width : r.Height) * 0.5f;
    Pen ink(Color(Alpha(255, t), 62, 50, 40), 2.0f);
    ink.SetLineJoin(LineJoinRound);

    Color paper(Alpha(255, t), 248, 246, 240);
    Color cover(Alpha(255, t), 244, 240, 230);
    Color band (Alpha(255, t), 150, 40, 40);
    const wchar_t* title = L"";
    float tw = 0.0f, th = 0.0f;

    if (kind == 0) {          // 物理白皮：白色硬皮本
        paper = Color(Alpha(255, t), 252, 251, 246);
        band  = Color(Alpha(255, t), 120, 140, 190);
        title = L"物理";
    } else if (kind == 1) {   // 语文真题卷：浅黄色试卷
        paper = Color(Alpha(255, t), 250, 238, 186);
        band  = Color(Alpha(255, t), 196, 150, 60);
        title = L"语文";
    } else if (kind == 2) {   // 数学自编资料：浅蓝色书
        paper = Color(Alpha(255, t), 198, 224, 244);
        band  = Color(Alpha(255, t), 70, 110, 170);
        title = L"数学";
    } else if (kind == 3) {   // 45套：深蓝色试卷集
        paper = Color(Alpha(255, t), 66, 96, 158);
        band  = Color(Alpha(255, t), 40, 60, 110);
        title = L"45";
    } else {                  // 课外教辅：棕色书
        paper = Color(Alpha(255, t), 176, 126, 78);
        band  = Color(Alpha(255, t), 110, 74, 44);
        title = L"教辅";
    }

    if (kind == 3) {
        // 一摞深蓝试卷
        for (int i = 2; i >= 0; --i) {
            float oy = cy - s * 0.30f + (float)i * s * 0.22f;
            GraphicsPath sh;
            MakeRoundRect(sh, RectF(cx - s * 0.78f, oy - s * 0.46f, s * 1.56f, s * 0.46f), 3.0f);
            SolidBrush f(Color(Alpha(255, t), (BYTE)(66 + i * 16), (BYTE)(96 + i * 14), (BYTE)(158 + i * 12)));
            g.FillPath(&f, &sh);
            g.DrawPath(&ink, &sh);
        }
    } else {
        // 一本书：封皮 + 书脊
        GraphicsPath bk;
        MakeRoundRect(bk, RectF(cx - s * 0.70f, cy - s * 0.86f, s * 1.40f, s * 1.72f), 4.0f);
        SolidBrush f(paper);
        g.FillPath(&f, &bk);
        g.DrawPath(&ink, &bk);
        // 书脊
        SolidBrush sp(band);
        g.FillRectangle(&sp, cx - s * 0.70f, cy - s * 0.86f, s * 0.20f, s * 1.72f);
        Pen spi(Color(Alpha(255, t), 62, 50, 40), 1.6f);
        g.DrawRectangle(&spi, cx - s * 0.70f, cy - s * 0.86f, s * 0.20f, s * 1.72f);
        // 标题
        float em = (kind == 3) ? 20.0f : 18.0f;
        if (kind == 3) {
            DrawTextCenter(g, title, *g_famUI, FontStyleBold, em,
                           cx + s * 0.06f, cy - s * 0.06f, 0.0f,
                           Color(Alpha(255, t), 246, 246, 240), 0.0f);
        } else {
            // 竖排两个字
            wchar_t c1[2] = { title[0], 0 };
            wchar_t c2[2] = { title[1], 0 };
            DrawTextCenter(g, c1, *g_famUI, FontStyleBold, em,
                           cx + s * 0.14f, cy - s * 0.28f, 0.0f,
                           Color(Alpha(255, t), 56, 48, 40), 0.0f);
            DrawTextCenter(g, c2, *g_famUI, FontStyleBold, em,
                           cx + s * 0.14f, cy + s * 0.28f, 0.0f,
                           Color(Alpha(255, t), 56, 48, 40), 0.0f);
        }
    }
    (void)tw; (void)th;
}

// ================= 第十步：对话框 / 剧情 / 事务安排 =================
//  开发说明：
//    · 对话框在屏幕下方，左上角显示说话人名字，右下角有「>继续」
//    · 文字逐个浮现；遇到选项时选项以长方形上下排列、对齐对话框，此时没有「>继续」
//    · day1 清晨剧情三句，说完第三句后物品栏加入「事务安排」（蓝色日记本）
//    · 左键点「事务安排」可查看：七页白纸横线，右上角红叉关闭（不改时间），
//      左下角「<-」往前翻、右下角「->」往后翻
//    · 课表按「事务安排」原文，到了对应时段在教学楼界面换上对应老师

// ---- 对话框 ----
struct DlgLine {
    const wchar_t* who;
    const wchar_t* text;
    int            giveItem;      // 说完这一句之后往物品栏塞的东西（0=不塞）
};

static const DlgLine kStoryDay1[] = {
    { L"日日老师", L"欢迎同学们开始这个高中的学习哎", ITEM_NONE },
    { L"日日老师", L"同学们要安静，要专心致志地干哎", ITEM_NONE },
    { L"日日老师", L"接下来，我发给大家这一周地事情安排", ITEM_PLAN }
};
static const int STORY_DAY1_N = 3;

// ---- 第十一步：说完这批台词之后做什么 ----
enum { DLG_AFTER_NONE = 0, DLG_AFTER_NEXT_TIME = 1,
       DLG_AFTER_ASK = 2,           // 第十五/十六步：台词播完 → 弹选项（内容取自 g_askPrompt + g_dlgOpt[]）
       DLG_AFTER_FARM_LEAVE = 3,    // 第十五步：旁白播完 → 回地图并跳过一个时间段
       DLG_AFTER_EVENT_PICK = 4,    // 第二十步：事件1 台词播完 → 让玩家选物品栏里的东西
       DLG_AFTER_EVENT_END  = 5,    // 第二十步：事件1 结尾台词播完 → 饱食+30 并跳到午时
       DLG_AFTER_GAOKAO_END = 6,    // 第二十三步：讲高考最后一段播完 → 结算（精神+30 / 知识-50）
       DLG_AFTER_GAOKAO_SEG = 7 };  // 第二十三步：讲高考一段听完 → 推一节 + 弹「跑路 / 继续听」

static void Event1BeginPick();      // 定义在后面（第二十步）
static void Event1Finish();
static void GaokaoAsk();            // 第二十三步（新增4）：讲高考，定义在后面
static void GaokaoFinish();

static const wchar_t* g_askPrompt = L"";   // 「台词播完要弹的选项框」左边显示的问题
static int            g_askOptN   = 0;     // 对应的选项个数（DlgEnd 会把 g_dlgOptN 清零，所以另存一份）

static void SetScene(int scene);      // 定义在后面，农田台词播完要用

static const DlgLine* g_dlgScript  = nullptr;   // 当前正在播的脚本
static int            g_dlgScriptN = 0;
static int            g_dlgAfter   = DLG_AFTER_NONE;
static bool           g_dlgAsk     = false;     // true = 「这节课该如何度过」选项框
static const wchar_t* g_dlgPrompt  = L"这节课该如何度过";   // 选项框左边显示的问题
static int            g_dlgAskSubj = -1;
static int            g_dormChoice  = 0;   // 宿舍选了什么（挑动画用）
static int            g_classChoice = 0;   // 第十八步：上课选了什么（等「确定」后再播台词）        // 选项框对应哪门课

// 对话框版面（底边卡在物品栏上方，物品栏仍然看得见）
static const float DLG_X = 90.0f, DLG_Y = 436.0f, DLG_W = 1100.0f, DLG_H = 180.0f;
static const float DLG_NAME_W = 210.0f, DLG_NAME_H = 46.0f;
static const float DLG_PAD_X = 42.0f, DLG_TEXT_TOP = 34.0f;
static const float DLG_EM = 25.0f, DLG_LH = 36.0f;

// 第十三步：「缩小的对话框」——只有选项、没有台词时用（宿舍）
static bool  g_dlgSmall = false;
static const float DLG_SM_W = 660.0f, DLG_SM_H = 168.0f;

static RectF DlgBoxRect()
{
    if (!g_dlgSmall) return RectF(DLG_X, DLG_Y, DLG_W, DLG_H);
    float x = (float)BASE_W * 0.5f - DLG_SM_W * 0.5f;
    return RectF(x, (SLOT_Y - 8.0f) - DLG_SM_H, DLG_SM_W, DLG_SM_H);
}

// 第十一步：选项框摆在对话框右侧，上下端与对话框上下端对齐
static const float DLG_OPT_W     = 430.0f;   // 选项列宽
static const float DLG_OPT_INSET = 8.0f;     // 与对话框内沿的留白
static const float DLG_OPT_GAP   = 6.0f;     // 选项之间的间距

static float DlgOptLeft() { return DLG_X + DLG_W - DLG_OPT_INSET - DLG_OPT_W; }

// 第 i 个选项的矩形（画和点都用这一个，保证一致）
static RectF DlgOptRect(int i, int n)
{
    if (n <= 0) n = 1;
    RectF box = DlgBoxRect();
    if (g_dlgSmall) {
        // 缩小框：没有台词，选项铺满整个框
        float inset = 10.0f, gap = 8.0f;
        float oh = (box.Height - inset * 2.0f - gap * (float)(n - 1)) / (float)n;
        return RectF(box.X + inset, box.Y + inset + (float)i * (oh + gap),
                     box.Width - inset * 2.0f, oh);
    }
    float oh = (DLG_H - DLG_OPT_INSET * 2.0f - DLG_OPT_GAP * (float)(n - 1)) / (float)n;
    return RectF(DlgOptLeft(), DLG_Y + DLG_OPT_INSET + (float)i * (oh + DLG_OPT_GAP),
                 DLG_OPT_W, oh);
}

// 问题文字能用的宽度（要避开右边那列选项）
static float DlgAskTextW()
{
    return (DlgOptLeft() - 18.0f) - (DLG_X + DLG_PAD_X);
}

// 当前这一句换行后的结果（逐字浮现要在固定断行上做，否则会来回跳）
static std::vector<std::wstring> g_dlgWrap;
static int g_dlgTotal = 0;

// 选项（开发说明要求支持；day1 剧情本身没有选项，先留好接口）
static int g_dlgOptN = 0;
static const wchar_t* g_dlgOpt[4] = { nullptr, nullptr, nullptr, nullptr };

// 估算一段文字的宽度（与排版规则一致）
static float TextWidth(const wchar_t* s, float em)
{
    float w = 0.0f;
    for (const wchar_t* p = s; p && *p; ++p)
        w += ((unsigned)*p >= 0x2E80u) ? (em + 2.0f) : (em * 0.56f);
    return w;
}

// 按宽度把一句话折成若干行
static void WrapChalkText(const wchar_t* text, float em, float maxW,
                          std::vector<std::wstring>& out)
{
    out.clear();
    if (!text || !*text) return;
    std::wstring cur;
    float w = 0.0f;
    for (const wchar_t* p = text; *p; ++p) {
        float cw = ((unsigned)*p >= 0x2E80u) ? (em + 2.0f) : (em * 0.56f);
        if (w + cw > maxW && !cur.empty()) {
            out.push_back(cur);
            cur.clear();
            w = 0.0f;
        }
        cur.push_back(*p);
        w += cw;
    }
    if (!cur.empty()) out.push_back(cur);
}

// 进入某一句：算好断行和总字数
static void DlgSetLine(int i)
{
    g_dlgLine = i;
    g_dlgChars = 0.0f;
    g_dlgWrap.clear();
    g_dlgTotal = 0;
    if (g_dlgAsk) {
        // 选项框：问题句整条直接显示，不做逐字（宽度避开右侧选项列）
        WrapChalkText(g_dlgPrompt ? g_dlgPrompt : L"", DLG_EM, DlgAskTextW(), g_dlgWrap);
        for (size_t k = 0; k < g_dlgWrap.size(); ++k) g_dlgTotal += (int)g_dlgWrap[k].size();
        g_dlgChars = (float)g_dlgTotal;
        return;
    }
    if (!g_dlgScript || i < 0 || i >= g_dlgScriptN) return;
    WrapChalkText(g_dlgScript[i].text, DLG_EM, DLG_W - DLG_PAD_X * 2.0f, g_dlgWrap);
    for (size_t k = 0; k < g_dlgWrap.size(); ++k) g_dlgTotal += (int)g_dlgWrap[k].size();
}

static void DlgStart()
{
    g_dlgActive = true;
    g_dlgAsk    = false;
    g_dlgSmall  = false;          // 普通台词一律用大框
    g_dlgScript = kStoryDay1;
    g_dlgScriptN = STORY_DAY1_N;
    g_dlgAfter  = DLG_AFTER_NONE;
    g_dlgOptN   = 0;
    DlgSetLine(0);
}

static void DlgEnd()
{
    g_dlgActive = false;
    g_dlgAsk    = false;
    g_dlgOptN   = 0;
    g_dlgScript = nullptr;
    g_dlgScriptN = 0;
    g_dlgAfter  = DLG_AFTER_NONE;
    g_dlgWrap.clear();
}

// 点一下：没浮完就直接全出来，浮完了就下一句
static void DlgAdvance()
{
    if (!g_dlgActive || g_dlgAsk) return;         // 选项框靠点选项推进，不靠「继续」
    if (g_dlgChars < (float)g_dlgTotal) {         // 先补全这一句
        g_dlgChars = (float)g_dlgTotal;
        return;
    }
    // 这一句的附带效果
    if (g_dlgScript && g_dlgLine >= 0 && g_dlgLine < g_dlgScriptN) {
        int give = g_dlgScript[g_dlgLine].giveItem;
        if (give == ITEM_PLAN) { g_hasPlan = true; g_inv[1] = ITEM_PLAN; }
    }

    if (g_dlgScript && g_dlgLine + 1 < g_dlgScriptN) {
        DlgSetLine(g_dlgLine + 1);
    } else {
        int after = g_dlgAfter;
        DlgEnd();
        if (after == DLG_AFTER_NEXT_TIME) {
            // 上完课直接跳到下个时间段，不经过地图
            AdvanceTime();
        } else if (after == DLG_AFTER_ASK) {
            // 第十五/十六步：台词播完 → 弹选项（左边保留 g_askPrompt 那句问话）
            g_dlgActive = true;
            g_dlgAsk    = true;
            g_dlgSmall  = false;
            g_dlgAskSubj = -1;
            g_dlgPrompt = g_askPrompt;
            g_dlgOptN   = g_askOptN;      // 选项内容在 g_dlgOpt[] 里还留着
            DlgSetLine(0);
        } else if (after == DLG_AFTER_EVENT_PICK) {
            Event1BeginPick();
        } else if (after == DLG_AFTER_EVENT_END) {
            Event1Finish();
        } else if (after == DLG_AFTER_GAOKAO_SEG) {
            // 第二十三步：听完一节 → 时间推进一节，界面留在讲高考，弹「跑路 / 继续听」
            AdvanceTime();
            GaokaoAsk();
        } else if (after == DLG_AFTER_GAOKAO_END) {
            GaokaoFinish();                  // 第二十三步：全听完 → 结算回地图
        } else if (after == DLG_AFTER_FARM_LEAVE) {
            // 第十五步：逛完农田 → 回地图并跳过一个时间段
            AdvanceTime();
            SetScene(SCENE_MAP);
        }
    }
}

// ================= 第十一步：上课选项 + 各科老师台词 =================
static const wchar_t* kClassOpt[3] = { L"认真听课", L"随便听课", L"直接摸鱼" };

// 上完课要说的一串台词（带角色名的句子运行时拼，缓冲要一直活着）
static DlgLine      kClassScript[8];
static std::wstring kClassBuf[8];
static int          kClassN = 0;

static void ClassPush(const wchar_t* who, const wchar_t* text)
{
    if (kClassN >= 8) return;
    kClassScript[kClassN].who      = who;
    kClassScript[kClassN].text     = text;
    kClassScript[kClassN].giveItem = ITEM_NONE;
    ++kClassN;
}

// 把角色名插进台词里（开发说明里的 "XX"）
static void ClassPushHero(const wchar_t* who, const wchar_t* pre, const wchar_t* post)
{
    if (kClassN >= 8) return;
    int hero = (g_chosen >= 0 && g_chosen < HERO_COUNT) ? g_chosen : 0;
    kClassBuf[kClassN] = std::wstring(pre) + kHeroName[hero] + post;
    ClassPush(who, kClassBuf[kClassN].c_str());
}

// 按「学科 + 选项」拼出这一节课的台词
static void ClassBuildScript(int subj, int choice)
{
    kClassN = 0;
    switch (subj) {
    case SUBJ_PHYS:
        ClassPush(L"日日老师", L"你看这个小量近似怎么搞哎");
        ClassPush(L"日日老师", L"这个这个火车小船怎么动哎");
        ClassPush(L"日日老师", L"同学们都明白了吗");
        if (choice == 0)
            ClassPushHero(L"日日老师", L"你看那个", L"学习很投入哎");
        break;

    case SUBJ_CHINESE:
        ClassPush(L"玉皇大帝老师", L"同学们，我们今天来学习《纪念刘和珍君》");
        ClassPush(L"玉皇大帝老师", L"把那个《纪念刘和珍君》跟《为了忘却的纪念》联系起来");
        break;

    case SUBJ_MATH:
        ClassPush(L"吉利老师", L"今天我们来讲二次曲线");
        ClassPush(L"吉利老师", L"今天轻松点");
        if (choice == 0)
            ClassPushHero(L"吉利老师", L"", L"很厉害，大学一定要学数学系");
        else
            ClassPush(L"吉利老师", L"听不懂再游一游");
        break;

    case SUBJ_ENGLISH:
        ClassPush(L"厄文老师", L"呃啊，我们把这个应用文看一下");
        if (choice == 0)
            ClassPushHero(L"厄文老师", L"", L"的感觉是非常好的");
        else if (choice == 2)
            ClassPushHero(L"厄文老师", L"", L"赶紧去厕所吃一顿，这个错误太丑了");
        break;

    default:
        break;
    }
}

// 这节课开始时弹出「这节课该如何度过」
static void ClassAsk(int subj)
{
    g_dlgActive  = true;
    g_dlgAsk     = true;
    g_dlgSmall   = false;
    g_dlgAskSubj = subj;
    g_dlgPrompt  = L"这节课该如何度过";
    g_dlgScript  = nullptr;
    g_dlgScriptN = 0;
    g_dlgAfter   = DLG_AFTER_NONE;
    g_dlgOptN    = 3;
    for (int i = 0; i < 3; ++i) g_dlgOpt[i] = kClassOpt[i];
    DlgSetLine(0);
}

// 玩家选了某个选项
static void ClassChoose(int choice)
{
    if (choice < 0 || choice > 2) return;
    int subj = g_dlgAskSubj;

    // 开发说明的数值：一 知识+20 精神-10 快乐-10；二 知识+10 精神-5 快乐-5；三 快乐+5
    // 知识会先经过物品栏里的教辅加成（第十四步）
    if (choice == 0) {
        AddKnowledge(20, subj); AddStat(2, -10); AddStat(3, -10);
    } else if (choice == 1) {
        AddKnowledge(10, subj); AddStat(2, -5); AddStat(3, -5);
    } else {
        AddStat(3,   5);
    }
    g_faceState.Refresh(g_stat[2], g_stat[3]);

    // 第十八步：先把属性变化显示出来，点确定后再播老师的台词
    g_classChoice = choice;
    g_dlgAsk      = false;
    g_dlgOptN     = 0;
    CommitDeltas(DN_CLASS_TALK);
}

// 画对话框
static void DrawDialogue(Graphics& g, float el)
{
    if (!g_dlgActive) return;
    if (!g_dlgAsk) {
        if (!g_dlgScript || g_dlgLine < 0 || g_dlgLine >= g_dlgScriptN) return;
    }
    const DlgLine* ln = (!g_dlgAsk && g_dlgScript) ? &g_dlgScript[g_dlgLine] : nullptr;

    RectF box = DlgBoxRect();

    // 背景压暗：上面渐变、下面实压，没有硬边
    {
        LinearGradientBrush vb(Point(0, 0), Point(0, (INT)box.Y),
                               Color(0, 0, 0, 0), Color(120, 0, 0, 0));
        g.FillRectangle(&vb, 0.0f, 0.0f, (float)BASE_W, box.Y);
        SolidBrush dim(Color(120, 0, 0, 0));
        g.FillRectangle(&dim, 0.0f, box.Y, (float)BASE_W, (float)BASE_H - box.Y);
    }

    // 面板
    {
        GraphicsPath sp;
        MakeRoundRect(sp, RectF(box.X + 7.0f, box.Y + 9.0f, box.Width, box.Height), 18.0f);
        SolidBrush sb(Color(110, 0, 0, 0));
        g.FillPath(&sb, &sp);
    }
    GraphicsPath cp;
    MakeRoundRect(cp, box, 18.0f);
    {
        SolidBrush fill(Color(250, 253, 250, 241));
        g.FillPath(&fill, &cp);
        Pen ink(Color(255, 34, 30, 26), 3.2f);
        ink.SetLineJoin(LineJoinRound);
        g.DrawPath(&ink, &cp);
    }

    // 左上角说话人名牌（选项框没有说话人，不画）
    if (ln && !g_dlgSmall) {
        RectF nr(box.X + 26.0f, box.Y - DLG_NAME_H * 0.55f, DLG_NAME_W, DLG_NAME_H);
        GraphicsPath np;
        MakeRoundRect(np, nr, 12.0f);
        SolidBrush nf(Color(255, 246, 222, 158));
        g.FillPath(&nf, &np);
        Pen np2(Color(255, 34, 30, 26), 2.8f);
        np2.SetLineJoin(LineJoinRound);
        g.DrawPath(&np2, &np);
        DrawTextCenter(g, ln->who, *g_famUI, FontStyleBold, 23.0f,
                       nr.X + nr.Width * 0.5f, nr.Y + nr.Height * 0.5f, 1.0f,
                       Color(255, 52, 40, 30), 0.4f);
    }

    // 正文：逐字浮现（选项框的问题句一次性显示，并在左半边垂直居中）
    if (!g_dlgSmall) {
        int left = (int)g_dlgChars;
        float y = box.Y + DLG_TEXT_TOP;
        if (g_dlgAsk) {
            float h = (float)g_dlgWrap.size() * DLG_LH;
            y = box.Y + (box.Height - h) * 0.5f;
        }
        for (size_t i = 0; i < g_dlgWrap.size() && left > 0; ++i) {
            int n = (int)g_dlgWrap[i].size();
            int take = (left < n) ? left : n;
            std::wstring part = g_dlgWrap[i].substr(0, (size_t)take);
            DrawInkStringMixed(g, part.c_str(), *g_famUI,
                          g_dlgAsk ? FontStyleBold : FontStyleRegular, DLG_EM,
                          box.X + DLG_PAD_X, y,
                          g_dlgAsk ? Color(255, 176, 62, 52) : Color(255, 40, 36, 30));
            left -= take;
            y += DLG_LH;
        }
    }

    // 右下角「>继续」——有选项时不显示
    bool done = (g_dlgChars >= (float)g_dlgTotal);
    if (g_dlgOptN == 0) {
        float blink = 0.72f + 0.28f * (float)sin((double)el / 260.0);
        const wchar_t* tip = L">继续";
        float tw = TextWidth(tip, 21.0f);
        DrawInkString(g, tip, *g_famUI, FontStyleBold, 21.0f,
                      box.X + box.Width - DLG_PAD_X - tw, box.Y + box.Height - 42.0f,
                      Color(Alpha(230, blink * (done ? 1.0f : 0.55f)), 190, 84, 60));
    }

    // 选项：长方形上下排列
    //   普通框 → 摆在对话框右侧，上下端与对话框上下端对齐
    //   缩小框 → 没有台词，选项直接铺满整个框
    if (g_dlgOptN > 0) {
        for (int i = 0; i < g_dlgOptN; ++i) {
            RectF r = DlgOptRect(i, g_dlgOptN);
            float hot = g_hoverAnim[25 + i];
            GraphicsPath op;
            MakeRoundRect(op, r, 12.0f);
            SolidBrush f(Color(252, (BYTE)(246 - 12 * hot), (BYTE)(241 - 14 * hot),
                               (BYTE)(226 - 12 * hot)));
            g.FillPath(&f, &op);
            Pen pn(Color(255, 34, 30, 26), 2.6f + 0.9f * hot);
            pn.SetLineJoin(LineJoinRound);
            g.DrawPath(&pn, &op);
            DrawTextCenter(g, g_dlgOpt[i], *g_famUI, FontStyleBold,
                           g_dlgSmall ? 21.0f : 22.0f,
                           r.X + r.Width * 0.5f, r.Y + r.Height * 0.5f, 1.0f,
                           Color(255, 44, 38, 32), 0.0f);
        }
    }
}

// ---- 事务安排（蓝色日记本） ----
static const float PLAN_W = 760.0f, PLAN_H = 520.0f;
static const float PLAN_CX = 640.0f, PLAN_CY = 356.0f;
static const float PLAN_LINE_GAP = 34.0f, PLAN_TEXT_TOP = 104.0f;

// 蓝色小本子图标（物品栏用）
static void DrawPlanIcon(Graphics& g, RectF r, float t)
{
    float w = r.Width * 0.52f, h = r.Height * 0.62f;
    float x = r.X + (r.Width - w) * 0.5f, y = r.Y + (r.Height - h) * 0.5f - 2.0f;

    GraphicsPath sp;
    MakeRoundRect(sp, RectF(x + 3.0f, y + 4.0f, w, h), 4.0f);
    SolidBrush sh(Color(Alpha(70, t), 0, 0, 0));
    g.FillPath(&sh, &sp);

    GraphicsPath cp;
    MakeRoundRect(cp, RectF(x, y, w, h), 4.0f);
    SolidBrush cover(Color(Alpha(255, t), 56, 108, 190));      // 蓝色封皮
    g.FillPath(&cover, &cp);
    Pen ink(Color(Alpha(255, t), 28, 52, 96), 2.0f);
    ink.SetLineJoin(LineJoinRound);
    g.DrawPath(&ink, &cp);

    // 书脊 + 三条白色横线（内页）
    {
        Pen spine(Color(Alpha(200, t), 34, 66, 122), 2.0f);
        g.DrawLine(&spine, x + w * 0.16f, y + 2.0f, x + w * 0.16f, y + h - 2.0f);
        Pen line(Color(Alpha(210, t), 236, 240, 244), 1.8f);
        for (int i = 0; i < 3; ++i) {
            float ly = y + h * 0.28f + i * h * 0.20f;
            g.DrawLine(&line, x + w * 0.26f, ly, x + w * 0.86f, ly);
        }
    }
}

static void DrawPlanView(Graphics& g, float el)
{
    if (!g_planOpen && g_planAnim <= 0.001f) return;
    float a = EaseOut(Clamp01(g_planAnim));

    // 全屏压暗
    SolidBrush dim(Color(Alpha(150, a), 0, 0, 0));
    g.FillRectangle(&dim, 0.0f, 0.0f, (float)BASE_W, (float)BASE_H);

    float sc = 0.94f + 0.06f * a;
    float pw = PLAN_W * sc, ph = PLAN_H * sc;
    RectF paper(PLAN_CX - pw * 0.5f, PLAN_CY - ph * 0.5f, pw, ph);

    // 纸张阴影
    {
        GraphicsPath sp;
        MakeRoundRect(sp, RectF(paper.X + 9.0f, paper.Y + 11.0f, pw, ph), 10.0f);
        SolidBrush sb(Color(Alpha(120, a), 0, 0, 0));
        g.FillPath(&sb, &sp);
    }
    // 白纸
    GraphicsPath pp;
    MakeRoundRect(pp, paper, 10.0f);
    {
        SolidBrush fill(Color(Alpha(252, a), 252, 250, 243));
        g.FillPath(&fill, &pp);
        Pen ink(Color(Alpha(255, a), 60, 52, 42), 2.6f);
        ink.SetLineJoin(LineJoinRound);
        g.DrawPath(&ink, &pp);
    }

    // 横线（只在纸内）
    {
        GraphicsState st = g.Save();
        g.SetClip(&pp);
        Pen ln(Color(Alpha(120, a), 150, 176, 206), 1.6f);
        for (float y = paper.Y + PLAN_TEXT_TOP - 10.0f; y < paper.Y + ph - 46.0f; y += PLAN_LINE_GAP)
            g.DrawLine(&ln, paper.X + 34.0f, y, paper.X + pw - 34.0f, y);
        // 左侧红色竖线（在文字左边，不压字）
        Pen rl(Color(Alpha(110, a), 214, 122, 118), 1.8f);
        g.DrawLine(&rl, paper.X + 104.0f, paper.Y + 26.0f, paper.X + 104.0f, paper.Y + ph - 30.0f);
        g.Restore(st);
    }

    // 标题：Day N
    {
        wchar_t buf[32];
        swprintf(buf, 32, L"Day%d", g_planPage + 1);
        DrawInkString(g, buf, *g_famTitle, FontStyleBold, 34.0f,
                      paper.X + 46.0f, paper.Y + 32.0f, Color(Alpha(255, a), 44, 38, 30));
        DrawInkString(g, L"事务安排", *g_famUI, FontStyleRegular, 19.0f,
                      paper.X + pw - 160.0f, paper.Y + 42.0f, Color(Alpha(180, a), 96, 88, 76));
    }

    // 正文
    {
        const PlanPage& pg = kPlanPages[g_planPage];
        float y = paper.Y + PLAN_TEXT_TOP;
        if (pg.n <= 0) {
            DrawInkString(g, L"待后续开发", *g_famUI, FontStyleRegular, 24.0f,
                          paper.X + 124.0f, y, Color(Alpha(200, a), 120, 112, 100));
        } else {
            for (int i = 0; i < pg.n; ++i) {
                DrawInkString(g, pg.lines[i], *g_famBody, FontStyleRegular, 23.0f,
                              paper.X + 124.0f, y, Color(Alpha(255, a), 48, 42, 34));
                y += PLAN_LINE_GAP;
            }
        }
    }

    // 右上角红叉
    {
        float cx = paper.X + pw - 30.0f, cy = paper.Y + 28.0f, r = 11.0f;
        float hot = g_hoverAnim[30];
        Pen rp(Color(Alpha(240, a), 198, 62, 56), 4.0f + 1.0f * hot);
        rp.SetStartCap(LineCapRound);
        rp.SetEndCap(LineCapRound);
        g.DrawLine(&rp, cx - r, cy - r, cx + r, cy + r);
        g.DrawLine(&rp, cx + r, cy - r, cx - r, cy + r);
    }

    // 左下 <- / 右下 ->
    {
        const wchar_t* prev = L"←";
        const wchar_t* next = L"→";
        float py = paper.Y + ph - 34.0f;
        bool canPrev = (g_planPage > 0);
        bool canNext = (g_planPage < 6);
        DrawInkString(g, prev, *g_famTitle, FontStyleBold, 34.0f,
                      paper.X + 30.0f, py - 26.0f,
                      Color(Alpha(canPrev ? 255 : 90, a), canPrev ? 44 : 150,
                            canPrev ? 38 : 146, canPrev ? 30 : 140));
        DrawInkString(g, next, *g_famTitle, FontStyleBold, 34.0f,
                      paper.X + pw - 62.0f, py - 26.0f,
                      Color(Alpha(canNext ? 255 : 90, a), canNext ? 44 : 150,
                            canNext ? 38 : 146, canNext ? 30 : 140));

        wchar_t pgbuf[32];
        swprintf(pgbuf, 32, L"%d / 7", g_planPage + 1);
        DrawTextCenter(g, pgbuf, *g_famUI, FontStyleRegular, 18.0f,
                       paper.X + pw * 0.5f, py - 8.0f, 1.0f,
                       Color(Alpha(170, a), 96, 88, 76), 0.0f);
    }
}

// ================= 第十二步：食堂和小卖部 =================
//  开发说明：
//    · 地图点「7 食堂和小卖部」进入本界面，背景用「食堂和小卖部界面.jpg」
//    · 界面保留游戏内 UI，对话框右边两个选项：购买 / 离开
//    · 点「购买」→ 屏幕中间弹出「食堂商店」，四种商品横排长方形展示
//    · 钱不够 → 正中「预算不足」停 1 秒；背包满 → 正中「背包已满」停 1 秒
//    · 商店右上红叉回食堂界面；点「离开」回地图

static void SetScene(int scene);        // 定义在后面，这里先用
static void Toast(const wchar_t* text);

// 食堂只在饭点开放：清晨(0) / 中午下课(6) / 中午吃饭时间(7) / 中午午时(8) /
// 晚饭前(14) / 晚饭时间(15)
static bool CanteenOpen(int t)
{
    return (t == 0 || t == 6 || t == 7 || t == 8 || t == 14 || t == 15);
}

// 商品：toBag=1 进背包（左键使用生效），0 = 当场吃掉并跳过一个时间节点
// 分配：健康饭菜 / 不那么健康食品 当场吃；小卖部饼干 / 压缩饼干 / 咖啡 进背包
struct ShopItem {
    const wchar_t* name;
    const wchar_t* effect;
    int price;
    int satiety, health, happy;     // 属性变化（咖啡的精神单独在 UseItem 里算）
    int toBag;
    int icon;                       // 0 饭菜 / 1 圆饼干 / 2 汉堡 / 3 方饼干 / 4 咖啡
};

static const ShopItem kShop[5] = {
    { L"健康饭菜",       L"饱食 +50   健康 +10   ※直接食用·耗时",  20, 50,  10,   0, 0, 0 },
    { L"小卖部饼干",     L"饱食 +40   健康 -5   快乐 -5",   5, 40,  -5,  -5, 1, 1 },
    { L"不那么健康食品", L"饱食 +40   健康 -10  快乐 +20   ※直接食用·耗时", 10, 40, -10,  20, 0, 2 },
    { L"压缩饼干",       L"饱食 +40   健康 -10  快乐 -20",  2, 40, -10, -20, 1, 3 },
    { L"咖啡",           L"精神 +30（继科额外 +15）  健康 -10", 20, 0, -10,  0, 1, 4 }
};

static const int SHOP_N = 5;        // 商品件数

// 咖啡的精神回复：继科额外 +15
static int CoffeeSpirit()
{
    int hero = (g_chosen >= 0 && g_chosen < HERO_COUNT) ? g_chosen : 0;
    return (hero == 1) ? 45 : 30;
}

// 商店版面
static const float SHOP_W = 720.0f, SHOP_H = 560.0f;
static const float SHOP_CX = 640.0f, SHOP_CY = 340.0f;
static const float ROW_H = 78.0f, ROW_GAP = 12.0f, ROW_TOP = 148.0f;

static float ShopRowY(int i) { return ROW_TOP + (float)i * (ROW_H + ROW_GAP); }
static float ShopRowX() { return SHOP_CX - SHOP_W * 0.5f + 36.0f; }
static float ShopRowW() { return SHOP_W - 72.0f; }

// 食物小图标（程序画，不用外部素材）
static void DrawFoodIcon(Graphics& g, RectF r, int kind, float t)
{
    float cx = r.X + r.Width * 0.5f, cy = r.Y + r.Height * 0.5f;
    float s  = (r.Width < r.Height ? r.Width : r.Height) * 0.5f;
    Pen ink(Color(Alpha(255, t), 62, 50, 40), 2.0f);
    ink.SetLineJoin(LineJoinRound);

    if (kind == 0) {
        // 饭菜：碗 + 白饭 + 两片绿菜
        GraphicsPath bowl;
        bowl.AddBezier(cx - s * 0.82f, cy - s * 0.10f, cx - s * 0.70f, cy + s * 0.82f,
                       cx + s * 0.70f, cy + s * 0.82f, cx + s * 0.82f, cy - s * 0.10f);
        bowl.AddLine(cx + s * 0.82f, cy - s * 0.10f, cx - s * 0.82f, cy - s * 0.10f);
        SolidBrush white(Color(Alpha(255, t), 250, 248, 240));
        g.FillPath(&white, &bowl);
        g.DrawPath(&ink, &bowl);

        GraphicsPath rice;
        rice.AddEllipse(cx - s * 0.60f, cy - s * 0.55f, s * 1.20f, s * 0.60f);
        SolidBrush rfill(Color(Alpha(255, t), 252, 250, 242));
        g.FillPath(&rfill, &rice);
        g.DrawPath(&ink, &rice);

        SolidBrush green(Color(Alpha(255, t), 126, 178, 106));
        GraphicsPath veg;
        veg.AddEllipse(cx - s * 0.34f, cy - s * 0.86f, s * 0.46f, s * 0.26f);
        veg.AddEllipse(cx + s * 0.02f, cy - s * 0.96f, s * 0.42f, s * 0.24f);
        g.FillPath(&green, &veg);
        g.DrawPath(&ink, &veg);
    } else if (kind == 1) {
        // 圆形饼干
        GraphicsPath c;
        c.AddEllipse(cx - s * 0.78f, cy - s * 0.78f, s * 1.56f, s * 1.56f);
        SolidBrush f(Color(Alpha(255, t), 214, 168, 104));
        g.FillPath(&f, &c);
        g.DrawPath(&ink, &c);
        SolidBrush dot(Color(Alpha(255, t), 150, 108, 58));
        const float dx[5] = { 0.0f, -0.34f, 0.34f, -0.20f, 0.22f };
        const float dy[5] = { 0.0f, -0.18f, -0.10f, 0.30f, 0.28f };
        for (int i = 0; i < 5; ++i) {
            GraphicsPath d;
            d.AddEllipse(cx + dx[i] * s - s * 0.08f, cy + dy[i] * s - s * 0.08f, s * 0.16f, s * 0.16f);
            g.FillPath(&dot, &d);
        }
    } else if (kind == 2) {
        // 汉堡：上包 + 肉饼 + 下包
        GraphicsPath top;
        top.AddBezier(cx - s * 0.86f, cy - s * 0.06f, cx - s * 0.86f, cy - s * 0.86f,
                      cx + s * 0.86f, cy - s * 0.86f, cx + s * 0.86f, cy - s * 0.06f);
        top.AddLine(cx + s * 0.86f, cy - s * 0.06f, cx - s * 0.86f, cy - s * 0.06f);
        SolidBrush bun(Color(Alpha(255, t), 226, 176, 104));
        g.FillPath(&bun, &top);
        g.DrawPath(&ink, &top);

        SolidBrush meat(Color(Alpha(255, t), 148, 88, 58));
        g.FillRectangle(&meat, cx - s * 0.86f, cy - s * 0.02f, s * 1.72f, s * 0.30f);
        g.DrawRectangle(&ink, cx - s * 0.86f, cy - s * 0.02f, s * 1.72f, s * 0.30f);

        SolidBrush let(Color(Alpha(255, t), 132, 186, 104));
        g.FillRectangle(&let, cx - s * 0.84f, cy + s * 0.30f, s * 1.68f, s * 0.16f);

        GraphicsPath bot;
        bot.AddBezier(cx - s * 0.86f, cy + s * 0.50f, cx - s * 0.70f, cy + s * 0.92f,
                      cx + s * 0.70f, cy + s * 0.92f, cx + s * 0.86f, cy + s * 0.50f);
        bot.AddLine(cx - s * 0.86f, cy + s * 0.50f, cx + s * 0.86f, cy + s * 0.50f);
        g.FillPath(&bun, &bot);
        g.DrawPath(&ink, &bot);
    } else if (kind == 3) {
        // 方形饼干
        GraphicsPath sq;
        MakeRoundRect(sq, RectF(cx - s * 0.72f, cy - s * 0.72f, s * 1.44f, s * 1.44f), 4.0f);
        SolidBrush f(Color(Alpha(255, t), 206, 160, 98));
        g.FillPath(&f, &sq);
        g.DrawPath(&ink, &sq);
        SolidBrush dot(Color(Alpha(255, t), 142, 100, 52));
        for (int iy = 0; iy < 3; ++iy)
            for (int ix = 0; ix < 3; ++ix) {
                float px = cx + (float)(ix - 1) * s * 0.42f - s * 0.07f;
                float py = cy + (float)(iy - 1) * s * 0.42f - s * 0.07f;
                GraphicsPath d;
                d.AddEllipse(px, py, s * 0.14f, s * 0.14f);
                g.FillPath(&dot, &d);
            }
    } else {
        // 咖啡：杯身 + 把手 + 咖啡液 + 热气
        Pen steam(Color(Alpha(190, t), 168, 160, 148), 1.8f);
        steam.SetStartCap(LineCapRound);
        steam.SetEndCap(LineCapRound);
        for (int i = 0; i < 2; ++i) {
            float sx = cx - s * 0.26f + (float)i * s * 0.52f;
            GraphicsPath w;
            w.AddBezier(sx, cy - s * 0.52f, sx + s * 0.22f, cy - s * 0.76f,
                        sx - s * 0.22f, cy - s * 0.92f, sx + s * 0.04f, cy - s * 1.14f);
            g.DrawPath(&steam, &w);
        }

        // 杯身（上宽下窄）
        GraphicsPath cup;
        cup.AddLine(cx - s * 0.62f, cy - s * 0.42f, cx + s * 0.62f, cy - s * 0.42f);
        cup.AddLine(cx + s * 0.62f, cy - s * 0.42f, cx + s * 0.42f, cy + s * 0.60f);
        cup.AddLine(cx + s * 0.42f, cy + s * 0.60f, cx - s * 0.42f, cy + s * 0.60f);
        cup.AddLine(cx - s * 0.42f, cy + s * 0.60f, cx - s * 0.62f, cy - s * 0.42f);
        SolidBrush cupFill(Color(Alpha(255, t), 248, 244, 234));
        g.FillPath(&cupFill, &cup);
        g.DrawPath(&ink, &cup);

        // 咖啡液
        SolidBrush liq(Color(Alpha(255, t), 96, 62, 40));
        GraphicsPath lq;
        lq.AddEllipse(cx - s * 0.55f, cy - s * 0.52f, s * 1.10f, s * 0.26f);
        g.FillPath(&liq, &lq);
        g.DrawPath(&ink, &lq);

        // 把手
        GraphicsPath hd;
        hd.AddBezier(cx + s * 0.60f, cy - s * 0.24f, cx + s * 1.04f, cy - s * 0.30f,
                     cx + s * 1.04f, cy + s * 0.34f, cx + s * 0.56f, cy + s * 0.34f);
        Pen hp(Color(Alpha(255, t), 62, 50, 40), 3.0f);
        hp.SetStartCap(LineCapRound);
        hp.SetEndCap(LineCapRound);
        g.DrawPath(&hp, &hd);

        // 碟子
        GraphicsPath sa;
        sa.AddEllipse(cx - s * 0.86f, cy + s * 0.56f, s * 1.72f, s * 0.28f);
        SolidBrush sf(Color(Alpha(255, t), 240, 236, 226));
        g.FillPath(&sf, &sa);
        g.DrawPath(&ink, &sa);
    }
}

// 屏幕正中提示（1 秒）：预算不足 / 背包满了 / 该休息了
static void CenterMsg(const wchar_t* text)
{
    g_centerMsg = text;
    g_centerMsgUntil = GetTickCount64() + 1000;
}

// 第二十三步：任何购买行为都要弹「已经购买XX」。
// 注意传进来的字符串必须是静态的（CenterMsg 只存指针），所以这里现拼进缓冲区
static wchar_t g_buyMsg[96];
static void CenterMsgBuy(const wchar_t* item)
{
    // 注意：MinGW 的 swprintf 里 %s 会按窄字符来读宽字符串（实测会把「健康饭菜」读成 eP·^m），
    // 所以这里直接拼 std::wstring，不碰格式化
    std::wstring s = std::wstring(L"已经购买") + (item ? item : L"");
    size_t n = s.size();
    if (n > 95) n = 95;
    for (size_t i = 0; i < n; ++i) g_buyMsg[i] = s[i];
    g_buyMsg[n] = L'\0';
    CenterMsg(g_buyMsg);
}

// 背包里第一个空格（1~8），没有就返回 -1
static int InvFreeSlot()
{
    for (int i = 1; i < SLOT_N; ++i)
        if (g_inv[i] == ITEM_EMPTY) return i;
    return -1;
}

// 时间跳出食堂开放时段 → 直接回地图并提示
static void CanteenCheckOpen()
{
    if (g_loc != LOC_CANTEEN) return;
    if (CanteenOpen(g_time)) return;

    g_shopOpen = false;
    g_shopAnim = 0.0f;
    g_centerMsg = nullptr;
    g_dlgActive = false;
    g_dlgAsk    = false;
    g_dlgOptN   = 0;
    g_loc = LOC_CLASS;
    SetScene(SCENE_MAP);
    Toast(L"食堂已经关闭");
}

// 买第 i 件商品
static void ShopBuy(int idx)
{
    if (idx < 0 || idx >= SHOP_N) return;
    const ShopItem& it = kShop[idx];

    if (g_money < it.price) {                    // 钱不够
        CenterMsg(L"预算不足");
        return;
    }
    if (it.toBag && InvFreeSlot() < 0) {         // 要进背包但满了
        CenterMsg(L"背包已满");
        return;
    }

    g_money -= it.price;

    if (it.toBag) {
        int slot = InvFreeSlot();
        if (slot >= 0) {
            if (idx == 1)      g_inv[slot] = ITEM_BISCUIT;
            else if (idx == 3) g_inv[slot] = ITEM_CRACKER;
            else if (idx == 4) g_inv[slot] = ITEM_COFFEE;
            else               g_inv[slot] = ITEM_BURGER;
        }
    } else {
        // 当场吃掉：直接结算属性，再跳过一个时间节点
        AddStat(1, it.satiety);
        AddStat(0, it.health);
        AddStat(3, it.happy);
        g_faceState.Refresh(g_stat[2], g_stat[3]);
        // 第十八步：确定后再推时段
        CommitDeltas(DN_CANTEEN_EAT);
    }
    CenterMsgBuy(it.name);          // 第二十三步：「已经购买XX」停 1 秒
}

// 使用物品栏第 slot 格的东西
static void UseItem(int slot)
{
    if (slot < 1 || slot >= SLOT_N) return;
    int kind = g_inv[slot];
    if (kind == ITEM_EMPTY) return;

    if (kind == ITEM_PLAN) {                     // 事务安排：打开本子
        g_planOpen = true;
        g_planPage = (g_day >= 1 && g_day <= 7) ? (g_day - 1) : 0;
        return;
    }
    if (kind == ITEM_BISCUIT) {
        AddStat(1, 40); AddStat(0, -5); AddStat(3, -5);
    } else if (kind == ITEM_BURGER) {
        AddStat(1, 40); AddStat(0, -10); AddStat(3, 20);
    } else if (kind == ITEM_CRACKER) {
        AddStat(1, 40); AddStat(0, -10); AddStat(3, -20);
    } else if (kind == ITEM_COFFEE) {
        // 回复 30 精神，继科额外 +15；健康 -10
        AddStat(2, CoffeeSpirit());
        AddStat(0, -10);
    } else if (kind == ITEM_CORN) {
        // 第二十一步：玉米 → 饱食 +50、健康 +20
        AddStat(1, 50);
        AddStat(0, 20);
    }
    g_inv[slot] = ITEM_EMPTY;                    // 用掉就消失
    g_faceState.Refresh(g_stat[2], g_stat[3]);
    CommitDeltas(DN_NONE);                       // 第十八步：显示属性变化
}

// 第二十三步：食堂的「购买 / 离开」对话框抽成函数。
//  原来只有在「进食堂」时才设一次：商店红叉关掉、或者当场吃完（属性面板 → 动画）之后，
//  对话框被清掉就不再出现，界面上没有任何出口 → 卡死。现在这两处都重新调一次。
static void CanteenAsk()
{
    g_dlgActive  = true;
    g_dlgAsk     = true;
    g_dlgAskSubj = -1;
    g_dlgPrompt  = L"";
    g_dlgOptN    = 2;
    g_dlgOpt[0]  = L"购买";
    g_dlgOpt[1]  = L"离开";
    g_dlgScript  = nullptr;
    g_dlgScriptN = 0;
    g_dlgAfter   = DLG_AFTER_NONE;
    g_dlgSmall   = false;
    DlgSetLine(0);
}

// 进入食堂：常驻「购买 / 离开」对话框
static void EnterCanteen()
{
    g_loc = LOC_CANTEEN;
    g_shopOpen = false;
    g_shopKind = 0;                 // 第二十三步：从书屋过来要切回食堂商店
    g_shopAnim = 0.0f;
    g_centerMsg = nullptr;
    g_dlgSmall  = false;
    CanteenAsk();
}

static void LeaveCanteen()
{
    g_shopOpen = false;
    g_shopAnim = 0.0f;
    g_centerMsg = nullptr;
    g_dlgActive = false;
    g_dlgAsk    = false;
    g_dlgOptN   = 0;
    g_loc = LOC_CLASS;
    SetScene(SCENE_MAP);
}

// 商店界面
static void DrawShopView(Graphics& g, float el)
{
    if (!g_shopOpen && g_shopAnim <= 0.001f) return;
    float a = EaseOut(Clamp01(g_shopAnim));

    SolidBrush dim(Color(Alpha(150, a), 0, 0, 0));
    g.FillRectangle(&dim, 0.0f, 0.0f, (float)BASE_W, (float)BASE_H);

    float sc = 0.95f + 0.05f * a;
    float pw = SHOP_W * sc, ph = SHOP_H * sc;
    RectF p(SHOP_CX - pw * 0.5f, SHOP_CY - ph * 0.5f, pw, ph);

    {
        GraphicsPath sp;
        MakeRoundRect(sp, RectF(p.X + 9.0f, p.Y + 11.0f, pw, ph), 16.0f);
        SolidBrush sb(Color(Alpha(120, a), 0, 0, 0));
        g.FillPath(&sb, &sp);
    }
    GraphicsPath pp;
    MakeRoundRect(pp, p, 16.0f);
    {
        SolidBrush fill(Color(Alpha(250, a), 252, 249, 240));
        g.FillPath(&fill, &pp);
        Pen ink(Color(Alpha(255, a), 58, 48, 38), 3.0f);
        ink.SetLineJoin(LineJoinRound);
        g.DrawPath(&ink, &pp);
    }

    // 标题
    DrawTextCenter(g, (g_shopKind == 0) ? L"食堂商店" : L"书屋",
                   *g_famTitle, FontStyleBold, 34.0f,
                   SHOP_CX, p.Y + 44.0f, 3.0f, Color(Alpha(255, a), 48, 42, 34), 1.0f);

    // 商品
    for (int i = 0; i < SHOP_N; ++i) {
        RectF r(ShopRowX(), ShopRowY(i), ShopRowW(), ROW_H);
        float hot = g_hoverAnim[50 + i];

        GraphicsPath rp;
        MakeRoundRect(rp, r, 12.0f);
        SolidBrush rf(Color(Alpha(250, a), (BYTE)(246 - 10 * hot), (BYTE)(241 - 12 * hot),
                            (BYTE)(226 - 10 * hot)));
        g.FillPath(&rf, &rp);
        Pen rpen(Color(Alpha(255, a), 62, 52, 42), 2.4f + 0.8f * hot);
        rpen.SetLineJoin(LineJoinRound);
        g.DrawPath(&rpen, &rp);

        RectF ir(r.X + 12.0f, r.Y + 10.0f, ROW_H - 20.0f, ROW_H - 20.0f);
        const wchar_t* nm = nullptr;
        const wchar_t* ef = nullptr;
        int price = 0;
        if (g_shopKind == 0) {
            DrawFoodIcon(g, ir, kShop[i].icon, a);
            nm = kShop[i].name; ef = kShop[i].effect; price = kShop[i].price;
        } else {
            DrawBookIcon(g, ir, kBooks[i].kind, a);
            nm = kBooks[i].name; ef = kBooks[i].effect; price = kBooks[i].price;
        }

        DrawInkString(g, nm, *g_famUI, FontStyleBold, 23.0f,
                      r.X + 82.0f, r.Y + 14.0f, Color(Alpha(255, a), 44, 38, 30));
        DrawInkString(g, ef, *g_famUI, FontStyleRegular, 16.0f,
                      r.X + 82.0f, r.Y + 50.0f, Color(Alpha(205, a), 96, 88, 76));

        wchar_t buf[24];
        swprintf(buf, 24, L"¥%d", price);
        float tw = TextWidth(buf, 24.0f);
        DrawInkString(g, buf, *g_famUI, FontStyleBold, 24.0f,
                      r.X + r.Width - tw - 18.0f, r.Y + ROW_H * 0.5f - 20.0f,
                      Color(Alpha(255, a), 178, 118, 34));
    }

    // 右上角红叉
    {
        float cx = p.X + pw - 32.0f, cy = p.Y + 32.0f, rr = 12.0f;
        float hot = g_hoverAnim[55];
        Pen rp2(Color(Alpha(240, a), 198, 62, 56), 4.0f + 1.0f * hot);
        rp2.SetStartCap(LineCapRound);
        rp2.SetEndCap(LineCapRound);
        g.DrawLine(&rp2, cx - rr, cy - rr, cx + rr, cy + rr);
        g.DrawLine(&rp2, cx + rr, cy - rr, cx - rr, cy + rr);
    }
}

// 正中提示条：预算不足 / 背包满了 / 该休息了（停 1 秒）
static void DrawCenterMsg(Graphics& g)
{
    if (!g_centerMsg) return;
    if (GetTickCount64() >= g_centerMsgUntil) return;

    float mw = 320.0f, mh = 76.0f;
    RectF mr((float)BASE_W * 0.5f - mw * 0.5f, (float)BASE_H * 0.5f - mh * 0.5f, mw, mh);
    GraphicsPath mp;
    MakeRoundRect(mp, mr, 14.0f);
    SolidBrush mf(Color(238, 52, 42, 34));
    g.FillPath(&mf, &mp);
    Pen mpen(Color(255, 250, 246, 238), 2.4f);
    g.DrawPath(&mpen, &mp);
    DrawTextCenter(g, g_centerMsg, *g_famUI, FontStyleBold, 26.0f,
                   mr.X + mw * 0.5f, mr.Y + mh * 0.5f, 2.0f,
                   Color(255, 252, 248, 240), 0.0f);
}

// ============================================================
//  第二十二步：成就 + 结局（开发说明原文）
//
//  成就（可以在成就查看页面查看对应）
//    赫鲁晓夫   —— 在考试摘玉米中获得80分以上
//    铁锅炖白皮 —— 在课外实践活动中选择"物理白皮"
//    学霸       —— 检测到知识达到300触发
//
//  结局时屏幕会变黑并显示相应的结局，屏幕上有结局名字和结局的描述文字
//    任何时候精神为0 -> 精神病院进修结局
//                      由于你的精神过于健康被保送至精神病院大学攻读被研究专业
//    任何时候健康为0 -> 医院结局
//                      由于你的健康状况堪忧被医生认定应该送去治疗
//    任何时候饱食为0 -> 饿晕结局
//                      你的同学在路边发现了饿晕的你并送往了医院
//    任何时候快乐为0 -> 抑郁结局
//                      努力了这么久，你终于考上了抑郁症
//
//  当时间到day4时屏幕会显示「此为本次测试的全部内容」，下方有按钮返回菜单页面
// ============================================================


static std::wstring ExeDir();          // 定义在后面（4843 行附近）
// 成就存到本地（LOCALAPPDATA），重开游戏 / 换存档也留着；取不到就退回 exe 同目录
static std::wstring AppDataDir()
{
    wchar_t buf[MAX_PATH * 2];
    if (SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, buf) == S_OK) {
        std::wstring d = std::wstring(buf) + L"\\高中的一周";
        CreateDirectoryW(d.c_str(), nullptr);
        return d;
    }
    return ExeDir();
}
static std::wstring AchFilePath()  { return AppDataDir() + L"\\achievements.txt"; }
static std::wstring SaveFilePath() { return AppDataDir() + L"\\save.txt"; }

// 文件内容两行，全是 '0'/'1'，不做编码转换，免得多一层宽窄字符的坑
//   第 1 行：成就  第 2 行：达成过的结局
static void AchSave()
{
    FILE* f = _wfopen(AchFilePath().c_str(), L"wb");
    if (!f) return;
    char buf[64];
    for (int i = 0; i < ACH_COUNT; ++i) buf[i] = g_ach[i] ? '1' : '0';
    buf[ACH_COUNT] = '\n';
    for (int i = 0; i < END_COUNT; ++i) buf[ACH_COUNT + 1 + i] = g_endGot[i] ? '1' : '0';
    buf[ACH_COUNT + 1 + END_COUNT] = '\n';
    fwrite(buf, 1, ACH_COUNT + 1 + END_COUNT + 1, f);
    fclose(f);
}
static void AchLoad()
{
    FILE* f = _wfopen(AchFilePath().c_str(), L"rb");
    if (!f) return;
    char buf[64] = { 0 };
    size_t n = fread(buf, 1, 63, f);
    fclose(f);
    for (size_t i = 0; i < n && i < (size_t)ACH_COUNT; ++i) g_ach[i] = (buf[i] == '1');
    for (size_t i = 0; i + ACH_COUNT + 1 < n && i < (size_t)END_COUNT; ++i)
        g_endGot[i] = (buf[ACH_COUNT + 1 + i] == '1');
}
// 结局达成：记下来 + 立刻写盘（成就页要看）
static void MarkEnding(int e)
{
    if (e <= END_NONE || e >= END_COUNT || g_endGot[e]) return;
    g_endGot[e] = true;
    AchSave();
}

// ============================================================
//  第二十三步：存档 / 读档
//    · 游戏界面右上角「保存」按钮（时间 UI 右侧）→ 写盘 + 正中「数据已保存」停 1 秒
//    · 菜单「继续游戏」读档接着玩；「开始游戏」重新选角 = 新开一局
//    · 存成一行纯数字（窄字符，不用管编码），顺序见下面两处注释
// ============================================================
static bool HasSave()
{
    FILE* f = _wfopen(SaveFilePath().c_str(), L"rb");
    if (!f) return false;
    fclose(f);
    return true;
}
static void SaveGame()
{
    FILE* f = _wfopen(SaveFilePath().c_str(), L"wb");
    if (!f) return;
    // 顺序：版本 角色 天 时段 | 五维 | 钱 | 背包9格 | 各种进度 | 场地 | 三个去重戳 | 两个事件
    // 版本 3：多了 day2 两个事件（课外实践 / 讲高考）的完成标记，读档后不会重播、也不会丢
    fprintf(f, "%d %d %d %d ", 3, g_chosen, g_day, g_time);
    for (int i = 0; i < 5; ++i) fprintf(f, "%d ", g_stat[i]);
    fprintf(f, "%d ", g_money);
    for (int i = 0; i < 9; ++i) fprintf(f, "%d ", g_inv[i]);
    fprintf(f, "%d %d %d %d %d %d %d %d %d %d ",
             g_hasPlan ? 1 : 0, g_storyDone ? 1 : 0,
             g_cornDone ? 1 : 0, g_cornPlayed ? 1 : 0,
             g_examDone ? 1 : 0, g_examStamp, g_examIdx, g_examPhase,
             g_examMathCorn ? 1 : 0, g_pianoPlayed ? 1 : 0);
    for (int i = 0; i < 4; ++i) fprintf(f, "%d ", g_examScore[i]);
    fprintf(f, "%d %d %d %d ", g_loc, g_dormStamp, g_classStamp, g_playStamp);
    fprintf(f, "%d %d %d %d\n",
            g_event1Done ? 1 : 0, g_event1Stamp,
            g_gaokaoDone ? 1 : 0, g_gaokaoStamp);
    fclose(f);
}
static bool LoadGame()
{
    FILE* f = _wfopen(SaveFilePath().c_str(), L"rb");
    if (!f) return false;
    int ver = 0;
    if (fscanf(f, "%d", &ver) != 1 || (ver < 1 || ver > 3)) { fclose(f); return false; }
    int chosen = 0, day = 1, tm = 0;
    if (fscanf(f, "%d %d %d", &chosen, &day, &tm) != 3) { fclose(f); return false; }

    int st[5] = { 70, 70, 70, 70, 0 }, money = 200, inv[9];
    for (int i = 0; i < 9; ++i) inv[i] = 0;
    for (int i = 0; i < 5; ++i) if (fscanf(f, "%d", &st[i]) != 1) { fclose(f); return false; }
    if (fscanf(f, "%d", &money) != 1) { fclose(f); return false; }
    for (int i = 0; i < 9; ++i) if (fscanf(f, "%d", &inv[i]) != 1) { fclose(f); return false; }

    int hasPlan = 0, storyDone = 0, cornDone = 0, cornPlayed = 0,
        examDone = 0, examStamp = -1, examIdx = 0, examPhase = 0, examMathCorn = 0,
        pianoPlayed = 0;
    // 版本 1 的旧存档没有钢琴进度这一项，少读一个字段（向后兼容，旧档照样能接着玩）
    int got = (ver >= 2)
        ? fscanf(f, "%d %d %d %d %d %d %d %d %d %d",
                 &hasPlan, &storyDone, &cornDone, &cornPlayed,
                 &examDone, &examStamp, &examIdx, &examPhase, &examMathCorn,
                 &pianoPlayed)
        : fscanf(f, "%d %d %d %d %d %d %d %d %d",
                 &hasPlan, &storyDone, &cornDone, &cornPlayed,
                 &examDone, &examStamp, &examIdx, &examPhase, &examMathCorn);
    if (got < 9) { fclose(f); return false; }
    int sc[4] = { 0, 0, 0, 0 };
    for (int i = 0; i < 4; ++i) if (fscanf(f, "%d", &sc[i]) != 1) { fclose(f); return false; }
    int loc = 0, dormStamp = -1, classStamp = -1, playStamp = -1;
    if (fscanf(f, "%d %d %d %d", &loc, &dormStamp, &classStamp, &playStamp) != 4) {
        fclose(f); return false;
    }
    // 版本 3 才有的两个事件进度；旧档当作「还没做过」
    int ev1Done = 0, ev1Stamp = -1, gkDone = 0, gkStamp = -1;
    if (ver >= 3) {
        if (fscanf(f, "%d %d %d %d", &ev1Done, &ev1Stamp, &gkDone, &gkStamp) != 4) {
            fclose(f); return false;
        }
    }
    fclose(f);

    ResetGameState();                 // 先清干净，再逐项覆盖
    g_chosen = (chosen >= 0 && chosen < HERO_COUNT) ? chosen : 0;
    g_day  = day;  g_time = tm;
    for (int i = 0; i < 5; ++i) g_stat[i] = st[i];
    g_money = money;
    for (int i = 0; i < 9; ++i) g_inv[i] = inv[i];
    g_hasPlan = (hasPlan != 0);  g_storyDone = (storyDone != 0);
    g_cornDone = (cornDone != 0); g_cornPlayed = (cornPlayed != 0);
    g_examDone = (examDone != 0); g_examStamp = examStamp;
    g_examIdx = examIdx; g_examPhase = examPhase; g_examMathCorn = (examMathCorn != 0);
    g_pianoPlayed = (pianoPlayed != 0);
    for (int i = 0; i < 4; ++i) g_examScore[i] = sc[i];
    g_loc = loc; g_dormStamp = dormStamp; g_classStamp = classStamp; g_playStamp = playStamp;
    g_event1Done = (ev1Done != 0); g_event1Stamp = ev1Stamp;
    g_gaokaoDone = (gkDone != 0);  g_gaokaoStamp = gkStamp;

    // 读档后回到地图：各场地的对话框要重新摆一次，不然没有出口
    g_loc = LOC_CLASS;
    g_dlgActive = false; g_dlgAsk = false; g_dlgOptN = 0;
    g_shopOpen = false;  g_shopKind = 0;
    g_faceState.force = -1;
    g_faceState.Refresh(g_stat[2], g_stat[3]);
    return true;
}
static void UnlockAch(int i)
{
    if (i < 0 || i >= ACH_COUNT || g_ach[i]) return;
    g_ach[i] = true;
    AchSave();
    std::wstring msg = std::wstring(L"解锁成就：") + kAchName[i];
    Toast(msg.c_str());
}

// ================= 第十五步：农田 =================
//  开发说明原文：
//    · 初始背景「农田界面.jpg」；玉米事件完成后换成「农田结束界面.jpg」（玉米事件后续开发）
//    · 进入农田：我"现在的玉米还没有成熟" >继续
//                我"要随便看看吗" → 选项「要 / 离开」
//    · 选「要」：玉米事件前 回复15点快乐 5点精神；玉米事件后 只回复5点快乐
//                然后旁白"在农田转了转" >继续 → 回地图并跳过一个时间段
//    · 选「离开」：返回地图（不消耗时间）
static const DlgLine kFarmScriptA[2] = {   // 玉米事件前
    { L"我", L"现在的玉米还没有成熟", ITEM_NONE },
    { L"我", L"要随便看看吗",         ITEM_NONE }
};
static const DlgLine kFarmScriptB[2] = {   // 玉米事件后
    { L"我", L"玉米还没有成熟",       ITEM_NONE },
    { L"我", L"要随便看看吗",         ITEM_NONE }
};
static const DlgLine kFarmEnd[1] = {
    { L"旁白", L"在农田转了转", ITEM_NONE }
};

static void EnterFarm()
{
    g_loc = LOC_FARM;
    g_shopOpen = false;
    g_shopKind = 0;
    g_shopAnim = 0.0f;
    g_dlgSmall  = false;
    g_dlgActive = true;
    g_dlgAsk    = false;              // 先播两句台词，播完再弹选项
    g_dlgAskSubj = -1;
    g_dlgScript = g_cornDone ? kFarmScriptB : kFarmScriptA;
    g_dlgScriptN = 2;
    g_askPrompt = L"要随便看看吗";
    g_dlgOptN   = 0;                  // 第二十三步：台词期间不能有选项（漏了这句台词就会和选项同时出现）
    g_askOptN   = 2;                  // 台词播完才把 g_dlgOptN 置 2
    g_dlgOpt[0] = L"要";
    g_dlgOpt[1] = L"离开";
    g_dlgAfter  = DLG_AFTER_ASK;
    DlgSetLine(0);
}

// 选「要」：回复属性 → 播旁白 → 播完自动回地图并跳过一个时间段
static void FarmStroll()
{
    if (g_cornDone) {
        AddStat(3, 5);                // 玉米事件后只有快乐 +5
    } else {
        AddStat(3, 15);               // 快乐 +15
        AddStat(2, 5);                // 精神 +5
    }
    g_faceState.Refresh(g_stat[2], g_stat[3]);

    g_dlgAsk    = false;
    g_dlgOptN   = 0;
    CommitDeltas(DN_FARM_TALK);            // 第十八步：确定后再播旁白
}

static void FarmChoose(int idx)
{
    if (idx == 0) FarmStroll();
    else {
        g_dlgActive = false;
        g_dlgAsk    = false;
        g_dlgOptN   = 0;
        g_loc = LOC_CLASS;
        SetScene(SCENE_MAP);          // 离开：回地图，不消耗时间
    }
}

// ================= 第十六步：操场 =================
//  开发说明原文：
//    · 地图点 3 号进入；**当前时段是体育课时强制跳转**操场，正中提示「现在是体育课时间」1 秒
//    · 体育课：我"体育老师又不知道去哪里了" >继续 / 我"体育课该干些什么呢" → 跑步吧 / 跑路了兄弟们
//        跑步吧 → Q版小人跑步 5 秒，健康+20（继科额外+10），精神-10，跳到下一个时间段
//        跑路了兄弟们 → 回地图
//    · 非体育课：我"要做些什么吗" → 跑步吧 / 离开
//        跑步吧 → Q版小人跑步 5 秒，健康+10（继科额外+5），精神-10，跳到下一个时间段
//        离开 → 回地图

static bool IsPEClass(int d, int t) { return SubjectAt(d, t) == SUBJ_PE; }

static const DlgLine kPlayPEScript[2] = {
    { L"我", L"体育老师又不知道去哪里了", ITEM_NONE },
    { L"我", L"体育课该干些什么呢",       ITEM_NONE }
};

// Q 版小人：头像当脑袋，身子手脚程序化画，phase 控制手脚摆动
static void DrawRunningChibi(Graphics& g, float cx, float feetY, float phase, float a)
{
    int hero = (g_chosen >= 0 && g_chosen < HERO_COUNT) ? g_chosen : 0;
    Bitmap* av = g_avatar[hero][g_faceState.cur];
    if (!av) av = g_avatar[hero][FACE_CALM];

    const float headD = 112.0f;
    float headCX = cx, headCY = feetY - 192.0f;
    float bodyTop = headCY + headD * 0.5f - 12.0f;
    float bodyH   = 66.0f, bodyW = 80.0f;
    float bodyBot = bodyTop + bodyH;
    float hipY    = bodyBot - 4.0f;

    float sw  = sinf(phase);                 // 前腿相位
    float sw2 = sinf(phase + 3.14159f);      // 后腿相位

    Pen ink(Color(Alpha(255, a), 58, 46, 38), 4.6f);
    ink.SetStartCap(LineCapRound);
    ink.SetEndCap(LineCapRound);
    ink.SetLineJoin(LineJoinRound);

    // 影子
    {
        SolidBrush sh(Color(Alpha(64, a), 40, 40, 40));
        g.FillEllipse(&sh, cx - 56.0f, feetY - 13.0f, 112.0f, 26.0f);
    }

    // ---- 后腿（先画） ----
    {
        float kx = cx - 10.0f + sw2 * 34.0f;
        float ky = hipY + 34.0f;
        float fx = cx - 10.0f + sw2 * 20.0f;
        float fy = feetY - 6.0f - fabsf(sw2) * 16.0f;
        g.DrawLine(&ink, cx - 10.0f, hipY, kx, ky);
        g.DrawLine(&ink, kx, ky, fx, fy);
        SolidBrush shoe(Color(Alpha(255, a), 236, 232, 222));
        g.FillEllipse(&shoe, fx - 9.0f, fy - 5.0f, 20.0f, 11.0f);
    }
    // ---- 后臂 ----
    {
        float shY = bodyTop + 20.0f;
        float x0 = cx - bodyW * 0.5f + 3.0f;
        float ex = x0 - 16.0f + sw * 24.0f, ey = shY + 30.0f;
        g.DrawLine(&ink, x0, shY, ex, ey);
        g.DrawLine(&ink, ex, ey, ex - 2.0f + sw * 20.0f, ey - 16.0f);
    }

    // ---- 身体（校服） ----
    {
        GraphicsPath bp;
        MakeRoundRect(bp, RectF(cx - bodyW * 0.5f, bodyTop, bodyW, bodyH), 18.0f);
        SolidBrush shirt(Color(Alpha(255, a), 78, 112, 92));
        g.FillPath(&shirt, &bp);
        g.DrawPath(&ink, &bp);
        Pen zip(Color(Alpha(210, a), 232, 236, 228), 2.2f);
        g.DrawLine(&zip, cx, bodyTop + 6.0f, cx, bodyTop + 12.0f);
    }

    // ---- 前腿 ----
    {
        float kx = cx + 10.0f + sw * 34.0f;
        float ky = hipY + 34.0f;
        float fx = cx + 10.0f + sw * 20.0f;
        float fy = feetY - 6.0f - fabsf(sw) * 16.0f;
        g.DrawLine(&ink, cx + 10.0f, hipY, kx, ky);
        g.DrawLine(&ink, kx, ky, fx, fy);
        SolidBrush shoe(Color(Alpha(255, a), 244, 240, 230));
        g.FillEllipse(&shoe, fx - 9.0f, fy - 5.0f, 20.0f, 11.0f);
    }
    // ---- 前臂 ----
    {
        float shY = bodyTop + 20.0f;
        float x0 = cx + bodyW * 0.5f - 3.0f;
        float ex = x0 + 16.0f - sw2 * 24.0f, ey = shY + 30.0f;
        g.DrawLine(&ink, x0, shY, ex, ey);
        g.DrawLine(&ink, ex, ey, ex + 2.0f - sw2 * 20.0f, ey - 16.0f);
    }

    // ---- 脑袋：头像圆形裁切 ----
    if (av) {
        GraphicsState st = g.Save();
        GraphicsPath clip;
        clip.AddEllipse(cx - headD * 0.5f, headCY - headD * 0.5f, headD, headD);
        g.SetClip(&clip);
        float sc = headD * 1.40f;
        float iw = (float)av->GetWidth(), ih = (float)av->GetHeight();
        float k = sc / (iw < ih ? iw : ih);
        float dw = iw * k, dh = ih * k;
        g.DrawImage(av, RectF(cx - dw * 0.5f, headCY - dh * 0.46f, dw, dh));
        g.Restore(st);
        Pen ring(Color(Alpha(255, a), 58, 46, 38), 3.4f);
        GraphicsPath ringP;
        ringP.AddEllipse(cx - headD * 0.5f, headCY - headD * 0.5f, headD, headD);
        g.DrawPath(&ring, &ringP);
    }
}

// 跑步的整屏演出（5 秒）
static void DrawRunScene(Graphics& g, float el)
{
    if (!g_runActive || g_animKind != ANIM_RUN) return;
    ULONGLONG now = GetTickCount64();
    float ms = (now > g_runStart) ? (float)(now - g_runStart) : 0.0f;
    float p  = ms / 1000.0f;
    float a  = Clamp01(ms / 200.0f);

    SolidBrush dim(Color(Alpha(120, a), 0, 0, 0));
    g.FillRectangle(&dim, 0.0f, 0.0f, (float)BASE_W, (float)BASE_H);

    // 脚底落在跑道近端（操场界面.jpg 里红色跑道大概在 y 557~720）
    float groundY = 594.0f;

    // 地面 + 滚动刻度（让人看着在往前跑）
    {
        Pen gl(Color(Alpha(150, a), 246, 240, 226), 3.0f);
        g.DrawLine(&gl, 40.0f, groundY + 12.0f, 1240.0f, groundY + 12.0f);

        Pen tick(Color(Alpha(110, a), 246, 240, 226), 2.0f);
        float off = fmodf(p * 620.0f, 90.0f);
        for (int i = 0; i < 15; ++i) {
            float x = 40.0f + (float)i * 90.0f - off;
            if (x < 40.0f) continue;
            g.DrawLine(&tick, x, groundY + 12.0f, x - 22.0f, groundY + 12.0f);
        }
    }

    // 速度线（贴着跑道往后甩）
    {
        Pen sp(Color(Alpha(120, a), 250, 246, 232), 3.0f);
        sp.SetStartCap(LineCapRound);
        sp.SetEndCap(LineCapRound);
        for (int i = 0; i < 5; ++i) {
            float y = groundY - 46.0f + (float)i * 14.0f;
            float len = 50.0f + 70.0f * fabsf(sinf(p * 5.0f + (float)i));
            float x2 = 520.0f - (float)i * 16.0f;
            g.DrawLine(&sp, x2 - len, y, x2, y);
        }
    }

    // 小人（上下小幅起伏）
    float bob = 7.0f * sinf(p * 9.4f);
    DrawRunningChibi(g, 640.0f, groundY + 10.0f + bob, p * 9.4f, a);

    // 尘土
    {
        SolidBrush dust(Color(Alpha(90, a), 240, 232, 214));
        for (int i = 0; i < 5; ++i) {
            float k = fmodf(p * 1.6f + (float)i * 0.21f, 1.0f);
            float x = 520.0f - k * 150.0f;
            float y = groundY + 4.0f - k * 26.0f;
            float r = 8.0f + k * 16.0f;
            g.FillEllipse(&dust, x - r * 0.5f, y - r * 0.5f, r, r);
        }
    }

    // 顶部标题 + 剩余进度
    {
        const wchar_t* tip = L"跑步中……";
        DrawTextCenter(g, tip, *g_famTitle, FontStyleBold, 34.0f,
                       640.0f, 132.0f, 3.0f, Color(Alpha(255, a), 250, 244, 228), 1.4f);

        float bw = 420.0f, bh = 14.0f;
        RectF br(640.0f - bw * 0.5f, 176.0f, bw, bh);
        GraphicsPath bp;
        MakeRoundRect(bp, br, 7.0f);
        SolidBrush bg2(Color(Alpha(150, a), 0, 0, 0));
        g.FillPath(&bg2, &bp);
        float k = Clamp01(ms / (float)RUN_MS);
        GraphicsPath fp;
        MakeRoundRect(fp, RectF(br.X, br.Y, bw * k, bh), 7.0f);
        SolidBrush fg(Color(Alpha(240, a), 246, 214, 122));
        g.FillPath(&fg, &fp);
    }
}

// 进入操场：pe = 是否体育课
static void EnterPlayground(bool pe)
{
    g_loc = LOC_PLAY;
    g_playPE = pe;
    g_shopOpen = false;
    g_shopKind = 0;
    g_shopAnim = 0.0f;
    g_dlgSmall = false;
    g_dlgActive = true;
    g_dlgAskSubj = -1;
    g_dlgScriptN = 0;

    if (pe) {
        // 两句台词，播完弹「跑步吧 / 跑路了兄弟们」
        g_dlgAsk    = false;
        g_dlgScript = kPlayPEScript;
        g_dlgScriptN = 2;
        g_askPrompt = L"体育课该干些什么呢";
        g_dlgOptN   = 0;              // 第二十三步：同上，台词期间先收起选项
        g_askOptN   = 2;              // 台词播完才把 g_dlgOptN 置 2
        g_dlgOpt[0] = L"跑步吧";
        g_dlgOpt[1] = L"跑路了兄弟们";
        g_dlgAfter  = DLG_AFTER_ASK;
    } else {
        // 直接弹「要做些什么吗」+「跑步吧 / 离开」
        g_dlgAsk    = true;
        g_dlgScript = nullptr;
        g_dlgPrompt = L"要做些什么吗";
        g_dlgOptN   = 2;
        g_dlgOpt[0] = L"跑步吧";
        g_dlgOpt[1] = L"离开";
        g_dlgAfter  = DLG_AFTER_NONE;
    }
    DlgSetLine(0);
}

// 启动一个 5 秒动作动画，播完接着做 after（DN_*）
static void StartAnim(int kind, int after)
{
    g_runActive = true;
    g_animKind  = kind;
    g_animAfter = after;
    g_runStart  = GetTickCount64();
    g_dlgActive = false;
    g_dlgAsk    = false;
    g_dlgOptN   = 0;
    g_dlgScript = nullptr;
    g_dlgScriptN = 0;
    g_dlgAfter  = DLG_AFTER_NONE;
}

// ---------------- 第二十步：各动作的 5 秒演出 ----------------
//  统一成一套：整屏压暗 + 标题 + 进度条 + 一个小人做动作，播满 5 秒后接着做 g_animAfter
static void DrawAnimHud(Graphics& g, float a, float ms, const wchar_t* title)
{
    DrawTextCenter(g, title, *g_famTitle, FontStyleBold, 34.0f,
                   640.0f, 132.0f, 3.0f, Color(Alpha(255, a), 250, 244, 228), 1.4f);

    float bw = 420.0f, bh = 14.0f;
    RectF br(640.0f - bw * 0.5f, 176.0f, bw, bh);
    GraphicsPath bp;
    MakeRoundRect(bp, br, 7.0f);
    SolidBrush bg2(Color(Alpha(150, a), 0, 0, 0));
    g.FillPath(&bg2, &bp);
    float k = Clamp01(ms / (float)RUN_MS);
    GraphicsPath fp;
    MakeRoundRect(fp, RectF(br.X, br.Y, bw * k, bh), 7.0f);
    SolidBrush fg(Color(Alpha(240, a), 246, 214, 122));
    g.FillPath(&fg, &fp);
}

// 简笔小人：头像当脑袋，身子按姿势摆
//  pose 0=站/走  1=坐（吃饭）  2=躺（睡觉）  3=伏案（学习/熬夜）
static void DrawChibiPose(Graphics& g, float cx, float groundY, int pose,
                          float phase, float a, int prop)
{
    int hero = (g_chosen >= 0 && g_chosen < HERO_COUNT) ? g_chosen : 0;
    Bitmap* av = g_avatar[hero][g_faceState.cur];
    if (!av) av = g_avatar[hero][FACE_CALM];

    Pen ink(Color(Alpha(255, a), 58, 46, 38), 4.2f);
    ink.SetStartCap(LineCapRound);
    ink.SetEndCap(LineCapRound);
    ink.SetLineJoin(LineJoinRound);

    const float headD = 104.0f;
    float headCX = cx, headCY = groundY - 170.0f;
    if (pose == 2) { headCX = cx - 118.0f; headCY = groundY - 54.0f; }

    // ---- 影子 ----
    {
        SolidBrush sh(Color(Alpha(60, a), 30, 28, 26));
        g.FillEllipse(&sh, cx - 60.0f, groundY - 12.0f, 120.0f, 24.0f);
    }

    if (pose == 2) {
        // ---- 躺：一条被子 + 头 ----
        GraphicsPath bl;
        MakeRoundRect(bl, RectF(cx - 110.0f, groundY - 96.0f, 250.0f, 90.0f), 40.0f);
        SolidBrush quilt(Color(Alpha(255, a), 96, 128, 158));
        g.FillPath(&quilt, &bl);
        g.DrawPath(&ink, &bl);
        Pen stripe(Color(Alpha(120, a), 230, 236, 240), 3.0f);
        for (int i = 0; i < 4; ++i)
            g.DrawLine(&stripe, cx - 70.0f + i * 56.0f, groundY - 92.0f,
                                cx - 70.0f + i * 56.0f, groundY - 12.0f);
        // 枕头
        GraphicsPath pil;
        MakeRoundRect(pil, RectF(cx - 158.0f, groundY - 78.0f, 84.0f, 52.0f), 22.0f);
        SolidBrush pb(Color(Alpha(255, a), 244, 240, 230));
        g.FillPath(&pb, &pil);
        g.DrawPath(&ink, &pil);
        // Zzz
        for (int i = 0; i < 3; ++i) {
            float t2 = fmodf(phase * 0.55f + (float)i * 0.33f, 1.0f);
            wchar_t z[4] = { L'Z', 0, 0, 0 };
            if (i > 0) z[0] = (i == 1) ? L'z' : L'.';
            DrawTextCenter(g, z, *g_famUI, FontStyleBold, 22.0f + i * 8.0f,
                           headCX + 66.0f + t2 * 26.0f, headCY - 40.0f - t2 * 54.0f,
                           0.0f, Color((BYTE)(200 * (1.0f - t2) * a), 210, 230, 245), 0.0f);
        }
    } else if (pose == 1) {
        // ---- 坐：上身 + 盘着的腿 ----
        GraphicsPath body;
        MakeRoundRect(body, RectF(cx - 42.0f, groundY - 104.0f, 84.0f, 74.0f), 18.0f);
        SolidBrush shirt(Color(Alpha(255, a), 78, 112, 92));
        g.FillPath(&shirt, &body);
        g.DrawPath(&ink, &body);
        Pen leg(Color(Alpha(255, a), 64, 78, 96), 13.0f);
        g.DrawLine(&leg, cx - 26.0f, groundY - 34.0f, cx - 56.0f, groundY - 8.0f);
        g.DrawLine(&leg, cx + 26.0f, groundY - 34.0f, cx + 56.0f, groundY - 8.0f);
    } else if (pose == 3) {
        // ---- 伏案：上身 + 桌子 ----
        GraphicsPath body;
        MakeRoundRect(body, RectF(cx - 40.0f, groundY - 150.0f, 80.0f, 84.0f), 18.0f);
        SolidBrush shirt(Color(Alpha(255, a), 78, 112, 92));
        g.FillPath(&shirt, &body);
        g.DrawPath(&ink, &body);
        // 桌子
        GraphicsPath desk;
        MakeRoundRect(desk, RectF(cx - 170.0f, groundY - 74.0f, 340.0f, 22.0f), 8.0f);
        SolidBrush wood(Color(Alpha(255, a), 158, 116, 78));
        g.FillPath(&wood, &desk);
        g.DrawPath(&ink, &desk);
        Pen dleg(Color(Alpha(255, a), 120, 88, 60), 8.0f);
        g.DrawLine(&dleg, cx - 150.0f, groundY - 52.0f, cx - 150.0f, groundY - 6.0f);
        g.DrawLine(&dleg, cx + 150.0f, groundY - 52.0f, cx + 150.0f, groundY - 6.0f);
        if (prop == 0) {
            // 摊开的书
            GraphicsPath bk;
            MakeRoundRect(bk, RectF(cx - 66.0f, groundY - 92.0f, 132.0f, 18.0f), 4.0f);
            SolidBrush paper(Color(Alpha(255, a), 248, 244, 232));
            g.FillPath(&paper, &bk);
            g.DrawPath(&ink, &bk);
        } else {
            // 手机（熬夜玩）
            GraphicsPath ph;
            MakeRoundRect(ph, RectF(cx - 30.0f, groundY - 104.0f, 60.0f, 30.0f), 6.0f);
            SolidBrush glow(Color(Alpha(255, a), 130, 200, 245));
            g.FillPath(&glow, &ph);
            g.DrawPath(&ink, &ph);
            SolidBrush lg(Color(Alpha(90, a), 150, 220, 255));
            g.FillEllipse(&lg, cx - 90.0f, groundY - 150.0f, 180.0f, 90.0f);
        }
        // 台灯
        Pen lamp(Color(Alpha(220, a), 96, 96, 100), 5.0f);
        g.DrawLine(&lamp, cx + 120.0f, groundY - 74.0f, cx + 120.0f, groundY - 150.0f);
        PointF sh[3] = { PointF(cx + 120.0f, groundY - 152.0f),
                         PointF(cx + 70.0f, groundY - 128.0f),
                         PointF(cx + 138.0f, groundY - 128.0f) };
        SolidBrush lb(Color(Alpha(240, a), 246, 206, 120));
        g.FillPolygon(&lb, sh, 3);
        if (prop == 0) {
            SolidBrush lt(Color(Alpha(46, a), 255, 226, 150));
            PointF lm[3] = { PointF(cx + 104.0f, groundY - 128.0f),
                             PointF(cx + 30.0f, groundY - 90.0f),
                             PointF(cx + 168.0f, groundY - 90.0f) };
            g.FillPolygon(&lt, lm, 3);
        }
    } else {
        // ---- 站/走 ----
        float sw = sinf(phase);
        float bodyTop = groundY - 232.0f, bodyH = 66.0f, bodyW = 78.0f;
        GraphicsPath body;
        MakeRoundRect(body, RectF(cx - bodyW * 0.5f, bodyTop, bodyW, bodyH), 18.0f);
        SolidBrush shirt(Color(Alpha(255, a), 78, 112, 92));
        g.FillPath(&shirt, &body);
        g.DrawPath(&ink, &body);
        Pen leg(Color(Alpha(255, a), 64, 78, 96), 12.0f);
        g.DrawLine(&leg, cx - 16.0f, bodyTop + bodyH - 4.0f,
                         cx - 16.0f + sw * 26.0f, groundY - 8.0f);
        g.DrawLine(&leg, cx + 16.0f, bodyTop + bodyH - 4.0f,
                         cx + 16.0f - sw * 26.0f, groundY - 8.0f);
        headCY = bodyTop - headD * 0.5f + 16.0f;
    }

    // ---- 脑袋（除躺着之外都在这里画）----
    if (av) {
        GraphicsState st = g.Save();
        GraphicsPath clip;
        clip.AddEllipse(headCX - headD * 0.5f, headCY - headD * 0.5f, headD, headD);
        g.SetClip(&clip);
        float sc = headD * 1.40f;
        float iw = (float)av->GetWidth(), ih = (float)av->GetHeight();
        float k = sc / (iw < ih ? iw : ih);
        float dw = iw * k, dh = ih * k;
        g.DrawImage(av, RectF(headCX - dw * 0.5f, headCY - dh * 0.46f, dw, dh));
        g.Restore(st);
        Pen ring(Color(Alpha(255, a), 58, 46, 38), 3.4f);
        GraphicsPath ringP;
        ringP.AddEllipse(headCX - headD * 0.5f, headCY - headD * 0.5f, headD, headD);
        g.DrawPath(&ring, &ringP);
    }
}

// 吃饭：桌上摆碗 + 冒热气
static void DrawEatingProps(Graphics& g, float cx, float groundY, float phase, float a)
{
    Pen ink(Color(Alpha(255, a), 58, 46, 38), 3.6f);
    // 碗
    GraphicsPath bowl;
    MakeRoundRect(bowl, RectF(cx + 66.0f, groundY - 44.0f, 96.0f, 40.0f), 16.0f);
    SolidBrush rice(Color(Alpha(255, a), 246, 242, 232));
    g.FillPath(&rice, &bowl);
    g.DrawPath(&ink, &bowl);
    SolidBrush soup(Color(Alpha(255, a), 226, 196, 138));
    g.FillEllipse(&soup, cx + 74.0f, groundY - 42.0f, 80.0f, 16.0f);
    // 筷子（上下动）
    float k = sinf(phase * 4.2f) * 10.0f;
    Pen stick(Color(Alpha(255, a), 150, 110, 70), 4.0f);
    g.DrawLine(&stick, cx + 150.0f, groundY - 96.0f + k, cx + 96.0f, groundY - 46.0f);
    g.DrawLine(&stick, cx + 158.0f, groundY - 92.0f + k, cx + 104.0f, groundY - 42.0f);
    // 热气
    for (int i = 0; i < 3; ++i) {
        float t2 = fmodf(phase * 0.7f + (float)i * 0.33f, 1.0f);
        SolidBrush st(Color((BYTE)(110 * (1.0f - t2) * a), 250, 250, 244));
        g.FillEllipse(&st, cx + 92.0f + (float)i * 18.0f - 8.0f,
                      groundY - 60.0f - t2 * 70.0f, 18.0f, 18.0f);
    }
}

// 农田闲逛：一排庄稼 + 走动的影子
static void DrawFarmProps(Graphics& g, float groundY, float phase, float a)
{
    Pen stalk(Color(Alpha(200, a), 128, 160, 96), 4.0f);
    for (int i = 0; i < 14; ++i) {
        float x = 40.0f + (float)i * 95.0f + sinf(phase * 1.6f + (float)i) * 6.0f;
        float h = 70.0f + ((i * 37) % 40);
        g.DrawLine(&stalk, x, groundY - 6.0f, x, groundY - 6.0f - h);
        SolidBrush leaf(Color(Alpha(190, a), 146, 186, 108));
        g.FillEllipse(&leaf, x - 9.0f, groundY - 26.0f - h * 0.35f, 18.0f, 26.0f);
    }
}

// 各种动作的整屏演出
static void DrawPoseScene(Graphics& g)
{
    ULONGLONG now = GetTickCount64();
    float ms = (now > g_runStart) ? (float)(now - g_runStart) : 0.0f;
    float p  = ms / 1000.0f;
    float a  = Clamp01(ms / 200.0f);

    SolidBrush dim(Color(Alpha(130, a), 0, 0, 0));
    g.FillRectangle(&dim, 0.0f, 0.0f, (float)BASE_W, (float)BASE_H);

    float groundY = 560.0f;
    const wchar_t* title = L"";
    switch (g_animKind) {
    case ANIM_EAT:         title = L"吃饭中……";     break;
    case ANIM_NAP:         title = L"小睡中……";     break;
    case ANIM_SLEEP:       title = L"睡觉中……";     break;
    case ANIM_NIGHT_STUDY: title = L"熬夜学习中……"; break;
    case ANIM_NIGHT_GAME:  title = L"熬夜玩中……";   break;
    case ANIM_FARM_WALK:   title = L"农田闲逛中……"; break;
    default:               title = L"学习中……";     break;
    }

    // 地面
    {
        Pen gl(Color(Alpha(140, a), 246, 240, 226), 3.0f);
        g.DrawLine(&gl, 60.0f, groundY + 12.0f, 1220.0f, groundY + 12.0f);
    }

    if (g_animKind == ANIM_FARM_WALK) {
        DrawFarmProps(g, groundY, p, a);
        DrawChibiPose(g, 500.0f + sinf(p * 0.9f) * 120.0f, groundY, 0, p * 5.2f, a, 0);
    } else if (g_animKind == ANIM_EAT) {
        DrawChibiPose(g, 480.0f, groundY, 1, p, a, 0);
        DrawEatingProps(g, 480.0f, groundY, p, a);
    } else if (g_animKind == ANIM_NAP || g_animKind == ANIM_SLEEP) {
        DrawChibiPose(g, 640.0f, groundY, 2, p, a, 0);
    } else {
        // 学习 / 熬夜学习 / 熬夜玩：都是伏案
        int prop = (g_animKind == ANIM_NIGHT_GAME) ? 1 : 0;
        DrawChibiPose(g, 640.0f, groundY, 3, p, a, prop);
    }

    DrawAnimHud(g, a, ms, title);
}

// 总入口：跑步沿用原来的横版演出，其余走姿态演出
static void DrawActionScene(Graphics& g)
{
    if (!g_runActive) return;
    if (g_animKind == ANIM_RUN) { DrawRunScene(g, 0.0f); return; }
    DrawPoseScene(g);
}

static void PlayChoose(int idx)
{
    if (idx == 0) {
        // 跑步：加属性，然后播 5 秒小人跑步
        bool jike = (g_chosen == 1);
        if (g_playPE) AddStat(0, 20 + (jike ? 10 : 0));
        else          AddStat(0, 10 + (jike ?  5 : 0));
        AddStat(2, -10);                  // 精力（精神）-10
        g_faceState.Refresh(g_stat[2], g_stat[3]);
        g_dlgActive = false;
        g_dlgAsk    = false;
        g_dlgOptN   = 0;
        CommitDeltas(DN_RUN);             // 第十八步：确定后再开始跑步演出
    } else {
        // 跑路了兄弟们 / 离开：回地图
        g_dlgActive = false;
        g_dlgAsk    = false;
        g_dlgOptN   = 0;
        g_loc = LOC_CLASS;
        SetScene(SCENE_MAP);
    }
}

// ================= 第十七步：篮球场 + 投篮小游戏 =================
//  开发说明原文：
//    · 篮球场 = 地图 5 号区域，背景「篮球场界面.jpg」，保留游戏内 UI
//    · 点 5 号 → 篮球场界面，对话：我"练习投篮？" → 选项「来 / 溜了」
//    · 选「来」→ 打开投篮游戏：精神-10，健康+10*得分、快乐+10*得分（继科额外 +15 健康），
//      然后回地图并跳到下一个时间段
//    · 选「溜了」→ 回地图
//  投篮游戏移植自 D:\Cplus2\一周\投篮小游戏\投篮游戏.cpp：
//    第一人称持球，长按蓄力松手投出，共 3 次机会（静止 / 左右移动 / 全屏快速移动篮筐）
enum { BK_START = 0, BK_AIM, BK_FLY, BK_WAIT, BK_END };
static int   g_bkState    = BK_START;
static int   g_bkShot     = 0;               // 当前第几球 0/1/2
static int   g_bkScore    = 0;
static bool  g_bkCharging = false;
static float g_bkPower    = 0.15f;
static float g_bkPowerDir = 1.0f;
static int   g_bkWait     = 0;
static bool  g_bkLastScored = false;

struct BkBall { float x, y, vx, vy, r, angle; int bounces; bool scored; };
static BkBall g_bkBall = { 0, 0, 0, 0, 44, 0, 0, false };
static float g_bkPrevY = 0;
static std::vector<PointF> g_bkTrail;
static float g_bkHoopT  = 0;
static float g_bkHoopCx = 0;

static const float BK_RIM_HALF = 78.0f;       // 篮筐半径（比原版略放大，1280 画布上更好看）
static const float BK_GRAVITY  = 0.55f;
static const float BK_ANGLE    = 66.0f * 3.14159265f / 180.0f;

static float BkRimY()    { return (float)BASE_H * 0.32f; }
static float BkGroundY() { return (float)BASE_H - 90.0f; }
static float BkStartX()  { return (float)BASE_W * 0.30f; }
static float BkStartY()  { return (float)BASE_H - 135.0f; }

// 场景配色（柔和粉彩，跟小游戏一致）
static const Color BC_OUTLINE (255, 110, 120, 135);
static const Color BC_NET     (255, 250, 252, 250);
static const Color BC_RIM     (255, 235, 110,  55);
static const Color BC_POLE    (255,  85, 110, 120);
static const Color BC_BOARD   (255, 252, 252, 248);
static const Color BC_COURT   (255, 170, 182, 152);
static const Color BC_COURT_DK(255, 155, 167, 138);
static const Color BC_LINE_W  (255, 250, 252, 250);
static const Color BC_WALL    (255, 243, 239, 229);
static const Color BC_FENCE   (255,  90,  96, 108);

static void BkUpdateHoop()
{
    switch (g_bkShot) {
    case 0:                                          // 第 1 球：静止
        g_bkHoopCx = (float)BASE_W * 0.62f;
        break;
    case 1:                                          // 第 2 球：左右移动
        g_bkHoopT += 0.022f;
        g_bkHoopCx = (float)BASE_W * 0.5f + sinf(g_bkHoopT) * (float)BASE_W * 0.20f;
        break;
    default:                                         // 第 3 球：全屏快速移动
        g_bkHoopT += 0.034f;
        {
            float m = 90.0f;
            g_bkHoopCx = m + ((float)BASE_W - 2.0f * m) * (0.5f + 0.5f * sinf(g_bkHoopT));
        }
        break;
    }
}

static void BkShoot()
{
    // 1280 画布比原版 1024 宽，出手速度按 sqrt(宽度比) 补一点，保持原来的力度手感
    float s = (10.0f + 17.0f * g_bkPower) * 1.12f;
    g_bkBall.x = BkStartX();
    g_bkBall.y = BkStartY();
    g_bkBall.vx = s * cosf(BK_ANGLE);
    g_bkBall.vy = -s * sinf(BK_ANGLE);
    g_bkBall.bounces = 0;
    g_bkBall.scored = false;
    g_bkBall.angle = 0.0f;
    g_bkTrail.clear();
    g_bkState = BK_FLY;
}

static void BkEndShot(bool scored)
{
    g_bkLastScored = scored;
    g_bkWait  = 55;
    g_bkState = BK_WAIT;
}

static void BkStepBall()
{
    g_bkPrevY = g_bkBall.y;
    g_bkBall.vy += BK_GRAVITY;
    g_bkBall.x  += g_bkBall.vx;
    g_bkBall.y  += g_bkBall.vy;
    g_bkBall.angle += g_bkBall.vx * 0.03f;

    // 越远越小，营造纵深
    float rimY = BkRimY();
    float t = (g_bkBall.y - rimY) / ((float)BASE_H - rimY);
    t = fmaxf(0.0f, fminf(1.0f, t));
    g_bkBall.r = 22.0f + t * 24.0f;

    g_bkTrail.push_back(PointF(g_bkBall.x, g_bkBall.y));
    if (g_bkTrail.size() > 10) g_bkTrail.erase(g_bkTrail.begin());

    // 进球判定：下行穿越篮筐平面且落在筐内
    if (!g_bkBall.scored && g_bkPrevY < rimY && g_bkBall.y >= rimY && g_bkBall.vy > 0 &&
        g_bkBall.x > g_bkHoopCx - BK_RIM_HALF + 12 &&
        g_bkBall.x < g_bkHoopCx + BK_RIM_HALF - 12) {
        g_bkBall.scored = true;
        ++g_bkScore;
    }

    // 篮板碰撞
    float boardL = g_bkHoopCx + BK_RIM_HALF, boardR = boardL + 14.0f;
    float boardT = rimY - 130.0f, boardB = rimY + 12.0f;
    if (g_bkBall.x + g_bkBall.r > boardL && g_bkBall.x - g_bkBall.r < boardR &&
        g_bkBall.y > boardT && g_bkBall.y < boardB && g_bkBall.vx > 0) {
        g_bkBall.x = boardL - g_bkBall.r;
        g_bkBall.vx *= -0.6f;
    }

    // 篮筐前后沿
    for (int i = 0; i < 2; ++i) {
        float px = g_bkHoopCx + (i ? BK_RIM_HALF : -BK_RIM_HALF);
        float dx = g_bkBall.x - px, dy = g_bkBall.y - rimY;
        float d  = sqrtf(dx * dx + dy * dy);
        float rr = g_bkBall.r + 6.0f;
        if (d < rr && d > 0.01f) {
            float nx = dx / d, ny = dy / d;
            float dot = g_bkBall.vx * nx + g_bkBall.vy * ny;
            if (dot < 0) {
                g_bkBall.vx -= 2.0f * dot * nx;
                g_bkBall.vy -= 2.0f * dot * ny;
                g_bkBall.vx *= 0.65f;
                g_bkBall.vy *= 0.65f;
            }
            g_bkBall.x = px + nx * rr;
            g_bkBall.y = rimY + ny * rr;
        }
    }

    // 地面反弹
    float gy = BkGroundY();
    if (g_bkBall.y + g_bkBall.r > gy) {
        g_bkBall.y = gy - g_bkBall.r;
        if (fabsf(g_bkBall.vy) > 2.5f) {
            g_bkBall.vy *= -0.55f;
            g_bkBall.vx *= 0.75f;
            ++g_bkBall.bounces;
        } else {
            g_bkBall.vy = 0.0f;
            g_bkBall.vx *= 0.90f;
        }
    }

    // 本球结束
    if (g_bkBall.scored && g_bkBall.y > rimY + 70.0f)                       BkEndShot(true);
    else if (g_bkBall.bounces > 3)                                          BkEndShot(g_bkBall.scored);
    else if (g_bkBall.x > BASE_W + 80 || g_bkBall.x < -80 ||
             g_bkBall.y > BASE_H + 80)                                      BkEndShot(g_bkBall.scored);
}

static void BkNextShot()
{
    ++g_bkShot;
    if (g_bkShot > 2) { g_bkState = BK_END; return; }
    g_bkCharging = false;
    g_bkPower = 0.15f;
    g_bkPowerDir = 1.0f;
    g_bkTrail.clear();
    g_bkState = BK_AIM;
}

static void BkReset()
{
    g_bkShot = 0; g_bkScore = 0; g_bkHoopT = 0;
    g_bkHoopCx = (float)BASE_W * 0.62f;       // 开始页里篮筐也摆在正常位置
    g_bkCharging = false; g_bkPower = 0.15f; g_bkPowerDir = 1.0f;
    g_bkTrail.clear();
    g_bkState = BK_AIM;
}

// 每帧逻辑（WM_TIMER 里调）
static void BkUpdate()
{
    if (g_bkState == BK_AIM || g_bkState == BK_FLY) BkUpdateHoop();

    if (g_bkState == BK_AIM && g_bkCharging) {
        g_bkPower += 0.016f * g_bkPowerDir;
        if (g_bkPower >= 1.0f)   { g_bkPower = 1.0f;   g_bkPowerDir = -1.0f; }
        if (g_bkPower <= 0.15f)  { g_bkPower = 0.15f;  g_bkPowerDir =  1.0f; }
    }
    if (g_bkState == BK_FLY) BkStepBall();
    if (g_bkState == BK_WAIT && --g_bkWait <= 0) BkNextShot();
}

// 小游戏里的文字（沿用主程序的字体，带柔和描边）
static void BkText(Graphics& g, const wchar_t* s, float x, float y, float size,
                   Color col, int align)
{
    Font font(g_famUI, size, FontStyleBold, UnitPixel);
    StringFormat sf;
    RectF layout;
    if (align == 0)      { layout = RectF(x - 500.0f, y, 1000.0f, size * 2.2f); sf.SetAlignment(StringAlignmentCenter); }
    else if (align == 1) { layout = RectF(x, y, 700.0f, size * 2.2f);           sf.SetAlignment(StringAlignmentNear); }
    else                 { layout = RectF(x - 700.0f, y, 700.0f, size * 2.2f);  sf.SetAlignment(StringAlignmentFar); }
    SolidBrush outline(BC_OUTLINE);
    for (int dx = -2; dx <= 2; dx += 2)
        for (int dy = -2; dy <= 2; dy += 2)
            if (dx || dy) {
                RectF o = layout;
                o.X += (REAL)dx;
                o.Y += (REAL)dy;
                g.DrawString(s, -1, &font, o, &sf, &outline);
            }
    SolidBrush fill(col);
    g.DrawString(s, -1, &font, layout, &sf, &fill);
}

static void BkCloud(Graphics& g, float x, float y, float s)
{
    SolidBrush white(Color(255, 255, 255, 255));
    SolidBrush under(Color(255, 222, 236, 248));
    g.FillEllipse(&under, x - 52 * s, y + 2 * s, 104 * s, 26 * s);
    g.FillEllipse(&white, x - 50 * s, y - 14 * s, 44 * s, 30 * s);
    g.FillEllipse(&white, x - 26 * s, y - 26 * s, 52 * s, 42 * s);
    g.FillEllipse(&white, x +  2 * s, y - 16 * s, 46 * s, 34 * s);
    g.FillEllipse(&white, x - 40 * s, y -  4 * s, 84 * s, 24 * s);
}

static void BkTree(Graphics& g, float x, float baseY, float s)
{
    SolidBrush trunk(Color(255, 150, 122, 100));
    SolidBrush leafD(Color(255, 118, 168, 108));
    SolidBrush leafL(Color(255, 152, 196, 128));
    g.FillRectangle(&trunk, x - 5 * s, baseY - 34 * s, 10 * s, 34 * s);
    g.FillEllipse(&leafD, x - 30 * s, baseY - 78 * s, 60 * s, 52 * s);
    g.FillEllipse(&leafL, x - 20 * s, baseY - 88 * s, 38 * s, 36 * s);
    g.FillEllipse(&leafL, x -  2 * s, baseY - 72 * s, 30 * s, 30 * s);
}

static void BkBuilding(Graphics& g, float x, float y, float w, float h)
{
    SolidBrush body(Color(255, 238, 228, 208));
    SolidBrush roof(Color(255, 168, 172, 168));
    SolidBrush win (Color(255, 178, 208, 224));
    SolidBrush winD(Color(255, 150, 184, 204));
    g.FillRectangle(&body, x, y, w, h);
    g.FillRectangle(&roof, x - 6, y - 10, w + 12, 12.0f);
    for (int r = 0; r < 2; ++r)
        for (int c = 0; c < (int)(w / 52); ++c) {
            float wx = x + 14 + c * 52, wy = y + 16 + r * (h - 34);
            g.FillRectangle(&win,  wx, wy, 28.0f, 34.0f);
            g.FillRectangle(&winD, wx + 20, wy, 8.0f, 34.0f);
        }
}

static void BkFence(Graphics& g, float y1, float y2)
{
    Pen bar(BC_FENCE, 3.0f);
    Pen mesh(Color(255, 110, 115, 126), 1.6f);
    Pen post(BC_FENCE, 5.0f);
    g.DrawLine(&bar, 0.0f, y1, (float)BASE_W, y1);
    g.DrawLine(&bar, 0.0f, y2, (float)BASE_W, y2);
    for (float x = 8; x < (float)BASE_W; x += 22)
        g.DrawLine(&mesh, x, y1, x, y2);
    for (float x = 0; x <= (float)BASE_W; x += 130)
        g.DrawLine(&post, x, y1 - 8, x, y2 + 6);
}

static void BkDrawBall(Graphics& g, float x, float y, float r, float angle)
{
    SolidBrush orange(Color(255, 238, 126, 46));
    Pen outline(Color(255, 160, 85, 35), 2.5f);
    g.FillEllipse(&orange, x - r, y - r, r * 2, r * 2);
    g.DrawEllipse(&outline, x - r, y - r, r * 2, r * 2);
    g.TranslateTransform(x, y);
    g.RotateTransform(angle);
    g.DrawArc(&outline, -r, -r, 2 * r, 2 * r, 20.0f, 140.0f);
    g.DrawArc(&outline, -r, -r, 2 * r, 2 * r, 200.0f, 140.0f);
    g.DrawLine(&outline, -r, 0.0f, r, 0.0f);
    g.ResetTransform();
}

static void BkDrawBG(Graphics& g)
{
    float gy = BkGroundY();
    ULONGLONG tick = GetTickCount64();

    LinearGradientBrush sky(Point(0, 0), Point(0, (INT)(BASE_H * 0.55f)),
                            Color(255, 100, 172, 235), Color(255, 196, 228, 248));
    g.FillRectangle(&sky, 0.0f, 0.0f, (float)BASE_W, BASE_H * 0.55f);

    SolidBrush halo(Color(90, 255, 250, 215));
    SolidBrush sun (Color(255, 255, 248, 220));
    g.FillEllipse(&halo, (float)BASE_W - 190.0f, 22.0f, 130.0f, 130.0f);
    g.FillEllipse(&sun,  (float)BASE_W - 165.0f, 47.0f, 80.0f, 80.0f);

    BkCloud(g, 210, 110, 1.2f);
    BkCloud(g, 620,  70, 0.9f);
    BkCloud(g, 950, 150, 1.0f);
    BkCloud(g, 400, 185, 0.7f);

    BkBuilding(g, -30,  168, 300, 130);
    BkBuilding(g, 880,  178, 360, 120);

    for (int i = 0; i < 9; ++i)
        BkTree(g, 70 + i * 140, 300, 0.9f + ((i * 37) % 10) * 0.04f);

    // 奶白围墙 + 黑色铁栅栏
    float wallTop = 262.0f, wallBot = 344.0f;
    SolidBrush wallB(BC_WALL);
    g.FillRectangle(&wallB, 0.0f, wallTop, (float)BASE_W, wallBot - wallTop);
    SolidBrush cap(Color(255, 228, 223, 212));
    g.FillRectangle(&cap, 0.0f, wallTop - 6, (float)BASE_W, 8.0f);
    Pen seam(Color(255, 224, 218, 207), 2.0f);
    for (float x = 85; x < (float)BASE_W; x += 170)
        g.DrawLine(&seam, x, wallTop, x, wallBot);
    g.DrawLine(&seam, 0.0f, wallBot - 4, (float)BASE_W, wallBot - 4);
    BkFence(g, wallTop + 14, wallBot + 2);

    // 球场地面
    SolidBrush courtDk(BC_COURT_DK);
    g.FillRectangle(&courtDk, 0.0f, wallBot, (float)BASE_W, 26.0f);
    SolidBrush courtB(BC_COURT);
    g.FillRectangle(&courtB, 0.0f, wallBot + 26, (float)BASE_W, (float)BASE_H - wallBot - 26);
    Pen lineW(BC_LINE_W, 3.0f);
    g.DrawLine(&lineW, 0.0f, wallBot + 30, (float)BASE_W, wallBot + 30);
    g.DrawArc(&lineW, BkStartX() - 150, gy - 46, 300.0f, 92.0f, 180.0f, 180.0f);
    g.DrawEllipse(&lineW, (float)BASE_W / 2.0f - 190, (float)BASE_H + 40, 380.0f, 150.0f);
    SolidBrush gleam(Color(26, 255, 255, 235));
    PointF g1[3] = { PointF(120, (float)BASE_H), PointF(320, wallBot + 26), PointF(400, wallBot + 26) };
    g.FillPolygon(&gleam, g1, 3);
    PointF g2[3] = { PointF(620, (float)BASE_H), PointF(800, wallBot + 26), PointF(860, wallBot + 26) };
    g.FillPolygon(&gleam, g2, 3);

    // 拖尾
    if (g_bkState == BK_FLY && g_bkTrail.size() > 1) {
        for (size_t i = 0; i < g_bkTrail.size(); ++i) {
            int a = (int)(90 * i / g_bkTrail.size());
            SolidBrush tb(Color(a, 255, 240, 200));
            float rr = 3.0f + 2.5f * i / g_bkTrail.size();
            g.FillEllipse(&tb, g_bkTrail[i].X - rr, g_bkTrail[i].Y - rr, rr * 2, rr * 2);
        }
    }
    // 球的影子
    if (g_bkState == BK_FLY || g_bkState == BK_WAIT) {
        float sw = g_bkBall.r * 1.3f;
        SolidBrush shadow(Color(45, 90, 110, 90));
        g.FillEllipse(&shadow, g_bkBall.x - sw, gy - 8, sw * 2, 13.0f);
    }
    (void)tick;
}

static void BkDrawHoopBack(Graphics& g)
{
    float rimY = BkRimY();
    float bx = g_bkHoopCx + BK_RIM_HALF;
    Pen po(BC_OUTLINE, 2.5f);
    Pen pole(BC_POLE, 10.0f);
    g.DrawLine(&pole, bx + 7, rimY + 10, bx + 7, BkGroundY());
    g.DrawLine(&po,   bx + 7, rimY + 10, bx + 7, BkGroundY());
    SolidBrush board(BC_BOARD);
    g.FillRectangle(&board, bx, rimY - 130, 14.0f, 142.0f);
    g.DrawRectangle(&po, bx, rimY - 130, 14.0f, 142.0f);
    Pen inner(Color(255, 225, 130, 100), 2.0f);
    g.DrawRectangle(&inner, bx + 3, rimY - 46, 8.0f, 42.0f);
}

static void BkDrawHoopFront(Graphics& g)
{
    float rimY = BkRimY();
    ULONGLONG tick = GetTickCount64();
    float sway = g_bkBall.scored ? sinf((float)tick * 0.02f) * 6.0f
                                 : sinf((float)tick * 0.004f) * 2.0f;
    Pen netPen(BC_NET, 2.2f);
    for (int i = 0; i <= 4; ++i) {
        float tx = g_bkHoopCx - BK_RIM_HALF + i * (BK_RIM_HALF * 2 / 4.0f);
        float bxx = g_bkHoopCx + (tx - g_bkHoopCx) * 0.35f + sway;
        g.DrawLine(&netPen, tx, rimY, bxx, rimY + 52);
    }
    g.DrawLine(&netPen, g_bkHoopCx - BK_RIM_HALF * 0.35f + sway, rimY + 52,
                        g_bkHoopCx + BK_RIM_HALF * 0.35f + sway, rimY + 52);
    Pen rim(BC_RIM, 6.5f);
    g.DrawLine(&rim, g_bkHoopCx - BK_RIM_HALF, rimY, g_bkHoopCx + BK_RIM_HALF, rimY);
}

static void BkDrawHandsAndBall(Graphics& g)
{
    ULONGLONG tick = GetTickCount64();
    float bx = BkStartX(), by = BkStartY();
    if (g_bkCharging) {
        by += g_bkPower * 16;
        bx -= g_bkPower * 8;
        by += sinf((float)tick * 0.04f) * 2.5f * g_bkPower;
    }
    float r = 46.0f;
    SolidBrush shadow(Color(45, 90, 110, 90));
    g.FillEllipse(&shadow, bx - r, BkGroundY() - 10, r * 2, 15.0f);
    BkDrawBall(g, bx, by, r, (float)tick * 0.005f);

    Pen armOut(Color(255, 150, 110, 95), 28.0f);
    Pen armIn (Color(255, 255, 216, 182), 21.0f);
    PointF l1(bx - 150, (float)BASE_H + 30), l2(bx - r * 0.55f, by + r * 0.45f);
    PointF r1(bx + 150, (float)BASE_H + 30), r2(bx + r * 0.55f, by + r * 0.45f);
    g.DrawLine(&armOut, l1, l2); g.DrawLine(&armIn, l1, l2);
    g.DrawLine(&armOut, r1, r2); g.DrawLine(&armIn, r1, r2);
    Pen po(Color(255, 160, 105, 90), 2.5f);
    SolidBrush skin(Color(255, 255, 216, 182));
    g.FillEllipse(&skin, l2.X - 15, l2.Y - 15, 30.0f, 30.0f);
    g.DrawEllipse(&po,   l2.X - 15, l2.Y - 15, 30.0f, 30.0f);
    g.FillEllipse(&skin, r2.X - 15, r2.Y - 15, 30.0f, 30.0f);
    g.DrawEllipse(&po,   r2.X - 15, r2.Y - 15, 30.0f, 30.0f);

    // 蓄力条
    float px = bx - 85, py = by + 66, pw = 170, ph = 20;
    SolidBrush bgB(Color(200, 55, 70, 85));
    g.FillRectangle(&bgB, px, py, pw, ph);
    Color pc = g_bkPower < 0.5f ? Color(255, 140, 215, 130)
             : g_bkPower < 0.8f ? Color(255, 250, 205, 100)
                                : Color(255, 242, 130, 105);
    SolidBrush pb(pc);
    g.FillRectangle(&pb, px + 3, py + 3, (pw - 6) * g_bkPower, ph - 6);
    Pen pp(BC_OUTLINE, 2.5f);
    g.DrawRectangle(&pp, px, py, pw, ph);
    BkText(g, L"蓄力", px + pw / 2, py + 24, 17, Color(255, 70, 85, 105), 0);
}

static void BkDrawHUD(Graphics& g)
{
    wchar_t buf[64];
    swprintf(buf, 64, L"第 %d / 3 球", g_bkShot + 1);
    BkText(g, buf, 24, 24, 25, Color(255, 255, 255, 255), 1);
    swprintf(buf, 64, L"得分: %d", g_bkScore);
    BkText(g, buf, 24, 58, 25, Color(255, 255, 230, 140), 1);
    const wchar_t* hint[3] = { L"篮筐：静止", L"篮筐：左右移动", L"篮筐：全屏快速移动！" };
    BkText(g, hint[g_bkShot], (float)BASE_W - 24, 24, 23, Color(255, 255, 255, 255), 2);
    BkText(g, g_bkCharging ? L"松开投篮！" : L"按住鼠标蓄力",
           (float)BASE_W - 24, 58, 21, Color(255, 255, 230, 140), 2);
}

static void BkDrawScene(Graphics& g)
{
    BkDrawBG(g);
    BkDrawHoopBack(g);
    if (g_bkState == BK_FLY || g_bkState == BK_WAIT)
        BkDrawBall(g, g_bkBall.x, g_bkBall.y, g_bkBall.r, g_bkBall.angle);
    BkDrawHoopFront(g);
    if (g_bkState == BK_AIM) BkDrawHandsAndBall(g);
    if (g_bkState == BK_AIM || g_bkState == BK_FLY || g_bkState == BK_WAIT) BkDrawHUD(g);

    float cw = (float)BASE_W * 0.5f;
    if (g_bkState == BK_WAIT) {
        if (g_bkLastScored) BkText(g, L"球进了！ +1 分", cw, (float)BASE_H * 0.42f, 54, Color(255, 255, 225, 130), 0);
        else                BkText(g, L"没进……",        cw, (float)BASE_H * 0.42f, 46, Color(255, 250, 250, 250), 0);
    }
    if (g_bkState == BK_START) {
        SolidBrush ov(Color(140, 35, 55, 85));
        g.FillRectangle(&ov, 0.0f, 0.0f, (float)BASE_W, (float)BASE_H);
        BkText(g, L"投 篮", cw, (float)BASE_H * 0.16f, 96, Color(255, 255, 255, 255), 0);
        BkText(g, L"— 高中校园 · 室外篮球场 —", cw, (float)BASE_H * 0.38f, 28, Color(255, 235, 245, 250), 0);
        BkText(g, L"按住鼠标左键蓄力，松开投篮（力度会来回摆动，看准时机！）", cw, (float)BASE_H * 0.48f, 24, Color(255, 240, 245, 248), 0);
        BkText(g, L"共 3 次机会：静止篮筐 → 左右移动 → 全屏快速移动", cw, (float)BASE_H * 0.55f, 24, Color(255, 240, 245, 248), 0);
        BkText(g, L"每进一球得 1 分，打板反弹也能进哦", cw, (float)BASE_H * 0.62f, 24, Color(255, 240, 245, 248), 0);
        if (((GetTickCount64() / 500) % 2) == 0)
            BkText(g, L"— 点击鼠标开始 —", cw, (float)BASE_H * 0.74f, 34, Color(255, 255, 230, 140), 0);
    }
    if (g_bkState == BK_END) {
        SolidBrush ov(Color(140, 35, 55, 85));
        g.FillRectangle(&ov, 0.0f, 0.0f, (float)BASE_W, (float)BASE_H);
        BkText(g, L"比赛结束！", cw, (float)BASE_H * 0.22f, 64, Color(255, 255, 255, 255), 0);
        wchar_t buf[32];
        swprintf(buf, 32, L"最终得分: %d / 3", g_bkScore);
        BkText(g, buf, cw, (float)BASE_H * 0.40f, 42, Color(255, 255, 230, 140), 0);
        const wchar_t* comment =
            g_bkScore >= 3 ? L"神射手！百发百中！" :
            g_bkScore == 2 ? L"相当不错，就差一点！" :
            g_bkScore == 1 ? L"进了就好，继续练！" : L"热身而已，再来一局！";
        BkText(g, comment, cw, (float)BASE_H * 0.52f, 28, Color(255, 240, 245, 248), 0);
        if (((GetTickCount64() / 500) % 2) == 0)
            BkText(g, L"— 点击鼠标结算并返回 —", cw, (float)BASE_H * 0.66f, 30, Color(255, 255, 230, 140), 0);
    }
}

// 投篮结束：结算属性、回地图、跳一个时间段
static void FinishBasket()
{
    bool jike = (g_chosen == 1);
    g_basketActive = false;

    AddStat(2, -10);                                    // 精力 -10
    AddStat(0, 10 * g_bkScore + (jike ? 15 : 0));       // 健康 +10*得分（继科额外 +15）
    AddStat(3, 10 * g_bkScore);                         // 快乐 +10*得分
    g_faceState.Refresh(g_stat[2], g_stat[3]);

    g_loc = LOC_CLASS;
    CommitDeltas(DN_BASKET_END);                        // 第十八步：确定后再推时段回地图
}

// 进入篮球场：对话「练习投篮？」→ 来 / 溜了
static void EnterCourt()
{
    g_loc = LOC_COURT;
    g_shopOpen = false;
    g_shopKind = 0;
    g_shopAnim = 0.0f;
    g_dlgSmall  = false;
    g_dlgActive = true;
    g_dlgAsk    = true;
    g_dlgAskSubj = -1;
    g_dlgPrompt = L"练习投篮？";
    g_dlgScript = nullptr;
    g_dlgScriptN = 0;
    g_dlgAfter  = DLG_AFTER_NONE;
    g_dlgOptN   = 2;
    g_dlgOpt[0] = L"来";
    g_dlgOpt[1] = L"溜了";
    DlgSetLine(0);
}

static void CourtChoose(int idx)
{
    if (idx == 0) {
        // 来：打开投篮游戏
        g_dlgActive = false;
        g_dlgAsk    = false;
        g_dlgOptN   = 0;
        g_basketActive = true;
        g_bkState = BK_START;
        g_bkShot = 0;
        g_bkScore = 0;
        g_bkHoopT = 0;
        g_bkHoopCx = (float)BASE_W * 0.62f;
        g_bkCharging = false;
        g_bkPower = 0.15f;
        g_bkPowerDir = 1.0f;
        g_bkTrail.clear();
    } else {
        // 溜了：回地图
        g_dlgActive = false;
        g_dlgAsk    = false;
        g_dlgOptN   = 0;
        g_loc = LOC_CLASS;
        SetScene(SCENE_MAP);
    }
}

// ================= 第二十步：事件1 课外实践活动 =================
//  开发说明原文：
//    · day2 上午第 5 节（time 5）跳转到「课外事件活动1.jpg」，保留第四步 UI
//    · 对话：我"今天课外实践活动在外面做饭" >继续
//            我"我们班居然没准备食材"     >继续
//            我"该怎么办呢"               >继续
//            → 选择物品栏里的一个物品
//    · 选「小卖部饼干 / 压缩饼干 / 咖啡」→ 正常使用（物品消失）
//    · 选「物理白皮」→ 铁锅炖白皮成就（后续完善）、书消失、快乐回满、知识+50，
//                     背景换成「课外实践活动2.jpg」
//    · 选其它物品 → 物品不消失
//    · 结尾对话：我"好在后来家长送来了食物" → 饱食+30 → 回地图并跳到中午午时（time 8）
static Bitmap* g_event1Src = nullptr;      // 课外事件活动1.jpg
static Bitmap* g_event1Src2 = nullptr;     // 课外实践活动2.jpg
static Bitmap* g_event1Bg  = nullptr;
static int     g_event1BgW = 0, g_event1BgH = 0;
static const DlgLine kEvent1Script[3] = {
    { L"我", L"今天课外实践活动在外面做饭", ITEM_NONE },
    { L"我", L"我们班居然没准备食材",       ITEM_NONE },
    { L"我", L"该怎么办呢",                 ITEM_NONE }
};
static const DlgLine kEvent1End[1] = {
    { L"我", L"好在后来家长送来了食物", ITEM_NONE }
};

static void EnterEvent1()
{
    g_loc = LOC_EVENT1;
    g_event1Second = false;
    g_event1Done   = true;
    g_pickItemMode = false;
    g_shopOpen = false;
    g_shopKind = 0;
    g_shopAnim = 0.0f;
    g_dlgSmall  = false;
    g_dlgActive = true;
    g_dlgAsk    = false;
    g_dlgAskSubj = -1;
    g_dlgOptN   = 0;
    g_dlgScript = kEvent1Script;
    g_dlgScriptN = 3;
    g_dlgAfter  = DLG_AFTER_EVENT_PICK;
    DlgSetLine(0);
}

// 台词播完：把对话框收起来，让玩家点物品栏
static void Event1BeginPick()
{
    g_dlgActive = false;
    g_dlgAsk    = false;
    g_dlgOptN   = 0;

    // 背包里一件东西都没有就直接走结尾
    bool any = false;
    for (int i = 1; i < SLOT_N; ++i) if (g_inv[i] != ITEM_EMPTY) any = true;
    if (!any) { Event1Finish(); return; }
    g_pickItemMode = true;
}

// 玩家点了物品栏第 slot 格
static void Event1Pick(int slot)
{
    if (!g_pickItemMode) return;
    if (slot < 1 || slot >= SLOT_N) return;
    int kind = g_inv[slot];
    if (kind == ITEM_EMPTY) return;

    g_pickItemMode = false;
    bool consumed = false;

    if (kind == ITEM_BISCUIT) {
        // 小卖部饼干：正常使用
        AddStat(1, 40); AddStat(0, -5); AddStat(3, -5);
        consumed = true;
    } else if (kind == ITEM_CRACKER) {
        // 压缩饼干：正常使用
        AddStat(1, 40); AddStat(0, -10); AddStat(3, -20);
        consumed = true;
    } else if (kind == ITEM_COFFEE) {
        // 咖啡：正常使用
        AddStat(2, CoffeeSpirit());
        AddStat(0, -10);
        consumed = true;
    } else if (kind == ITEM_BOOK_PHYS) {
        // 物理白皮 → 铁锅炖白皮
        AddStat(3, 100 - g_stat[3]);       // 快乐回复至 100
        AddStat(4, 50);                    // 知识 +50
        consumed = true;
        g_event1Second = true;             // 换成「课外实践活动2.jpg」
        UnlockAch(ACH_TIEGUO);                  // 第二十二步：正式记进成就系统
    }

    if (consumed) g_inv[slot] = ITEM_EMPTY;   // 其它物品不消失
    g_faceState.Refresh(g_stat[2], g_stat[3]);

    // 接着播结尾台词
    g_dlgActive   = true;
    g_dlgAsk      = false;
    g_dlgSmall    = false;
    g_dlgScript   = kEvent1End;
    g_dlgScriptN  = 1;
    g_dlgAfter    = DLG_AFTER_EVENT_END;
    DlgSetLine(0);
}

// 结尾台词播完：饱食 +30，弹属性变化，确定后回地图并跳到中午午时
static void Event1Finish()
{
    g_pickItemMode = false;
    g_dlgActive    = false;
    g_dlgAsk       = false;
    g_dlgOptN      = 0;
    AddStat(1, 30);                    // 饱食 +30
    g_faceState.Refresh(g_stat[2], g_stat[3]);
    CommitDeltas(DN_EVENT1_END);
}

static void BuildEvent1Bg(int W, int H)
{
    Image* src = g_event1Second ? (Image*)g_event1Src2 : (Image*)g_event1Src;
    static Image* lastSrc = nullptr;
    static int    cw = 0, ch = 0;
    if (g_event1Bg && cw == W && ch == H && lastSrc == src) return;
    if (lastSrc != src) {                  // 换图源要强制重建
        delete g_event1Bg;
        g_event1Bg = nullptr;
    }
    BuildDimBg(src, &g_event1Bg, &cw, &ch, W, H);
    lastSrc = src;
}

// ================= 第二十步：事件2 北京名师讲高考 =================
//  开发说明第二十步原文：day2 下午第一节强制进「讲高考.jpg」，讲到下午第四节；
//  每听完一节弹一次「跑路 / 继续听」，跑路直接回地图，听完最后一段：
//  我「ZZZ」→ 旁白「你睡着了」→ 回地图，时间变为晚饭前，精神 +30、知识 -50（不低于 0）
static Bitmap* g_gaokaoSrc = nullptr;
static Bitmap* g_gaokaoBg  = nullptr;
static int     g_gaokaoBgW = 0, g_gaokaoBgH = 0;

// 四段台词（开发说明原文，表情符号照样写进台词里；画的时候单独用 emoji 字体）
//   第 0 段 = 下午一节（time 10），第 1 段 = 下午二节，第 2 段 = 下午三节，第 3 段 = 下午四节
static const DlgLine kGaokaoSeg0[6] = {
    { L"名师", L"同学们，我今天从北京飞过来给你们讲高考", ITEM_NONE },
    { L"名师", L"我们的对象是什么呀？\U0001F9D0", ITEM_NONE },
    { L"名师", L"\U0001F914是在师范大学接受呢，呃，这个，呃，这个师范大学教育的", ITEM_NONE },
    { L"名师", L"接受呢这个文学语言专业培养的老师们！", ITEM_NONE },
    { L"名师", L"所以这个啊，我得很严肃的!", ITEM_NONE },
    { L"名师", L"\U0001F624不是说那种题就不行。嗯，不信你给我搞几个题来！", ITEM_NONE }
};
static const DlgLine kGaokaoSeg1[6] = {
    { L"名师", L"我我跟，我跟你，呃，我我跟你这个品一品！\U0001F50D", ITEM_NONE },
    { L"名师", L"普遍的不好！❌ 普遍的假大空套！", ITEM_NONE },
    { L"名师", L"\U0001F612\U0001F4A8 而且当全国这么流行的时候", ITEM_NONE },
    { L"名师", L"浙江是高考大省啊！\U0001F4DA\U0001F525 啊！", ITEM_NONE },
    { L"名师", L"浙江如果说有百分之六十以上的人都写那种标题的话\U0001F4CA", ITEM_NONE },
    { L"名师", L"那一年的高考作文就要泡汤了！\U0001F4A5\U0001F62D", ITEM_NONE }
};
static const DlgLine kGaokaoSeg2[6] = {
    { L"名师", L"可是满分作文标题都……\U0001F92B", ITEM_NONE },
    { L"名师", L"所以，说到这个自拟题目\U0001F4D6", ITEM_NONE },
    { L"名师", L"自拟题目是有方法的！☝️", ITEM_NONE },
    { L"名师", L"是有专业讲究的！\U0001F393", ITEM_NONE },
    { L"名师", L"我的基本讲，这个讲法就是：让你的标题，让阅卷老师亮瞎眼！\U0001F440✨\U0001F4A5", ITEM_NONE },
    { L"名师", L"但绝不是那种口号式的！\U0001F6AB\U0001F4E2", ITEM_NONE }
};
static const DlgLine kGaokaoSeg3[5] = {
    { L"名师", L"哦，绝不是那种，特别是口号式的对联标题！❌\U0001F9E7", ITEM_NONE },
    { L"名师", L"啊！好的，回到这里来！\U0001F519\U0001F4D6", ITEM_NONE },
    { L"名师", L"呃，至于规限制性要求……\U0001F4DD", ITEM_NONE },
    { L"我",   L"ZZZ", ITEM_NONE },
    { L"旁白", L"你睡着了", ITEM_NONE }
};
static const DlgLine* kGaokaoSegPtr[4] = { kGaokaoSeg0, kGaokaoSeg1, kGaokaoSeg2, kGaokaoSeg3 };
static const int      kGaokaoSegN[4]   = { 6, 6, 6, 5 };

// 播第 seg 段台词
static void GaokaoPlaySeg(int seg)
{
    if (seg < 0) seg = 0;
    if (seg > 3) seg = 3;
    g_gaokaoSeg  = seg;
    g_shopOpen   = false;
    g_shopKind   = 0;
    g_shopAnim   = 0.0f;
    g_dlgSmall   = false;
    g_dlgActive  = true;
    g_dlgAsk     = false;
    g_dlgAskSubj = -1;
    g_dlgOptN    = 0;
    g_dlgScript  = kGaokaoSegPtr[seg];
    g_dlgScriptN = kGaokaoSegN[seg];
    // 最后一段播完就结算，前面几段播完推一节并弹「跑路 / 继续听」
    g_dlgAfter   = (seg >= 3) ? DLG_AFTER_GAOKAO_END : DLG_AFTER_GAOKAO_SEG;
    DlgSetLine(0);
}

// 听完一节：弹「跑路 / 继续听」（界面不动，还是讲高考那张图）
static void GaokaoAsk()
{
    g_dlgSmall   = false;
    g_dlgActive  = true;
    g_dlgAsk     = true;
    g_dlgAskSubj = -1;
    g_dlgPrompt  = L"名师还在讲……";   // 左边垫一句状态提示，免得对话框空一半
    g_dlgScript  = nullptr;
    g_dlgScriptN = 0;
    g_dlgAfter   = DLG_AFTER_NONE;
    g_dlgOptN    = 2;
    g_dlgOpt[0]  = L"跑路";
    g_dlgOpt[1]  = L"继续听";
    DlgSetLine(0);
}

// 选了「跑路 0 / 继续听 1」
static void GaokaoChoose(int idx)
{
    g_dlgAsk    = false;
    g_dlgOptN   = 0;
    if (idx == 0) {                       // 跑路：直接回地图，时间停在刚听完的那一节
        g_dlgActive = false;
        SetScene(SCENE_MAP);
        return;
    }
    GaokaoPlaySeg(g_gaokaoSeg + 1);        // 继续听：接着播下一段
}

// 全部听完：精神 +30、知识 -50（不低于 0）→ 回地图，时间变为晚饭前
static void GaokaoFinish()
{
    g_dlgActive = false;
    g_dlgAsk    = false;
    g_dlgOptN   = 0;
    AddStat(2, 30);                                   // 精神 +30
    int know = g_stat[4] - 50;
    if (know < 0) know = 0;                           // 知识 -50，不低于 0
    AddStat(4, know - g_stat[4]);
    g_faceState.Refresh(g_stat[2], g_stat[3]);
    CommitDeltas(DN_GAOKAO_END);
}

static void EnterGaokao()
{
    g_loc = LOC_GAOKAO;
    g_gaokaoDone = true;
    g_gaokaoSeg  = 0;
    g_time = 10;                 // 一律从下午第一节开始讲（中途进来也不会少听一段）
    GaokaoPlaySeg(0);
}

static void BuildGaokaoBg(int W, int H)
{
    BuildDimBg(g_gaokaoSrc, &g_gaokaoBg, &g_gaokaoBgW, &g_gaokaoBgH, W, H);
}

// ================= 第十四步：行政楼与图书馆（书屋） =================
//  · 地图点 6 号先弹确认框「是否前往」，是才去；进入不消耗时间，离开时才跳一个时段
//  · 对话框右边两个选项：购买 / 离开（离开回地图并跳到下一个时间点）
//  · 点购买 → 屏幕中间「书屋」，五本教辅全部进背包
//  · 右键点背包里的书 → 弹出「是否确认扔掉这本书」
// 第二十三步：书屋的「购买 / 离开」同样抽成函数（关闭商店后重新显示）
static void BookAsk()
{
    g_dlgSmall   = false;
    g_dlgActive  = true;
    g_dlgAsk     = true;
    g_dlgAskSubj = -1;
    g_dlgPrompt  = L"";
    g_dlgScript  = nullptr;
    g_dlgScriptN = 0;
    g_dlgAfter   = DLG_AFTER_NONE;
    g_dlgOptN    = 2;
    g_dlgOpt[0]  = L"购买";
    g_dlgOpt[1]  = L"离开";
    DlgSetLine(0);
}

static void EnterBookHouse()
{
    g_loc = LOC_BOOK;
    g_shopOpen = false;
    g_shopKind = 1;
    g_shopAnim = 0.0f;
    g_dlgSmall  = false;
    BookAsk();
}

static void LeaveBookHouse()
{
    g_shopOpen = false;
    g_shopKind = 0;
    g_shopAnim = 0.0f;
    g_dlgActive = false;
    g_dlgAsk    = false;
    g_dlgOptN   = 0;
    g_loc = LOC_CLASS;
    SetScene(SCENE_MAP);
    AdvanceTime();          // 离开会跳到下一个时间点
}

// 买书屋里的第 i 本书
static void BuyBook(int idx)
{
    if (idx < 0 || idx >= BOOK_N) return;
    const BookItem& b = kBooks[idx];

    if (g_money < b.price) { CenterMsg(L"预算不足"); return; }
    if (InvFreeSlot() < 0) { CenterMsg(L"背包已满"); return; }

    g_money -= b.price;
    int slot = InvFreeSlot();
    if (slot >= 0) g_inv[slot] = b.itemId;
    CenterMsgBuy(b.name);           // 第二十三步：「已经购买XX」停 1 秒
}

// 右键扔书：先弹确认
static void AskThrowBook(int slot)
{
    if (slot < 1 || slot >= SLOT_N) return;
    int kind = g_inv[slot];
    if (kind < ITEM_BOOK_PHYS || kind > ITEM_BOOK_GUIDE) return;   // 只有书能扔
    g_confirmKind = CONF_THROW_BOOK;
    g_confirmText = L"是否确认扔掉这本书";
    g_confirmSlot = slot;
}

// 通用「是 / 否」确认弹窗
static void DrawConfirmBox(Graphics& g)
{
    if (g_confirmKind == CONF_NONE || !g_confirmText) return;

    float w = 600.0f, h = 224.0f;
    RectF r(640.0f - w * 0.5f, 258.0f, w, h);
    {
        GraphicsPath sp;
        MakeRoundRect(sp, RectF(r.X + 8.0f, r.Y + 10.0f, w, h), 16.0f);
        SolidBrush sb(Color(120, 0, 0, 0));
        g.FillPath(&sb, &sp);
    }
    GraphicsPath pp;
    MakeRoundRect(pp, r, 16.0f);
    {
        SolidBrush fill(Color(250, 253, 250, 241));
        g.FillPath(&fill, &pp);
        Pen ink(Color(255, 34, 30, 26), 3.0f);
        ink.SetLineJoin(LineJoinRound);
        g.DrawPath(&ink, &pp);
    }

    // 文字（可能两行）
    {
        std::vector<std::wstring> wrap;
        WrapChalkText(g_confirmText, 23.0f, w - 64.0f, wrap);
        float y = r.Y + 34.0f;
        for (size_t i = 0; i < wrap.size(); ++i) {
            DrawTextCenter(g, wrap[i].c_str(), *g_famUI, FontStyleBold, 23.0f,
                           r.X + w * 0.5f, y, 1.0f, Color(255, 44, 38, 30), 0.0f);
            y += 34.0f;
        }
    }

    // 是 / 否
    {
        RectF yes(r.X + 70.0f, r.Y + 134.0f, 190.0f, 58.0f);
        RectF no (r.X + 340.0f, r.Y + 134.0f, 190.0f, 58.0f);
        const wchar_t* labels[2] = { L"是", L"否" };
        RectF rects[2] = { yes, no };
        int ids[2] = { 60, 61 };
        for (int i = 0; i < 2; ++i) {
            float hot = g_hoverAnim[ids[i]];
            GraphicsPath bp;
            MakeRoundRect(bp, rects[i], 12.0f);
            SolidBrush f(Color(252, (BYTE)(248 - 12 * hot), (BYTE)(243 - 14 * hot),
                               (BYTE)(228 - 12 * hot)));
            g.FillPath(&f, &bp);
            Pen pen(Color(255, 34, 30, 26), 2.8f + 0.8f * hot);
            pen.SetLineJoin(LineJoinRound);
            g.DrawPath(&pen, &bp);
            DrawTextCenter(g, labels[i], *g_famUI, FontStyleBold, 23.0f,
                           rects[i].X + rects[i].Width * 0.5f,
                           rects[i].Y + rects[i].Height * 0.5f, 1.0f,
                           Color(255, 44, 38, 32), 0.0f);
        }
    }
}

// ================= 第十三步：宿舍界面 =================
//  开发说明：
//    · 地图点 9 宿舍进入；时间为「晚寝时间」时强制跳到宿舍，正中弹「该休息了」1 秒
//    · 背景「宿舍界面.jpg」，保留游戏内 UI
//    · 非 晚自习下课 / 晚寝 / 凌晨 → 缩小的对话框，两个选项：小睡一会 / 离开
//        小睡一会：跳过一个时间段，精神 +20、快乐 +5
//        离开：返回地图
//    · 晚自习下课 / 晚寝 / 凌晨 → 三个选项：睡觉 / 熬夜学习 / 熬夜玩
//        睡觉：精神回满 100
//        熬夜学习：精神 -40、健康 -20、知识 +100
//        熬夜玩：精神 -40、健康 -20、快乐回满 100
//        芋头做这两种熬夜：精神只 -10、健康只 -10，知识/快乐获取不变
//        三个选项都会跳到清晨

static bool DormLateNight(int t)
{
    return (t == 19 || t == 20 || t == 21);      // 晚自习下课 / 晚寝 / 凌晨
}

// 「缩小的对话框」：只有选项没有台词
static void DormSetOptions()
{
    g_dlgSmall   = true;
    g_dlgActive  = true;
    g_dlgAsk     = true;
    g_dlgAskSubj = -1;
    g_dlgPrompt  = L"";
    g_dlgScript  = nullptr;
    g_dlgScriptN = 0;
    g_dlgAfter   = DLG_AFTER_NONE;

    if (DormLateNight(g_time)) {
        g_dlgOptN   = 3;
        g_dlgOpt[0] = L"睡觉";
        g_dlgOpt[1] = L"熬夜学习";
        g_dlgOpt[2] = L"熬夜玩";
    } else {
        g_dlgOptN   = 2;
        g_dlgOpt[0] = L"小睡一会";
        g_dlgOpt[1] = L"离开";
    }
    DlgSetLine(0);
}

static void EnterDorm(bool forced)
{
    g_loc      = LOC_DORM;
    g_shopOpen = false;
    g_shopAnim = 0.0f;
    DormSetOptions();
    if (forced) CenterMsg(L"该休息了");
}

static void LeaveDorm()
{
    g_dlgActive = false;
    g_dlgAsk    = false;
    g_dlgOptN   = 0;
    g_dlgSmall  = false;
    g_loc = LOC_CLASS;
    SetScene(SCENE_MAP);
}

// 睡觉 / 熬夜后直接到第二天清晨
static void JumpToMorning()
{
    g_day++;
    if (g_day > 7) g_day = 1;
    g_time = 0;
    g_faceState.Refresh(g_stat[2], g_stat[3]);
    DormSetOptions();                 // 清晨的选项（小睡一会 / 离开）
}

static void DormChoose(int idx)
{
    if (idx < 0 || idx >= g_dlgOptN) return;
    g_dormChoice = idx;               // 第二十步：挑对应的动画（0 睡觉 1 熬夜学习 2 熬夜玩）

    if (!DormLateNight(g_time)) {
        if (idx == 0) {
            // 小睡一会：跳过一个时间段，精神 +20、快乐 +5
            AddStat(2, 20);
            AddStat(3, 5);
            g_faceState.Refresh(g_stat[2], g_stat[3]);
            AdvanceTime();
            g_dlgOptN   = 0;
            g_dlgActive = false;              // 面板期间把对话框收起来
            CommitDeltas(DN_DORM_OPTIONS);   // 第十八步：先显示变化，确定后再刷新选项
        } else {
            LeaveDorm();
        }
        return;
    }

    // 晚自习下课 / 晚寝 / 凌晨
    if (idx == 0) {
        AddStat(2, 100 - g_stat[2]);  // 睡觉：精神回满
    } else {
        // 熬夜学习 / 熬夜玩：芋头（0 号角色）消耗更小
        bool yutou = (g_chosen == 0);
        AddStat(2, -(yutou ? 10 : 40));
        AddStat(0, -(yutou ? 10 : 20));
        if (idx == 1) AddStat(4, 100);     // 熬夜学习：知识 +100
        else          AddStat(3, 100);     // 熬夜玩：快乐回满
    }
    g_faceState.Refresh(g_stat[2], g_stat[3]);
    g_dlgOptN   = 0;
    g_dlgActive = false;
    CommitDeltas(DN_DORM_MORNING);         // 第十八步：确定后再跳到第二天清晨
}


// ================= 第十九步：音乐楼 + 钢琴键小游戏 =================
//  开发说明原文：
//    · 音乐楼 = 地图新增的第 10 号区域，背景「音乐楼界面.jpg」，保留游戏内 UI
//    · 点 10 号 → 音乐楼，对话：我"要弹奏一曲吗" → 选项「展示我的音乐才华 / 保留择偶权」
//    · 选「展示我的音乐才华」→ 开始钢琴键游戏，依据积分跳转到下一个时间段
//    · 选「保留择偶权」→ 回地图
//  钢琴键小游戏整段移植自 D:\Cplus2\一周\钢琴键游戏\main.cpp：
//    黑底 7 条透视道路、蓝色琴键由远及近、1~7 击打、按对变绿加分、按错变红扣分、
//    右上角计分器、中上方歌名；音频走 Media Foundation 解码 + waveOut 输出。
//  与主程序的对接只有三处：PtBegin() 进入、PtKeyDown() 收键盘、PtSettle() 结算。
static std::wstring ExeDir();          // 定义在后面
static void         ResInit();         // 第二十二步：单文件版，释放内嵌资源
static std::wstring ResDir();          // 资源目录（打包版 = 缓存目录，开发版 = exe 同目录）
// 第二十三步（新增3）：本帧「逻辑画布 → 屏幕」的缩放系数（1.0 = 窗口正好 1280×720）。
// 大于 1 说明窗口比逻辑画布大，GDI+ 的抗锯齿填充是按屏幕像素算钱的，
// 钢琴键游戏据此自动降质（关辉光 / 减发光层数 / 关文字外发光）。
static float g_viewScale = 1.0f;
static float g_viewOffX  = 0.0f, g_viewOffY = 0.0f;   // 逻辑画布在窗口里的居中偏移（黑边）

static bool ptActive = false;          // 小游戏开着：整屏接管 + 吃掉键盘
// ============================ 常量 ============================
static const int   LANES       = 7;      // 7 条道路
static const double HIT_WINDOW = 0.175;  // 击打判定窗口（秒，正负各 0.175s）
static const double MISS_AFTER = 0.16;   // 超过这个时间没打就判定漏接
static const float TRAVEL      = 2.85f;  // 琴键从最远处飞到判定线所需时间（秒）
static const float GAMMA_P     = 1.90f;  // 透视曲线指数，越大越"后面加速"
static const float TILE_T      = 0.17f;  // 一个琴键在时间上占的长度（秒）
static const double MAX_PLAY_SEC = 151.0; // 一局最长秒数（想打完整首歌就调大）

// ============================ 数据结构 ============================
struct PtNote {                 // 一个音符（一个琴键）
    int    lane;              // 所在车道 0..6
    double t;                 // 应该被击打的时间点（秒）
    int    state;             // 0=待击打 1=已击中 2=漏接
    double hitT;              // 击中时刻（用于绿色闪光）
    double redT;              // 被误判染红的时刻
};

struct PtSong {
    const wchar_t* title;     // 歌名
    const wchar_t* artist;    // 演唱者
    const wchar_t* key;       // 匹配音频文件名用的关键字
    double         bpm;       // 速度
    unsigned       seed;      // 谱面随机种子（固定后每次谱面一致）
    double         len;       // 谱面时长（秒）
};

struct PtFX {                   // 命中特效（扩散光环）
    int    lane;
    double age;
    double life;
    int    type;              // 0=绿色命中 1=红色错误
};

struct PtParticle {             // 火花粒子
    float  x, y, vx, vy;
    float  age, life, size;
    Color  c;
};

struct PtDust {                 // 背景微尘
    float x, y, vy, size, alpha;
};

// 透视视图参数
struct PtView {
    float W, H;
    float cx;          // 灭点（水平中心）
    float horizonY;    // 地平线 y
    float hitY;        // 判定线 y
    float nearHalf;    // 判定线处道路半宽
    float lw;          // 判定线处单条车道宽度
    float kFar;        // 最远处的缩放系数
    float S;           // 界面缩放系数（以 812 高度为基准 1.0）

    void Setup(float w, float h) {
        W = w; H = h;
        cx        = w * 0.5f;
        horizonY  = h * 0.135f;
        hitY      = h * 0.815f;
        nearHalf  = w * 0.47f;          // 判定线处整条路宽 ≈ 0.94W
        lw        = nearHalf * 2.0f / LANES;
        kFar      = 0.055f;
        S         = std::min(w / 1280.0f, h / 812.0f);   // 宽高都考虑，窄高窗口也不会挤变形
        if (S < 0.55f) S = 0.55f;
        if (S > 2.60f) S = 2.60f;
    }
    // 深度 k（0=无限远, 1=判定线）→ 屏幕 y
    inline float KY(float k) const { return horizonY + (hitY - horizonY) * k; }
    // 世界 x（以判定线为基准）→ 屏幕 x
    inline float PX(float xn, float k) const { return cx + (xn - cx) * k; }
    // 第 i 条车道边界在判定线处的屏幕 x（i = 0..7）
    inline float XB(int i) const { return cx + (i - 3.5f) * lw; }
    // 进程 p（0=最远, 1=判定线）→ 深度 k。用幂曲线代替严格 1/Z，
    // 既保留"越近越快"的透视感，又不会在终点突然飞出去。
    inline float KofP(float p) const {
        if (p < 0.0f) p = 0.0f;
        if (p > 1.40f) p = 1.40f;
        return kFar + (1.0f - kFar) * powf(p, GAMMA_P);
    }
    // 距击打时间还剩多少秒 → 进度 p
    inline float PofT(double remain) const { return 1.0f - (float)(remain / (double)TRAVEL); }
};

// ============================ 全局状态 ============================
enum PtState { ST_MENU, ST_LOADING, ST_COUNTDOWN, ST_PLAY, ST_PAUSE, ST_RESULT };

static const PtSong ptSongs[4] = {
    { L"The Phantom of the Opera", L"Andrew Lloyd Webber · 歌剧魅影", L"Phantom",        124.0, 1001u, 112.0 },
    { L"City of Stars",            L"La La Land · 爱乐之城",       L"City Of Stars",   92.0, 2002u, 104.0 },
    { L"My Shot",                  L"Hamilton · 汉密尔顿",         L"My Shot",        132.0, 3003u, 110.0 },
    { L"Look Down",                L"Les Misérables · 悲惨世界",    L"Look Down",       96.0, 4004u, 106.0 },
};

static HWND    ptHwnd     = nullptr;
// 后备缓冲用 GDI 的 DIB 段来做：GDI+ 直接往这段内存里画，
// 画完用 BitBlt 一次性贴到窗口。比 Graphics::DrawImage 写窗口 DC 快一个数量级。
static Bitmap* ptBack     = nullptr;
static HBITMAP ptDib      = nullptr;
static HDC     ptMemDC    = nullptr;
static int     ptBW = 0, ptBH = 0;
static bool    ptRunning  = true;
static bool    ptNoSound  = false;   // 离屏出图时静音
static bool    ptShowFps  = false;   // F3 开关帧率显示
static double  ptFrameMs  = 16.7;    // 平滑后的单帧耗时
static double  ptFpsAvg   = 60.0;
static double  ptAnimT    = 0.0;      // 全局动画时钟
static double  ptAutoCloseAt = 0;     // 调试：定时自动关窗
static bool    ptRecordFrames = false;
static bool    ptNoVsync      = false;   // 调试：强制关掉垂直同步
static double  ptTScene = 0, ptTBlit = 0, ptTUpd = 0;   // 分段耗时累计
static int     ptTnScene = 0, ptTnBlit = 0, ptTnUpd = 0;
static std::vector<double> ptFrameLog;
static PtView    ptView;

static PtState   ptState    = ST_MENU;
static int     ptSel      = 0;
static double  ptT        = 0.0;      // 谱面时间（秒）
static double  ptSongLen  = 100.0;    // 当前曲目实际可玩时长（跟随音频真实长度）
static std::vector<PtNote> ptNotes;

static int     ptScore = 0, ptCombo = 0, ptMaxCombo = 0;
static int     ptHits = 0, ptMiss = 0, ptWrong = 0, ptTotal = 0;
static double  ptWrongFlash[LANES] = { 0,0,0,0,0,0,0 };

static std::vector<PtFX>       ptFX;
static std::vector<PtParticle> ptParts;
static std::vector<PtDust>     ptDust;

static std::wstring ptDir;            // exe 所在目录
static bool         ptHasMusic = false;

// ============================ 计时 ============================
static LARGE_INTEGER ptFreq;
static double NowSec() {
    LARGE_INTEGER c; QueryPerformanceCounter(&c);
    return (double)c.QuadPart / (double)ptFreq.QuadPart;
}

#if defined(HSW_MAC)
// ======================== 音频：AVAudioPlayer（macOS） ========================
// 接口和 Windows 版（MF 解码 + waveOut）保持一致，实现在 shim_mac.mm：
//   Load(文件) / LoadMem(内存 mp3) / Play / Pause / Resume / Stop / PosSec / Update
struct PtAudio {
    bool ready  = false;
    bool paused = false;
    double fileDur = 0;              // 音频真实时长（秒）
    std::vector<short> pcm;          // Windows 版才有，这里留着只为接口一致
    int  sr = 44100, ch = 2;

    bool Load(const std::wstring& path, double maxSec) {
        ready = HswAudLoadFile(path.c_str(), maxSec);
        fileDur = ready ? HswAudDur() : 0;
        return ready;
    }
    bool LoadMem(const void* data, size_t size, double maxSec) {
        ready = HswAudLoadMem(data, size, maxSec);
        fileDur = ready ? HswAudDur() : 0;
        return ready;
    }
    void Close()  { HswAudStop(); ready = false; }
    void Play()   { HswAudPlay();  paused = false; }
    void Pause()  { HswAudPause(); paused = true;  }
    void Resume() { HswAudPlay();  paused = false; }
    void Stop()   { HswAudStop();  paused = false; }
    double PosSec() { return HswAudPos(); }
    void Update() {}
};
static PtAudio ptAudio;
#else
// ============================ 音频：MF 解码 + waveOut 播放 ============================
// 说明：MCI 的 mpegvideo 和 DirectShow 在部分 Win11 上解不了 MP3，
//       这里统一走 Media Foundation 解码成 16bit PCM，再交给 waveOut 输出。
typedef HRESULT (WINAPI* PFN_CreateReader)(LPCWSTR, IMFAttributes*, IMFSourceReader**);
typedef HRESULT (WINAPI* PFN_ReaderFromStream)(IMFByteStream*, IMFAttributes*, IMFSourceReader**);
typedef HRESULT (WINAPI* PFN_BsOnStream)(IStream*, IMFByteStream**);
static HMODULE ptMfReadWrite = nullptr;
static PFN_CreateReader      ptPfnCreateReader      = nullptr;
static PFN_ReaderFromStream ptPfnReaderFromStream   = nullptr;
static PFN_BsOnStream       ptPfnBsOnStream         = nullptr;

// 延迟加载 Media Foundation 的几个导出函数（MinGW 8.1 没有 libmfreadwrite.a）
static bool PtInitMf() {
    if (!ptMfReadWrite) {
        ptMfReadWrite = LoadLibraryW(L"mfreadwrite.dll");
        if (ptMfReadWrite) {
            ptPfnCreateReader    = (PFN_CreateReader)GetProcAddress(ptMfReadWrite, "MFCreateSourceReaderFromURL");
            ptPfnReaderFromStream = (PFN_ReaderFromStream)GetProcAddress(ptMfReadWrite, "MFCreateSourceReaderFromByteStream");
        }
        HMODULE mp = GetModuleHandleW(L"mfplat.dll");
        if (mp) ptPfnBsOnStream = (PFN_BsOnStream)GetProcAddress(mp, "MFCreateMFByteStreamOnStream");
    }
    return ptPfnCreateReader != nullptr;
}

struct PtAudio {
    bool   ready  = false;
    bool   paused = false;
    int    sr     = 44100;
    int    ch     = 2;
    double fileDur = 0;          // 音频文件的真实时长（秒），0 表示未知
    double baseRaw = 0;          // Play() 时的设备采样计数原点

    std::vector<short> pcm;      // 交错 PCM
    size_t totalBytes = 0;
    size_t nextByte   = 0;

    static const int NB = 4;     // 4 个缓冲轮转
    HWAVEOUT   wo = nullptr;
    WAVEFORMATEX wf;
    WAVEHDR    hdr[NB];
    bool       prepared[NB];
    bool       queued[NB];
    size_t     bufBytes = 0;

    PtAudio() { memset(hdr, 0, sizeof(hdr)); memset(&wf, 0, sizeof(wf));
              memset(prepared, 0, sizeof(prepared)); memset(queued, 0, sizeof(queued)); }

    void Close() {
        if (wo) {
            waveOutReset(wo);
            for (int i = 0; i < NB; i++) {
                if (prepared[i]) { waveOutUnprepareHeader(wo, &hdr[i], sizeof(WAVEHDR)); prepared[i] = false; }
            }
            waveOutClose(wo);
            wo = nullptr;
        }
        ready = false;
        paused = false;
        pcm.clear(); pcm.shrink_to_fit();
        totalBytes = nextByte = 0;
    }

    // ---- 打开 waveOut 并把 PCM 数据准备好 ----
    bool OpenDevice() {
        if (pcm.empty()) return false;
        totalBytes = pcm.size() * 2;

        memset(&wf, 0, sizeof(wf));
        wf.wFormatTag      = WAVE_FORMAT_PCM;
        wf.nChannels       = (WORD)ch;
        wf.nSamplesPerSec  = (DWORD)sr;
        wf.wBitsPerSample  = 16;
        wf.nBlockAlign     = (WORD)(ch * 2);
        wf.nAvgBytesPerSec = (DWORD)(sr * ch * 2);
        wf.cbSize          = 0;

        if (waveOutOpen(&wo, WAVE_MAPPER, &wf, 0, 0, 0) != MMSYSERR_NOERROR) {
            wo = nullptr;
            pcm.clear();
            return false;
        }
        bufBytes = (size_t)(sr * ch * 2 * 0.20);     // 每个缓冲 0.2 秒
        if (bufBytes < 2048) bufBytes = 2048;
        ready = true;
        return true;
    }

    // ---- 用一个 SourceReader 解码到内存（最多 maxSec 秒），reader 由调用方释放 ----
    bool DecodeFrom(IMFSourceReader* rdr, double maxSec) {
        fileDur = 0;
        {
            // 先问一下这首歌有多长（100ns 为单位），后面用来决定谱面长度
            PROPVARIANT pv;
            PropVariantInit(&pv);
            if (SUCCEEDED(rdr->GetPresentationAttribute((DWORD)MF_SOURCE_READER_MEDIASOURCE,
                                                        MF_PD_DURATION, &pv)) && pv.vt == VT_UI8)
                fileDur = (double)pv.uhVal.QuadPart / 1e7;
            PropVariantClear(&pv);
        }

        rdr->SetStreamSelection((DWORD)MF_SOURCE_READER_ALL_STREAMS, FALSE);
        rdr->SetStreamSelection((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE);

        IMFMediaType* mt = nullptr;
        MFCreateMediaType(&mt);
        mt->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
        mt->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
        mt->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16);
        HRESULT hr = rdr->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, mt);
        mt->Release();
        if (FAILED(hr)) return false;

        IMFMediaType* cur = nullptr;
        UINT32 s32 = 0, c32 = 0;
        if (SUCCEEDED(rdr->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, &cur)) && cur) {
            cur->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &s32);
            cur->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &c32);
            cur->Release();
        }
        sr = (int)(s32 ? s32 : 44100);
        ch = (int)(c32 ? c32 : 2);
        if (ch < 1) ch = 1;
        if (ch > 2) ch = 2;

        size_t limit = (size_t)(sr * ch * 2 * maxSec);
        pcm.clear();
        pcm.reserve(limit < (size_t)40000000 ? limit : (size_t)40000000);

        for (;;) {
            DWORD flags = 0; LONGLONG ts = 0; IMFSample* smp = nullptr;
            hr = rdr->ReadSample((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, nullptr, &flags, &ts, &smp);
            if (FAILED(hr)) break;
            if (flags & MF_SOURCE_READERF_ENDOFSTREAM) { if (smp) smp->Release(); break; }
            if (smp) {
                IMFMediaBuffer* mb = nullptr;
                if (SUCCEEDED(smp->ConvertToContiguousBuffer(&mb)) && mb) {
                    BYTE* q = nullptr; DWORD len = 0;
                    if (SUCCEEDED(mb->Lock(&q, nullptr, &len))) {
                        size_t old = pcm.size();
                        pcm.resize(old + len / 2);
                        memcpy(&pcm[old], q, len);
                        bool full = (pcm.size() * 2 >= limit);
                        mb->Unlock();
                        mb->Release();
                        smp->Release();
                        if (full) break;
                        continue;
                    }
                    mb->Unlock();
                    mb->Release();
                }
                smp->Release();
            }
        }
        return OpenDevice();
    }

    // 从磁盘文件解码
    bool Load(const std::wstring& path, double maxSec) {
        Close();
        if (!PtInitMf() || !ptPfnCreateReader) return false;
        IMFSourceReader* rdr = nullptr;
        if (FAILED(ptPfnCreateReader(path.c_str(), nullptr, &rdr)) || !rdr) return false;
        bool ok = DecodeFrom(rdr, maxSec);
        rdr->Release();
        if (!ok) Close();
        return ok;
    }

    // 从内存里的音频数据解码（内嵌资源用，不落地临时文件）
    bool LoadMem(const void* data, size_t size, double maxSec) {
        Close();
        if (!PtInitMf()) return false;
        if (!ptPfnReaderFromStream || !ptPfnBsOnStream) return false;
        bool ok = false;
        IStream* ms = SHCreateMemStream((const BYTE*)data, (UINT)size);
        if (ms) {
            IMFByteStream* bs = nullptr;
            if (SUCCEEDED(ptPfnBsOnStream(ms, &bs)) && bs) {
                IMFSourceReader* rdr = nullptr;
                if (SUCCEEDED(ptPfnReaderFromStream(bs, nullptr, &rdr)) && rdr) {
                    ok = DecodeFrom(rdr, maxSec);
                    rdr->Release();
                }
                bs->Release();
            }
            ms->Release();
        }
        if (!ok) Close();
        return ok;
    }

    // 把下一段数据塞进第 i 个缓冲
    void Submit(int i) {
        if (!wo || nextByte >= totalBytes) return;
        size_t n = std::min(bufBytes, totalBytes - nextByte);
        n -= n % (size_t)wf.nBlockAlign;             // 保证整帧对齐
        if (n == 0) return;
        if (prepared[i]) { waveOutUnprepareHeader(wo, &hdr[i], sizeof(WAVEHDR)); prepared[i] = false; }
        memset(&hdr[i], 0, sizeof(WAVEHDR));
        hdr[i].lpData          = (LPSTR)((char*)pcm.data() + nextByte);
        hdr[i].dwBufferLength  = (DWORD)n;
        if (waveOutPrepareHeader(wo, &hdr[i], sizeof(WAVEHDR)) == MMSYSERR_NOERROR) {
            prepared[i] = true;
            if (waveOutWrite(wo, &hdr[i], sizeof(WAVEHDR)) == MMSYSERR_NOERROR) {
                queued[i] = true;
                nextByte += n;
            }
        }
    }

    void Play() {
        if (!ready || !wo) return;
        waveOutReset(wo);
        for (int i = 0; i < NB; i++) queued[i] = false;
        nextByte = 0;
        for (int i = 0; i < NB; i++) Submit(i);
        MMTIME mm; memset(&mm, 0, sizeof(mm)); mm.wType = TIME_SAMPLES;
        baseRaw = 0;
        if (waveOutGetPosition(wo, &mm, sizeof(mm)) == MMSYSERR_NOERROR && mm.wType == TIME_SAMPLES)
            baseRaw = (double)mm.u.sample;
        paused = false;
        waveOutSetVolume(wo, MAKELONG(0xB000, 0xB000));   // 约 70% 音量
        waveOutRestart(wo);
    }
    void Pause()  { if (wo && ready && !paused) { waveOutPause(wo);  paused = true;  } }
    void Resume() { if (wo && ready && paused)  { waveOutRestart(wo); paused = false; } }
    void Stop()   { if (wo && ready) { waveOutReset(wo); for (int i = 0; i < NB; i++) queued[i] = false; paused = false; } }

    // 当前播放位置（秒）；不可用时返回 -1
    double PosSec() {
        if (!ready || !wo) return -1.0;
        MMTIME mm; memset(&mm, 0, sizeof(mm)); mm.wType = TIME_SAMPLES;
        if (waveOutGetPosition(wo, &mm, sizeof(mm)) != MMSYSERR_NOERROR) return -1.0;
        if (mm.wType != TIME_SAMPLES) return -1.0;
        double v = ((double)mm.u.sample - baseRaw) / (double)sr;
        return v < 0 ? 0 : v;
    }

    // 每帧调用：把播完的缓冲补上新数据
    void Update() {
        if (!ready || paused || !wo) return;
        for (int i = 0; i < NB; i++) {
            if (queued[i] && (hdr[i].dwFlags & WHDR_DONE)) {
                queued[i] = false;
                Submit(i);
            }
        }
    }
};
static PtAudio ptAudio;
#endif

// ============================ 音效（每个车道一个音高） ============================
static std::vector<char> ptWav[LANES];

static std::vector<char> PtMakeWav(double freq, double dur, double amp) {
    const int SR = 44100;
    int n = (int)(SR * dur);
    int dataBytes = n * 2;
    std::vector<char> b(44 + dataBytes, 0);
    auto put32 = [&](int off, unsigned v) { memcpy(&b[off], &v, 4); };
    auto put16 = [&](int off, unsigned short v) { memcpy(&b[off], &v, 2); };
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

static void PtInitSfx() {
    // C 大调音阶，7 个车道 7 个音
    static const double f[LANES] = { 523.25, 587.33, 659.26, 783.99, 880.00, 1046.50, 1174.66 };
    for (int i = 0; i < LANES; i++) ptWav[i] = PtMakeWav(f[i], 0.26, 0.55);
}
static void PtPlayTone(int lane) {
    if (lane < 0 || lane >= LANES || ptWav[lane].empty()) return;
    PlaySoundA(ptWav[lane].data(), nullptr, SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
}

// ============================ 查找音频文件 ============================
static std::wstring PtLower(std::wstring s) {
    for (auto& c : s) c = (wchar_t)towlower(c);
    return s;
}
static std::wstring PtFindAudio(const std::wstring& key) {
    std::wstring pat = ptDir + L"\\*";
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW(pat.c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return L"";
    std::wstring lowKey = PtLower(key), hit;
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        std::wstring name = fd.cFileName;
        std::wstring low = PtLower(name);
        bool extOk = false;
        for (const wchar_t* e : { L".mp3", L".wav", L".m4a", L".wma", L".flac" }) {
            size_t el = wcslen(e);
            if (low.size() > el && low.compare(low.size() - el, el, e) == 0) { extOk = true; break; }
        }
        if (!extOk) continue;
        if (low.find(lowKey) == std::wstring::npos) continue;
        hit = ptDir + L"\\" + name;
        break;
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    return hit;
}

// ============================ 内嵌音频资源 ============================
// 每个曲目在 exe 里埋了一份 mp3（资源 id 101~104），这样单独一个 exe 就能跑。
// 若 exe 同目录有同名关键字的音频文件，优先用外面的文件，方便换歌。
static const void* PtResData(int id, DWORD& size) {
    size = 0;
    HRSRC h = FindResourceW(nullptr, MAKEINTRESOURCEW(id), RT_RCDATA);
    if (!h) return nullptr;
    HGLOBAL g = LoadResource(nullptr, h);
    if (!g) return nullptr;
    size = SizeofResource(nullptr, h);
    return LockResource(g);
}
static bool PtHasEmbedded(int idx) {
    DWORD sz = 0;
    const void* d = PtResData(101 + idx, sz);
    return d != nullptr && sz > 1024;
}

// 缓存四个曲目的音频路径（菜单每帧都可能要用，不能每帧去枚举目录）
static std::wstring ptAudioPath[4];
static bool         ptAudioFound[4] = { false, false, false, false };
static void PtScanAudio() {
    for (int i = 0; i < 4; i++) {
        ptAudioPath[i]  = PtFindAudio(ptSongs[i].key);
        ptAudioFound[i] = !ptAudioPath[i].empty() || PtHasEmbedded(i);
    }
}

// 载入某首歌的音频：先找 exe 同目录的外部文件，找不到就用 exe 内嵌的那份
static bool PtLoadSongAudio(int idx, double maxSec) {
    if (!ptAudioPath[idx].empty() && ptAudio.Load(ptAudioPath[idx], maxSec)) return true;
    DWORD sz = 0;
    const void* d = PtResData(101 + idx, sz);
    if (d && sz > 1024) return ptAudio.LoadMem(d, sz, maxSec);
    return false;
}

// ============================ 生成谱面 ============================
static void PtBuildChart(const PtSong& s, double len, std::vector<PtNote>& out) {
    out.clear();
    std::mt19937 rng(s.seed);
    std::uniform_real_distribution<double> ur(0.0, 1.0);

    double step = 60.0 / s.bpm / 2.0;          // 八分音符为最小格
    int total = (int)((len - 4.0) / step);
    int lastLane = -1;

    for (int i = 0; i < total; i++) {
        double t = 2.0 + i * step;

        // 用 16 步为一段做“强弱起伏”，避免整首歌节奏一模一样
        int sec = (i / 16) % 4;
        double dens = (sec == 0) ? 0.32 : (sec == 1 ? 0.58 : (sec == 2 ? 0.80 : 0.46));
        if (ur(rng) > dens) continue;

        int lane;
        int guard = 0;
        do { lane = (int)(ur(rng) * LANES); } while (lane == lastLane && ur(rng) < 0.75 && ++guard < 8);
        if (lane >= LANES) lane = LANES - 1;

        PtNote n; n.lane = lane; n.t = t; n.state = 0; n.hitT = -1; n.redT = -1;
        out.push_back(n);
        lastLane = lane;

        // 偶尔来一个双键和弦
        if (ur(rng) < 0.09) {
            int l2;
            int g2 = 0;
            do { l2 = (int)(ur(rng) * LANES); } while (l2 == lane && ++g2 < 8);
            if (l2 < LANES && l2 != lane) {
                PtNote m; m.lane = l2; m.t = t; m.state = 0; m.hitT = -1; m.redT = -1;
                out.push_back(m);
            }
        }
    }
    std::stable_sort(out.begin(), out.end(),
                     [](const PtNote& a, const PtNote& b) { return a.t < b.t; });
}



// ============================ 开始 / 复位 ============================
static void PtStartSong(int idx) {
    ptSel = idx;
    ptScore = 0; ptCombo = 0; ptMaxCombo = 0;
    ptHits = 0; ptMiss = 0; ptWrong = 0;
    ptFX.clear(); ptParts.clear();
    for (int i = 0; i < LANES; i++) ptWrongFlash[i] = 0;

    // 先画一帧“解码中”，再同步解码（一首歌通常 1~2 秒）
    ptHasMusic = false;
    ptAudio.Close();
    ptState = ST_LOADING;

    PtScanAudio();
    ptHasMusic = PtLoadSongAudio(ptSel, MAX_PLAY_SEC + 1.0);

    // 谱面长度跟着音频真实时长走：音乐放完，这一局也就结束了
    double dur = ptSongs[ptSel].len;                 // 没音频时的默认长度
    if (ptHasMusic) {
        double real = ptAudio.fileDur > 0 ? ptAudio.fileDur
                                         : ptAudio.pcm.size() / (double)(ptAudio.sr * ptAudio.ch);
        if (real > 0) dur = real;
    }
    double playLen = (dur > MAX_PLAY_SEC) ? MAX_PLAY_SEC : (dur - 1.2);
    if (playLen < 30.0) playLen = dur;             // 太短的曲子不截断

    ptSongLen = playLen;
    PtBuildChart(ptSongs[ptSel], ptSongLen, ptNotes);
    ptTotal = (int)ptNotes.size();

    ptT = -3.0;                 // 3 秒倒计时，期间琴键由远及近
    ptState = ST_COUNTDOWN;
}
static void PtBackToMenu() {
    ptAudio.Close();
    PtScanAudio();
    ptHasMusic = false;
    ptState = ST_MENU;
    ptNotes.clear();
    ptFX.clear(); ptParts.clear();
}

// ============================ 击打判定 ============================
static void PtPress(int lane) {
    if (lane < 0 || lane >= LANES) return;
    if (ptState != ST_PLAY) return;

    PtNote* best = nullptr;
    double bestD = 1e9;
    for (auto& n : ptNotes) {
        if (n.state != 0 || n.lane != lane) continue;
        double d = fabs(n.t - ptT);
        if (d < bestD) { bestD = d; best = &n; }
    }

    if (best && bestD <= HIT_WINDOW) {
        // ---------- 打对：加 1 分，琴键变绿 ----------
        best->state = 1;
        best->hitT  = ptT;
        ptScore++; ptHits++; ptCombo++;
        if (ptCombo > ptMaxCombo) ptMaxCombo = ptCombo;

        PtFX f; f.lane = lane; f.age = 0; f.life = 0.42; f.type = 0;
        ptFX.push_back(f);

        float cxLane = (ptView.XB(lane) + ptView.XB(lane + 1)) * 0.5f;
        for (int i = 0; i < 16; i++) {
            PtParticle p;
            float ang = (float)(-M_PI * 0.5 + ((rand() % 1000) / 1000.0 - 0.5) * 2.4);
            float sp  = 140.0f + (rand() % 260);
            p.x = cxLane + (float)((rand() % 100) - 50) * 0.8f;
            p.y = ptView.hitY;
            p.vx = cosf(ang) * sp;
            p.vy = sinf(ang) * sp;
            p.age = 0;
            p.life = 0.35f + (rand() % 40) / 100.0f;
            p.size = 1.6f + (rand() % 3);
            int g = 220 + rand() % 35;
            p.c = Color(255, 130, g, 200);
            ptParts.push_back(p);
        }
        PtPlayTone(lane);
    } else {
        // ---------- 打错：扣 1 分（不低于 0），车道闪红 ----------
        ptScore = (ptScore > 0) ? ptScore - 1 : 0;
        ptCombo = 0;
        ptWrong++;
        ptWrongFlash[lane] = 0.34;

        PtFX f; f.lane = lane; f.age = 0; f.life = 0.34; f.type = 1;
        ptFX.push_back(f);

        // 如果这个车道上有马上要到的琴键，把它也染红
        PtNote* nNear = nullptr; double nd = 1e9;
        for (auto& n : ptNotes) {
            if (n.state != 0 || n.lane != lane) continue;
            double d = n.t - ptT;
            if (d > 0 && d < 0.55 && d < nd) { nd = d; nNear = &n; }
        }
        if (nNear) nNear->redT = ptT;
    }
}

// ============================ 更新 ============================
static void PtUpdate(double dt) {
    ptAnimT += dt;
    ptAudio.Update();          // 给 waveOut 补投递缓冲

    for (auto& d : ptDust) { d.y += d.vy * dt; if (d.y > 1.05f) d.y -= 1.1f; }
    for (auto& f : ptFX) f.age += dt;
    ptFX.erase(std::remove_if(ptFX.begin(), ptFX.end(),
              [](const PtFX& f) { return f.age > f.life; }), ptFX.end());
    for (auto& p : ptParts) {
        p.vy += 980.0f * (float)dt;
        p.x += p.vx * (float)dt;
        p.y += p.vy * (float)dt;
        p.age += (float)dt;
    }
    ptParts.erase(std::remove_if(ptParts.begin(), ptParts.end(),
                 [](const PtParticle& p) { return p.age > p.life; }), ptParts.end());
    for (int i = 0; i < LANES; i++) if (ptWrongFlash[i] > 0) ptWrongFlash[i] -= dt;

    if (ptState == ST_COUNTDOWN) {
        ptT += dt;
        if (ptT >= 0.0) {
            ptT = 0.0;
            ptState = ST_PLAY;
            if (ptHasMusic && !ptNoSound) ptAudio.Play();
        }
        return;
    }
    if (ptState != ST_PLAY) return;

    ptT += dt;

    // 让谱面时间缓缓对齐音乐实际播放位置。
    // 加了死区：设备报位置本身是分块更新的（有几毫秒抖动），
    // 80ms 以内的差当作噪声不管，免得把抖动带进琴键的移动里。
    if (ptHasMusic) {
        double mp = ptAudio.PosSec();
        if (mp > 0.10) {
            double diff = ptT - mp;
            if (fabs(diff) > 1.0)        ptT -= diff;          // 明显错位，直接对齐
            else if (fabs(diff) > 0.08)  ptT -= diff * 0.035;  // 超出死区才慢慢拉回
        }
    }

    // 漏接判定
    for (auto& n : ptNotes) {
        if (n.state == 0 && ptT > n.t + MISS_AFTER) {
            n.state = 2;
            ptCombo = 0;
            ptMiss++;
        }
    }

    if (ptT > ptSongLen - 0.35) {
        ptState = ST_RESULT;
        ptAudio.Stop();
    }
}
// ============================ 绘图小工具 ============================
// 注意：这里用 new 出来且不释放。若用静态对象，程序退出时它的析构函数会在
// GdiplusShutdown() 之后才执行，对已关闭的 GDI+ 调 GdipDeleteFontFamily 会崩溃。
static const FontFamily* PtFF(int which) {
    static FontFamily* ya = new FontFamily(L"Microsoft YaHei");
    static FontFamily* se = new FontFamily(L"Segoe UI");
    static FontFamily* ar = new FontFamily(L"Arial");
    static BOOL yaOk = ya->IsAvailable();
    static BOOL seOk = se->IsAvailable();
    if (which == 0 && yaOk) return ya;
    if (which == 1 && seOk) return se;
    return ar;
}

// 第二十三步（新增3）：Font 对象缓存。
// 以前每帧要临时构造十几个 Font（键帽数字、HUD、菜单…），GDI+ 每次构造都要重新解析
// 字体族 + 字号。缓存用「字体族 + 字号 + 字形」做键，全部 new 出来故意不 delete：
// 静态对象会在 GdiplusShutdown() 之后析构，调 GdipDeleteFont 会崩。
static Font* PtGetFontF(const FontFamily* fam, float size, int style) {
    struct Item { const FontFamily* fam; float sz; int st; Font* f; };
    static std::vector<Item> pool;
    for (size_t i = 0; i < pool.size(); ++i)
        if (pool[i].fam == fam && pool[i].st == style && fabsf(pool[i].sz - size) < 0.01f)
            return pool[i].f;
    Item it;
    it.fam = fam; it.sz = size; it.st = style;
    it.f   = new Font(fam, size, style, UnitPixel);
    pool.push_back(it);
    return it.f;
}
static Font* PtGetFont(int which, float size, int style) {
    return PtGetFontF(PtFF(which), size, style);
}

// 大屏降质开关（由 RenderScene 每帧写入的缩放系数决定）
static bool ptLowQ = false;

static void PtText(Graphics& g, const std::wstring& s, Font& f, const RectF& rc,
                  Color c, StringAlignment ha = StringAlignmentCenter,
                  StringAlignment va = StringAlignmentCenter, bool glow = false) {
    StringFormat fmt;
    fmt.SetAlignment(ha);
    fmt.SetLineAlignment(va);
    SolidBrush br(c);
    if (glow && !ptLowQ) {      // 第二十三步：大屏时省掉这 4 次叠绘
        // 只在 4 个对角方向叠一层半透明，效果和 8 方向几乎没差别，
        // 但少了一半 DrawString 调用（文字渲染是 GDI+ 里比较贵的操作）
        SolidBrush gb(Color(c.GetA() / 5, c.GetR(), c.GetG(), c.GetB()));
        static const int off[4][2] = { { -2, -2 }, { 2, -2 }, { -2, 2 }, { 2, 2 } };
        for (int k = 0; k < 4; k++)
            g.DrawString(s.c_str(), -1, &f,
                         RectF(rc.X + off[k][0], rc.Y + off[k][1], rc.Width, rc.Height), &fmt, &gb);
    }
    g.DrawString(s.c_str(), -1, &f, rc, &fmt, &br);
}

// 把字号缩到能塞进 maxW 的最大值（长歌名在小窗口里不会顶到计分器）
static float PtFitSize(Graphics& g, const std::wstring& s, const FontFamily* fam,
                     float sz, int style, float maxW) {
    if (maxW <= 20) return sz * 0.5f;
    Font& f = *PtGetFontF(fam, sz, style);        // 第二十三步：走缓存，别每帧新建
    RectF box(0, 0, 0, 0);
    g.MeasureString(s.c_str(), -1, &f, PointF(0, 0), &box);
    if (box.Width <= 0 || box.Width <= maxW) return sz;
    float k = maxW / box.Width;
    if (k < 0.42f) k = 0.42f;
    return sz * k;
}

static void PtRoundRect(GraphicsPath& p, const RectF& r, float rad) {
    p.Reset();
    float d = rad * 2;
    p.AddArc(r.X, r.Y, d, d, 180, 90);
    p.AddArc(r.GetRight() - d, r.Y, d, d, 270, 90);
    p.AddArc(r.GetRight() - d, r.GetBottom() - d, d, d, 0, 90);
    p.AddArc(r.X, r.GetBottom() - d, d, d, 90, 90);
    p.CloseFigure();
}

// 沿形状外扩绘制一层柔光
// layers：发光层数（3 = 最细致；大屏或远处的键会降到 1 或 0）
static void PtGlowPoly(Graphics& g, PointF* pts, int n, Color c, float step, int layers = 3) {
    if (layers <= 0) return;
    float cxs = 0, cys = 0;
    for (int i = 0; i < n; i++) { cxs += pts[i].X; cys += pts[i].Y; }
    cxs /= n; cys /= n;
    for (int layer = layers; layer >= 1; layer--) {
        float s = 1.0f + step * layer;
        PointF t[8];
        for (int i = 0; i < n; i++) {
            t[i].X = cxs + (pts[i].X - cxs) * s;
            t[i].Y = cys + (pts[i].Y - cys) * s;
        }
        SolidBrush br(Color((BYTE)(c.GetA() / (layer + 1)), c.GetR(), c.GetG(), c.GetB()));
        g.FillPolygon(&br, t, n);
    }
}

// ============================ 画一个琴键 ============================
// pFront: 琴键前沿的进度（0=最远, 1=判定线）
// mode:   0 正常蓝  1 命中绿  2 误按红 ；alpha 用于淡出
static void PtDrawTile(Graphics& g, int lane, float pFront, int mode, float alpha) {
    float kf = ptView.KofP(pFront);
    float kb = ptView.KofP(pFront - TILE_T / TRAVEL);

    float m = ptView.lw * 0.055f;                 // 左右留缝，形成琴键之间的间隙
    float xn0 = ptView.XB(lane) + m;
    float xn1 = ptView.XB(lane + 1) - m;

    float yb  = ptView.KY(kb),  yf  = ptView.KY(kf);
    float xb0 = ptView.PX(xn0, kb), xb1 = ptView.PX(xn1, kb);
    float xf0 = ptView.PX(xn0, kf), xf1 = ptView.PX(xn1, kf);
    if (yf - yb < 0.8f) yb = yf - 0.8f;          // 极远处至少留 1 像素

    PointF q[4] = { PointF(xb0, yb), PointF(xb1, yb), PointF(xf1, yf), PointF(xf0, yf) };

    // 第二十三步（新增3）：细节分级。远处的键只有几像素高，发光和键面高光根本看不出来，
    // 但一样要走抗锯齿填充 —— 大屏上这部分是最贵的，直接按高度砍掉。
    float th = yf - yb;
    int   glowLayers = 3;
    if (ptLowQ)              glowLayers = 1;     // 大屏：只留一层
    if (th < 26.0f)          glowLayers = 0;     // 远处的小键：完全不发光
    bool  drawHi = (th >= 26.0f);                // 键面中缝高光同理

    Color cTop, cBot, cGlow, cEdge;
    if (mode == 1) {          // 命中 → 绿色
        cTop  = Color((BYTE)(235 * alpha),   6, 140,  62);
        cBot  = Color((BYTE)(255 * alpha), 120, 255, 150);
        cGlow = Color((BYTE)(175 * alpha),  90, 255, 150);
        cEdge = Color((BYTE)(255 * alpha), 215, 255, 225);
    } else if (mode == 2) {   // 误按 → 红色
        cTop  = Color((BYTE)(235 * alpha), 110,   6,  12);
        cBot  = Color((BYTE)(255 * alpha), 255,  90,  90);
        cGlow = Color((BYTE)(175 * alpha), 255,  70,  70);
        cEdge = Color((BYTE)(255 * alpha), 255, 210, 210);
    } else {                  // 正常 → 炫酷蓝
        cTop  = Color((BYTE)(238 * alpha),   8,  30, 128);
        cBot  = Color((BYTE)(255 * alpha),  70, 190, 255);
        cGlow = Color((BYTE)(190 * alpha),  30, 130, 255);
        cEdge = Color((BYTE)(255 * alpha), 190, 250, 255);
    }

    // 外发光
    PtGlowPoly(g, q, 4, cGlow, 0.055f, glowLayers);

    // 主体渐变
    float midX = (xb0 + xb1) * 0.5f;
    LinearGradientBrush lgb(PointF(midX, yb), PointF(midX, yf), cTop, cBot);
    g.FillPolygon(&lgb, q, 4);

    // 键面中缝高光（让键看起来是“立体的”）
    if (!drawHi) {                       // 远处的小键省掉这一层填充
        Pen pen(cEdge, 1.2f + 2.4f * (kf > 1.5f ? 1.0f : kf / 1.5f));
        g.DrawLine(&pen, q[3], q[2]);
        return;
    }
    float inset = 0.30f;
    PointF hi[4];
    float cyh = (yb + yf) * 0.5f;
    float cx4 = (xb0 + xb1 + xf0 + xf1) * 0.25f;
    for (int i = 0; i < 4; i++) {
        hi[i].X = cx4 + (q[i].X - cx4) * (1.0f - inset * 0.4f);
        hi[i].Y = cyh + (q[i].Y - cyh) * (1.0f - inset * 2.2f);
    }
    SolidBrush hb(Color((BYTE)(40 * alpha), 255, 255, 255));
    g.FillPolygon(&hb, hi, 4);

    // 前沿亮边 + 两侧描边
    float penW = 1.2f + 2.4f * (kf > 1.5f ? 1.0f : kf / 1.5f);
    Pen pen(cEdge, penW);
    g.DrawLine(&pen, q[3], q[2]);
    Pen side(Color((BYTE)(150 * alpha), 140, 215, 255), 1.1f);
    g.DrawLine(&side, q[0], q[3]);
    g.DrawLine(&side, q[1], q[2]);
}

// ============================ 场景绘制 ============================
// 静态部分：渐变底 + 地平线辉光（内容不变，做成缓存层只画一次）
static void PtDrawBackdrop(Graphics& g, float W, float H) {
    // 垂直渐变底
    LinearGradientBrush bg(PointF(0, 0), PointF(0, H),
                           Color(255, 3, 5, 15), Color(255, 0, 0, 1));
    g.FillRectangle(&bg, RectF(0, 0, W, H));

    // 地平线上的辉光
    GraphicsPath gp;
    gp.AddEllipse(-W * 0.15f, ptView.horizonY - H * 0.16f, W * 1.30f, H * 0.36f);
    PathGradientBrush pgb(&gp);
    pgb.SetCenterColor(Color(120, 24, 78, 210));
    Color surround[1] = { Color(0, 0, 0, 0) };
    int cnt = 1;
    pgb.SetSurroundColors(surround, &cnt);
    g.FillPath(&pgb, &gp);
}

// 动态部分：慢慢飘的微尘
static void PtDrawDust(Graphics& g, float W, float H, float dim = 1.0f) {
    SolidBrush b(Color(255, 120, 190, 255));
    for (auto& d : ptDust) {
        b.SetColor(Color((BYTE)(d.alpha * 255 * dim), 120, 190, 255));
        float r = d.size;
        g.FillEllipse(&b, d.x * W - r, d.y * H - r, r * 2, r * 2);
    }
}

static void PtDrawRoad(Graphics& g) {
    const PtView& V = ptView;

    // 路面：从最远处一直延伸出屏幕底边
    float kEnd = 2.4f;
    PointF road[4] = {
        PointF(V.PX(V.XB(0), V.kFar), V.KY(V.kFar)),
        PointF(V.PX(V.XB(LANES), V.kFar), V.KY(V.kFar)),
        PointF(V.PX(V.XB(LANES), kEnd), V.KY(kEnd)),
        PointF(V.PX(V.XB(0), kEnd), V.KY(kEnd))
    };
    LinearGradientBrush rbg(PointF(0, V.KY(V.kFar)), PointF(0, V.KY(kEnd)),
                            Color(255, 8, 14, 34), Color(255, 3, 6, 18));
    g.FillPolygon(&rbg, road, 4);

    // 远处横向网格（增强纵深感）
    for (int i = 1; i <= 9; i++) {
        float z = 1.0f + i * 1.9f;
        float k = 1.0f / z;
        if (k < V.kFar) break;
        Pen p(Color(20, 70, 140, 235), 1.0f);
        g.DrawLine(&p, PointF(V.PX(V.XB(0), k), V.KY(k)),
                       PointF(V.PX(V.XB(LANES), k), V.KY(k)));
    }

    // 7 条车道之间的分隔线（严格交于灭点）
    for (int i = 0; i <= LANES; i++) {
        float a = (i == 0 || i == LANES) ? 130 : 84;
        Color c(a, 70, 160, 255);
        Pen p(c, (i == 0 || i == LANES) ? 2.0f : 1.2f);
        g.DrawLine(&p,
            PointF(V.PX(V.XB(i), V.kFar), V.KY(V.kFar)),
            PointF(V.PX(V.XB(i), kEnd),   V.KY(kEnd)));
    }

    // 车道底色（冷暖相间，视觉上更有层次）
    for (int i = 0; i < LANES; i++) {
        if (i % 2 == 0) continue;
        PointF q[4] = {
            PointF(V.PX(V.XB(i), V.kFar),     V.KY(V.kFar)),
            PointF(V.PX(V.XB(i + 1), V.kFar), V.KY(V.kFar)),
            PointF(V.PX(V.XB(i + 1), kEnd),   V.KY(kEnd)),
            PointF(V.PX(V.XB(i), kEnd),       V.KY(kEnd))
        };
        SolidBrush b(Color(22, 34, 84, 190));
        g.FillPolygon(&b, q, 4);
    }
}

// ============================ 静态背景层缓存 ============================
// 背景渐变、辉光、道路网格、车道线这些东西每帧都一样，
// 缓存成一张图，每帧只要 blit 一次，省下 9ms 左右。
static void PtDrawHitLine(Graphics& g);
static void PtDrawPads(Graphics& g);

static Bitmap* ptStaticLayer = nullptr;
static Bitmap* ptMenuLayer   = nullptr;   // 静态层 + 菜单遮罩，菜单直接贴这张
static int     ptStaticW = 0, ptStaticH = 0;   // 缓存的实际像素尺寸（= 逻辑尺寸 × viewScale）

// 第二十三步（新增3）：静态层按「屏幕像素」尺寸烘，而不是 1280×720。
// 原来缓存是逻辑尺寸，大屏时每帧都要 GDI+ 缩放一张 1280×720 的图 ——
// 1080p 实测单这一项就 140ms/帧（720p 是 1:1 拷贝，所以只有小窗不卡）。
static void PtBuildStatic(int w, int h) {
    float sc = (g_viewScale > 0.05f) ? g_viewScale : 1.0f;
    int pw = (int)((float)w * sc + 0.5f), ph = (int)((float)h * sc + 0.5f);
    if (pw < 1) pw = 1;
    if (ph < 1) ph = 1;

    delete ptStaticLayer;
    ptStaticLayer = new Bitmap(pw, ph, PixelFormat32bppPARGB);
    Graphics g(ptStaticLayer);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetPixelOffsetMode(PixelOffsetModeHalf);
    g.SetCompositingQuality(CompositingQualityHighSpeed);
    g.SetCompositingMode(CompositingModeSourceCopy);
    g.Clear(Color(255, 0, 0, 0));
    g.SetCompositingMode(CompositingModeSourceOver);
    g.ScaleTransform(sc, sc);                 // 之后一律用逻辑坐标画，落盘就是屏幕像素

    ptView.Setup((float)w, (float)h);
    PtDrawBackdrop(g, (float)w, (float)h);
    PtDrawRoad(g);

    // 再烘一张"压暗版"给菜单用，省掉每帧一次全屏半透明填充
    delete ptMenuLayer;
    ptMenuLayer = new Bitmap(pw, ph, PixelFormat32bppPARGB);
    {
        Graphics g2(ptMenuLayer);
        g2.SetCompositingMode(CompositingModeSourceCopy);
        g2.DrawImage(ptStaticLayer, 0, 0, pw, ph);   // 1:1，不走缩放
        g2.SetCompositingMode(CompositingModeSourceOver);
        // 判定线和键帽在菜单里永远是"暗着"的状态，一起烘进来
        g2.ScaleTransform(sc, sc);
        PtDrawHitLine(g2);
        PtDrawPads(g2);
        g2.ResetTransform();
        SolidBrush veil(Color(205, 0, 0, 0));
        g2.FillRectangle(&veil, RectF(0, 0, (float)pw, (float)ph));
    }

    ptStaticW = pw; ptStaticH = ph;
}

static void PtDrawHitLine(Graphics& g) {
    const PtView& V = ptView;
    float pulse = 0.5f + 0.5f * sinf((float)ptAnimT * 4.2f);

    // 判定线辉光
    for (int i = 3; i >= 1; i--) {
        Pen p(Color((BYTE)(28 * i * 0.6f + 20), 60, 200, 255), 2.0f + i * 3.0f);
        g.DrawLine(&p, PointF(V.XB(0), V.hitY), PointF(V.XB(LANES), V.hitY));
    }
    Pen core(Color(255, (BYTE)(180 + 60 * pulse), 245, 255), 2.6f);
    g.DrawLine(&core, PointF(V.XB(0), V.hitY), PointF(V.XB(LANES), V.hitY));
}

static void PtDrawPads(Graphics& g) {
    const PtView& V = ptView;
    float top = V.hitY + 7.0f;
    float hgt = V.H - top - 8.0f;
    if (hgt < 20) return;

    for (int i = 0; i < LANES; i++) {
        RectF r(V.XB(i) + 4.0f, top, V.lw - 8.0f, hgt);

        bool  wrong = ptWrongFlash[i] > 0;

        // 这个车道最近是否刚命中
        float lit = 0;
        for (auto& f : ptFX) {
            if (f.lane != i) continue;
            float a = 1.0f - (float)(f.age / f.life);
            if (f.type == 0) lit = std::max(lit, a);
        }

        Color fill, border;
        if (wrong) {
            fill   = Color(150, 120, 12, 20);
            border = Color(235, 255, 80, 80);
        } else if (lit > 0) {
            fill   = Color((BYTE)(50 + 70 * lit), 18, (BYTE)(64 + 70 * lit), 58);
            border = Color(235, 120, 255, 170);
        } else {
            fill   = Color(56, 16, 44, 96);
            border = Color(150, 60, 140, 235);
        }

        GraphicsPath p;
        PtRoundRect(p, r, 8.0f);
        SolidBrush fb(fill);
        g.FillPath(&fb, &p);
        Pen bp(border, 1.8f);
        g.DrawPath(&bp, &p);

        // 键帽上的数字
        wchar_t num[4];
        swprintf(num, 4, L"%d", i + 1);
        float fs = V.lw * 0.40f;
        if (fs > 30) fs = 30;
        if (fs < 12) fs = 12;
        Font& f = *PtGetFont(1, fs, FontStyleBold);
        Color tc = wrong ? Color(255, 255, 170, 170)
                  : (lit > 0 ? Color(255, 235, 255, 245) : Color(215, 175, 215, 255));
        // 键帽数字不加辉光：7 个数字 ×4 次叠绘，省下来很划算，肉眼几乎看不出差别
        PtText(g, num, f, r, tc, StringAlignmentCenter, StringAlignmentCenter, false);
    }
}

static void PtDrawTiles(Graphics& g) {
    // 从远到近画，保证近处的琴键盖住远处的
    for (int idx = (int)ptNotes.size() - 1; idx >= 0; idx--) {
        const PtNote& n = ptNotes[idx];
        if (n.state == 2) continue;                     // 漏接的不再画

        if (n.state == 1) {                             // 已命中：定格在判定线，绿色闪光
            double age = ptT - n.hitT;
            if (age > 0.30) continue;
            float alpha = (float)(1.0 - age / 0.30);
            PtDrawTile(g, n.lane, 1.0f, 1, alpha * 0.95f + 0.05f);
            continue;
        }

        float p = ptView.PofT(n.t - ptT);
        if (p < 0.0f) continue;                         // 还在极远处
        if (p > 1.10f) continue;                        // 已经滑过判定线

        bool red = (n.redT >= 0 && ptT - n.redT < 0.32);
        PtDrawTile(g, n.lane, p, red ? 2 : 0, 1.0f);
    }
}

static void PtDrawFX(Graphics& g) {
    const PtView& V = ptView;
    for (auto& f : ptFX) {
        float a = 1.0f - (float)(f.age / f.life);
        if (a <= 0) continue;
        float cxL = (V.XB(f.lane) + V.XB(f.lane + 1)) * 0.5f;

        if (f.type == 0) {   // 绿色扩散环
            float r = 12.0f + 150.0f * (float)(f.age / f.life);
            Pen p(Color((BYTE)(190 * a), 120, 255, 160), 3.0f * a + 0.6f);
            g.DrawEllipse(&p, cxL - r, V.hitY - r * 0.42f, r * 2, r * 0.84f);

            // 车道上竖直的光柱
            SolidBrush sb(Color((BYTE)(70 * a), 120, 255, 190));
            float hw = V.lw * 0.42f;
            g.FillRectangle(&sb, cxL - hw, V.hitY - 8 - 220 * (1 - a), hw * 2, 220 * (1 - a) + 8);
        } else {             // 红色警示环
            float r = 10.0f + 90.0f * (float)(f.age / f.life);
            Pen p(Color((BYTE)(200 * a), 255, 80, 80), 2.6f * a + 0.4f);
            g.DrawEllipse(&p, cxL - r, V.hitY - r * 0.4f, r * 2, r * 0.8f);
        }
    }

    // 火花（第二十三步：大屏时限量，超密的火花既看不清又拖帧）
    int drawn = 0;
    for (auto& p : ptParts) {
        float a = 1.0f - p.age / p.life;
        if (a <= 0) continue;
        if (ptLowQ && ++drawn > 40) break;
        SolidBrush b(Color((BYTE)(255 * a), p.c.GetR(), p.c.GetG(), p.c.GetB()));
        g.FillEllipse(&b, p.x - p.size, p.y - p.size, p.size * 2, p.size * 2);
    }
}

// ============================ HUD ============================
static void PtDrawHUD(Graphics& g) {
    const PtView& V = ptView;
    float W = V.W, H = V.H, S = V.S;

    // 计分器宽度先算出来，歌名要避开它
    float panelW = std::min(230.0f * S, W * 0.30f);

    // ---- 中上方：歌名 ----
    {
        float tsz = PtFitSize(g, ptSongs[ptSel].title, PtFF(0), 34.0f * S, FontStyleBold,
                            W - 2.0f * (panelW + 30.0f * S) - 20.0f * S);
        Font& ft = *PtGetFont(0, tsz, FontStyleBold);
        PtText(g, ptSongs[ptSel].title, ft, RectF(0, H * 0.020f, W, 48 * S),
              Color(255, 205, 242, 255), StringAlignmentCenter, StringAlignmentCenter, true);

        Font& fa = *PtGetFont(0, 15.0f * S, FontStyleRegular);
        PtText(g, ptSongs[ptSel].artist, fa, RectF(0, H * 0.020f + 44 * S, W, 26 * S),
              Color(185, 130, 185, 245), StringAlignmentCenter, StringAlignmentCenter, false);
    }

    // ---- 右上角：计分器 ----
    {
        float ph = 124.0f * S;
        RectF panel(W - panelW - 26 * S, 20 * S, panelW, ph);

        GraphicsPath p;
        PtRoundRect(p, panel, 12.0f * S);
        SolidBrush bg(Color(120, 6, 16, 40));
        g.FillPath(&bg, &p);
        Pen border(Color(150, 60, 170, 255), 1.6f);
        g.DrawPath(&border, &p);

        Font& fl = *PtGetFont(1, 13.0f * S, FontStyleBold);
        PtText(g, L"得  分", fl, RectF(panel.X, panel.Y + 6 * S, panel.Width, 18 * S),
              Color(190, 110, 190, 255), StringAlignmentCenter, StringAlignmentCenter, false);

        wchar_t buf[32];
        swprintf(buf, 32, L"%d", ptScore);
        Font& fs = *PtGetFont(1, 52.0f * S, FontStyleBold);
        PtText(g, buf, fs, RectF(panel.X, panel.Y + 20 * S, panel.Width, 64 * S),
              Color(255, 130, 235, 255), StringAlignmentCenter, StringAlignmentCenter, true);

        // 连击
        wchar_t cb[64];
        if (ptCombo >= 1) swprintf(cb, 64, L"COMBO  %d", ptCombo);
        else            swprintf(cb, 64, L"COMBO  -");
        Font& fc = *PtGetFont(1, 15.0f * S, FontStyleBold);
        Color cc = ptCombo >= 10 ? Color(255, 255, 220, 120) : Color(200, 150, 210, 255);
        PtText(g, cb, fc, RectF(panel.X, panel.Y + 86 * S, panel.Width, 22 * S),
              cc, StringAlignmentCenter, StringAlignmentCenter, ptCombo >= 10);
    }

    // ---- 左上角：进度 / 提示 ----
    {
        double cur = std::max(0.0, ptT), tot = ptSongLen;
        int cm = (int)(cur / 60), cs = (int)(cur) % 60;
        int tm = (int)(tot / 60), ts = (int)(tot) % 60;
        wchar_t tb[64];
        swprintf(tb, 64, L"%d:%02d / %d:%02d", cm, cs, tm, ts);
        Font& ft = *PtGetFont(1, 15.0f * S, FontStyleBold);
        PtText(g, tb, ft, RectF(24 * S, 22 * S, 220 * S, 22 * S), Color(200, 140, 200, 255),
              StringAlignmentNear, StringAlignmentCenter, false);

        Font& fh = *PtGetFont(0, 13.0f * S, FontStyleRegular);
        PtText(g, L"1~7 击键   SPACE 暂停   ESC 选曲", fh,
              RectF(24 * S, 46 * S, 460 * S, 20 * S),
              Color(130, 110, 150, 200), StringAlignmentNear, StringAlignmentCenter, false);

        if (!ptHasMusic) {
            PtText(g, L"♪ 该曲目无音频，静音跑谱面", fh, RectF(24 * S, 68 * S, 420 * S, 20 * S),
                  Color(150, 220, 170, 120), StringAlignmentNear, StringAlignmentCenter, false);
        }
    }

    // ---- 顶部进度条 ----
    {
        float prog = (float)(std::max(0.0, ptT) / ptSongLen);
        if (prog > 1) prog = 1;
        SolidBrush bg(Color(90, 40, 90, 160));
        g.FillRectangle(&bg, RectF(0, 0, W, 3 * S));
        SolidBrush fg(Color(220, 90, 210, 255));
        g.FillRectangle(&fg, RectF(0, 0, W * prog, 3 * S));
    }

    // ---- 倒计时 ----
    if (ptState == ST_COUNTDOWN) {
        int n = (int)ceil(-ptT);
        wchar_t buf[8];
        swprintf(buf, 8, L"%d", n > 0 ? n : 0);
        Font& f = *PtGetFont(1, 128.0f * S, FontStyleBold);
        float p = (float)(-ptT) - floorf((float)(-ptT));
        BYTE a = (BYTE)(120 + 135 * p);
        PtText(g, buf, f, RectF(0, H * 0.30f, W, 150 * S), Color(a, 150, 230, 255),
              StringAlignmentCenter, StringAlignmentCenter, true);
    }
}

// ============================ 菜单 / 结算 / 暂停 ============================
static void PtDrawMenu(Graphics& g) {
    const PtView& V = ptView;
    float W = V.W, H = V.H, S = V.S;

    Font& ft = *PtGetFont(1, std::min(64.0f * S, H * 0.10f), FontStyleBold);
    PtText(g, L"PIANO  TILES", ft, RectF(0, H * 0.085f, W, 78 * S),
          Color(255, 150, 235, 255), StringAlignmentCenter, StringAlignmentCenter, true);

    Font& fs = *PtGetFont(0, 17.0f * S, FontStyleRegular);
    PtText(g, L"钢 琴 键 · 七 键 节 奏", fs, RectF(0, H * 0.085f + 72 * S, W, 28 * S),
          Color(180, 120, 180, 240), StringAlignmentCenter, StringAlignmentCenter, false);

    // 曲目列表：宽度随窗口缩放，最高不超过窗口的 86%
    float rw    = std::min(640.0f * S, W * 0.86f);
    float rowH  = 60.0f * S;
    float listT = H * 0.30f;
    float x0    = W * 0.5f - rw * 0.5f;

    for (int i = 0; i < 4; i++) {
        RectF row(x0, listT + i * rowH, rw, rowH - 10 * S);
        bool sel = (i == ptSel);

        if (sel) {
            GraphicsPath p;
            PtRoundRect(p, row, 10.0f * S);
            SolidBrush b(Color(190, 14, 60, 130));
            g.FillPath(&b, &p);
            Pen bp(Color(240, 90, 210, 255), 2.0f);
            g.DrawPath(&bp, &p);
        }

        wchar_t idx[8];
        swprintf(idx, 8, L"%d", i + 1);
        Font& fn = *PtGetFont(1, 20.0f * S, FontStyleBold);
        PtText(g, idx, fn, RectF(row.X + 12 * S, row.Y, 30 * S, row.Height),
              sel ? Color(255, 150, 230, 255) : Color(140, 110, 140, 200),
              StringAlignmentCenter, StringAlignmentCenter, false);

        Font& f1 = *PtGetFont(0, 21.0f * S, FontStyleBold);
        PtText(g, ptSongs[i].title, f1, RectF(row.X + 50 * S, row.Y + 3 * S, rw * 0.62f, 30 * S),
              sel ? Color(255, 225, 250, 255) : Color(175, 150, 175, 230),
              StringAlignmentNear, StringAlignmentCenter, false);

        Font& f2 = *PtGetFont(0, 13.0f * S, FontStyleRegular);
        PtText(g, ptSongs[i].artist, f2, RectF(row.X + 50 * S, row.Y + 30 * S, rw * 0.66f, 22 * S),
              Color(150, 120, 160, 230), StringAlignmentNear, StringAlignmentCenter, false);

        Font& f3 = *PtGetFont(0, 12.0f * S, FontStyleRegular);
        PtText(g, ptAudioFound[i] ? L"♪ 音频就绪" : L"— 无音频 —",
              f3, RectF(row.GetRight() - 138 * S, row.Y, 128 * S, row.Height),
              ptAudioFound[i] ? Color(200, 120, 220, 160) : Color(150, 210, 130, 130),
              StringAlignmentFar, StringAlignmentCenter, false);
    }

    Font& fh = *PtGetFont(0, 15.0f * S, FontStyleRegular);
    PtText(g, L"↑ ↓ 选曲      ENTER 开始      1~7 击打      ESC 离开音乐楼", fh,
          RectF(0, listT + 4 * rowH + 16 * S, W, 26 * S), Color(160, 150, 190, 235),
          StringAlignmentCenter, StringAlignmentCenter, false);
}

static void PtDrawPause(Graphics& g) {
    const PtView& V = ptView;
    float S = V.S;
    SolidBrush veil(Color(170, 0, 0, 0));
    g.FillRectangle(&veil, RectF(0, 0, V.W, V.H));
    Font& f1 = *PtGetFont(1, 56.0f * S, FontStyleBold);
    PtText(g, L"PAUSED", f1, RectF(0, V.H * 0.40f, V.W, 70 * S),
          Color(255, 140, 220, 255), StringAlignmentCenter, StringAlignmentCenter, true);
    Font& f2 = *PtGetFont(0, 17.0f * S, FontStyleRegular);
    PtText(g, L"SPACE 继续      ESC 返回菜单      R 重新开始", f2,
          RectF(0, V.H * 0.40f + 78 * S, V.W, 28 * S), Color(190, 170, 200, 235),
          StringAlignmentCenter, StringAlignmentCenter, false);
}

static void PtDrawResult(Graphics& g) {
    const PtView& V = ptView;
    float W = V.W, H = V.H, S = V.S;
    SolidBrush veil(Color(190, 0, 0, 0));
    g.FillRectangle(&veil, RectF(0, 0, W, H));

    Font& f1 = *PtGetFont(1, 52.0f * S, FontStyleBold);
    PtText(g, L"COMPLETE", f1, RectF(0, H * 0.14f, W, 66 * S),
          Color(255, 150, 240, 255), StringAlignmentCenter, StringAlignmentCenter, true);

    float tsz = PtFitSize(g, ptSongs[ptSel].title, PtFF(0), 20.0f * S, FontStyleRegular, W * 0.90f);
    Font& ft = *PtGetFont(0, tsz, FontStyleRegular);
    PtText(g, ptSongs[ptSel].title, ft, RectF(0, H * 0.14f + 64 * S, W, 30 * S),
          Color(200, 140, 200, 245), StringAlignmentCenter, StringAlignmentCenter, false);

    // 大分数
    wchar_t buf[64];
    swprintf(buf, 64, L"%d", ptScore);
    Font& fs = *PtGetFont(1, 92.0f * S, FontStyleBold);
    PtText(g, buf, fs, RectF(0, H * 0.28f, W, 108 * S),
          Color(255, 120, 230, 255), StringAlignmentCenter, StringAlignmentCenter, true);

    double acc = ptTotal > 0 ? (100.0 * ptHits / ptTotal) : 0.0;

    wchar_t b1[64], b2[64], b3[64], b4[64];
    swprintf(b1, 64, L"%d", ptHits);
    swprintf(b2, 64, L"%d", ptMaxCombo);
    swprintf(b3, 64, L"%.1f%%", acc);
    swprintf(b4, 64, L"%d", ptWrong);

    struct Item { const wchar_t* label; std::wstring val; };
    Item items[4] = {
        { L"命中 / 总数", std::wstring(b1) + L" / " + std::to_wstring(ptTotal) },
        { L"最高连击",   b2 },
        { L"准确率",     b3 },
        { L"误按次数",   b4 },
    };

    // 四栏随窗口宽度自适应：优先按比例，窗口太窄就把间距压小
    float bw = 168.0f * S, gap = 16.0f * S;
    if (bw * 4 + gap * 3 > W * 0.92f) {
        bw = W * 0.92f / 4 * 0.94f;
        gap = W * 0.92f / 4 * 0.06f;
    }
    float startX = W * 0.5f - (bw * 4 + gap * 3) * 0.5f;
    float by = H * 0.50f;
    for (int i = 0; i < 4; i++) {
        RectF r(startX + i * (bw + gap), by, bw, 84 * S);
        GraphicsPath p;
        PtRoundRect(p, r, 10.0f * S);
        SolidBrush fb(Color(150, 10, 26, 58));
        g.FillPath(&fb, &p);
        Pen bp(Color(140, 70, 170, 255), 1.3f);
        g.DrawPath(&bp, &p);

        Font& fv = *PtGetFont(1, 26.0f * S, FontStyleBold);
        PtText(g, items[i].val, fv, RectF(r.X, r.Y + 10 * S, r.Width, 36 * S),
              Color(255, 200, 245, 255), StringAlignmentCenter, StringAlignmentCenter, false);
        Font& fl = *PtGetFont(0, 13.0f * S, FontStyleRegular);
        PtText(g, items[i].label, fl, RectF(r.X, r.Y + 50 * S, r.Width, 24 * S),
              Color(180, 150, 185, 235), StringAlignmentCenter, StringAlignmentCenter, false);
    }

    Font& fh = *PtGetFont(0, 17.0f * S, FontStyleRegular);
    PtText(g, L"ENTER / 点击 结算并返回音乐楼      R 再来一次      ESC 返回选曲", fh,
          RectF(0, by + 108 * S, W, 28 * S), Color(190, 170, 200, 240),
          StringAlignmentCenter, StringAlignmentCenter, false);
}

// ============================ 垂直同步 ============================
// DwmFlush() 会一直阻塞到桌面下一次合成（也就是显示器刷新），
// 让每帧都正好落在一次刷新上，不会出现"画了 120 帧、丢了一半"的抖动。
typedef HRESULT (WINAPI* PFN_DwmFlush)();
static PFN_DwmFlush ptDwmFlush = nullptr;
static void PtRender(Graphics& g, int w, int h) {
    // 第二十三步（新增3）：窗口比逻辑画布大（>1.25 倍）就降一档画质。
    // 抗锯齿是按屏幕像素算钱的：1440p 的填充面积是 720p 的 4 倍，4K 是 9 倍。
    ptLowQ = (g_viewScale > 1.25f);
    g.SetSmoothingMode(ptLowQ ? SmoothingModeHighSpeed : SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
    g.SetPixelOffsetMode(PixelOffsetModeHalf);
    g.SetCompositingQuality(CompositingQualityHighSpeed);

    ptView.Setup((float)w, (float)h);

    // 背景 + 道路整层直接贴缓存（不透明，用 SourceCopy 走最快的拷贝路径）
    bool menu = (ptState == ST_MENU);
    float sc0 = (g_viewScale > 0.05f) ? g_viewScale : 1.0f;
    int   pw0 = (int)((float)w * sc0 + 0.5f), ph0 = (int)((float)h * sc0 + 0.5f);
    if (pw0 < 1) pw0 = 1;
    if (ph0 < 1) ph0 = 1;
    if (!ptStaticLayer || ptStaticW != pw0 || ptStaticH != ph0) PtBuildStatic(w, h);
    {
        // 临时换成「屏幕像素 + 居中偏移」的单位变换，让这张图 1:1 贴上去（不缩放 = 最快）
        Matrix back;
        g.GetTransform(&back);
        g.ResetTransform();
        g.TranslateTransform(g_viewOffX, g_viewOffY);
        g.SetCompositingMode(CompositingModeSourceCopy);
        g.DrawImage(menu ? ptMenuLayer : ptStaticLayer, 0, 0, pw0, ph0);
        g.SetCompositingMode(CompositingModeSourceOver);
        g.SetTransform(&back);
    }

    PtDrawDust(g, (float)w, (float)h, menu ? 0.18f : 1.0f);
    if (!menu) {                 // 菜单层里已经烘好了判定线和键帽
        PtDrawTiles(g);
        PtDrawHitLine(g);
        PtDrawPads(g);
        PtDrawFX(g);
    }

    if (ptShowFps) {
        wchar_t fb[64];
        swprintf(fb, 64, L"%.0f FPS   %.2f ms", ptFpsAvg, ptFrameMs);
        Font& ff = *PtGetFont(1, 13.0f * ptView.S, FontStyleBold);
        PtText(g, fb, ff, RectF(14 * ptView.S, h - 26 * ptView.S, 220 * ptView.S, 20 * ptView.S),
              Color(220, 130, 255, 170), StringAlignmentNear, StringAlignmentCenter, false);
    }

    if (ptState == ST_MENU) {
        PtDrawMenu(g);
    } else if (ptState == ST_LOADING) {
        SolidBrush veil(Color(235, 0, 0, 0));
        g.FillRectangle(&veil, RectF(0, 0, (float)w, (float)h));
        Font& f1 = *PtGetFont(0, 26.0f, FontStyleBold);
        PtText(g, L"正在解码音乐…", f1, RectF(0, h * 0.44f, (float)w, 40),
              Color(255, 150, 230, 255), StringAlignmentCenter, StringAlignmentCenter, true);
        Font& f2 = *PtGetFont(0, 14.0f, FontStyleRegular);
        PtText(g, ptSongs[ptSel].title, f2, RectF(0, h * 0.44f + 44, (float)w, 26),
              Color(170, 130, 175, 235), StringAlignmentCenter, StringAlignmentCenter, false);
    } else if (ptState == ST_RESULT) {
        PtDrawResult(g);
    } else {
        PtDrawHUD(g);
        if (ptState == ST_PAUSE) PtDrawPause(g);
    }
}
// ---------------- 小游戏与主程序的对接 ----------------
static void PtBegin()
{
    static bool mfStarted = false;
    if (!mfStarted) { MFStartup(MF_VERSION); mfStarted = true; }   // 解码 mp3 前必须启动

    ptDir = ResDir() + L"\\钢琴键游戏";     // 音频从「钢琴键游戏」目录里找 mp3
    ptActive = true;
    ptNoSound = false;
    PtInitSfx();
    PtScanAudio();
    ptHasMusic = false;
    ptAudio.Close();
    ptState = ST_MENU;
    ptSel = 0;
    ptNotes.clear();
    ptFX.clear();
    ptParts.clear();
    ptAnimT = 0.0;
}

// 离开小游戏、回到音乐楼的对话（没弹就不消耗时间）
static void PtQuit();

// 结算：依据积分
static void PtSettle()
{
    ptAudio.Close();
    ptActive = false;
    g_pianoPlayed = true;                // 第二十三步（新增3）：弹完这一曲，本周不能再弹

    int sc = ptScore;
    AddStat(2, -10);                     // 弹琴费神
    AddStat(3, sc / 5);                  // 快乐随积分（开发说明只写了「依据积分」，数值待确认）
    g_faceState.Refresh(g_stat[2], g_stat[3]);
    g_loc = LOC_CLASS;
    CommitDeltas(DN_MUSIC_END);          // 弹「属性变化」，确定后推时段回地图
}

static void PtKeyDown(WPARAM vk)
{
    // 第二十三步（新增3）：已经弹过一曲 → R（再来一次）不再生效，直接退出
    if (g_pianoPlayed && (vk == 'R')) {
        Toast(L"这周已经弹过一曲了");
        PtQuit();
        return;
    }
    if (ptState == ST_MENU) {
        if (vk == VK_UP)        ptSel = (ptSel + 3) % 4;
        else if (vk == VK_DOWN) ptSel = (ptSel + 1) % 4;
        else if (vk == VK_RETURN) PtStartSong(ptSel);
        else if (vk == VK_ESCAPE) PtQuit();
        return;
    }
    if (ptState == ST_RESULT) {
        if (vk == VK_RETURN)      PtSettle();          // 结算并返回
        else if (vk == 'R')       PtStartSong(ptSel);  // 再来一次
        else if (vk == VK_ESCAPE) PtBackToMenu();
        return;
    }
    if (ptState == ST_PAUSE) {
        if (vk == VK_SPACE) { ptState = ST_PLAY; if (ptHasMusic && !ptNoSound) ptAudio.Play(); }
        else if (vk == VK_ESCAPE) PtBackToMenu();
        else if (vk == 'R')       PtStartSong(ptSel);
        return;
    }
    if (vk == VK_ESCAPE) { PtBackToMenu(); return; }
    if (vk == VK_SPACE) {
        ptState = ST_PAUSE;
        if (ptHasMusic) ptAudio.Pause();
        return;
    }
    int lane = -1;
    if (vk >= '1' && vk <= '7')                        lane = (int)(vk - '1');
    else if (vk >= VK_NUMPAD1 && vk <= VK_NUMPAD7)     lane = (int)(vk - VK_NUMPAD1);
    if (lane >= 0) PtPress(lane);
}

// ================= 音乐楼 =================
static void EnterMusic()
{
    g_loc = LOC_MUSIC;
    g_shopOpen = false;
    g_shopKind = 0;
    g_shopAnim = 0.0f;
    g_dlgSmall  = false;
    g_dlgActive = true;
    g_dlgAsk    = true;
    g_dlgAskSubj = -1;
    g_dlgPrompt = L"要弹奏一曲吗";
    g_dlgScript = nullptr;
    g_dlgScriptN = 0;
    g_dlgAfter  = DLG_AFTER_NONE;
    g_dlgOptN   = 2;
    g_dlgOpt[0] = L"展示我的音乐才华";
    g_dlgOpt[1] = L"保留择偶权";
    DlgSetLine(0);
}

// 第二十三步（新增3）：离开音乐楼回地图（弹过之后就不再问「要弹奏一曲吗」了）
static void MusicLeave()
{
    g_dlgActive = false;
    g_dlgAsk    = false;
    g_dlgOptN   = 0;
    g_loc = LOC_CLASS;
    SetScene(SCENE_MAP);
}

static void PtQuit()
{
    ptAudio.Close();
    ptActive = false;
    // 第二十三步（新增3）：钢琴每周目只能玩一次，弹过就直接离开音乐楼
    if (g_pianoPlayed) { MusicLeave(); return; }
    EnterMusic();               // 回到「要弹奏一曲吗」
}

static void MusicChoose(int idx)
{
    if (idx == 0) {
        // 展示我的音乐才华 → 打开钢琴键游戏
        if (g_pianoPlayed) {                  // 第二十三步（新增3）：已经弹过一曲了
            Toast(L"这周已经弹过一曲了");
            MusicLeave();
            return;
        }
        g_dlgActive = false;
        g_dlgAsk    = false;
        g_dlgOptN   = 0;
        PtBegin();
    } else {
        // 保留择偶权 → 回地图
        MusicLeave();
    }
}

// ============================================================
//  第二十一步：事件4 摘玉米小游戏
//  移植自「摘玉米游戏\摘玉米游戏.cpp」
//     · 第一人称站在玉米田里，按住 W（或 ↑）前进，远处的玉米从雾里由小变大
//     · 点击走近的玉米 → 伸手摘下，得 1 分；限时 30 秒
//     · 每周目只能玩一次（g_cornPlayed）
//  移植改动：
//     · 整屏接管（cnActive），坐标直接用 1280x720 逻辑画布，不再有虚拟画布缩放
//     · 玉米走 GDI+ 的 mip 链绘制（原版是 DIB + AlphaBlend），文字沿用本工程字体
//     · 音效去掉（主游戏没有音频线程），逻辑完全一致
// ============================================================
// （cnActive 声明在 g_basketActive 旁边，ResetGameState 也要用）
enum { CN_START = 0, CN_PLAY, CN_END };
static int   cnState    = CN_START;
static float cnTime     = 0.0f;
static float cnTimeLeft = 30.0f;
static DWORD cnStartTick = 0;
static int   cnScore    = 0;
static float cnWalk     = 0.0f;
static float cnLastSpawn = 0.0f;
static float cnSpawnGap = 12.0f;
static float cnBobPhase  = 0.0f;
static float cnMoveBlend = 0.0f;
static float cnCamX = 0.0f, cnCamY = 0.0f;
static float cnMouseX = -1.0f, cnMouseY = -1.0f;
static float cnLastMove = 0.0f;
static int   cnLastTickSec = -1;

static const float CN_DMAX      = 100.0f;   // 可视最远距离
static const float CN_PICK_D    = 34.0f;    // 够得着就能摘
static const float CN_WALK_SPD  = 40.0f;    // 前进速度（单位/秒）
static const float CN_GAME_TIME = 30.0f;    // 限时 30 秒
static const float CN_CORN_H    = 300.0f;   // 最近处玉米的像素高度（720 画布上比原版放大一点）
static const float CN_HAND_DUR  = 0.74f;

struct CnCorn { float d, lane, rot, phase, size; };
static std::vector<CnCorn> cnCorns;

struct CnHand { bool active; float t, tx, ty, cornScale; };
static CnHand cnHand = { false, 0.0f, 0.0f, 0.0f, 1.0f };

struct CnPart { float x, y, vx, vy, rot, vr, life, maxLife, size; int r, g, b; };
static std::vector<CnPart> cnParts;
struct CnPop  { float x, y, life, maxLife, size; std::wstring txt; Color col; };
static std::vector<CnPop>  cnPops;

// 玉米贴图（mip 链）+ 玉米田背景
static Bitmap* cnCornMip[10] = { nullptr };
static int     cnCornMipN = 0;
static float   cnCornAR   = 1.0f;
static Bitmap* cnBgSrc    = nullptr;      // 玉米田.jpg 原图
static Bitmap* cnBgCache  = nullptr;      // 按 1280x720 预缩放 + 上下压暗
static Bitmap* cnIcon     = nullptr;      // 物品栏用的小玉米图标

static inline float CnClamp(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
static inline float CnRnd(float a, float b) { return a + (b - a) * (rand() / (float)RAND_MAX); }
static inline float CnEaseOut3(float t) { float u = 1.0f - t; return 1.0f - u * u * u; }
static inline float CnEaseIn3(float t)  { return t * t * t; }

// 透视：z = 0（最远）~ 1（贴脸）
static inline float CnZ(float d)     { return 1.0f - d / CN_DMAX; }
static float CnScaleAt(float z)      { return 0.052f + 0.948f * powf(z > 0.0f ? z : 0.0f, 1.85f); }
static float CnYBase(float z)
{
    float vpy = (float)BASE_H * 0.5f;
    return vpy + ((float)BASE_H * 1.02f - vpy) * powf(z > 0.0f ? z : 0.0f, 1.55f);
}
static float CnXAt(float z, float lane)
{
    return (float)BASE_W * 0.5f + lane * ((float)BASE_W * 0.36f) * powf(z > 0.0f ? z : 0.0f, 1.70f);
}

struct CnView { float cx, yb, w, h, scale, alpha; bool pickable; };
static CnView CnViewOf(const CnCorn& c)
{
    float z = CnZ(c.d);
    CnView v;
    v.scale    = CnScaleAt(z);
    v.h        = CN_CORN_H * v.scale * c.size;
    v.w        = v.h * cnCornAR;
    v.cx       = CnXAt(z, c.lane);
    v.yb       = CnYBase(z);
    v.alpha    = CnClamp((z - 0.05f) / 0.32f, 0.09f, 1.0f);
    v.pickable = (c.d <= CN_PICK_D && c.d > -8.0f);
    return v;
}

// ---------------- 资源 ----------------
// 兜底：玉米.jpg 是白底，现场把接近纯白的像素变透明
static void CnRemoveWhite(Bitmap* bmp)
{
    if (!bmp || bmp->GetLastStatus() != Ok) return;
    int w = bmp->GetWidth(), h = bmp->GetHeight();
    Rect rc(0, 0, w, h);
    BitmapData bd;
    if (bmp->LockBits(&rc, ImageLockModeRead | ImageLockModeWrite, PixelFormat32bppARGB, &bd) != Ok) return;
    BYTE* base = (BYTE*)bd.Scan0;
    for (int y = 0; y < h; ++y) {
        BYTE* row = base + (long)y * bd.Stride;
        for (int x = 0; x < w; ++x) {
            BYTE* p = row + x * 4;
            BYTE b = p[0], g = p[1], r = p[2];
            int mx = r > g ? (r > b ? r : b) : (g > b ? g : b);
            int mn = r < g ? (r < b ? r : b) : (g < b ? g : b);
            int lum = (r * 30 + g * 59 + b * 11) / 100;
            if (lum >= 232 && mx - mn <= 18) p[3] = 0;
        }
    }
    bmp->UnlockBits(&bd);
}

static Bitmap* CnLoadImg(const wchar_t* sub, const wchar_t* name)
{
    std::wstring path = ResDir() + L"\\" + sub + L"\\" + name;
    Bitmap* b = Bitmap::FromFile(path.c_str());
    if (b && b->GetLastStatus() != Ok) { delete b; b = nullptr; }
    return b;
}

// 按 1280x720 预生成背景（原图 1280x1700，取偏下的一段，正好是玉米地）
static void CnBuildBg()
{
    delete cnBgCache;
    cnBgCache = nullptr;
    if (!cnBgSrc) return;
    cnBgCache = new Bitmap(BASE_W, BASE_H, PixelFormat32bppARGB);
    Graphics g(cnBgCache);
    g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    g.SetPixelOffsetMode(PixelOffsetModeHighQuality);
    float bw = (float)cnBgSrc->GetWidth(), bh = (float)cnBgSrc->GetHeight();
    float sc = ((float)BASE_W / bw > (float)BASE_H / bh) ? ((float)BASE_W / bw) : ((float)BASE_H / bh);
    float dw = bw * sc, dh = bh * sc;
    g.DrawImage(cnBgSrc, RectF(((float)BASE_W - dw) * 0.5f, ((float)BASE_H - dh) * 0.58f, dw, dh),
                0.0f, 0.0f, bw, bh, UnitPixel);
    LinearGradientBrush top(Point(0, 0), Point(0, 110), Color(90, 20, 45, 30), Color(0, 20, 45, 30));
    g.FillRectangle(&top, 0.0f, 0.0f, (float)BASE_W, 110.0f);
    LinearGradientBrush bot(Point(0, BASE_H - 130), Point(0, BASE_H), Color(0, 12, 30, 18), Color(80, 12, 30, 18));
    g.FillRectangle(&bot, 0.0f, (float)(BASE_H - 130), (float)BASE_W, 130.0f);
}

static void CnLoad()
{
    cnBgSrc = CnLoadImg(L"摘玉米游戏", L"玉米田.jpg");
    Bitmap* corn = CnLoadImg(L"摘玉米游戏", L"玉米_去白.png");
    if (!corn) {
        corn = CnLoadImg(L"摘玉米游戏", L"玉米.jpg");
        if (corn) CnRemoveWhite(corn);
    }
    if (corn) {
        cnCornMip[0] = corn;
        cnCornMipN   = 1;
        while (cnCornMipN < 8 && cnCornMip[cnCornMipN - 1]->GetWidth() > 80) {
            Bitmap* s = cnCornMip[cnCornMipN - 1];
            int nw = (int)(s->GetWidth()  * 0.72f), nh = (int)(s->GetHeight() * 0.72f);
            if (nw < 1) nw = 1;
            if (nh < 1) nh = 1;
            Bitmap* b = new Bitmap(nw, nh, PixelFormat32bppARGB);
            Graphics gg(b);
            gg.SetInterpolationMode(InterpolationModeHighQualityBicubic);
            gg.SetPixelOffsetMode(PixelOffsetModeHighQuality);
            gg.DrawImage(s, RectF(0.0f, 0.0f, (REAL)nw, (REAL)nh),
                         0.0f, 0.0f, (REAL)s->GetWidth(), (REAL)s->GetHeight(), UnitPixel);
            cnCornMip[cnCornMipN++] = b;
        }
        cnCornAR = (float)corn->GetWidth() / (float)corn->GetHeight();
        // 物品栏用的小图标（先缩好；水彩玉米偏淡，缩小时顺手把饱和度提一点）
        int ih = 88, iw = (int)(ih * cnCornAR);
        cnIcon = new Bitmap(iw, ih, PixelFormat32bppARGB);
        Graphics gi(cnIcon);
        gi.SetInterpolationMode(InterpolationModeHighQualityBicubic);
        gi.SetPixelOffsetMode(PixelOffsetModeHighQuality);
        gi.DrawImage(corn, RectF(0.0f, 0.0f, (REAL)iw, (REAL)ih),
                     0.0f, 0.0f, (REAL)corn->GetWidth(), (REAL)corn->GetHeight(), UnitPixel);
    }
    CnBuildBg();
}

// ---------------- 逻辑 ----------------
static void CnAddPop(float x, float y, const wchar_t* t, Color c, float size)
{
    CnPop p;
    p.x = x; p.y = y; p.txt = t; p.col = c; p.size = size;
    p.maxLife = p.life = 0.85f;
    cnPops.push_back(p);
}

static void CnSpawnCorn(float d)
{
    CnCorn c;
    c.d     = d;
    c.lane  = (rand() % 2) ? CnRnd(0.55f, 0.99f) : -CnRnd(0.55f, 0.99f);
    c.rot   = CnRnd(-6.0f, 6.0f);
    c.phase = CnRnd(0.0f, 6.28f);
    c.size  = CnRnd(0.88f, 1.12f);
    cnCorns.push_back(c);
}

static void CnAdvance(float step)
{
    if (cnState != CN_PLAY) return;
    cnWalk     += step;
    cnLastMove  = 0.0f;
    cnBobPhase += step * 0.19f;
    for (size_t i = 0; i < cnCorns.size(); ++i) cnCorns[i].d -= step;
    if (cnWalk - cnLastSpawn > cnSpawnGap) {
        cnLastSpawn = cnWalk;
        cnSpawnGap  = CnRnd(8.0f, 12.0f);
        if (cnCorns.size() < 14) CnSpawnCorn(CN_DMAX + CnRnd(0.0f, 6.0f));
    }
}

static void CnBurstLeaves(float x, float y, float scale)
{
    for (int i = 0; i < 9; ++i) {
        CnPart p;
        p.x = x + CnRnd(-12, 12) * scale;
        p.y = y + CnRnd(-18, 18) * scale;
        float a  = CnRnd(-2.6f, 2.6f);
        float sp = CnRnd(60.0f, 190.0f) * scale;
        p.vx = sinf(a) * sp;
        p.vy = -cosf(a) * sp * 0.8f - 60.0f * scale;
        p.rot = CnRnd(0, 6.28f);
        p.vr  = CnRnd(-8.0f, 8.0f);
        p.size = CnRnd(5.0f, 11.0f) * (0.6f + scale * 0.7f);
        p.maxLife = p.life = CnRnd(0.45f, 0.8f);
        int t = rand() % 3;
        p.r = (t == 0) ? 232 : (t == 1 ? 196 : 250);
        p.g = (t == 0) ? 210 : (t == 1 ? 224 : 232);
        p.b = (t == 0) ? 128 : (t == 1 ? 120 : 150);
        cnParts.push_back(p);
    }
}

static void CnStartGame()
{
    cnState      = CN_PLAY;
    cnStartTick  = GetTickCount();
    cnTimeLeft   = CN_GAME_TIME;
    cnScore      =  0;
    cnWalk       = 0.0f;
    cnLastSpawn  = 0.0f;
    cnSpawnGap   = CnRnd(10.0f, 14.0f);
    cnLastTickSec = -1;
    cnLastMove   = 0.0f;
    cnCorns.clear();
    cnParts.clear();
    cnPops.clear();
    cnHand.active = false;

    float d = 24.0f;                       // 开场先摆一排，从近到远
    while (d < CN_DMAX - 4.0f) {
        CnSpawnCorn(d + CnRnd(-2.0f, 2.0f));
        d += CnRnd(10.0f, 13.0f);
    }
}

static int CnPickHitTest(float mx, float my)
{
    int   best = -1;
    float bestD = 1e9f;
    for (size_t i = 0; i < cnCorns.size(); ++i) {
        CnView v = CnViewOf(cnCorns[i]);
        float hw  = v.w * 0.46f + 6.0f;
        float top = v.yb - v.h * 0.94f;
        float bot = v.yb - v.h * 0.06f;
        if (mx > v.cx - hw && mx < v.cx + hw && my > top && my < bot) {
            if (cnCorns[i].d < bestD) { bestD = cnCorns[i].d; best = (int)i; }
        }
    }
    return best;
}

static void CnPick(int idx)
{
    CnView v = CnViewOf(cnCorns[idx]);
    cnScore++;
    cnHand.active    = true;
    cnHand.t         = 0.0f;
    cnHand.tx        = v.cx;
    cnHand.ty        = v.yb - v.h * 0.55f;
    cnHand.cornScale = (v.scale > 0.55f) ? v.scale : 0.55f;
    CnBurstLeaves(v.cx, v.yb - v.h * 0.5f, CnClamp(v.scale, 0.4f, 1.2f));
    CnAddPop(v.cx, v.yb - v.h * 0.95f, L"+1", Color(255, 255, 236, 120), 40.0f + 14.0f * v.scale);
    cnCorns.erase(cnCorns.begin() + idx);
}

static void CnUpdate(float dt)
{
    cnTime += dt;

    if (cnState == CN_PLAY) {
        float elapsed = (float)(GetTickCount() - cnStartTick) / 1000.0f;
        cnTimeLeft = CN_GAME_TIME - elapsed;
        if (cnTimeLeft <= 0.0f) { cnTimeLeft = 0.0f; cnState = CN_END; }

        bool moving = (GetAsyncKeyState('W') & 0x8000) != 0 ||
                      (GetAsyncKeyState(VK_UP) & 0x8000) != 0;
        if (moving) { CnAdvance(CN_WALK_SPD * dt); cnMoveBlend = CnClamp(cnMoveBlend + dt * 7.0f, 0.0f, 1.0f); }
        else        { cnLastMove += dt;            cnMoveBlend = CnClamp(cnMoveBlend - dt * 5.0f, 0.0f, 1.0f); }

        for (size_t i = 0; i < cnCorns.size();) {          // 走过头的就错过了
            if (cnCorns[i].d < -4.0f) cnCorns.erase(cnCorns.begin() + i);
            else ++i;
        }
        int sec = (int)ceilf(cnTimeLeft);
        if (sec <= 5 && sec > 0) cnLastTickSec = sec;
    }

    cnCamY = sinf(cnBobPhase) * 3.4f * cnMoveBlend;
    cnCamX = cosf(cnBobPhase * 0.5f) * 1.9f * cnMoveBlend;

    if (cnHand.active) {
        cnHand.t += dt;
        if (cnHand.t > CN_HAND_DUR) cnHand.active = false;
    }
    for (size_t i = 0; i < cnParts.size();) {
        CnPart& p = cnParts[i];
        p.life -= dt;
        if (p.life <= 0) { cnParts.erase(cnParts.begin() + i); continue; }
        p.vy += 620.0f * dt;
        p.vx *= (1.0f - 1.6f * dt);
        p.x  += p.vx * dt;
        p.y  += p.vy * dt;
        p.rot += p.vr * dt;
        ++i;
    }
    for (size_t i = 0; i < cnPops.size();) {
        CnPop& p = cnPops[i];
        p.life -= dt;
        if (p.life <= 0) { cnPops.erase(cnPops.begin() + i); continue; }
        p.y -= 46.0f * dt;
        ++i;
    }
}

// ---------------- 绘制 ----------------
static void CnText(Graphics& g, const wchar_t* s, float cx, float cy, float size,
                   Color col, float width = 900.0f)
{
    Font font(g_famUI, size, FontStyleBold, UnitPixel);
    StringFormat sf;
    sf.SetAlignment(StringAlignmentCenter);
    sf.SetLineAlignment(StringAlignmentCenter);
    RectF layout(cx - width * 0.5f, cy - size * 1.1f, width, size * 2.2f);
    SolidBrush outline(Color(255, 40, 66, 44));
    for (int dx = -2; dx <= 2; dx += 2)
        for (int dy = -2; dy <= 2; dy += 2)
            if (dx || dy) {
                RectF o = layout;
                o.X += (REAL)dx;
                o.Y += (REAL)dy;
                g.DrawString(s, -1, &font, o, &sf, &outline);
            }
    SolidBrush fill(col);
    g.DrawString(s, -1, &font, layout, &sf, &fill);
}

static void CnPanel(Graphics& g, RectF r, Color fill)
{
    GraphicsPath sp;
    MakeRoundRect(sp, RectF(r.X + 5.0f, r.Y + 6.0f, r.Width, r.Height), 16.0f);
    SolidBrush sb(Color(60, 0, 0, 0));
    g.FillPath(&sb, &sp);
    GraphicsPath p;
    MakeRoundRect(p, r, 16.0f);
    SolidBrush f(fill);
    g.FillPath(&f, &p);
    Pen pen(Color(255, 44, 66, 50), 3.0f);
    pen.SetLineJoin(LineJoinRound);
    g.DrawPath(&pen, &p);
}

static Bitmap* CnMipFor(float w)
{
    if (cnCornMipN <= 0) return nullptr;
    int mi = 0;
    for (int i = 0; i < cnCornMipN; ++i)
        if ((float)cnCornMip[i]->GetWidth() >= w * 0.9f) mi = i;
    return cnCornMip[mi];
}

// white = true 时整颗涂白，用来给「可采摘」的玉米描一圈白色贴纸边
static void CnDrawCornAt(Graphics& g, float cx, float yb, float w, float h, float alpha, bool white)
{
    Bitmap* src = CnMipFor(w);
    if (!src || alpha <= 0.02f) return;
    float wr = white ? 1.0f : 0.0f;
    float kr = white ? 0.0f : 1.0f;
    ColorMatrix cm = {
        kr,   0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, kr,   0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, kr,   0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, alpha, 0.0f,
        wr,   wr,   wr,   0.0f, 1.0f
    };
    ImageAttributes ia;
    ia.SetColorMatrix(&cm, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);
    g.DrawImage(src, RectF(cx - w * 0.5f, yb - h, w, h),
                0.0f, 0.0f, (REAL)src->GetWidth(), (REAL)src->GetHeight(), UnitPixel, &ia);
}

// 物品栏 / HUD 里的小玉米
static void CnDrawIcon(Graphics& g, float cx, float cy, float h)
{
    if (!cnIcon) return;
    float w = h * cnCornAR;
    g.DrawImage(cnIcon, RectF(cx - w * 0.5f, cy + h * 0.5f - h, w, h),
                0.0f, 0.0f, (REAL)cnIcon->GetWidth(), (REAL)cnIcon->GetHeight(), UnitPixel);
}

// 物品栏格子里的玉米（没有图就程序画一根）
static void DrawCornIcon(Graphics& g, RectF r, float t)
{
    if (cnIcon) {
        float bw = (float)cnIcon->GetWidth(), bh = (float)cnIcon->GetHeight();
        float sc = (r.Width / bw < r.Height / bh) ? (r.Width / bw) : (r.Height / bh);
        sc *= 1.06f;
        float dw = bw * sc, dh = bh * sc;
        g.DrawImage(cnIcon, RectF(r.X + (r.Width - dw) * 0.5f, r.Y + (r.Height - dh) * 0.5f, dw, dh),
                    0.0f, 0.0f, bw, bh, UnitPixel);
        return;
    }
    float cx = r.X + r.Width * 0.5f;
    float cy = r.Y + r.Height * 0.5f;
    float hh = r.Height * 0.62f, ww = hh * 0.46f;
    SolidBrush body(Color(Alpha(255, t), 250, 214, 84));
    g.FillEllipse(&body, cx - ww * 0.5f, cy - hh * 0.5f, ww, hh);
    Pen ink(Color(Alpha(255, t), 120, 92, 40), 2.2f);
    g.DrawEllipse(&ink, cx - ww * 0.5f, cy - hh * 0.5f, ww, hh);
    SolidBrush leaf(Color(Alpha(255, t), 96, 168, 76));
    g.FillEllipse(&leaf, cx - ww * 0.5f - 5.0f, cy - 6.0f, ww * 0.7f, hh * 0.34f);
}

static void CnDrawCorns(Graphics& g)
{
    static std::vector<int> order;
    order.resize(cnCorns.size());
    for (size_t i = 0; i < order.size(); ++i) order[i] = (int)i;
    std::sort(order.begin(), order.end(), [](int a, int b) {
        return cnCorns[a].d > cnCorns[b].d;                 // 远的先画
    });

    int hover = -1;
    if (cnMouseX >= 0.0f) {
        int hh = CnPickHitTest(cnMouseX, cnMouseY);
        if (hh >= 0 && cnCorns[hh].d <= CN_PICK_D) hover = hh;
    }

    for (size_t oi = 0; oi < order.size(); ++oi) {
        int i = order[oi];
        const CnCorn& c = cnCorns[i];
        CnView v = CnViewOf(c);
        float z = CnZ(c.d);
        if (z < -0.02f) continue;

        float sway = sinf(cnTime * 1.5f + c.phase) * 1.4f;
        float bob  = sinf(cnTime * 1.5f + c.phase + 1.0f) * 1.6f * v.scale;
        float cx = v.cx + cnCamX + sway * v.scale;
        float yb = v.yb + cnCamY + bob;
        float boost = (i == hover) ? 1.05f : 1.0f;
        float w = v.w * boost, h = v.h * boost;
        bool  hot = (v.pickable && cnState == CN_PLAY);

        // 地面阴影
        if (v.scale > 0.12f) {
            float sw = w * 0.78f, sh = h * 0.22f;
            SolidBrush shb(Color((BYTE)(90 * v.alpha), 12, 30, 16));
            g.FillEllipse(&shb, cx - sw * 0.5f, yb - sh * 0.55f, sw, sh);
        }
        if (hot) CnDrawCornAt(g, cx, yb + h * 0.012f, w * 1.07f, h * 1.07f, 1.0f, true);
        CnDrawCornAt(g, cx, yb, w, h, v.alpha, false);

        if (hot) {                                   // 可采摘：头顶一个小三角
            float pulse = 0.5f + 0.5f * sinf(cnTime * 5.2f + c.phase);
            float ay  = yb - h * 0.99f - 14.0f - pulse * 7.0f;
            float asz = 15.0f + 9.0f * v.scale;
            PointF tri[3] = { PointF(cx, ay + asz), PointF(cx - asz * 0.9f, ay - asz * 0.55f),
                              PointF(cx + asz * 0.9f, ay - asz * 0.55f) };
            SolidBrush tb(Color(255, 255, 232, 110));
            g.FillPolygon(&tb, tri, 3);
            Pen tp(Color(255, 62, 80, 42), 2.0f);
            g.DrawPolygon(&tp, tri, 3);
        }
    }
}

static void CnDrawParticles(Graphics& g)
{
    for (size_t i = 0; i < cnParts.size(); ++i) {
        const CnPart& p = cnParts[i];
        BYTE a = (BYTE)(255.0f * (p.life / p.maxLife));
        SolidBrush br(Color(a, (BYTE)p.r, (BYTE)p.g, (BYTE)p.b));
        GraphicsState st = g.Save();
        g.TranslateTransform(p.x, p.y);
        g.RotateTransform(p.rot * 57.2958f);
        g.FillEllipse(&br, -p.size * 0.6f, -p.size * 0.22f, p.size * 1.2f, p.size * 0.45f);
        g.Restore(st);
    }
}

static void CnDrawPopups(Graphics& g)
{
    for (size_t i = 0; i < cnPops.size(); ++i) {
        const CnPop& p = cnPops[i];
        float k = p.life / p.maxLife;
        BYTE a = (BYTE)(255.0f * CnClamp(k * 1.6f, 0.0f, 1.0f));
        CnText(g, p.txt.c_str(), p.x, p.y, p.size * (0.85f + 0.25f * (1.0f - k)),
               Color(a, p.col.GetR(), p.col.GetG(), p.col.GetB()), 400.0f);
    }
}

// 第一人称的手：从右下角伸出去抓住玉米再收回来
static void CnDrawHand(Graphics& g)
{
    if (!cnHand.active) return;
    float t = cnHand.t;
    float grabT = 0.30f, holdT = 0.14f, backStart = grabT + holdT;
    float s;
    if (t < grabT)          s = CnEaseOut3(t / grabT);
    else if (t < backStart) s = 1.0f;
    else                    s = 1.0f - CnEaseIn3(CnClamp((t - backStart) / (CN_HAND_DUR - backStart), 0.0f, 1.0f));

    float restX = (float)BASE_W * 0.94f, restY = (float)BASE_H + 170.0f;
    float hx = restX + (cnHand.tx - restX) * s;
    float hy = restY + (cnHand.ty - restY) * s;
    float alpha = 1.0f;
    if (t > backStart) alpha = 1.0f - 0.92f * CnClamp((t - backStart) / (CN_HAND_DUR - backStart), 0.0f, 1.0f);
    BYTE A = (BYTE)(255.0f * alpha);

    SolidBrush skin(Color(A, 253, 228, 196));
    SolidBrush skinD(Color(A, 236, 202, 166));
    Pen outline(Color(A, 150, 102, 72), 4.0f);
    outline.SetLineJoin(LineJoinRound);

    {   // 手臂：从画面右下角外伸进来
        float dx = restX - hx, dy = restY - hy;
        float len = sqrtf(dx * dx + dy * dy);
        if (len < 1.0f) len = 1.0f;
        float ux = dx / len, uy = dy / len;
        float nx = -uy, ny = ux;
        float w0 = 40.0f, w1 = 54.0f, L = 1100.0f;
        PointF pts[4] = {
            PointF(hx + 6 + nx * w0, hy + 46 + ny * w0),
            PointF(hx + 6 + ux * L + nx * w1, hy + 46 + uy * L + ny * w1),
            PointF(hx + 6 + ux * L - nx * w1, hy + 46 + uy * L - ny * w1),
            PointF(hx + 6 - nx * w0, hy + 46 - ny * w0)
        };
        SolidBrush armB(Color(A, 244, 238, 220));
        g.FillPolygon(&armB, pts, 4);
        PointF shp[4] = {
            PointF(hx + 6 + nx * w0 * 0.35f, hy + 46 + ny * w0 * 0.35f),
            PointF(hx + 6 + ux * L + nx * w1 * 0.35f, hy + 46 + uy * L + ny * w1 * 0.35f),
            PointF(hx + 6 + ux * L + nx * w1, hy + 46 + uy * L + ny * w1),
            PointF(hx + 6 + nx * w0, hy + 46 + ny * w0)
        };
        SolidBrush armSh(Color(A, 220, 210, 188));
        g.FillPolygon(&armSh, shp, 4);
        g.DrawPolygon(&outline, pts, 4);

        PointF c0(hx + 6 + ux * 30 + nx * w0 * 1.02f, hy + 46 + uy * 30 + ny * w0 * 1.02f);
        PointF c1(hx + 6 + ux * 30 - nx * w0 * 1.02f, hy + 46 + uy * 30 - ny * w0 * 1.02f);
        Pen cuffP(Color(A, 208, 222, 182), 23.0f);
        g.DrawLine(&cuffP, c0, c1);
        Pen cuffO(Color(A, 150, 102, 72), 2.6f);
        g.DrawLine(&cuffO, c0, c1);
    }

    GraphicsState gs = g.Save();
    g.TranslateTransform(hx, hy);
    g.RotateTransform(-16.0f);

    float ch = 212.0f * ((cnHand.cornScale > 0.95f) ? cnHand.cornScale : 0.95f);
    if (s > 0.35f) {
        float ca = CnClamp((s - 0.35f) / 0.4f, 0.0f, 1.0f) * alpha;
        CnDrawCornAt(g, 6.0f, 62.0f, ch * cnCornAR, ch, ca, false);
    }

    GraphicsPath palm;
    MakeRoundRect(palm, RectF(-38.0f, -8.0f, 76.0f, 72.0f), 30.0f);
    g.FillPath(&skin, &palm);
    g.DrawPath(&outline, &palm);

    float fx[4] = { -27.0f, -9.0f, 9.0f, 26.0f };
    float fh[4] = {  34.0f, 41.0f, 40.0f, 32.0f };
    float ft[4] = { -15.0f, -6.0f, 2.0f,  9.0f };
    for (int i = 0; i < 4; ++i) {
        GraphicsState fs = g.Save();
        g.TranslateTransform(fx[i] + 8.5f, 8.0f);
        g.RotateTransform(ft[i]);
        GraphicsPath f;
        MakeRoundRect(f, RectF(-8.5f, -20.0f - fh[i], 17.0f, fh[i] + 30.0f), 8.5f);
        g.FillPath(&skin, &f);
        g.DrawPath(&outline, &f);
        Pen knuckle(Color(A, 214, 166, 128), 1.8f);
        g.DrawLine(&knuckle, -4.0f, -10.0f, 4.5f, -10.0f);
        g.DrawLine(&knuckle, -4.0f, -0.6f * fh[i], 4.5f, -0.6f * fh[i]);
        g.Restore(fs);
    }
    {
        GraphicsState ts = g.Save();
        g.TranslateTransform(-34.0f, 24.0f);
        g.RotateTransform(-26.0f);
        GraphicsPath th;
        MakeRoundRect(th, RectF(-16.0f, -11.0f, 50.0f, 23.0f), 11.0f);
        g.FillPath(&skinD, &th);
        g.DrawPath(&outline, &th);
        g.Restore(ts);
    }
    {
        GraphicsPath wr;
        MakeRoundRect(wr, RectF(-30.0f, 44.0f, 60.0f, 40.0f), 18.0f);
        g.FillPath(&skin, &wr);
    }
    g.Restore(gs);
}

// 面板上的深色字（不要描边，描边会把字糊成一团）
static void CnInk(Graphics& g, const wchar_t* s, float cx, float cy, float size, Color col)
{
    DrawTextCenter(g, s, *g_famUI, FontStyleBold, size, cx, cy, 0.0f, col, 0.0f);
}

static void CnDrawHUD(Graphics& g)
{
    wchar_t buf[64];
    // 分数（左上）
    RectF r(24.0f, 20.0f, 210.0f, 62.0f);
    CnPanel(g, r, Color(252, 253, 249, 238));
    CnDrawIcon(g, r.X + 52.0f, r.Y + 31.0f, 42.0f);
    swprintf(buf, 64, L"× %d", cnScore);
    CnInk(g, buf, r.X + 134.0f, r.Y + 31.0f, 32.0f, Color(255, 62, 84, 52));

    // 倒计时（右上）
    RectF r2((float)BASE_W - 234.0f, 20.0f, 210.0f, 62.0f);
    bool urgent = (cnTimeLeft <= 5.5f);
    CnPanel(g, r2, urgent ? Color(252, 240, 232, 236) : Color(252, 253, 249, 238));
    int sec = (int)ceilf(cnTimeLeft);
    swprintf(buf, 64, L"%d", sec);
    CnInk(g, buf, r2.X + 84.0f, r2.Y + 31.0f, 34.0f,
          urgent ? Color(255, 214, 74, 58) : Color(255, 62, 84, 52));
    CnInk(g, L"秒", r2.X + 142.0f, r2.Y + 34.0f, 20.0f, Color(255, 96, 116, 92));

    RectF bar((float)BASE_W - 234.0f, 88.0f, 210.0f, 13.0f);
    CnPanel(g, bar, Color(190, 34, 52, 38));
    float frac = CnClamp(cnTimeLeft / CN_GAME_TIME, 0.0f, 1.0f);
    if (frac > 0.004f) {
        SolidBrush br(urgent ? Color(255, 240, 96, 72) : Color(255, 158, 214, 96));
        g.FillRectangle(&br, bar.X + 3.0f, bar.Y + 3.0f, (bar.Width - 6.0f) * frac, bar.Height - 6.0f);
    }

    // 底部操作提示
    float alpha = 1.0f;
    if (cnLastMove > 4.5f) alpha = 0.55f + 0.45f * (0.5f + 0.5f * sinf(cnTime * 3.0f));
    RectF hint((float)BASE_W * 0.5f - 280.0f, (float)BASE_H - 92.0f, 560.0f, 46.0f);
    CnPanel(g, hint, Color((BYTE)(190 * alpha), 24, 44, 30));
    CnText(g, L"按住 W 前进    ·    点击近处的玉米采摘",
           (float)BASE_W * 0.5f, hint.Y + 23.0f, 22.0f,
           Color((BYTE)(255 * alpha), 246, 250, 240), 700.0f);
}

static void CnDrawStart(Graphics& g)
{
    SolidBrush ov(Color(120, 18, 40, 24));
    g.FillRectangle(&ov, 0.0f, 0.0f, (float)BASE_W, (float)BASE_H);

    CnText(g, L"摘玉米", 640.0f, 96.0f, 84.0f, Color(255, 255, 236, 132), 700.0f);
    CnText(g, L"— 玉米田 · 限时 30 秒 —", 640.0f, 172.0f, 26.0f, Color(255, 240, 250, 236), 700.0f);

    RectF p(340.0f, 220.0f, 600.0f, 204.0f);
    CnPanel(g, p, Color(252, 253, 247, 240));
    const wchar_t* lines[4] = {
        L"· 按住 W（或 ↑）在玉米地里往前走",
        L"· 远处的玉米藏在雾里，小得看不清",
        L"· 走近了，它才会由远及近一点点变大",
        L"· 点击靠近的玉米，伸手摘下，得 1 分"
    };
    for (int i = 0; i < 4; ++i) {
        CnInk(g, lines[i], 640.0f, p.Y + 44.0f + (float)i * 44.0f, 22.0f,
              Color(255, 58, 76, 50));
    }

    RectF b(490.0f, 458.0f, 300.0f, 76.0f);
    CnPanel(g, b, Color(255, 255, 226, 120));
    CnInk(g, L"开 始", 640.0f, b.Y + 38.0f, 38.0f, Color(255, 58, 82, 44));
    if (((GetTickCount64() / 500) % 2) == 0)
        CnText(g, L"点击 / 空格", 640.0f, b.Y + 106.0f, 22.0f, Color(255, 246, 251, 240), 400.0f);
}

static void CnDrawEnd(Graphics& g)
{
    SolidBrush ov(Color(120, 18, 40, 24));
    g.FillRectangle(&ov, 0.0f, 0.0f, (float)BASE_W, (float)BASE_H);

    CnText(g, L"时间到！", 640.0f, 100.0f, 64.0f, Color(255, 255, 250, 240), 700.0f);

    RectF p(390.0f, 170.0f, 500.0f, 290.0f);
    CnPanel(g, p, Color(252, 253, 247, 242));
    CnDrawIcon(g, 532.0f, p.Y + 140.0f, 156.0f);

    wchar_t buf[64];
    swprintf(buf, 64, L"%d", cnScore);
    CnInk(g, buf, 706.0f, p.Y + 104.0f, 96.0f, Color(255, 244, 196, 62));
    CnInk(g, L"根玉米", 706.0f, p.Y + 168.0f, 26.0f, Color(255, 76, 96, 62));

    const wchar_t* comment =
        cnScore >= 18 ? L"玉米田传说！这速度能帮全村收割了" :
        cnScore >= 12 ? L"摘玉米高手，篮子都快装不下了" :
        cnScore >= 7  ? L"收获不错，田里还有好多等你" :
        cnScore >= 3  ? L"有点手生，再走近一点试试" :
                        L"玉米们表示：今天很安全";
    CnInk(g, comment, 640.0f, p.Y + 248.0f, 23.0f, Color(255, 70, 90, 58));

    RectF b(490.0f, 500.0f, 300.0f, 72.0f);
    CnPanel(g, b, Color(255, 255, 226, 120));
    CnInk(g, L"点击结算并返回", 640.0f, b.Y + 36.0f, 28.0f, Color(255, 58, 82, 44));
}

static void CnDrawScene(Graphics& g)
{
    if (cnBgCache) g.DrawImage(cnBgCache, RectF(0.0f, 0.0f, (float)BASE_W, (float)BASE_H));
    else {
        SolidBrush b(Color(255, 60, 110, 70));
        g.FillRectangle(&b, 0.0f, 0.0f, (float)BASE_W, (float)BASE_H);
    }
    if (cnState == CN_START) { CnDrawStart(g); return; }

    CnDrawCorns(g);
    CnDrawParticles(g);
    CnDrawHand(g);
    CnDrawPopups(g);
    if (cnState == CN_PLAY) CnDrawHUD(g);
    else                    CnDrawEnd(g);
}

// ---------------- 对外的入口 / 出口 ----------------
static void CnBegin()
{
    cnActive     = true;
    cnState      = CN_START;
    cnTime       = 0.0f;
    cnTimeLeft   = CN_GAME_TIME;
    cnScore      = 0;
    cnWalk       = 0.0f;
    cnLastSpawn  = 0.0f;
    cnSpawnGap   = CnRnd(10.0f, 14.0f);
    cnBobPhase   = 0.0f;
    cnMoveBlend  = 0.0f;
    cnCamX = cnCamY = 0.0f;
    cnLastMove   = 0.0f;
    cnLastTickSec = -1;
    cnCorns.clear();
    cnParts.clear();
    cnPops.clear();
    cnHand.active = false;
    if (!cnBgCache) CnBuildBg();
}

// 结算：每 10 积分换 1 个玉米放进物品栏，然后回地图
static void CnSettle()
{
    cnActive = false;
    g_cornDone = true;                       // 玉米事件完成 → 农田换成「农田结束界面.jpg」

    int  n    = cnScore / 10;                // 每 10 积分 1 个玉米
    bool full = false;
    for (int i = 0; i < n; ++i) {
        int slot = InvFreeSlot();
        if (slot < 0) { full = true; break; }
        g_inv[slot] = ITEM_CORN;
    }
    if (full)       CenterMsg(L"背包满了");
    // 第二十二步：成就「赫鲁晓夫」= 在考试摘玉米中获得 80 分以上（看积分，不是看玉米个数）
    if (cnScore > 80) UnlockAch(ACH_HRUSHOV);

    CommitDeltas(DN_EXAM_END);
}

static void CnClick(float mx, float my)
{
    if (cnState == CN_START) { CnStartGame(); return; }
    if (cnState == CN_END)   { CnSettle();    return; }
    int idx = CnPickHitTest(mx, my);
    if (idx < 0) return;
    if (cnCorns[idx].d <= CN_PICK_D) CnPick(idx);
    else CnAddPop(mx, my - 20.0f, L"太远了…", Color(255, 64, 84, 50), 32.0f);
}

static void CnKey(WPARAM vk)
{
    if (vk == 'W' || vk == VK_UP) {
        if (cnState == CN_PLAY) { CnAdvance(6.0f); cnMoveBlend = 1.0f; }
    } else if (vk == VK_SPACE || vk == VK_RETURN) {
        if (cnState != CN_PLAY) CnStartGame();
    }
}

// ============================================================
//  第二十一步：事件3 考试
//  开发说明原文：
//    · day3 下午第一节（time 10）强制跳转到「考试.jpg」，正中提示「考试了」1 秒
//    · 下午一节物理 / 二节语文 / 三节英语 / 四节数学，每门：
//        广播"这是浙南联盟X考试" >继续 → 我"要合理分配精力" → 认真考 / 轻松考
//        认真考 精神-10，轻松考 精神-5，然后跳到下一节且不切到地图
//    · 数学考完再播：旁白"紧张数学考试中" → 广播"同学们，农田的玉米熟了快去摘玉米"
//        → 我"该怎么做呢" → 去摘玉米（触发摘玉米游戏）/ 去你*的（跳到下午五节）
//    · 成绩 = ((1 + 知识/300 - 精神/100) ^ (知识/300)) * (100/e)，±10 随机波动
//        物理不超过 100；语文/英语/数学 *1.5 不超过 150；数学选摘玉米再 *0.9
//        成绩只保存、不展示
// ============================================================
static const int      kExamSubj[4] = { SUBJ_PHYS, SUBJ_CHINESE, SUBJ_ENGLISH, SUBJ_MATH };
static const wchar_t* kExamName[4] = { L"物理", L"语文", L"英语", L"数学" };

static DlgLine      kExamScript[1];
static std::wstring kExamBuf;
static const DlgLine kExamEndScript[2] = {
    { L"旁白", L"紧张数学考试中",                       ITEM_NONE },
    { L"广播", L"同学们，农田的玉米熟了快去摘玉米",     ITEM_NONE }
};

// 考试成绩（开发说明给的公式，只保存不展示）
static int ExamScore(int subj)
{
    const double E = 2.71828;
    double k    = (double)g_stat[4] / 300.0;          // 知识
    double s    = (double)g_stat[2] / 100.0;          // 精神
    double base = 1.0 + k - s;
    if (base < 0.0) base = 0.0;
    double v = pow(base, k) * (100.0 / E);
    v += (double)((rand() % 21) - 10);                // ±10 的随机波动
    if (subj == SUBJ_PHYS) {
        if (v < 0.0)   v = 0.0;
        if (v > 100.0) v = 100.0;                     // 物理不超过 100
    } else {
        v *= 1.5;
        if (v < 0.0)   v = 0.0;
        if (v > 150.0) v = 150.0;                     // 其余三门不超过 150
    }
    return (int)floor(v + 0.5);                       // 保留整数
}

// 开考第 idx 门：广播一句 → 弹「认真考 / 轻松考」
static void ExamStart(int idx)
{
    g_examIdx   = idx;
    g_examPhase = 0;
    kExamBuf = std::wstring(L"这是浙南联盟") + kExamName[idx] + L"考试";
    kExamScript[0].who      = L"广播";
    kExamScript[0].text     = kExamBuf.c_str();
    kExamScript[0].giveItem = ITEM_NONE;

    g_shopOpen   = false;
    g_shopAnim   = 0.0f;
    g_dlgSmall   = false;
    g_dlgActive  = true;
    g_dlgAsk     = false;
    g_dlgAskSubj = -1;
    g_dlgScript  = kExamScript;
    g_dlgScriptN = 1;
    g_dlgOptN    = 0;                     // 台词播完才弹选项（DLG_AFTER_ASK 分支里赋回）
    g_askPrompt  = L"要合理分配精力";
    g_askOptN    = 2;
    g_dlgOpt[0]  = L"认真考";
    g_dlgOpt[1]  = L"轻松考";
    g_dlgAfter   = DLG_AFTER_ASK;
    DlgSetLine(0);
}

static void EnterExam()
{
    g_loc      = LOC_EXAM;
    g_examDone = true;
    g_examMathCorn = false;
    CenterMsg(L"考试了");
    ExamStart(0);
}

// 一门考完：跳到下一节（不切地图）；数学考完改播「玉米熟了」
static void ExamNext()
{
    if (g_examIdx < 3) {
        g_time = 10 + g_examIdx + 1;                  // 11/12/13 = 下午二 / 三 / 四节
        ExamStart(g_examIdx + 1);
        return;
    }
    g_examPhase  = 1;
    g_dlgSmall   = false;
    g_dlgActive  = true;
    g_dlgAsk     = false;
    g_dlgAskSubj = -1;
    g_dlgScript  = kExamEndScript;
    g_dlgScriptN = 2;
    g_dlgOptN    = 0;
    g_askPrompt  = L"该怎么做呢";
    g_askOptN    = 2;
    g_dlgOpt[0]  = L"去摘玉米";
    g_dlgOpt[1]  = L"去你*的";
    g_dlgAfter   = DLG_AFTER_ASK;
    DlgSetLine(0);
}

// 认真考 0 / 轻松考 1
static void ExamChoose(int idx)
{
    g_dlgAsk    = false;
    g_dlgActive = false;
    g_dlgOptN   = 0;
    AddStat(2, (idx == 0) ? -10 : -5);                // 认真考 -10 / 轻松考 -5
    g_faceState.Refresh(g_stat[2], g_stat[3]);
    g_examScore[g_examIdx] = ExamScore(kExamSubj[g_examIdx]);
    CommitDeltas(DN_EXAM_NEXT);                       // 属性变化 → 下一门
}

// 去摘玉米 0 / 去你*的 1
static void ExamCornChoose(int idx)
{
    g_dlgAsk    = false;
    g_dlgActive = false;
    g_dlgOptN   = 0;
    if (idx == 0 && !g_cornPlayed) {
        g_examMathCorn = true;
        g_cornPlayed   = true;                                     // 摘玉米只能玩一次
        g_examScore[3] = (int)floor(g_examScore[3] * 0.9 + 0.5);   // 数学成绩 *0.9
        CnBegin();                                                  // 打开摘玉米游戏
        return;
    }
    CommitDeltas(DN_EXAM_END);                        // 跳到下午五节并回地图
}

// 考试里两类选项的统一入口
static void ExamOption(int idx)
{
    if (g_examPhase == 0) ExamChoose(idx);
    else                  ExamCornChoose(idx);
}

static void BuildExamBg(int W, int H)
{
    BuildDimBg(g_examSrc, &g_examBg, &g_examBgW, &g_examBgH, W, H);
}

// 从地图返回教学楼界面时保留天数/时间/属性，只有重新选角才重置
// （第二十三步新增5：原来定义在 SetScene 旁边，TryForceEvent 要用，上移到这里）
static bool g_keepGameState = false;

// 第二十三步（新增5）：时段推进之后可能撞上的强制跳转（事件 / 体育课 / 晚寝）。
//   返回 true = 已经进了对应界面，调用方不要再「回地图」，否则事件当场被顶掉。
//   和 WM_TIMER 里那几段是同一套条件，靠「g_day*100+g_time」去重戳保证只触发一次。
static bool TryForceEvent()
{
    if (g_scene == SCENE_ENDING || g_scene == SCENE_TESTEND) return false;
    int stamp = g_day * 100 + g_time;

    // 第二十步：day2 上午第 5 节 —— 课外实践活动
    if (g_day == 2 && g_time == 5 && !g_event1Done && g_event1Stamp != stamp) {
        g_event1Stamp = stamp;
        g_keepGameState = true;
        EnterEvent1();
        SetScene(SCENE_GAME);
        return true;
    }
    // 第二十三步（新增4）：day2 下午 1~4 节 —— 北京名师讲高考
    if (g_day == 2 && g_time >= 10 && g_time <= 13 && !g_gaokaoDone && g_gaokaoStamp != stamp) {
        g_gaokaoStamp = stamp;
        g_keepGameState = true;
        EnterGaokao();
        SetScene(SCENE_GAME);
        return true;
    }
    // 第二十一步：day3 下午第一节 —— 浙南联盟考试
    if (g_day == 3 && g_time == 10 && !g_examDone && g_examStamp != stamp) {
        g_examStamp = stamp;
        g_keepGameState = true;
        EnterExam();
        SetScene(SCENE_GAME);
        return true;
    }
    // 第十六步：体育课强制拉去操场
    if (!g_runActive && IsPEClass(g_day, g_time) && g_playStamp != stamp) {
        g_playStamp = stamp;
        g_keepGameState = true;
        EnterPlayground(true);
        SetScene(SCENE_GAME);
        CenterMsg(L"现在是体育课时间");
        return true;
    }
    // 第十三步：晚寝强制拉去宿舍
    if (g_time == 20 && g_dormStamp != stamp) {
        g_dormStamp = stamp;
        g_keepGameState = true;
        EnterDorm(true);
        SetScene(SCENE_GAME);
        return true;
    }
    return false;
}

// 真正执行「确定之后要做什么」
static void DoDeltaNext(int next)
{
    if (next == DN_MAP) {
        SetScene(SCENE_MAP);
    } else if (next == DN_CLASS_TALK) {
        int subj = g_dlgAskSubj;
        ClassBuildScript(subj, g_classChoice);
        g_dlgAsk     = false;
        g_dlgOptN    = 0;             // 第二十三步：播台词前先收起选项
        g_dlgSmall   = false;
        g_dlgScript  = kClassScript;
        g_dlgScriptN = kClassN;
        g_dlgAfter   = DLG_AFTER_NEXT_TIME;
        DlgSetLine(0);
    } else if (next == DN_FARM_TALK) {
        g_dlgActive  = true;
        g_dlgAsk     = false;
        g_dlgOptN    = 0;             // 第二十三步：同上
        g_dlgSmall   = false;
        g_dlgScript  = kFarmEnd;
        g_dlgScriptN = 1;
        g_dlgAfter   = DLG_AFTER_FARM_LEAVE;
        DlgSetLine(0);
    } else if (next == DN_DORM_OPTIONS) {
        DormSetOptions();
    } else if (next == DN_DORM_MORNING) {
        JumpToMorning();
    } else if (next == DN_CANTEEN_EAT) {
        AdvanceTime();                 // 吃完才推时段，时间可能跳出食堂开放时段
        // 第二十三步（新增5）：推进后正好撞上事件 / 体育课 / 晚寝就照规矩跳过去，
        //   别让玩家留在食堂继续点（g_loc 变了下面那句自然不会执行）
        if (TryForceEvent()) return;
        CanteenCheckOpen();
        // 第二十三步：还在营业就把「购买 / 离开」重新显示出来，否则界面没有出口（卡死）
        if (g_loc == LOC_CANTEEN) CanteenAsk();
    } else if (next == DN_BASKET_END || next == DN_MUSIC_END || next == DN_RUN) {
        // 跑步 5 秒演出结束：推时段回地图
        AdvanceTime();
        // 第二十三步（新增5）：同上，撞上强制跳转就别回地图
        if (TryForceEvent()) return;
        SetScene(SCENE_MAP);
    } else if (next == DN_SELF_STUDY) {
        // 第二十三步（新增5）：自修推进时段后如果撞上事件 / 体育课 / 晚寝，
        //   直接进那个界面；没有才回地图（原来无条件回地图，事件会被当场顶掉）
        if (TryForceEvent()) return;
        SetScene(SCENE_MAP);
    } else if (next == DN_EVENT1_END) {
        g_time = 8;                    // 跳到中午午时
        SetScene(SCENE_MAP);
    } else if (next == DN_SHOW_DELTA) {
        ShowDeltaOrRun();              // 第二十一步：动画播完了，这时才显示属性变化
    } else if (next == DN_EXAM_NEXT) {
        ExamNext();                    // 第二十一步：下一门考试 / 数学考完播玉米广播
    } else if (next == DN_EXAM_END) {
        g_time = 14;                   // 下午五节
        g_loc = LOC_CLASS;
        SetScene(SCENE_MAP);
    } else if (next == DN_GAOKAO_END) {
        g_time = 14;                   // 第二十三步：晚饭前
        g_loc  = LOC_CLASS;
        SetScene(SCENE_MAP);
    }
}

// 某个后续动作对应的动画（没有就返回 ANIM_NONE）
static int AnimForNext(int next)
{
    if (next == DN_CANTEEN_EAT)       return ANIM_EAT;
    if (next == DN_DORM_OPTIONS)      return ANIM_NAP;
    if (next == DN_DORM_MORNING)
        return (g_dormChoice == 0) ? ANIM_SLEEP
             : (g_dormChoice == 1 ? ANIM_NIGHT_STUDY : ANIM_NIGHT_GAME);
    if (next == DN_FARM_TALK)         return ANIM_FARM_WALK;
    if (next == DN_CLASS_TALK)        return ANIM_STUDY;
    if (next == DN_SELF_STUDY)        return ANIM_STUDY;
    if (next == DN_RUN)               return ANIM_RUN;
    return ANIM_NONE;
}

// 第十八步：点「确定」之后继续原本的流程
// 第二十一步：动画已经在这之前播过了（CommitDeltas → 动画 → 面板 → 这里）
static void RunDeltaNext()
{
    int next = g_deltaNext;
    g_deltaNext = DN_NONE;
    ClearDeltas();                     // 数值已经看过了，清掉
    DoDeltaNext(next);
}

static void RenderGame(Graphics& g, float el)
{
    // 第十七步：投篮游戏开着时整屏接管
    if (g_basketActive) {
        BkDrawScene(g);
        return;
    }
    // 第十九步：钢琴键游戏开着时整屏接管
    if (ptActive) {
        PtRender(g, BASE_W, BASE_H);
        return;
    }
    // 第二十一步：摘玉米游戏开着时整屏接管
    if (cnActive) {
        CnDrawScene(g);
        return;
    }

    // 背景（教学楼 / 食堂 / 宿舍 / 书屋 / 农田 / 操场 / 篮球场各一张，都是压暗后的成品）
    Image* bg = (Image*)g_gameBg;
    if (g_loc == LOC_CANTEEN)  bg = (Image*)g_canteenBg;
    else if (g_loc == LOC_DORM) bg = (Image*)g_dormBg;
    else if (g_loc == LOC_BOOK) bg = (Image*)g_bookBg;
    else if (g_loc == LOC_FARM) bg = (Image*)g_farmBg;
    else if (g_loc == LOC_PLAY) bg = (Image*)g_playBg;
    else if (g_loc == LOC_COURT) bg = (Image*)g_courtBg;
    else if (g_loc == LOC_MUSIC) bg = (Image*)g_musicBg;
    else if (g_loc == LOC_EVENT1) bg = (Image*)g_event1Bg;
    else if (g_loc == LOC_GAOKAO) bg = (Image*)g_gaokaoBg;
    else if (g_loc == LOC_EXAM)   bg = (Image*)g_examBg;
    if (bg) g.DrawImage(bg, RectF(0.0f, 0.0f, (float)BASE_W, (float)BASE_H));

    float t = EaseOut(Clamp01((el - 80.0f) / 480.0f));
    if (t <= 0.0f) return;

    if (g_loc == LOC_CLASS) RenderClassLayer(g, t);   // 上课场景只属于教学楼

    DrawHudPanel(g, t);
    DrawDayBadge(g, t);
    DrawSaveButton(g, t);        // 第二十三步：时间 UI 右侧的保存按钮
    DrawItemBar(g, t);

    if (g_loc == LOC_CLASS) {
        DrawBackBtn(g, t);

        // 第八步：「自修一会」按钮（原「下一时间段」）—— 推进一个时间段
        float ta = EaseOut(Clamp01((el - 260.0f) / 420.0f));
        DrawButton(g, 22, RectF(1078.0f, 196.0f, 176.0f, 54.0f), L"自修一会",
                   g_hoverAnim[22], ta);
    }

    // 第十步：对话框（压在 UI 之上）、事务安排（压在最上）
    DrawDialogue(g, el);
    DrawPlanView(g, el);
    DrawShopView(g, el);          // 第十二步：商店在最上层
    // 第二十步：事件1 选物品时的提示（贴在物品栏上方）
    if (g_pickItemMode) {
        DrawTextCenter(g, L"点一件东西下锅……", *g_famUI, FontStyleBold, 26.0f,
                       640.0f, 586.0f, 2.0f, Color(255, 250, 246, 232), 1.2f);
    }
    DrawCenterMsg(g);             // 正中提示：预算不足 / 背包满了 / 该休息了 / 现在是体育课时间
    DrawActionScene(g);           // 第十六/二十步：5 秒动作演出（压在最上层）
}

// ================= 第八步：地图页 =================

// 手绘感的矩形（描两遍、略有错位，像随手框的），用于贴合建筑朝向
static void DrawHandRect(Graphics& g, float x1, float y1, float x2, float y2,
                         Color c, float w, float seed)
{
    float rw = x2 - x1, rh = y2 - y1;
    if (rw < 6.0f || rh < 6.0f) return;
    for (int pass = 0; pass < 2; ++pass) {
        float dx = (pass == 0) ? 0.0f : 2.4f * Noise1((int)seed + 31);
        float dy = (pass == 0) ? 0.0f : 2.2f * Noise1((int)seed + 57);
        float k  = (pass == 0) ? 0.0f : 0.030f * Noise1((int)seed + 83);
        GraphicsPath p;
        // 四条略带抖动的直线，组成手绘矩形
        float j0 = 2.0f * Noise1((int)seed + 11), j1 = 2.0f * Noise1((int)seed + 23);
        float j2 = 2.0f * Noise1((int)seed + 37), j3 = 2.0f * Noise1((int)seed + 53);
        p.AddLine(x1 + dx + rw * k + j0, y1 + dy + j1,
                  x2 + dx - rw * k - j2, y1 + dy - j3);
        p.AddLine(x2 + dx - rw * k + j2, y1 + dy + j3,
                  x2 + dx + rw * k - j0, y2 + dy - rh * k - j1);
        p.AddLine(x2 + dx + rw * k - j0, y2 + dy - rh * k + j1,
                  x1 + dx - rw * k + j2, y2 + dy + rh * k + j3);
        p.AddLine(x1 + dx - rw * k - j2, y2 + dy + rh * k - j3,
                  x1 + dx + rw * k + j0, y1 + dy - rh * k - j2);
        Pen pen(c, w * (pass == 0 ? 1.0f : 0.60f));
        pen.SetLineJoin(LineJoinRound);
        pen.SetStartCap(LineCapRound);
        pen.SetEndCap(LineCapRound);
        g.DrawPath(&pen, &p);
    }
}

// 悬停时显示建筑名称的气泡（放在矩形圈外侧上方，避免遮住圈里的编号）
static void DrawSpotName(Graphics& g, const wchar_t* name, float cx, float cy,
                         float rx, float ry, float a)
{
    float em  = 30.0f;
    float tw  = 0.0f;
    for (const wchar_t* p = name; *p; ++p)
        tw += ((unsigned)*p >= 0x2E80u) ? (em + 3.0f) : (em * 0.55f);
    float bw = tw + 44.0f, bh = em + 30.0f;

    float bx = cx - bw * 0.5f;
    float by = cy - ry - bh - 10.0f;            // 框顶外侧
    if (by < 6.0f) by = cy + ry + 10.0f;         // 上方放不下就翻到框底外侧
    if (bx < 6.0f) bx = 6.0f;
    if (bx + bw > BASE_W - 6.0f) bx = BASE_W - 6.0f - bw;

    RectF r(bx, by, bw, bh);
    GraphicsPath path;
    MakeRoundRect(path, r, 14.0f);
    {
        SolidBrush sb(Color(Alpha(80, a), 0, 0, 0));
        GraphicsPath sp;
        MakeRoundRect(sp, RectF(r.X + 5.0f, r.Y + 6.0f, r.Width, r.Height), 14.0f);
        g.FillPath(&sb, &sp);
    }
    {
        SolidBrush fill(Color(Alpha(250, a), 255, 251, 236));
        g.FillPath(&fill, &path);
        Pen pen(Color(Alpha(255, a), 34, 30, 26), 2.8f);
        pen.SetLineJoin(LineJoinRound);
        g.DrawPath(&pen, &path);
    }
    DrawTextCenter(g, name, *g_famUI, FontStyleBold, em, r.X + r.Width * 0.5f,
                   r.Y + r.Height * 0.5f, 3.0f, Color(Alpha(255, a), 30, 26, 22), 0.6f);
}

static void RenderMap(Graphics& g, float el)
{
    // 地图四周留白用与图片同色的浅灰（地图.jpg 四角就是 240,240,240）
    SolidBrush bg(Color(255, 240, 240, 240));
    g.FillRectangle(&bg, 0, 0, BASE_W, BASE_H);

    float t = EaseOut(Clamp01(el / 460.0f));
    if (t <= 0.0f) return;

    // 地图本体（整幅按高度铺满、水平居中，不裁掉任何建筑）
    if (g_mapScaled) {
        GraphicsState st = g.Save();
        g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
        g.DrawImage(g_mapScaled, RectF(MAP_OFF_X, MAP_OFF_Y, MAP_W, MAP_H));
        g.Restore(st);
    }

    // 悬停的建筑
    int hi = -1;
    if (g_hover >= 12 && g_hover <= 21) hi = g_hover - 12;

    // 10 个场地的橙色手绘矩形框+ 编号（矩形有方向，贴合各建筑朝向）
    for (int i = 0; i < 10; ++i) {
        const MapSpot& s = kMapSpots[i];
        float x1 = MapSX(s.x1), y1 = MapSY(s.y1);
        float x2 = MapSX(s.x2), y2 = MapSY(s.y2);

        float hot = (hi == i) ? g_hoverAnim[12 + i] : 0.0f;
        BYTE  a   = Alpha(255, t * (0.62f + 0.38f * hot));

        DrawHandRect(g, x1, y1, x2, y2, Color(a, 238, 122, 38), 5.0f, 17 + i * 13);

        // 编号
        float ax = MapSX(s.ax), ay = MapSY(s.ay);
        wchar_t num[4];
        swprintf(num, 4, L"%d", i + 1);
        DrawTextCenter(g, num, *g_famTitle, FontStyleBold, 112.0f, ax, ay,
                       0.0f, Color(Alpha(255, t), 26, 22, 20), 0.0f);
    }

    // 悬停：建筑名
    if (hi >= 0) {
        const MapSpot& s = kMapSpots[hi];
        float x1 = MapSX(s.x1), x2 = MapSX(s.x2);
        float y1 = MapSY(s.y1), y2 = MapSY(s.y2);
        DrawSpotName(g, s.name, (x1 + x2) * 0.5f, (y1 + y2) * 0.5f,
                     (x2 - x1) * 0.5f, (y2 - y1) * 0.5f, EaseOut(g_hoverAnim[12 + hi]));
    }

    // 左下角「返回」
    {
        float ta = EaseOut(Clamp01((el - 200.0f) / 400.0f));
        DrawButton(g, 23, RectF(MBACK_X, MBACK_Y, MBACK_W, MBACK_H), L"← 返回",
                   g_hoverAnim[23], ta);
    }
}

static void RenderDisclaimer(Graphics& g, float el)
{
    // 最后 350ms 淡出，衔接菜单
    float fade = 1.0f - Clamp01((el - 2650.0f) / 350.0f);

    float t1 = EaseOut(Clamp01((el - 120.0f) / 620.0f));
    BYTE  a1 = Alpha(255, t1 * fade);
    DrawTextLine(g, L"声明", *g_famTitle, FontStyleBold, 158.0f, 640.0f,
                 186.0f + 22.0f * (1.0f - t1), 58.0f, a1, 1.4f, 3);

    float t2 = EaseOut(Clamp01((el - 600.0f) / 520.0f));
    if (t2 > 0.0f) {
        BYTE  a2   = Alpha(205, t2 * fade);
        float half = 168.0f * t2;
        DrawWobbleLine(g, 640.0f, 372.0f, half, 4.0f, 0.7f, 4.0f, Color(a2, 255, 255, 255));
        DrawWobbleLine(g, 640.0f, 387.0f, half * 0.70f, 3.0f, 2.1f, 2.2f,
                       Color(Alpha(a2, 0.5f), 255, 255, 255));
    }

    // 第一步：免责声明三行（2026-10-23 按开发说明更新文案）
    static const wchar_t* kLines[3] = {
        L"本作品意在以更艺术与趣味的方式回忆高中生活",
        L"本作品尊重出现的所有角色",
        L"本作品无任何商业意图与收费手段"
    };
    for (int i = 0; i < 3; ++i) {
        float ti = EaseOut(Clamp01((el - (720.0f + i * 170.0f)) / 560.0f));
        BYTE  ai = Alpha(255, ti * fade);
        if (ai == 0) continue;
        DrawTextLine(g, kLines[i], *g_famBody, FontStyleRegular, 34.0f, 640.0f,
                     440.0f + i * 66.0f + 14.0f * (1.0f - ti), 2.0f, ai, 0.9f, 17 + i * 5);
    }

    float t4 = EaseOut(Clamp01((el - 1300.0f) / 420.0f));
    if (t4 > 0.0f) {
        DrawTextLine(g, L"按 ESC 退出", *g_famBody, FontStyleRegular, 20.0f, 1172.0f,
                     668.0f, 1.0f, Alpha(120, t4 * fade), 0.0f, 0);
    }
}

// ---------------- 一个漫画风按钮 ----------------
static void DrawButton(Graphics& g, int index, RectF r, const wchar_t* label,
                       float hover, float appear)
{
    float scale = 1.0f + 0.045f * hover;
    float cx = r.X + r.Width  * 0.5f;
    float cy = r.Y + r.Height * 0.5f;

    // 悬停时整块略微抬起
    float lift = -3.0f * hover;

    RectF rr(cx - r.Width * 0.5f * scale, cy - r.Height * 0.5f * scale + lift,
             r.Width * scale, r.Height * scale);

    GraphicsPath path;
    MakeRoundRect(path, rr, 16.0f);

    // 硬阴影（漫画感）
    {
        GraphicsPath sp;
        MakeRoundRect(sp, RectF(rr.X + 6.0f, rr.Y + 7.0f, rr.Width, rr.Height), 16.0f);
        SolidBrush sb(Color(Alpha(90, appear), 0, 0, 0));
        g.FillPath(&sb, &sp);
    }

    // 底色：米白 -> 悬停淡黄
    int cr = (int)(252 + (255 - 252) * hover);
    int cg = (int)(249 + (226 - 249) * hover);
    int cb = (int)(236 + (140 - 236) * hover);
    {
        SolidBrush fill(Color(Alpha(250, appear), (BYTE)cr, (BYTE)cg, (BYTE)cb));
        g.FillPath(&fill, &path);
    }

    // 描边：画两遍，稍微错开，像手勾的线
    {
        Pen pen1(Color(Alpha(255, appear), 34, 30, 26), 3.2f);
        pen1.SetLineJoin(LineJoinRound);
        g.DrawPath(&pen1, &path);

        Pen pen2(Color(Alpha(120, appear), 34, 30, 26), 1.4f);
        pen2.SetLineJoin(LineJoinRound);
        Matrix m;
        m.Translate(1.6f, 1.8f);
        path.Transform(&m);
        g.DrawPath(&pen2, &path);
    }

    // 文字
    BYTE ta = Alpha(255, appear);
    DrawTextCenter(g, label, *g_famUI, FontStyleBold, 31.0f, cx, cy + lift * 0.6f,
                   4.0f, Color(ta, 30, 26, 22), 0.6f);

    // 悬停时左侧一个小三角，像手画的箭头
    if (hover > 0.02f) {
        float ax = rr.X - 16.0f - 6.0f * (1.0f - hover);
        float ay = cy;
        PointF tri[3] = { PointF(ax, ay - 9.0f), PointF(ax + 13.0f, ay), PointF(ax, ay + 9.0f) };
        SolidBrush tb(Color(Alpha(220, hover * appear), 255, 214, 92));
        Pen tp(Color(Alpha(230, hover * appear), 34, 30, 26), 2.4f);
        tp.SetLineJoin(LineJoinRound);
        g.FillPolygon(&tb, tri, 3);
        g.DrawPolygon(&tp, tri, 3);
    }
}

// 提示气泡（还没做的功能）
static void DrawToast(Graphics& g, int W, int H, float toastY)
{
    if (g_toastText.empty()) return;
    ULONGLONG now = GetTickCount64();
    if (now >= g_toastUntil) { g_toastText.clear(); return; }

    float left  = (float)(g_toastUntil - now);
    float k     = Clamp01(left / 260.0f);       // 尾部淡出
    float total = 1700.0f;
    float appear = Clamp01((total - left) / 160.0f);
    float a = Clamp01(k * appear);

    float em = 24.0f;
    FontMetric fm = Metrics(*g_famUI, FontStyleRegular, em);
    float tw = 0.0f;
    for (const wchar_t* p = g_toastText.c_str(); *p; ++p)
        tw += ((unsigned)*p >= 0x2E80u) ? (em + 2.0f) : (em * 0.55f);
    float bw = tw + 56.0f, bh = fm.height + 34.0f;
    RectF r(640.0f - bw * 0.5f, toastY - bh * 0.5f, bw, bh);

    GraphicsPath path;
    MakeRoundRect(path, r, 14.0f);
    {
        SolidBrush sb(Color(Alpha(80, a), 0, 0, 0));
        GraphicsPath sp;
        MakeRoundRect(sp, RectF(r.X + 5, r.Y + 6, r.Width, r.Height), 14.0f);
        g.FillPath(&sb, &sp);
    }
    {
        SolidBrush fill(Color(Alpha(248, a), 255, 251, 236));
        g.FillPath(&fill, &path);
        Pen pen(Color(Alpha(240, a), 34, 30, 26), 2.6f);
        pen.SetLineJoin(LineJoinRound);
        g.DrawPath(&pen, &path);
    }
    DrawTextCenter(g, g_toastText.c_str(), *g_famUI, FontStyleRegular, em,
                   640.0f, r.Y + r.Height * 0.5f, 2.0f, Color(Alpha(255, a), 30, 26, 22), 0.0f);
}

// ---------------- 菜单界面 ----------------
static void RenderMenu(Graphics& g, float el)
{
    // 背景（已按窗口尺寸拼好并缓存）
    if (g_bgCache) g.DrawImage(g_bgCache, RectF(0.0f, 0.0f, (float)BASE_W, (float)BASE_H));

    // 按钮依次滑入
    for (int i = 0; i < 5; ++i) {
        float t = EaseOut(Clamp01((el - (140.0f + i * 90.0f)) / 420.0f));
        RectF r(BTN_X + (1.0f - t) * 70.0f, BTN_TOP + i * (BTN_H + BTN_GAP), BTN_W, BTN_H);
        DrawButton(g, i, r, kMenu[i].label, g_hoverAnim[i], t);
    }

    // 左下角小字：版本 + 全屏提示（第二十二步）
    DrawTextLine(g, L"v2.9", *g_famUI, FontStyleRegular, 18.0f, 86.0f, 668.0f,
                 1.0f, 110, 0.0f, 0);
    DrawTextLine(g, L"F11 全屏 / 小窗", *g_famUI, FontStyleRegular, 16.0f, 112.0f, 692.0f,
                 1.0f, 90, 0.0f, 0);

    // 从黑场淡入
    float fin = Clamp01(el / 420.0f);
    if (fin < 1.0f) {
        SolidBrush black(Color(Alpha(255, 1.0f - fin), 0, 0, 0));
        g.FillRectangle(&black, 0, 0, BASE_W, BASE_H);
    }
}

// ---------------- 第二十二步：结局（屏幕变黑 + 结局名 + 描述） ----------------
static void RenderEnding(Graphics& g, float el)
{
    SolidBrush bg(Color(255, 6, 6, 8));                 // 整屏变黑
    g.FillRectangle(&bg, 0, 0, BASE_W, BASE_H);

    float tn = EaseOut(Clamp01((el - 520.0f)  / 760.0f));
    float td = EaseOut(Clamp01((el - 1180.0f) / 760.0f));
    float tb = EaseOut(Clamp01((el - 1760.0f) / 520.0f));

    const wchar_t* nm = (g_ending > 0 && g_ending < END_COUNT) ? kEndName[g_ending] : L"";
    const wchar_t* ds = (g_ending > 0 && g_ending < END_COUNT) ? kEndDesc[g_ending] : L"";

    // 结局名
    DrawTextCenter(g, nm, *g_famTitle, FontStyleBold, 58.0f,
                   (float)BASE_W * 0.5f, 262.0f, 5.0f,
                   Color(Alpha(255, tn), 246, 240, 228), 0.0f);
    // 名字下面一条手绘线
    DrawWobbleLine(g, (float)BASE_W * 0.5f, 330.0f, 200.0f * tn, 3.0f, 1.1f, 2.6f,
                   Color(Alpha(130, tn), 198, 118, 88));

    // 结局描述
    DrawBodyText(g, ds, *g_famBody, FontStyleRegular, 30.0f,
                 (float)BASE_W * 0.5f - 430.0f, 388.0f, 860.0f, 48.0f,
                 Color(255, 226, 220, 210), td, 0.0f);

    // 返回菜单
    if (tb > 0.0f) {
        RectF r((float)BASE_W * 0.5f - 140.0f, 552.0f, 280.0f, 62.0f);
        DrawButton(g, 63, r, L"返回菜单", g_hoverAnim[63], tb);
    }
}

// ---------------- 第二十二步：Day4「此为本次测试的全部内容」 ----------------
static void RenderTestEnd(Graphics& g, float el)
{
    SolidBrush bg(Color(255, 6, 6, 8));
    g.FillRectangle(&bg, 0, 0, BASE_W, BASE_H);

    float tn = EaseOut(Clamp01((el - 420.0f)  / 720.0f));
    float tb = EaseOut(Clamp01((el - 1120.0f) / 520.0f));

    DrawTextCenter(g, L"此为本次测试的全部内容", *g_famTitle, FontStyleBold, 52.0f,
                   (float)BASE_W * 0.5f, (float)BASE_H * 0.5f - 46.0f, 5.0f,
                   Color(Alpha(255, tn), 246, 240, 228), 0.0f);
    DrawWobbleLine(g, (float)BASE_W * 0.5f, (float)BASE_H * 0.5f + 8.0f, 250.0f * tn,
                   3.0f, 1.4f, 2.6f, Color(Alpha(120, tn), 198, 118, 88));

    // 下方按钮：返回菜单页面
    if (tb > 0.0f) {
        RectF r((float)BASE_W * 0.5f - 140.0f, (float)BASE_H * 0.5f + 66.0f, 280.0f, 62.0f);
        DrawButton(g, 63, r, L"返回菜单", g_hoverAnim[63], tb);
    }
}

// ---------------- 整个界面的一帧 ----------------
static void RenderScene(HDC hdc, int W, int H)
{
    if (W <= 0 || H <= 0) return;

    BuildBackground(W, H);
    if (g_scene == SCENE_GAME) {
        if (g_loc == LOC_CANTEEN)  BuildCanteenBg(W, H);
        else if (g_loc == LOC_DORM) BuildDormBg(W, H);
        else if (g_loc == LOC_BOOK) BuildBookBg(W, H);
        else if (g_loc == LOC_FARM) BuildFarmBg(W, H);
        else if (g_loc == LOC_PLAY) BuildPlayBg(W, H);
        else if (g_loc == LOC_COURT) BuildCourtBg(W, H);
        else if (g_loc == LOC_MUSIC) BuildMusicBg(W, H);
        else if (g_loc == LOC_EVENT1) BuildEvent1Bg(W, H);
        else if (g_loc == LOC_GAOKAO) BuildGaokaoBg(W, H);
        else if (g_loc == LOC_EXAM)   BuildExamBg(W, H);
        else                        BuildGameBg(W, H);
    }

    // 第二十三步（新增3）：后备位图缓存复用。
    // 以前每帧 CreateDIBSection + DeleteObject：1080p 是 8MB、4K 是 33MB 的分配 + 清零，
    // 大屏时这一笔的开销比画面本身还大。尺寸没变就直接复用。
    static HDC     memDC   = nullptr;
    static HBITMAP memBmp  = nullptr;
    static int     memW = 0, memH = 0;
    void*   bits   = nullptr;
    if (!memDC) memDC = CreateCompatibleDC(hdc);
    if (memW != W || memH != H || !memBmp) {
        if (memBmp) { DeleteObject(memBmp); memBmp = nullptr; }
        BITMAPINFO bmi;
        ZeroMemory(&bmi, sizeof(bmi));
        bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth       = W;
        bmi.bmiHeader.biHeight      = -H;
        bmi.bmiHeader.biPlanes      = 1;
        bmi.bmiHeader.biBitCount    = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        memBmp = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
        SelectObject(memDC, memBmp);
        memW = W; memH = H;
    }

    {
        Graphics g(memDC);
        g.SetSmoothingMode(SmoothingModeAntiAlias);
        g.SetPixelOffsetMode(PixelOffsetModeHalf);
        g.Clear(Color(255, 0, 0, 0));

        float s = (float)W / BASE_W;
        float sy = (float)H / BASE_H;
        if (sy < s) s = sy;
        g_viewScale = s;                  // 第二十三步（新增3）：给小游戏判断「大屏」用
        g_viewOffX = ((float)W - BASE_W * s) * 0.5f;
        g_viewOffY = ((float)H - BASE_H * s) * 0.5f;
        g.TranslateTransform(g_viewOffX, g_viewOffY);
        g.ScaleTransform(s, s);

        float el = (float)(GetTickCount64() - g_t0);
        if (g_scene == SCENE_DISCLAIMER)      RenderDisclaimer(g, el);
        else if (g_scene == SCENE_HELP)       RenderHelp(g, el);
        else if (g_scene == SCENE_PICK)       RenderPick(g, el);
        else if (g_scene == SCENE_GAME)       RenderGame(g, el);
        else if (g_scene == SCENE_MAP)        RenderMap(g, el);
        else if (g_scene == SCENE_ACHIEVE)    RenderAchieve(g, el);
        else if (g_scene == SCENE_ENDING)     RenderEnding(g, el);
        else if (g_scene == SCENE_TESTEND)    RenderTestEnd(g, el);
        else                                  RenderMenu(g, el);

        // 提示条叠在所有场景之上（选角页放顶部，免得压住卡片名字）
        float toastY = 636.0f;
        if (g_scene == SCENE_PICK) toastY = 106.0f;
        else if (g_scene == SCENE_GAME) toastY = 556.0f;
        else if (g_scene == SCENE_MAP)  toastY = 566.0f;
        DrawToast(g, BASE_W, BASE_H, toastY);

        // 第十四步：确认弹窗压在最上面（地图上也会出现）
        DrawConfirmBox(g);
        // 第十八步：属性变化提示（比确认弹窗还高一层）
        DrawDeltaPanel(g);
    }

    BitBlt(hdc, 0, 0, W, H, memDC, 0, 0, SRCCOPY);   // 第二十三步：memDC / memBmp 复用，不再每帧销毁
}

// ---------------- 场景切换 ----------------
// 从地图返回教学楼界面时保留天数/时间/属性，只有重新选角才重置
// （变量定义已上移到 TryForceEvent 之前，它要用）

static void SetScene(int scene)
{
    g_scene = scene;
    g_t0 = GetTickCount64();
    g_hover = -1;
    g_pressed = -1;
    g_xHover = false;
    g_xAnim = 0.0f;
    g_detail = -1;
    g_detailOpen = false;
    g_detailAnim = 0.0f;
    g_backAnim = 0.0f;
    if (scene == SCENE_PICK) g_chosen = -1;
    if (scene == SCENE_ACHIEVE) g_achScroll = 0.0f;   // 第二十三步：每次进成就页都从顶部开始
    // 第二十三步（新增3）：事件场地只在事件期间有效，离开游戏画面（回地图 / 回菜单）时
    // 一律复位到教学楼。之前事件结束只切了场景、没复位 g_loc，回教室后背景还停在事件那张图上。
    if (scene == SCENE_MAP || scene == SCENE_MENU) LeaveEventLoc();
    if (scene == SCENE_GAME) {
        if (g_keepGameState) g_keepGameState = false;
        else ResetGameState();
    }
    for (int i = 0; i < 72; ++i) g_hoverAnim[i] = 0.0f;
}

// ---------------- 鼠标：像素坐标 -> 逻辑坐标 ----------------
static void ClientToLogical(int mx, int my, float& lx, float& ly)
{
    RECT rc;
    GetClientRect(g_hwnd, &rc);
    int W = rc.right, H = rc.bottom;
    float s = (float)W / BASE_W;
    float sy = (float)H / BASE_H;
    if (sy < s) s = sy;
    if (s <= 0.0f) s = 1.0f;
    float ox = ((float)W - BASE_W * s) * 0.5f;
    float oy = ((float)H - BASE_H * s) * 0.5f;
    lx = ((float)mx - ox) / s;
    ly = ((float)my - oy) / s;
}

static bool HitBackToMenu(float lx, float ly);   // 第二十二步，定义在后面

// 第二十三步：兜底出口 —— 只有「连对话框都没有」的时候才生效，
// 保证任何场地都不会出现点哪都没反应的死界面（返回菜单 / 自修一会）
static int FallbackHits(float lx, float ly)
{
    if (g_dlgActive || g_deltaActive || g_planOpen || g_shopOpen) return -1;
    if (lx >= GBACK_X && lx <= GBACK_X + GBACK_W &&
        ly >= GBACK_Y && ly <= GBACK_Y + GBACK_H) return 20;
    // 第二十三步（新增3）：事件还没结束时不给「自修一会」，否则会跳出事件、
    // 界面就不是「对应事件的界面」了（返回菜单 20 保留，作为卡住时的逃生口）
    if (IsEventLoc(g_loc)) return -1;
    if (lx >= 1078.0f && lx <= 1078.0f + 176.0f &&
        ly >= 196.0f && ly <= 196.0f + 54.0f) return 22;
    return -1;
}

static int HitTest(float lx, float ly)
{
    // 第十四步：确认弹窗是全局的，出现在任何场景都要先拦下来
    if (g_confirmKind != CONF_NONE) {
        float bw = 600.0f, bh = 224.0f;
        float bx = 640.0f - bw * 0.5f, by = 258.0f;
        if (ly >= by + 134.0f && ly <= by + 192.0f) {
            if (lx >= bx + 70.0f && lx <= bx + 260.0f) return 60;    // 是
            if (lx >= bx + 340.0f && lx <= bx + 530.0f) return 61;  // 否
        }
        return -1;
    }
    // 第二十步：事件1 正在选物品栏里的东西 → 只认物品格
    if (g_pickItemMode) {
        float total = SLOT_N * SLOT_S + (SLOT_N - 1) * SLOT_GAP;
        float x0 = 640.0f - total * 0.5f;
        for (int i = 1; i < SLOT_N; ++i) {
            if (g_inv[i] == ITEM_EMPTY) continue;
            float sx = x0 + (float)i * (SLOT_S + SLOT_GAP);
            if (lx >= sx && lx <= sx + SLOT_S && ly >= SLOT_Y && ly <= SLOT_Y + SLOT_S)
                return 40 + i;
        }
        return -1;
    }
    // 第十八步：属性变化面板开着时，只认「确定」
    if (g_deltaActive) {
        int n = 0;
        for (int i = 0; i < 5; ++i) if (g_deltaStat[i] != 0) ++n;
        float w = 520.0f;
        float h = 96.0f + (float)n * 46.0f + 86.0f;
        float cx = (float)BASE_W * 0.5f;
        RectF r(cx - w * 0.5f, (float)BASE_H * 0.5f - h * 0.5f, w, h);
        RectF btn(cx - 100.0f, r.Y + h - 78.0f, 200.0f, 56.0f);
        if (lx >= btn.X && lx <= btn.X + btn.Width &&
            ly >= btn.Y && ly <= btn.Y + btn.Height) return 62;
        return -1;
    }
    // 第十六步：跑步演出的 5 秒里不响应任何点击
    if (g_runActive) return -1;
    // 第十七步：投篮游戏开着时，整屏归小游戏
    if (g_basketActive) return -1;
    // 第十九步：钢琴键游戏同样整屏接管
    if (ptActive) return -1;
    // 第二十一步：摘玉米游戏同样整屏接管
    if (cnActive) return -1;
    if (g_scene == SCENE_MENU) {
        for (int i = 0; i < 5; ++i) {
            float y = BTN_TOP + i * (BTN_H + BTN_GAP);
            if (lx >= BTN_X && lx <= BTN_X + BTN_W && ly >= y && ly <= y + BTN_H)
                return i;
        }
        return -1;
    }
    // 第二十二步：成就页只认右上角的红叉；结局 / 测试结束页只认「返回菜单」
    if (g_scene == SCENE_ACHIEVE) return -1;
    if (g_scene == SCENE_ENDING || g_scene == SCENE_TESTEND) {
        return HitBackToMenu(lx, ly) ? 63 : -1;
    }
    if (g_scene == SCENE_PICK) {
        if (g_detailOpen) {                     // 面板打开时只能点「是 / 返回」
            float y = OK_Y + 22.0f * (1.0f - g_detailAnim);
            if (lx >= OK_X && lx <= OK_X + OK_W && ly >= y && ly <= y + OK_H) return 10;
            if (lx >= BACK_X && lx <= BACK_X + OK_W && ly >= y && ly <= y + OK_H) return 11;
            return -1;
        }
        for (int i = 0; i < HERO_COUNT; ++i) {
            float x = CardX(i);
            if (lx >= x && lx <= x + CARD_W && ly >= CARD_Y && ly <= CARD_Y + CARD_H)
                return i;
        }
        return -1;
    }
    if (g_scene == SCENE_GAME) {
        // 第十步：事务安排打开时，只认它自己的三个热区
        if (g_planOpen) {
            float px1 = PLAN_CX - PLAN_W * 0.5f, py1 = PLAN_CY - PLAN_H * 0.5f;
            float px2 = PLAN_CX + PLAN_W * 0.5f, py2 = PLAN_CY + PLAN_H * 0.5f;
            // 右上角红叉
            if (lx >= px2 - 54.0f && lx <= px2 - 6.0f &&
                ly >= py1 + 6.0f && ly <= py1 + 54.0f) return 30;
            // 左下 <- / 右下 ->
            if (ly >= py2 - 62.0f && ly <= py2 - 8.0f) {
                if (lx >= px1 + 10.0f && lx <= px1 + 80.0f) return (g_planPage > 0) ? 31 : -1;
                if (lx >= px2 - 84.0f && lx <= px2 - 10.0f) return (g_planPage < 6) ? 32 : -1;
            }
            return -1;
        }
        // 第十二步：商店开着时只认它自己的热区
        if (g_shopOpen) {
            float px1 = SHOP_CX - SHOP_W * 0.5f, py1 = SHOP_CY - SHOP_H * 0.5f;
            float px2 = SHOP_CX + SHOP_W * 0.5f;
            // 右上角红叉
            if (lx >= px2 - 56.0f && lx <= px2 - 8.0f &&
                ly >= py1 + 8.0f && ly <= py1 + 56.0f) return 55;
            // 商品
            for (int i = 0; i < SHOP_N; ++i) {
                float rx = ShopRowX(), ry = ShopRowY(i);
                if (lx >= rx && lx <= rx + ShopRowW() &&
                    ly >= ry && ly <= ry + ROW_H) return 50 + i;
            }
            return -1;
        }
        // 第二十三步：台词（没有选项）时，点对话框 = 「>继续」。
        //  之前只有「教学楼 / 农田 / 考试」这些走通用分支的场地能点，
        //  食堂 / 宿舍 / 音乐楼 / 操场各自有独立分支、只认选项 → 播台词时点击完全没反应（体育课第一句就卡住）
        if (g_dlgActive && !g_dlgAsk) {
            RectF db = DlgBoxRect();
            if (lx >= db.X && lx <= db.X + db.Width &&
                ly >= db.Y && ly <= db.Y + db.Height) return 24;
        }
        // 第二十三步：右上角「保存」按钮（id 64）
        if (lx >= SAVE_X && lx <= SAVE_X + SAVE_W &&
            ly >= SAVE_Y && ly <= SAVE_Y + SAVE_H) return 64;
        // 第十二步：食堂里，只认「购买 / 离开」和物品栏
        if (g_loc == LOC_CANTEEN) {
            for (int i = 0; i < g_dlgOptN; ++i) {
                RectF r = DlgOptRect(i, g_dlgOptN);
                if (lx >= r.X && lx <= r.X + r.Width &&
                    ly >= r.Y && ly <= r.Y + r.Height) return 25 + i;
            }
            float total = SLOT_N * SLOT_S + (SLOT_N - 1) * SLOT_GAP;
            float x0 = 640.0f - total * 0.5f;
            for (int i = 1; i < SLOT_N; ++i) {
                if (g_inv[i] == ITEM_EMPTY) continue;
                float sx = x0 + (float)i * (SLOT_S + SLOT_GAP);
                if (lx >= sx && lx <= sx + SLOT_S && ly >= SLOT_Y && ly <= SLOT_Y + SLOT_S)
                    return 40 + i;
            }
            { int ex = FallbackHits(lx, ly); if (ex >= 0) return ex; }   // 第二十三步：兜底出口
            return -1;
        }
        // 第十三步：宿舍里，只认「缩小的对话框」选项和物品栏
        if (g_loc == LOC_DORM) {
            for (int i = 0; i < g_dlgOptN; ++i) {
                RectF r = DlgOptRect(i, g_dlgOptN);
                if (lx >= r.X && lx <= r.X + r.Width &&
                    ly >= r.Y && ly <= r.Y + r.Height) return 25 + i;
            }
            float total = SLOT_N * SLOT_S + (SLOT_N - 1) * SLOT_GAP;
            float x0 = 640.0f - total * 0.5f;
            for (int i = 1; i < SLOT_N; ++i) {
                if (g_inv[i] == ITEM_EMPTY) continue;
                float sx = x0 + (float)i * (SLOT_S + SLOT_GAP);
                if (lx >= sx && lx <= sx + SLOT_S && ly >= SLOT_Y && ly <= SLOT_Y + SLOT_S)
                    return 40 + i;
            }
            { int ex = FallbackHits(lx, ly); if (ex >= 0) return ex; }   // 第二十三步：兜底出口
            return -1;
        }
        // 第十九步：音乐楼，只认选项
        if (g_loc == LOC_MUSIC) {
            for (int i = 0; i < g_dlgOptN; ++i) {
                RectF r = DlgOptRect(i, g_dlgOptN);
                if (lx >= r.X && lx <= r.X + r.Width &&
                    ly >= r.Y && ly <= r.Y + r.Height) return 25 + i;
            }
            { int ex = FallbackHits(lx, ly); if (ex >= 0) return ex; }   // 第二十三步：兜底出口
            return -1;
        }
        // 第十六步：操场上，只认选项（跑步演出期间不响应任何点击）
        if (g_loc == LOC_PLAY) {
            for (int i = 0; i < g_dlgOptN; ++i) {
                RectF r = DlgOptRect(i, g_dlgOptN);
                if (lx >= r.X && lx <= r.X + r.Width &&
                    ly >= r.Y && ly <= r.Y + r.Height) return 25 + i;
            }
            { int ex = FallbackHits(lx, ly); if (ex >= 0) return ex; }   // 第二十三步：兜底出口
            return -1;
        }
        // 对话框显示时，只认「继续」和选项
        if (g_dlgActive) {
            if (g_dlgOptN > 0) {
                for (int i = 0; i < g_dlgOptN; ++i) {
                    RectF r = DlgOptRect(i, g_dlgOptN);
                    if (lx >= r.X && lx <= r.X + r.Width &&
                        ly >= r.Y && ly <= r.Y + r.Height) return 25 + i;
                }
                return -1;
            }
            if (lx >= DLG_X && lx <= DLG_X + DLG_W &&
                ly >= DLG_Y && ly <= DLG_Y + DLG_H) return 24;
            return -1;
        }
        // 第二十三步（新增4）：事件（课外实践 / 讲高考 / 考试）进行中，这两个按钮本来就不画，
        //   但热区还在 —— 手一抖点上去就把事件跳过去了。事件期间一律屏蔽
        if (IsEventLoc(g_loc)) {
            float total = SLOT_N * SLOT_S + (SLOT_N - 1) * SLOT_GAP;
            float x0 = 640.0f - total * 0.5f;
            for (int i = 1; i < SLOT_N; ++i) {
                if (g_inv[i] == ITEM_EMPTY) continue;
                float sx = x0 + (float)i * (SLOT_S + SLOT_GAP);
                if (lx >= sx && lx <= sx + SLOT_S && ly >= SLOT_Y && ly <= SLOT_Y + SLOT_S)
                    return 40 + i;
            }
            return -1;
        }
        if (lx >= GBACK_X && lx <= GBACK_X + GBACK_W && ly >= GBACK_Y && ly <= GBACK_Y + GBACK_H)
            return 20;
        if (lx >= 1078.0f && lx <= 1078.0f + 176.0f && ly >= 196.0f && ly <= 196.0f + 54.0f)
            return 22;                          // 自修一会（原下一时间段）
        // 物品栏 1~8 格：有东西就能点
        {
            float total = SLOT_N * SLOT_S + (SLOT_N - 1) * SLOT_GAP;
            float x0 = 640.0f - total * 0.5f;
            for (int i = 1; i < SLOT_N; ++i) {
                if (g_inv[i] == ITEM_EMPTY) continue;
                float sx = x0 + (float)i * (SLOT_S + SLOT_GAP);
                if (lx >= sx && lx <= sx + SLOT_S && ly >= SLOT_Y && ly <= SLOT_Y + SLOT_S)
                    return 40 + i;
            }
        }
        return -1;
    }
    if (g_scene == SCENE_MAP) {
        if (lx >= MBACK_X && lx <= MBACK_X + MBACK_W && ly >= MBACK_Y && ly <= MBACK_Y + MBACK_H)
            return 23;                          // 返回（21 让给第 10 个场地音乐楼）
        // 9 个场地；判定范围就是画面上那个橙色手绘圈（椭圆），
        // 7 号与 9 号的圈有重叠，重叠处取圈中心离鼠标最近的那个
        int   best = -1;
        float bestD = 1e30f;
        for (int i = 0; i < 10; ++i) {
            const MapSpot& s = kMapSpots[i];
            float x1 = MapSX(s.x1), y1 = MapSY(s.y1);
            float x2 = MapSX(s.x2), y2 = MapSY(s.y2);
            if (lx < x1 || lx > x2 || ly < y1 || ly > y2) continue;   // 框外不算
            // 框内取离中心最近的一个（万一个别框轻微交叠）
            float cx = (x1 + x2) * 0.5f, cy = (y1 + y2) * 0.5f;
            float dx = (lx - cx) / ((x2 - x1) * 0.5f);
            float dy = (ly - cy) / ((y2 - y1) * 0.5f);
            float d = dx * dx + dy * dy;
            if (d < bestD) { bestD = d; best = i; }
        }
        if (best >= 0) return 12 + best;         // 12~21 -> 10 个场地
        return -1;
    }
    return -1;
}

static bool HitCross(float lx, float ly)
{
    if (g_scene != SCENE_HELP && g_scene != SCENE_PICK &&
        g_scene != SCENE_ACHIEVE) return false;            // 第二十二步：成就页也有红叉
    return (lx >= XCX - XHIT && lx <= XCX + XHIT &&
            ly >= XCY - XHIT && ly <= XCY + XHIT);
}

// 第二十二步：结局 / 测试结束页只有一个「返回菜单」按钮（热区 63）
static bool HitBackToMenu(float lx, float ly)
{
    if (g_scene != SCENE_ENDING && g_scene != SCENE_TESTEND) return false;
    float y = (g_scene == SCENE_ENDING) ? 552.0f : (float)BASE_H * 0.5f + 66.0f;
    return (lx >= (float)BASE_W * 0.5f - 140.0f && lx <= (float)BASE_W * 0.5f + 140.0f &&
            ly >= y && ly <= y + 62.0f);
}

static void Toast(const wchar_t* text)
{
    g_toastText = text;
    g_toastUntil = GetTickCount64() + 1700;
}

static void ActivateButton(int idx)
{
    if (idx < 0 || idx >= 5) return;
    switch (kMenu[idx].action) {
    case ACT_QUIT:
        PostMessage(g_hwnd, WM_CLOSE, 0, 0);      // 直接结束程序
        break;
    case ACT_HELP:
        SetScene(SCENE_HELP);                      // 第三步：进游戏说明界面
        break;
    case ACT_START:
        SetScene(SCENE_PICK);                      // 第四步：进入角色选择
        break;
    case ACT_CONTINUE:
        // 第二十三步：有存档就接着玩，没有就提示
        if (!HasSave()) { Toast(L"继续游戏：还没有存档"); break; }
        if (!LoadGame()) { Toast(L"继续游戏：存档读取失败"); break; }
        g_keepGameState = true;              // 别让 SetScene 把读出来的进度清掉
        SetScene(SCENE_MAP);
        Toast(L"已读取存档");
        break;
    case ACT_ACHIEVE:
        SetScene(SCENE_ACHIEVE);                 // 第二十二步：成就查看页
        break;
    }
}

// ============================================================
//  第二十二步：结局 / Day4 结束页的触发判定
//  只在游戏里（教学楼 / 地图）判定，并且要等当前这一套演出走完再切，
//  免得把对话、属性面板、动画、小游戏拦腰截断。
// ============================================================
static bool GameBusy()
{
    if (g_dlgActive || g_deltaActive || g_planOpen)  return true;
    if (g_animKind != ANIM_NONE || g_runActive)      return true;
    if (cnActive || g_basketActive || ptActive)      return true;
    if (g_pickItemMode || g_shopOpen)                return true;
    if (g_confirmKind != 0)                          return true;
    if (g_centerMsg && GetTickCount64() < g_centerMsgUntil) return true;
    return false;
}

// 属性顺序：0 健康 / 1 饱食 / 2 精神 / 3 快乐 / 4 知识
// 开发说明里结局的排列顺序是 精神 -> 健康 -> 饱食 -> 快乐
static int EndingForStats()
{
    if (g_stat[2] <= 0) return END_MENTAL;      // 任何时候精神为 0
    if (g_stat[0] <= 0) return END_HOSPITAL;    // 任何时候健康为 0
    if (g_stat[1] <= 0) return END_HUNGRY;      // 任何时候饱食为 0
    if (g_stat[3] <= 0) return END_DEPRESS;     // 任何时候快乐为 0
    return END_NONE;
}

static void CheckEnding()
{
    if (g_scene != SCENE_GAME && g_scene != SCENE_MAP) return;
    if (GameBusy()) return;
    int e = EndingForStats();
    if (e != END_NONE) { g_ending = e; MarkEnding(e); SetScene(SCENE_ENDING); return; }   // 第二十三步：记进成就页
    if (g_day >= 4) { SetScene(SCENE_TESTEND); return; }     // 时间到 day4
}

// ============================================================
//  第二十二步：全屏 / 小窗适配
//    · 窗口带标题栏、可拉伸、可最大化，拖边框改大小时锁 16:9
//    · 渲染本来就按逻辑分辨率 1280x720 等比缩放 + 居中黑边，
//      所以任意窗口尺寸下都不会变形，只是上下（或左右）多两条黑边
//    · F11 / Alt+Enter 在「全屏」和「小窗」之间来回切，切回小窗会还原原来的位置
// ============================================================
static bool g_fullScreen = false;
static RECT g_winRect = { 0, 0, 0, 0 };    // 记住窗口模式下的位置，切回来能还原
static const DWORD kFrameStyle = WS_CAPTION | WS_SYSMENU | WS_THICKFRAME |
                                 WS_MINIMIZEBOX | WS_MAXIMIZEBOX;

static void ToggleFullscreen()
{
    if (!g_hwnd) return;
    DWORD style = (DWORD)GetWindowLongPtr(g_hwnd, GWL_STYLE);

    if (!g_fullScreen) {
        GetWindowRect(g_hwnd, &g_winRect);        // 先记下当前窗口的位置和大小
        int mx = 0, my = 0;
        int sw = GetSystemMetrics(SM_CXSCREEN);
        int sh = GetSystemMetrics(SM_CYSCREEN);
        MONITORINFO mi;
        ZeroMemory(&mi, sizeof(mi));
        mi.cbSize = sizeof(mi);
        if (GetMonitorInfoW(MonitorFromWindow(g_hwnd, MONITOR_DEFAULTTONEAREST), &mi)) {
            mx = mi.rcMonitor.left;
            my = mi.rcMonitor.top;
            sw = mi.rcMonitor.right  - mi.rcMonitor.left;
            sh = mi.rcMonitor.bottom - mi.rcMonitor.top;
        }
        SetWindowLongPtr(g_hwnd, GWL_STYLE, style & ~kFrameStyle);
        SetWindowPos(g_hwnd, HWND_TOP, mx, my, sw, sh,
                     SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        g_fullScreen = true;
    } else {
        SetWindowLongPtr(g_hwnd, GWL_STYLE, style | kFrameStyle);
        int w = g_winRect.right  - g_winRect.left;
        int h = g_winRect.bottom - g_winRect.top;
        if (w <= 0 || h <= 0) { w = BASE_W; h = BASE_H; }
        SetWindowPos(g_hwnd, HWND_NOTOPMOST, g_winRect.left, g_winRect.top, w, h,
                     SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        g_fullScreen = false;
    }
    InvalidateRect(g_hwnd, nullptr, FALSE);
}

// ---------------- 窗口过程 ----------------
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        RenderScene(hdc, rc.right, rc.bottom);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_SIZE:
        if (g_bgCache) { delete g_bgCache; g_bgCache = nullptr; g_bgW = g_bgH = 0; }
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    case WM_MOUSEWHEEL:                 // 第二十三步：成就页内容装不下时滚轮滚动
        if (g_scene == SCENE_ACHIEVE) {
            int d = (int)(short)HIWORD(wParam);          // 向上滚为正
            g_achScroll -= (float)d / 120.0f * 90.0f;    // 向下滚 → 内容往上走
            if (g_achScroll < 0.0f) g_achScroll = 0.0f;
            if (g_achScroll > g_achScrollMax) g_achScroll = g_achScrollMax;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;

    // 第二十二步：拖动边框改大小时锁死 16:9，画面不会变形
    case WM_SIZING: {
        if (g_fullScreen) return TRUE;
        RECT* pr = (RECT*)lParam;
        RECT  nc = { 0, 0, 0, 0 };
        AdjustWindowRect(&nc, (DWORD)GetWindowLongPtr(hwnd, GWL_STYLE), FALSE);
        LONG dw = nc.right - nc.left;                 // 左右边框 + 标题栏带来的开销
        LONG dh = nc.bottom - nc.top;
        LONG cw = (pr->right - pr->left)  - dw;       // 客户区
        LONG ch = (pr->bottom - pr->top)  - dh;
        if (cw <= 0 || ch <= 0) return TRUE;
        const double kAR = (double)BASE_W / (double)BASE_H;      // 16:9
        if ((double)cw / (double)ch > kAR) cw = (LONG)(ch * kAR);
        else                               ch = (LONG)(cw / kAR);
        LONG nw = cw + dw, nh = ch + dh;
        switch (wParam) {
        case WMSZ_LEFT:          pr->left   = pr->right - nw;  break;
        case WMSZ_RIGHT:         pr->right  = pr->left  + nw;  break;
        case WMSZ_TOP:           pr->top    = pr->bottom - nh; break;
        case WMSZ_BOTTOM:        pr->bottom = pr->top   + nh;  break;
        case WMSZ_TOPLEFT:       pr->top = pr->bottom - nh; pr->left  = pr->right - nw; break;
        case WMSZ_TOPRIGHT:      pr->top = pr->bottom - nh; pr->right = pr->left  + nw; break;
        case WMSZ_BOTTOMLEFT:    pr->bottom = pr->top  + nh; pr->left  = pr->right - nw; break;
        case WMSZ_BOTTOMRIGHT:   pr->bottom = pr->top  + nh; pr->right = pr->left  + nw; break;
        default: break;
        }
        return TRUE;
    }

    case WM_TIMER:
        if (wParam == ID_TIMER) {
            // 免责界面 3 秒后自动进菜单
            if (g_scene == SCENE_DISCLAIMER &&
                GetTickCount64() - g_t0 >= DISCLAIMER_MS) {
                SetScene(SCENE_MENU);
            }
            // 悬停插值
            for (int i = 0; i < 72; ++i) {
                float target = (g_hover == i) ? 1.0f : 0.0f;
                g_hoverAnim[i] += (target - g_hoverAnim[i]) * 0.22f;
                if (fabsf(target - g_hoverAnim[i]) < 0.004f) g_hoverAnim[i] = target;
            }
            // 第二十二步：学霸成就（检测到知识达到 300 触发）
            if (g_stat[4] >= 300) UnlockAch(ACH_XUEBA);
            // 第二十二步：属性归零的结局 / 时间到 day4 的测试结束页
            CheckEnding();
            // 第十步：剧情触发 / 打字机 / 事务安排开合
            if (g_scene == SCENE_GAME) {
                if (g_loc == LOC_CLASS &&
                    !g_storyDone && !g_dlgActive && !g_planOpen &&
                    g_day == 1 && g_time == 0) {
                    g_storyDone = true;          // day1 清晨班会剧情，只播一次
                    DlgStart();
                }
                // 第十一步：到了有课的时段，弹出「这节课该如何度过」
                // day1 清晨是班会（剧情），不弹选项
                if (g_loc == LOC_CLASS && !g_dlgActive && !g_planOpen) {
                    bool banhui = (g_day == 1 && g_time == 0);
                    int  subj   = SubjectAt(g_day, g_time);
                    int  stamp  = g_day * 100 + g_time;
                    if (!banhui && subj >= 0 && subj < SUBJ_COUNT && g_classStamp != stamp) {
                        g_classStamp = stamp;
                        ClassAsk(subj);
                    }
                }
                if (g_dlgActive && !g_dlgAsk) {
                    if (g_dlgChars < (float)g_dlgTotal) {
                        g_dlgChars += 0.52f;     // 16ms 一帧 -> 约 32 字/秒
                        if (g_dlgChars > (float)g_dlgTotal) g_dlgChars = (float)g_dlgTotal;
                    }
                }
                float pt = g_planOpen ? 1.0f : 0.0f;
                g_planAnim += (pt - g_planAnim) * 0.20f;
                if (fabsf(pt - g_planAnim) < 0.004f) g_planAnim = pt;

                // 第十二步：商店开合
                float st = g_shopOpen ? 1.0f : 0.0f;
                g_shopAnim += (st - g_shopAnim) * 0.20f;
                if (fabsf(st - g_shopAnim) < 0.004f) g_shopAnim = st;
            }

            // 第十三步：晚寝时间强制拉去宿舍（在地图页就拦下来）
            if (g_scene == SCENE_MAP && g_time == 20 && !DeltaBusy()) {
                int stamp = g_day * 100 + g_time;
                if (g_dormStamp != stamp) {
                    g_dormStamp = stamp;
                    g_keepGameState = true;
                    EnterDorm(true);
                    SetScene(SCENE_GAME);
                }
            }

            // 第二十步：day2 上午第 5 节 —— 课外实践活动
            if ((g_scene == SCENE_MAP || (g_scene == SCENE_GAME && g_loc == LOC_CLASS))
                && g_day == 2 && g_time == 5 && !g_event1Done && !DeltaBusy()) {
                int stamp = g_day * 100 + g_time;
                if (g_event1Stamp != stamp) {
                    g_event1Stamp = stamp;
                    g_keepGameState = true;
                    EnterEvent1();
                    SetScene(SCENE_GAME);
                }
            }
            // 第二十步：day2 下午第一节 —— 北京名师讲高考
            // 第二十三步（新增4）：讲高考占下午四节，条件放宽到 10~13 节，
            //   这样中途存档 / 读档、或者下午第一节不在教学楼时也不会把整个事件漏掉
            if ((g_scene == SCENE_MAP || (g_scene == SCENE_GAME && g_loc == LOC_CLASS))
                && g_day == 2 && g_time >= 10 && g_time <= 13 && !g_gaokaoDone && !DeltaBusy()) {
                int stamp = g_day * 100 + g_time;
                if (g_gaokaoStamp != stamp) {
                    g_gaokaoStamp = stamp;
                    g_keepGameState = true;
                    EnterGaokao();
                    SetScene(SCENE_GAME);
                }
            }

            // 第二十一步：day3 下午第一节（time 10）—— 浙南联盟考试
            if ((g_scene == SCENE_MAP || (g_scene == SCENE_GAME && g_loc == LOC_CLASS))
                && g_day == 3 && g_time == 10 && !g_examDone && !DeltaBusy()) {
                int stamp = g_day * 100 + g_time;
                if (g_examStamp != stamp) {
                    g_examStamp = stamp;
                    g_keepGameState = true;
                    EnterExam();
                    SetScene(SCENE_GAME);
                }
            }

            // 第十六步：体育课强制拉去操场（地图页或教室里都拦）
            if ((g_scene == SCENE_MAP || (g_scene == SCENE_GAME && g_loc == LOC_CLASS))
                && !g_runActive && IsPEClass(g_day, g_time) && !DeltaBusy()) {
                int pstamp = g_day * 100 + g_time;
                if (g_playStamp != pstamp) {
                    g_playStamp = pstamp;
                    g_keepGameState = true;
                    EnterPlayground(true);
                    SetScene(SCENE_GAME);
                    CenterMsg(L"现在是体育课时间");
                }
            }

            // 第十六步：跑步演出满 5 秒 → 跳到下一个时间段并回地图
            if (g_runActive && GetTickCount64() - g_runStart >= RUN_MS) {
                g_runActive = false;                 // 第二十步：播完接着做原本的后续
                g_animKind  = 0;
                int after = g_animAfter;
                g_animAfter = 0;
                DoDeltaNext(after);
            }

            // 第十七步：投篮游戏每帧推进
            if (g_basketActive) BkUpdate();
            // 第十九步：钢琴键游戏每帧推进（16ms 一帧）
            if (ptActive) PtUpdate(0.016);
            // 第二十一步：摘玉米游戏每帧推进
            if (cnActive) CnUpdate(0.016f);
            // 表情：按状态自动切换，切换时 0.3s 左右交叉淡化
            {
                g_faceState.Refresh(g_stat[2], g_stat[3]);   // 精神、快乐
                g_faceState.Tick(0.016f);
            }
            // 角色介绍面板的开合
            {
                float target = g_detailOpen ? 1.0f : 0.0f;
                g_detailAnim += (target - g_detailAnim) * 0.22f;
                if (fabsf(target - g_detailAnim) < 0.004f) {
                    g_detailAnim = target;
                    if (target <= 0.001f) g_detail = -1;
                }
            }
            {
                float target = g_xHover ? 1.0f : 0.0f;
                g_xAnim += (target - g_xAnim) * 0.25f;
                if (fabsf(target - g_xAnim) < 0.004f) g_xAnim = target;
            }
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;

    case WM_MOUSEMOVE: {
        float lx, ly;
        ClientToLogical(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), lx, ly);
        int  h  = HitTest(lx, ly);
        bool xh = HitCross(lx, ly);
        if (cnActive) { cnMouseX = lx; cnMouseY = ly; }   // 第二十一步：摘玉米的悬停判定
        if (h != g_hover || xh != g_xHover) {
            g_hover  = h;
            g_xHover = xh;
            TRACKMOUSEEVENT tme;
            tme.cbSize = sizeof(tme);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hwnd;
            tme.dwHoverTime = 0;
            TrackMouseEvent(&tme);
            bool hand = (h >= 0) || xh;
            SetCursor(LoadCursor(nullptr, hand ? IDC_HAND : IDC_ARROW));
        }
        return 0;
    }

    case WM_MOUSELEAVE:
        g_hover = -1;
        g_xHover = false;
        SetCursor(LoadCursor(nullptr, IDC_ARROW));
        return 0;

    case WM_LBUTTONDOWN: {
        // 第十九步：钢琴键游戏里，结算页点一下等于按 ENTER
        if (ptActive) {
            if (ptState == ST_RESULT) PtSettle();
            return 0;
        }
        // 第二十一步：摘玉米游戏接管鼠标（点玉米 / 开始 / 结算）
        if (cnActive) {
            float lx, ly;
            ClientToLogical(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), lx, ly);
            CnClick(lx, ly);
            return 0;
        }
        // 第十七步：投篮游戏接管鼠标（按下 = 开始 / 蓄力）
        if (g_basketActive) {
            if (g_bkState == BK_START)      BkReset();
            else if (g_bkState == BK_END)   FinishBasket();
            else if (g_bkState == BK_AIM)   { g_bkCharging = true; g_bkPower = 0.15f; g_bkPowerDir = 1.0f; }
            return 0;
        }
        float lx, ly;
        ClientToLogical(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), lx, ly);
        g_pressed = HitTest(lx, ly);
        if (HitCross(lx, ly)) g_pressed = -2;      // 按在红叉上
        return 0;
    }

    case WM_LBUTTONUP: {
        // 第十七步：投篮游戏接管鼠标（松开 = 投篮）
        if (g_basketActive) {
            if (g_bkState == BK_AIM && g_bkCharging) { g_bkCharging = false; BkShoot(); }
            return 0;
        }
        float lx, ly;
        ClientToLogical(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), lx, ly);
        int h = HitTest(lx, ly);
        if (h >= 0 && h == g_pressed) {
            if (h == 63) {                          // 第二十二步：结局 / 测试结束 -> 返回菜单
                SetScene(SCENE_MENU);
            } else if (h == 62) {                   // 第十八步：属性变化「确定」
                RunDeltaNext();
            } else if (h == 60 || h == 61) {        // 第十四步：确认弹窗「是 / 否」
                bool yes = (h == 60);
                int  kind = g_confirmKind;
                int  slot = g_confirmSlot;
                g_confirmKind = CONF_NONE;
                g_confirmSlot = -1;
                if (kind == CONF_GOTO_BOOK) {
                    if (yes) {
                        g_keepGameState = true;
                        EnterBookHouse();   // 进入不消耗时间，离开时才消耗
                        SetScene(SCENE_GAME);
                    }
                    // 否：留在地图
                } else if (kind == CONF_THROW_BOOK) {
                    if (yes && slot >= 1 && slot < SLOT_N) g_inv[slot] = ITEM_EMPTY;
                }
            }
            else if (g_scene == SCENE_MENU) {
                ActivateButton(h);
            } else if (g_scene == SCENE_PICK) {
                if (h == 0 || h == 1) {            // 点角色卡 -> 看介绍
                    g_detail = h;
                    g_detailOpen = true;
                } else if (h == 10) {              // 是 -> 进入游戏 UI
                    g_chosen = g_detail;
                    g_detailOpen = false;
                    SetScene(SCENE_GAME);
                } else if (h == 11) {              // 返回选角界面
                    g_detailOpen = false;
                }
            } else if (g_scene == SCENE_GAME) {
                if (h == 30) {                        // 事务安排：红叉关闭（时间不变）
                    g_planOpen = false;
                } else if (h == 31) {                 // 往前翻
                    if (g_planPage > 0) g_planPage--;
                } else if (h == 32) {                 // 往后翻
                    if (g_planPage < 6) g_planPage++;
                } else if (h == 55) {                 // 第十二步：商店红叉 -> 回食堂 / 回书屋
                    g_shopOpen = false;
                    // 第二十三步（新增2）：关掉商店后要重新显示「购买 / 离开」，否则没有出口
                    if (g_loc == LOC_CANTEEN) CanteenAsk();
                    else if (g_loc == LOC_BOOK)  BookAsk();
                } else if (h >= 50 && h <= 54) {      // 商店：买第 i 件商品
                    if (g_shopKind == 0) ShopBuy(h - 50);
                    else                 BuyBook(h - 50);
                } else if (h >= 40 && h <= 48) {      // 物品栏：使用
                    if (g_pickItemMode) Event1Pick(h - 40);
                    else                UseItem(h - 40);
                } else if (h >= 25 && h <= 27) {      // 选项
                    if (g_loc == LOC_CANTEEN) {       // 食堂：购买 / 离开
                        if (h == 25) { g_shopOpen = true; g_shopKind = 0; }
                        else if (h == 26) LeaveCanteen();
                    } else if (g_loc == LOC_DORM) {   // 宿舍：小睡 / 睡觉 / 熬夜
                        DormChoose(h - 25);
                    } else if (g_loc == LOC_FARM) {   // 第十五步：农田：要 / 离开
                        FarmChoose(h - 25);
                    } else if (g_loc == LOC_PLAY) {   // 第十六步：操场：跑步 / 跑路（离开）
                        PlayChoose(h - 25);
                    } else if (g_loc == LOC_COURT) {  // 第十七步：篮球场：来 / 溜了
                        CourtChoose(h - 25);
                    } else if (g_loc == LOC_MUSIC) {  // 第十九步：音乐楼
                        MusicChoose(h - 25);
                    } else if (g_loc == LOC_EXAM) {   // 第二十一步：考试：认真考/轻松考、去摘玉米/去你*的
                        ExamOption(h - 25);
                    } else if (g_loc == LOC_GAOKAO) { // 第二十三步：讲高考：跑路 / 继续听
                        GaokaoChoose(h - 25);
                    } else if (g_loc == LOC_BOOK) {   // 第十四步：书屋：购买 / 离开
                        if (h == 25) { g_shopOpen = true; g_shopKind = 1; }
                        else if (h == 26) LeaveBookHouse();
                    } else if (g_dlgAsk) {
                        ClassChoose(h - 25);
                    } else {
                        DlgAdvance();
                    }
                } else if (h == 24) {                 // 对话框：继续
                    DlgAdvance();
                } else if (h == 64) {                 // 第二十三步：保存
                    SaveGame();
                    CenterMsg(L"数据已保存");
                } else if (h == 20) SetScene(SCENE_MENU);  // 左下角返回菜单
                else if (h == 22) {                  // 第十八步：自修一会
                    AddKnowledge(5, -1);             // 原本的属性变化之外 +5 知识
                    AdvanceTime();                   // 推进时段 + 时段自然衰减（不记录）
                    CommitDeltas(DN_SELF_STUDY);     // 弹「属性变化」→ 学习动画 → 回地图
                }
            } else if (g_scene == SCENE_MAP) {
                if (h == 23) {
                    g_keepGameState = true;        // 返回教学楼界面，保留进度
                    SetScene(SCENE_GAME);
                } else if (h >= 12 && h <= 21) {      // 10 个场地（21 = 音乐楼）
                    int spot = h - 12;
                    if (spot == 0) {
                        g_keepGameState = true;    // 点击 1 教学楼 -> 教学楼界面
                        g_loc = LOC_CLASS;
                        SetScene(SCENE_GAME);
                    } else if (spot == 6) {
                        // 第十二步：7 食堂和小卖部（只在饭点开放）
                        if (!CanteenOpen(g_time)) {
                            Toast(L"当前食堂还未开放");
                        } else {
                            g_keepGameState = true;
                            EnterCanteen();
                            SetScene(SCENE_GAME);
                        }
                    } else if (spot == 2) {
                        // 第十六步：3 操场（非体育课也能自己来）
                        g_keepGameState = true;
                        EnterPlayground(false);
                        SetScene(SCENE_GAME);
                    } else if (spot == 4) {
                        // 第十七步：5 篮球场
                        g_keepGameState = true;
                        EnterCourt();
                        SetScene(SCENE_GAME);
                    } else if (spot == 5) {
                        // 第十四步：6 行政楼与图书馆 —— 先问要不要去（会消耗时间）
                        g_confirmKind = CONF_GOTO_BOOK;
                        g_confirmText = L"去行政楼与图书馆会消耗时间，是否前往";
                        g_confirmSlot = -1;
                    } else if (spot == 7) {
                        // 第十五步：8 农田
                        g_keepGameState = true;
                        EnterFarm();
                        SetScene(SCENE_GAME);
                    } else if (spot == 9) {
                        // 第十九步：10 音乐楼
                        g_keepGameState = true;
                        EnterMusic();
                        SetScene(SCENE_GAME);
                    } else if (spot == 8) {
                        // 第十三步：9 宿舍
                        g_keepGameState = true;
                        EnterDorm(false);
                        SetScene(SCENE_GAME);
                    } else {
                        // 其余场地留待后续开发
                        std::wstring msg = std::wstring(L"「") + kMapSpots[spot].name +
                                           L"」将在后续开发中开放";
                        Toast(msg.c_str());
                    }
                }
            }
        } else if (g_pressed == -2 && HitCross(lx, ly)) {
            SetScene(SCENE_MENU);                  // 点红叉 -> 返回主菜单
        }
        g_pressed = -1;
        return 0;
    }

    // 第十四步：右键点背包里的书 → 弹出「是否确认扔掉这本书」
    case WM_RBUTTONUP: {
        float lx, ly;
        ClientToLogical(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), lx, ly);
        if (g_scene == SCENE_GAME && g_confirmKind == CONF_NONE) {
            float total = SLOT_N * SLOT_S + (SLOT_N - 1) * SLOT_GAP;
            float x0 = 640.0f - total * 0.5f;
            for (int i = 1; i < SLOT_N; ++i) {
                if (g_inv[i] == ITEM_EMPTY) continue;
                float sx = x0 + (float)i * (SLOT_S + SLOT_GAP);
                if (lx >= sx && lx <= sx + SLOT_S && ly >= SLOT_Y && ly <= SLOT_Y + SLOT_S) {
                    AskThrowBook(i);
                    break;
                }
            }
        }
        return 0;
    }

    case WM_KEYDOWN:
        // 第二十二步：F11 / Alt+Enter 全屏 <-> 小窗
        if (wParam == VK_F11 ||
            (wParam == VK_RETURN && (GetKeyState(VK_MENU) & 0x8000))) {
            ToggleFullscreen();
            return 0;
        }
        // 第十九步：钢琴键游戏开着时，所有按键都交给小游戏（1~7、SPACE、R、方向键、ESC）
        if (ptActive) {
            if ((lParam & 0x40000000) == 0) PtKeyDown(wParam);   // 忽略长按自动重复
            return 0;
        }
        // 第二十一步：摘玉米游戏期间，按键全部交给小游戏（W / 空格；ESC 也吃掉）
        if (cnActive) {
            if ((lParam & 0x40000000) == 0) CnKey(wParam);
            return 0;
        }
        if (wParam == VK_ESCAPE) {
            if (g_deltaActive) { RunDeltaNext(); break; }  // 第十八步：ESC 等同「确定」
            if (g_runActive) break;             // 第十六步：跑步演出期间不吃 ESC
            if (g_basketActive) break;          // 第十七步：投篮游戏期间不吃 ESC
            if (g_confirmKind != CONF_NONE) {   // 第十四步：ESC = 关掉确认框
                g_confirmKind = CONF_NONE;
                g_confirmSlot = -1;
            }
            else if (g_scene == SCENE_GAME && g_planOpen) {      // 先关事务安排
                g_planOpen = false;
            } else if (g_scene == SCENE_GAME && g_dlgActive) {
                DlgAdvance();
            } else if (g_scene == SCENE_MAP) {          // 地图页 ESC = 返回教学楼界面
                g_keepGameState = true;
                SetScene(SCENE_GAME);
            } else if (g_scene == SCENE_ACHIEVE ||     // 第二十二步：成就 / 结局 / 测试结束页
                       g_scene == SCENE_ENDING   ||    //         ESC = 返回主菜单
                       g_scene == SCENE_TESTEND) {
                SetScene(SCENE_MENU);
            } else {
                PostMessage(hwnd, WM_CLOSE, 0, 0);
            }
        } else if (g_scene == SCENE_GAME) {
            // 开发期预览表情：1~4 强制，5 恢复自动
            if (wParam >= '1' && wParam <= '4') {
                g_faceState.force = (int)(wParam - '1');
                g_faceState.Refresh(g_stat[2], g_stat[3]);
                std::wstring msg = std::wstring(L"表情预览：") + kFaceName[g_faceState.force] +
                                   L"（按 5 恢复自动）";
                Toast(msg.c_str());
            } else if (wParam == '5') {
                g_faceState.force = -1;
                g_faceState.Refresh(g_stat[2], g_stat[3]);
                Toast(L"表情已恢复自动");
            }
        }
        return 0;

    case WM_DESTROY:
        KillTimer(hwnd, ID_TIMER);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// ---------------- 加载同目录下的图片 ----------------
static std::wstring ExeDir()
{
    wchar_t buf[MAX_PATH * 2];
    DWORD n = GetModuleFileNameW(nullptr, buf, (DWORD)(sizeof(buf) / sizeof(buf[0])));
    std::wstring s(buf, n);
    size_t p = s.find_last_of(L"\\/");
    return (p == std::wstring::npos) ? std::wstring(L".") : s.substr(0, p);
}

// ============================================================
//  第二十二步：单文件打包
//  打包脚本把全部图片 / 音频打成一个二进制包，追加在 exe 文件尾部：
//    [exe 本体][资源包][32 字节尾巴]
//    尾巴 = 魔法串 "HSWRES01" + 包偏移 + 包大小 + 文件个数
//  第一次运行时把包里的文件释放到本地缓存目录并写个版本戳，
//  之后启动直接复用缓存；没打包的开发版 exe 找不到尾巴，仍然读同目录的文件。
// ============================================================
static const char   kResMagic[8]  = { 'H','S','W','R','E','S','0','1' };
static const UINT32 kPackMagic    = 0x314B5052u;        // "RPK1"

static bool                        g_resPacked = false;
static std::wstring                g_resDir;
static std::vector<unsigned char>  g_pack;              // 包体（只在启动时读一次，释放完就清掉）

struct ResEntry {
    std::wstring name;        // 相对名，子目录用 '/' 分隔，例如 "摘玉米游戏/玉米田.jpg"
    UINT64       off, size;
};
static std::vector<ResEntry> g_resList;

static std::wstring ResCacheDir()
{
    wchar_t buf[MAX_PATH * 2];
    if (SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, buf) == S_OK)
        return std::wstring(buf) + L"\\高中的一周\\res";
    return ExeDir() + L"\\_res";
}

static std::wstring Utf8ToWide(const char* p, size_t n)
{
    int m = MultiByteToWideChar(CP_UTF8, 0, p, (int)n, nullptr, 0);
    std::wstring r((size_t)(m > 0 ? m : 0), L'\0');
    if (m > 0) MultiByteToWideChar(CP_UTF8, 0, p, (int)n, &r[0], m);
    return r;
}

static void ResInit()
{
    wchar_t self[MAX_PATH * 2];
    if (!GetModuleFileNameW(nullptr, self, (DWORD)(sizeof(self) / sizeof(self[0])))) return;

    FILE* f = _wfopen(self, L"rb");
    if (!f) return;
    _fseeki64(f, 0, SEEK_END);
    LONG64 total = _ftelli64(f);
    if (total < 64) { fclose(f); return; }

    unsigned char tail[32];
    _fseeki64(f, total - 32, SEEK_SET);
    if (fread(tail, 1, 32, f) != 32) { fclose(f); return; }
    if (memcmp(tail, kResMagic, 8) != 0) { fclose(f); return; }   // 没打包 -> 走同目录

    UINT64 packOff = 0, packSize = 0;
    UINT32 count = 0;
    memcpy(&packOff,  tail + 8,  8);
    memcpy(&packSize, tail + 16, 8);
    memcpy(&count,    tail + 24, 4);
    if (packSize == 0 || packOff + packSize > (UINT64)total || count == 0 || count > 4096) {
        fclose(f); return;
    }
    g_pack.resize((size_t)packSize);
    _fseeki64(f, (LONG64)packOff, SEEK_SET);
    if (fread(g_pack.data(), 1, (size_t)packSize, f) != (size_t)packSize) {
        g_pack.clear(); fclose(f); return;
    }
    fclose(f);

    UINT32 pm = 0;
    memcpy(&pm, g_pack.data(), 4);
    if (pm != kPackMagic) { g_pack.clear(); return; }

    // 解析文件目录（目录之后紧跟数据区，entries 里的偏移是相对数据区起点的）
    size_t pos = 8;
    for (UINT32 i = 0; i < count; ++i) {
        if (pos + 4 > g_pack.size()) break;
        UINT32 nl = 0;
        memcpy(&nl, g_pack.data() + pos, 4); pos += 4;
        if (nl == 0 || pos + nl > g_pack.size()) break;
        std::wstring nm = Utf8ToWide((const char*)(g_pack.data() + pos), nl); pos += nl;
        if (pos + 16 > g_pack.size()) break;
        UINT64 off = 0, sz = 0;
        memcpy(&off, g_pack.data() + pos, 8); pos += 8;
        memcpy(&sz,  g_pack.data() + pos, 8); pos += 8;
        ResEntry e; e.name = nm; e.off = off; e.size = sz;
        g_resList.push_back(e);
    }
    if (g_resList.empty()) { g_pack.clear(); return; }
    const size_t dataOff = pos;                       // 数据区起点

    std::wstring dir = ResCacheDir();
    SHCreateDirectoryExW(nullptr, dir.c_str(), nullptr);

    // 版本戳 = 包大小 + 包前 4KB 的 FNV 校验，一致就直接用缓存，不重复释放
    UINT32 sum = 2166136261u;
    for (size_t i = 0; i < g_pack.size() && i < 4096; ++i) {
        sum ^= (UINT32)g_pack[i];
        sum *= 16777619u;
    }
    char stampBuf[96];
    _snprintf(stampBuf, sizeof(stampBuf), "%llu:%08x",
              (unsigned long long)packSize, (unsigned)sum);
    std::wstring stampPath = dir + L"\\_stamp.txt";
    bool fresh = false;
    if (FILE* sf = _wfopen(stampPath.c_str(), L"rb")) {
        char buf[96] = { 0 };
        size_t k = fread(buf, 1, 95, sf);
        fclose(sf);
        fresh = (k > 0 && strcmp(buf, stampBuf) == 0);
    }
    if (!fresh) {
        for (size_t i = 0; i < g_resList.size(); ++i) {
            const ResEntry& e = g_resList[i];
            if (dataOff + e.off + e.size > g_pack.size()) continue;
            std::wstring rel = e.name;
            for (size_t k = 0; k < rel.size(); ++k) if (rel[k] == L'/') rel[k] = L'\\';
            std::wstring out = dir + L"\\" + rel;
            size_t sp = out.find_last_of(L"\\");
            if (sp != std::wstring::npos) {
                std::wstring sd = out.substr(0, sp);
                if (sd.size() > dir.size()) SHCreateDirectoryExW(nullptr, sd.c_str(), nullptr);
            }
            if (FILE* of = _wfopen(out.c_str(), L"wb")) {
                fwrite(g_pack.data() + dataOff + e.off, 1, (size_t)e.size, of);
                fclose(of);
            }
        }
        if (FILE* sf = _wfopen(stampPath.c_str(), L"wb")) {
            fwrite(stampBuf, 1, strlen(stampBuf), sf);
            fclose(sf);
        }
    }
    g_pack.clear();            // 释放完就不占内存了
    g_resList.clear();
    g_resPacked = true;
    g_resDir    = dir;
}

static std::wstring ResDir()
{
    return g_resPacked ? g_resDir : ExeDir();
}

static Bitmap* LoadImageNear(const wchar_t* name)
{
    std::wstring path = ResDir() + L"\\" + name;
    Bitmap* img = Bitmap::FromFile(path.c_str());
    if (img && img->GetLastStatus() != Ok) { delete img; img = nullptr; }
    return img;
}

// ---------------- 第九步：把老师立绘的米色背景抠掉 ----------------
// 立绘是"人物 + 一整块米色底"的普通 PNG（没有透明通道），直接贴到教室上会带一块色块。
// 做法：从图片四边泛洪，只去掉与边框连通的米色区域（人物内部的浅色衣服不会误伤），
//       再沿边界做一圈羽化；顺便把整体压暗 28%，与 BuildGameBg 的压暗层亮度对得上，
//       最后算出人物的紧致包围盒，方便按身高摆放。
static void BuildTeacherSprite(int idx)
{
    if (idx < 0 || idx >= SUBJ_COUNT) return;
    Bitmap* src = LoadImageNear(kTeacherFile[idx]);
    if (!src) return;

    int W = (int)src->GetWidth();
    int H = (int)src->GetHeight();
    if (W <= 0 || H <= 0) { delete src; return; }

    Bitmap* bmp = new Bitmap(W, H, PixelFormat32bppARGB);
    {
        Graphics g(bmp);
        g.SetInterpolationMode(InterpolationModeNearestNeighbor);
        g.SetPixelOffsetMode(PixelOffsetModeHalf);
        g.DrawImage(src, RectF(0.0f, 0.0f, (REAL)W, (REAL)H));
    }
    delete src;

    Rect rcAll(0, 0, W, H);
    BitmapData bd;
    if (bmp->LockBits(&rcAll, ImageLockModeRead | ImageLockModeWrite,
                      PixelFormat32bppARGB, &bd) != Ok) { delete bmp; return; }

    BYTE*  base   = (BYTE*)bd.Scan0;
    size_t stride = (size_t)bd.Stride;

    // 1) 背景色 = 四边采样平均（内存里是 B,G,R,A）
    long sr = 0, sg = 0, sb = 0, n = 0;
    for (int x = 0; x < W; x += 4) {
        const BYTE* p1 = base + (size_t)x * 4;
        const BYTE* p2 = base + (size_t)(H - 1) * stride + (size_t)x * 4;
        sr += p1[2] + p2[2]; sg += p1[1] + p2[1]; sb += p1[0] + p2[0]; n += 2;
    }
    for (int y = 0; y < H; y += 4) {
        const BYTE* p1 = base + (size_t)y * stride;
        const BYTE* p2 = base + (size_t)y * stride + (size_t)(W - 1) * 4;
        sr += p1[2] + p2[2]; sg += p1[1] + p2[1]; sb += p1[0] + p2[0]; n += 2;
    }
    if (n <= 0) { bmp->UnlockBits(&bd); delete bmp; return; }
    const int bgR = (int)(sr / n), bgG = (int)(sg / n), bgB = (int)(sb / n);

    // 2) 泛洪：与边框颜色接近且连通的像素标记为背景
    const int TOL = 30;                       // 颜色容差
    std::vector<unsigned char> mask((size_t)W * (size_t)H, 0);
    std::vector<int> stack;
    for (int x = 0; x < W; ++x) { stack.push_back(x); stack.push_back((H - 1) * W + x); }
    for (int y = 0; y < H; ++y) { stack.push_back(y * W); stack.push_back(y * W + W - 1); }

    while (!stack.empty()) {
        int id = stack.back();
        stack.pop_back();
        if (id < 0 || id >= W * H) continue;
        if (mask[(size_t)id]) continue;
        int x = id % W, y = id / W;
        const BYTE* p = base + (size_t)y * stride + (size_t)x * 4;
        int dr = (int)p[2] - bgR, dg = (int)p[1] - bgG, db = (int)p[0] - bgB;
        if (dr * dr + dg * dg + db * db > TOL * TOL) continue;
        mask[(size_t)id] = 1;
        if (x > 0)     stack.push_back(id - 1);
        if (x < W - 1) stack.push_back(id + 1);
        if (y > 0)     stack.push_back(id - W);
        if (y < H - 1) stack.push_back(id + W);
    }

    // 3) 上色：背景全透明，边界做羽化；同时整体压暗（跟背景那层用同一个常量）
    const float DIM = 1.0f - (float)BG_DIM_A / 255.0f;
    const int   LO  = TOL * TOL;
    const int   HI  = 90 * 90;
    for (int y = 0; y < H; ++y) {
        BYTE* p = base + (size_t)y * stride;
        for (int x = 0; x < W; ++x, p += 4) {
            size_t id = (size_t)y * (size_t)W + (size_t)x;
            if (mask[id]) { p[3] = 0; continue; }

            bool nearBg = false;
            if (x > 0     && mask[id - 1]) nearBg = true;
            else if (x < W - 1 && mask[id + 1]) nearBg = true;
            else if (y > 0     && mask[id - W]) nearBg = true;
            else if (y < H - 1 && mask[id + W]) nearBg = true;

            if (nearBg) {
                int dr = (int)p[2] - bgR, dg = (int)p[1] - bgG, db = (int)p[0] - bgB;
                int d2 = dr * dr + dg * dg + db * db;
                if (d2 < HI) {
                    float k = (float)(d2 - LO) / (float)(HI - LO);
                    if (k < 0.0f) k = 0.0f;
                    BYTE a = (BYTE)(255.0f * k);
                    if (a < p[3]) p[3] = a;
                }
            }
            p[0] = (BYTE)((float)p[0] * DIM);
            p[1] = (BYTE)((float)p[1] * DIM);
            p[2] = (BYTE)((float)p[2] * DIM);
        }
    }

    // 4) 人物紧致包围盒
    int bx0 = W, by0 = H, bx1 = -1, by1 = -1;
    for (int y = 0; y < H; ++y) {
        const BYTE* p = base + (size_t)y * stride;
        for (int x = 0; x < W; ++x, p += 4) {
            if (p[3] > 8) {
                if (x < bx0) bx0 = x;
                if (x > bx1) bx1 = x;
                if (y < by0) by0 = y;
                if (y > by1) by1 = y;
            }
        }
    }
    bmp->UnlockBits(&bd);

    if (bx1 < 0) { delete bmp; return; }        // 一个像素都没剩下，放弃
    g_teacher[idx].img = bmp;
    g_teacher[idx].x0  = bx0;
    g_teacher[idx].y0  = by0;
    g_teacher[idx].x1  = bx1 + 1;
    g_teacher[idx].y1  = by1 + 1;
}

// ---------------- 入口 ----------------
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int)
{
    SetProcessDPIAware();

    GdiplusStartupInput gsi;
    ULONG_PTR gdipToken = 0;
    GdiplusStartup(&gdipToken, &gsi, nullptr);
    srand((unsigned)GetTickCount());          // 第二十一步：考试成绩的随机波动 / 玉米田的随机

    AchLoad();                                // 第二十二步：读本地已解锁的成就
    ResInit();                                // 第二十二步：单文件版 -> 释放内嵌资源

    g_famTitle = new FontFamily(L"Microsoft YaHei");
    if (!g_famTitle->IsAvailable()) { delete g_famTitle; g_famTitle = new FontFamily(L"SimHei"); }
    g_famBody = new FontFamily(L"KaiTi");
    if (!g_famBody->IsAvailable()) { delete g_famBody; g_famBody = new FontFamily(L"Microsoft YaHei"); }

    // 板书用手写体：优先华文行楷，没有就退回楷体
    g_famChalk = new FontFamily(L"华文行楷");
    if (!g_famChalk->IsAvailable()) { delete g_famChalk; g_famChalk = new FontFamily(L"KaiTi"); }
    if (!g_famChalk->IsAvailable()) { delete g_famChalk; g_famChalk = new FontFamily(L"Microsoft YaHei"); }
    g_famUI = new FontFamily(L"Microsoft YaHei");
    if (!g_famUI->IsAvailable()) { delete g_famUI; g_famUI = new FontFamily(L"SimHei"); }

    // 第二十三步（新增4）：讲高考台词里的表情符号用它来画；系统没有就退回普通字体
    g_famEmoji = new FontFamily(L"Segoe UI Emoji");
    if (!g_famEmoji->IsAvailable()) { delete g_famEmoji; g_famEmoji = nullptr; }

    // 第二步：菜单背景
    g_menuImg = LoadImageNear(kMenuBg);
    if (!g_menuImg) g_menuImg = LoadImageNear(L"校园门口.jpg");            // 兜底

    // 第三步：泛黄的纸张（程序生成，不用外部素材）
    BuildPaper();

    // 第四步：两位同学的 Q 版立绘
    for (int i = 0; i < HERO_COUNT; ++i) g_hero[i] = LoadImageNear(kHeroFile[i]);

    // 第五 / 六步：四张表情头像；第七步：教学楼场景背景
    for (int h = 0; h < HERO_COUNT; ++h)
        for (int f = 0; f < FACE_COUNT; ++f)
            g_avatar[h][f] = LoadImageNear(kFaceFile[h][f]);
    g_classBg = LoadImageNear(kGameBg);
    if (!g_classBg) g_classBg = LoadImageNear(L"校园门口.jpg");    // 兜底

    // 第十二步：食堂和小卖部的背景
    g_canteenSrc = LoadImageNear(L"食堂和小卖部界面.jpg");
    if (!g_canteenSrc) g_canteenSrc = LoadImageNear(L"校园门口.jpg");   // 兜底

    // 第十三步：宿舍背景
    g_dormSrc = LoadImageNear(L"宿舍界面.jpg");
    if (!g_dormSrc) g_dormSrc = LoadImageNear(L"校园门口.jpg");          // 兜底

    // 第十四步：行政楼与图书馆（书屋）
    g_bookSrc = LoadImageNear(L"书屋界面.jpg");
    if (!g_bookSrc) g_bookSrc = LoadImageNear(L"校园门口.jpg");          // 兜底

    // 第十五步：农田（玉米事件后的第二张背景有就加载，没有就先用同一张）
    g_farmSrc  = LoadImageNear(L"农田界面.jpg");
    if (!g_farmSrc) g_farmSrc = LoadImageNear(L"校园门口.jpg");          // 兜底
    g_farm2Src = LoadImageNear(L"农田结束界面.jpg");
    if (!g_farm2Src) g_farm2Src = g_farmSrc;

    // 第十六步：操场背景
    g_playSrc = LoadImageNear(L"操场界面.jpg");
    if (!g_playSrc) g_playSrc = LoadImageNear(L"校园门口.jpg");         // 兜底

    // 第十七步：篮球场背景
    g_courtSrc = LoadImageNear(L"篮球场界面.jpg");
    if (!g_courtSrc) g_courtSrc = LoadImageNear(L"校园门口.jpg");       // 兜底

    // 第二十步：事件背景
    g_event1Src  = LoadImageNear(L"课外事件活动1.jpg");
    if (!g_event1Src)  g_event1Src  = LoadImageNear(L"校园门口.jpg");
    g_event1Src2 = LoadImageNear(L"课外实践活动2.jpg");
    if (!g_event1Src2) g_event1Src2 = g_event1Src;
    g_gaokaoSrc  = LoadImageNear(L"讲高考.jpg");
    if (!g_gaokaoSrc)  g_gaokaoSrc  = LoadImageNear(L"校园门口.jpg");

    // 第二十一步：考试背景 + 摘玉米小游戏的素材（图片在「摘玉米游戏」子目录里）
    g_examSrc    = LoadImageNear(L"考试.jpg");
    if (!g_examSrc)    g_examSrc    = LoadImageNear(L"校园门口.jpg");
    CnLoad();

    // 第十九步：音乐楼背景
    g_musicSrc = LoadImageNear(L"音乐楼界面.jpg");
    if (!g_musicSrc) g_musicSrc = LoadImageNear(L"校园门口.jpg");       // 兜底

    // 第九步：四位老师的立绘（加载后顺手把米色底抠掉）
    for (int i = 0; i < SUBJ_COUNT; ++i) BuildTeacherSprite(i);

    // 第八步：地图
    g_mapImg = LoadImageNear(kMapBg);
    if (g_mapImg) {
        // 预缩放成 1195x720（逻辑画布里按高度铺满、水平居中）
        g_mapScaled = new Bitmap(1195, 720, PixelFormat32bppPARGB);
        Graphics gs(g_mapScaled);
        gs.SetInterpolationMode(InterpolationModeHighQualityBicubic);
        gs.SetPixelOffsetMode(PixelOffsetModeHighQuality);
        gs.DrawImage(g_mapImg, RectF(0.0f, 0.0f, 1195.0f, 720.0f));
    }

    WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = L"HighSchoolWeek";
    RegisterClassEx(&wc);

    // 第二十二步：加上可拉伸边框和最大化按钮，配合 WM_SIZING 锁 16:9
    DWORD style = WS_OVERLAPPED | kFrameStyle;
    RECT rc = { 0, 0, BASE_W, BASE_H };
    AdjustWindowRect(&rc, style, FALSE);
    int ww = rc.right - rc.left;
    int wh = rc.bottom - rc.top;
    int sx = (GetSystemMetrics(SM_CXSCREEN) - ww) / 2;
    int sy = (GetSystemMetrics(SM_CYSCREEN) - wh) / 2;
    if (sx < 0) sx = 0;
    if (sy < 0) sy = 0;

    g_hwnd = CreateWindowEx(0, L"HighSchoolWeek", L"高中的一周", style,
                            sx, sy, ww, wh, nullptr, nullptr, hInst, nullptr);
    if (!g_hwnd) return 0;

    // 深色标题栏（取不到就算了）
    {
        HMODULE dwm = LoadLibraryW(L"dwmapi.dll");
        if (dwm) {
            typedef HRESULT(WINAPI * PFN)(HWND, DWORD, LPCVOID, DWORD);
            PFN setAttr = (PFN)GetProcAddress(dwm, "DwmSetWindowAttribute");
            if (setAttr) {
                BOOL dark = TRUE;
                setAttr(g_hwnd, 20, &dark, sizeof(dark));
                setAttr(g_hwnd, 19, &dark, sizeof(dark));
            }
        }
    }

    g_t0 = GetTickCount64();
    SetTimer(g_hwnd, ID_TIMER, 16, nullptr);

    ShowWindow(g_hwnd, SW_SHOW);
    UpdateWindow(g_hwnd);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    delete g_bgCache;
    delete g_menuImg;
    delete g_paper;
    for (int i = 0; i < HERO_COUNT; ++i) delete g_hero[i];
    for (int h = 0; h < HERO_COUNT; ++h)
        for (int f = 0; f < FACE_COUNT; ++f) delete g_avatar[h][f];
    delete g_classBg;
    for (int i = 0; i < SUBJ_COUNT; ++i) delete g_teacher[i].img;
    delete g_gameBg;
    delete g_canteenSrc;
    delete g_canteenBg;
    delete g_dormSrc;
    delete g_dormBg;
    delete g_bookSrc;
    delete g_bookBg;
    delete g_farmSrc;
    delete g_farm2Src;
    delete g_farmBg;
    delete g_playSrc;
    delete g_playBg;
    delete g_courtSrc;
    delete g_courtBg;
    delete g_musicSrc;
    delete g_musicBg;
    delete g_event1Src;
    delete g_event1Src2;
    delete g_event1Bg;
    delete g_gaokaoSrc;
    delete g_gaokaoBg;
    delete g_examSrc;
    delete g_examBg;
    delete cnBgSrc;
    delete cnBgCache;
    delete cnIcon;
    for (int i = 0; i < cnCornMipN; ++i) delete cnCornMip[i];
    delete ptStaticLayer;
    delete ptMenuLayer;
    delete g_mapImg;
    delete g_mapScaled;
    delete g_famTitle;
    delete g_famBody;
    delete g_famUI;
    delete g_famChalk;
    delete g_famEmoji;
    GdiplusShutdown(gdipToken);
    return (int)msg.wParam;
}
