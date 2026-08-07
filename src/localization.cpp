#include "localization.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <array>
#include <string_view>

namespace {

UiLanguage g_language = UiLanguage::English;

struct Translation {
    const wchar_t* english;
    const wchar_t* chinese;
};

constexpr Translation TRANSLATIONS[] = {
    {L"Search...", L"搜索..."},
    {L"No matches", L"无匹配结果"},
    {L"%d of %zu", L"第 %d 项，共 %zu 项"},
    {L"Contents", L"目录"},
    {L"No headings", L"没有标题"},
    {L"Choose Theme", L"选择主题"},
    {L"The quick brown fox", L"敏捷的棕狐越过懒狗"},
    {L"hyperlink", L"超链接"},
    {L"LIGHT THEMES", L"浅色主题"},
    {L"DARK THEMES", L"深色主题"},
    {L"Keyboard Shortcuts", L"键盘快捷键"},
    {L"Scroll down", L"向下滚动"},
    {L"Scroll up", L"向上滚动"},
    {L"Page down", L"向下翻页"},
    {L"Page up", L"向上翻页"},
    {L"Jump to start / end", L"跳转到开头 / 结尾"},
    {L"Zoom in / out", L"放大 / 缩小"},
    {L"Search", L"搜索"},
    {L"Next search match", L"下一个搜索结果"},
    {L"Toggle folder browser", L"显示 / 隐藏文件浏览器"},
    {L"Toggle table of contents", L"显示 / 隐藏目录"},
    {L"Theme chooser", L"选择主题"},
    {L"Toggle stats", L"显示 / 隐藏性能统计"},
    {L"This help", L"显示此帮助"},
    {L"Enter edit mode", L"进入编辑模式"},
    {L"Save (in edit mode)", L"保存（编辑模式）"},
    {L"Show / hide preview pane", L"显示 / 隐藏预览窗格"},
    {L"Toggle word wrap", L"切换自动换行"},
    {L"Exit edit mode", L"退出编辑模式"},
    {L"Select all text", L"选择全部文本"},
    {L"Copy selection", L"复制所选内容"},
    {L"Close overlay / Quit", L"关闭浮层 / 退出"},
    {L"Quit", L"退出"},
    {L"NAVIGATION", L"导航"},
    {L"VIEW", L"视图"},
    {L"EDITING", L"编辑"},
    {L"GENERAL", L"常规"},
    {L"Press ESC or ? to close", L"按 ESC 或 ? 关闭"},
    {L"Copy", L"复制"},
    {L"Copied!", L"已复制！"},
    {L"Parse: %zu us | Layout: %zu us | Draw calls: %zu\nStartup: %.1fms (Win: %.1f | D2D: %.1f | DWrite: %.1f | File: %.1f)",
     L"解析：%zu 微秒 | 布局：%zu 微秒 | 绘制调用：%zu\n启动：%.1f 毫秒（窗口：%.1f | D2D：%.1f | DWrite：%.1f | 文件：%.1f）"},
    {L"No file loaded", L"未加载文件"},
    {L"Press ESC twice to exit edit mode", L"连续按两次 ESC 退出编辑模式"},
    {L"Unsaved changes! Y = save & exit, N = discard, ESC = cancel",
     L"有未保存的更改！Y = 保存并退出，N = 放弃更改，ESC = 取消"},
    {L"Saved!", L"已保存！"},
    {L"Save failed — file may be locked or read-only", L"保存失败——文件可能已锁定或为只读"},
    {L"Exit cancelled", L"已取消退出"},
    {L"Press ESC again to exit edit mode", L"再次按 ESC 退出编辑模式"},
    {L"Preview shown (Ctrl+P to hide)", L"已显示预览（按 Ctrl+P 隐藏）"},
    {L"Preview hidden (Ctrl+P to show)", L"已隐藏预览（按 Ctrl+P 显示）"},
    {L"Word wrap on (Ctrl+W to turn off)", L"已开启自动换行（按 Ctrl+W 关闭）"},
    {L"Word wrap off (Ctrl+W to turn on)", L"已关闭自动换行（按 Ctrl+W 开启）"},
    {L"Note", L"注意"},
    {L"Tip", L"提示"},
    {L"Important", L"重要"},
    {L"Warning", L"警告"},
    {L"Caution", L"危险"},
    {L"image", L"图片"},
    {L"YAML error: ", L"YAML 解析错误："},
    {L"No metadata fields", L"没有元数据字段"},
    {L"Inkdot - File Association", L"墨点 - 文件关联"},
    {L"Would you like to set Inkdot as the default viewer for Markdown and Mermaid files?\n\nWindows will open Settings where you can select Inkdot.",
     L"是否将墨点设为 Markdown 和 Mermaid 文件的默认阅读器？\n\nWindows 将打开“设置”，你可以在其中选择墨点。"},
    {L"Failed to add the .mmd file association. Run inkdot.exe /register to try again.",
     L"添加 .mmd 文件关联失败。请运行 inkdot.exe /register 重试。"},
    {L"Inkdot has been registered.\n\nIn the Settings window that opens:\n1. Search for '.md' or '.mmd'\n2. Click on the current default app\n3. Select 'Inkdot' from the list",
     L"墨点已注册。\n\n请在随后打开的“设置”窗口中：\n1. 搜索“.md”或“.mmd”\n2. 点击当前的默认应用\n3. 从列表中选择“墨点 Inkdot”"},
    {L"Almost done!", L"即将完成！"},
    {L"Failed to register file association. Try running as administrator.",
     L"注册文件关联失败。请尝试以管理员身份运行。"},
    {L"Error", L"错误"},
    {L"Failed to initialize Direct2D", L"Direct2D 初始化失败"},
    {L"Failed to create render target", L"创建渲染目标失败"},
    {L"Inkdot Document", L"墨点文档"},
    {L"A fast, lightweight Markdown and Mermaid reader", L"快速轻量的 Markdown 与 Mermaid 阅读器"},
};

static_assert(std::size(TRANSLATIONS) == static_cast<size_t>(UiText::ApplicationDescription) + 1,
              "translation table must match UiText");

constexpr const wchar_t* THEME_NAMES[][2] = {
    {L"Paper", L"纸张"},
    {L"Sakura", L"樱花"},
    {L"Arctic", L"北极"},
    {L"Meadow", L"草甸"},
    {L"Dusk", L"黄昏"},
    {L"Midnight", L"午夜"},
    {L"Dracula", L"德古拉"},
    {L"Forest", L"森林"},
    {L"Ember", L"余烬"},
    {L"Abyss", L"深渊"},
};

std::wstring utf8ToWide(const std::string& text) {
    if (text.empty()) return {};
    int length = MultiByteToWideChar(CP_UTF8, 0, text.data(),
                                     static_cast<int>(text.size()), nullptr, 0);
    std::wstring result(static_cast<size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                        result.data(), length);
    return result;
}

const wchar_t* chineseYamlError(std::string_view message) {
    struct ErrorTranslation { std::string_view english; const wchar_t* chinese; };
    static constexpr ErrorTranslation errors[] = {
        {"expected ':' in flow mapping", L"行内映射中缺少冒号"},
        {"unbalanced flow collection", L"行内集合的括号不匹配"},
        {"unterminated quoted scalar", L"带引号的值没有正确结束"},
        {"top-level content must not be indented", L"顶层内容不能缩进"},
        {"unexpected indentation", L"存在意外缩进"},
        {"inconsistent indentation", L"缩进不一致"},
        {"expected 'key: value'", L"应使用“键: 值”格式"},
        {"mapping key is empty", L"映射键不能为空"},
        {"unexpected indentation after sequence item", L"列表项后存在意外缩进"},
        {"tabs cannot be used for indentation", L"不能使用制表符缩进"},
        {"top-level Front Matter must be a YAML mapping", L"Front Matter 顶层必须是 YAML 映射"},
    };
    for (const auto& item : errors) {
        if (message == item.english) return item.chinese;
    }
    return nullptr;
}

} // namespace

UiLanguage uiLanguageForWindowsLangId(unsigned short languageId) {
    return PRIMARYLANGID(languageId) == LANG_CHINESE
        ? UiLanguage::SimplifiedChinese
        : UiLanguage::English;
}

void initializeLocalization() {
    g_language = uiLanguageForWindowsLangId(GetUserDefaultUILanguage());
}

UiLanguage currentUiLanguage() {
    return g_language;
}

bool isChineseUi() {
    return g_language == UiLanguage::SimplifiedChinese;
}

const wchar_t* uiText(UiText id) {
    size_t index = static_cast<size_t>(id);
    if (index >= std::size(TRANSLATIONS)) return L"";
    return isChineseUi() ? TRANSLATIONS[index].chinese : TRANSLATIONS[index].english;
}

const wchar_t* uiLocaleName() {
    return isChineseUi() ? L"zh-cn" : L"en-us";
}

const wchar_t* localizedThemeName(int themeIndex) {
    if (themeIndex < 0 || themeIndex >= static_cast<int>(std::size(THEME_NAMES))) return L"";
    return THEME_NAMES[themeIndex][isChineseUi() ? 1 : 0];
}

const char* localizedSampleMarkdown() {
    static constexpr const char* english = R"(# Welcome to Inkdot

**Inkdot** is a fast, lightweight Markdown and Mermaid viewer for Windows.

## Getting Started

- **Drag & drop** a `.md` or `.mmd` file onto this window
- Press **B** to browse and open files from a folder
- Press **?** for all available keyboard shortcuts

## Features

- Native YAML Front Matter and Mermaid rendering
- Edit mode with live preview — press **:**
- Search — press **F**
- Table of contents — press **Tab**
- 10 themes — press **T** to choose
)";
    static constexpr const char* chinese = u8R"(# 欢迎使用墨点

**墨点 Inkdot** 是一款适用于 Windows 的快速、轻量 Markdown 与 Mermaid 阅读器。

## 快速开始

- 将 `.md` 或 `.mmd` 文件拖放到此窗口
- 按 **B** 浏览文件夹并打开文件
- 按 **?** 查看全部键盘快捷键

## 主要功能

- 原生渲染 YAML Front Matter 和 Mermaid 图表
- 带实时预览的编辑模式——按 **:** 进入
- 搜索——按 **F**
- 文章目录——按 **Tab**
- 10 款主题——按 **T** 选择
)";
    return isChineseUi() ? chinese : english;
}

std::wstring localizeFrontMatterError(const std::string& error) {
    if (!isChineseUi()) return utf8ToWide(error);
    constexpr std::string_view linePrefix = "line ";
    if (error.rfind(linePrefix.data(), 0) == 0) {
        size_t colon = error.find(": ", linePrefix.size());
        if (colon != std::string::npos) {
            std::string line = error.substr(linePrefix.size(), colon - linePrefix.size());
            std::string_view message(error.data() + colon + 2, error.size() - colon - 2);
            if (const wchar_t* translated = chineseYamlError(message)) {
                return L"第 " + utf8ToWide(line) + L" 行：" + translated;
            }
        }
    }
    if (const wchar_t* translated = chineseYamlError(error)) return translated;
    return utf8ToWide(error);
}
