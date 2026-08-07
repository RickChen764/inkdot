#ifndef INKDOT_LOCALIZATION_H
#define INKDOT_LOCALIZATION_H

#include <string>

enum class UiLanguage {
    English,
    SimplifiedChinese,
};

enum class UiText {
    SearchPlaceholder,
    NoMatches,
    MatchCountFormat,
    Contents,
    NoHeadings,
    ChooseTheme,
    ThemePreviewSample,
    Hyperlink,
    LightThemes,
    DarkThemes,
    KeyboardShortcuts,
    ScrollDown,
    ScrollUp,
    PageDown,
    PageUp,
    JumpStartEnd,
    ZoomInOut,
    Search,
    NextSearchMatch,
    ToggleFolderBrowser,
    ToggleContents,
    ThemeChooser,
    ToggleStats,
    ThisHelp,
    EnterEditMode,
    SaveEditMode,
    TogglePreview,
    ToggleWordWrap,
    ExitEditMode,
    Undo,
    Redo,
    CutSelection,
    Paste,
    SelectAllText,
    CopySelection,
    CloseOverlayQuit,
    Quit,
    NavigationSection,
    ViewSection,
    EditingSection,
    GeneralSection,
    CloseHelpHint,
    Copy,
    Copied,
    StatsFormat,
    NoFileLoaded,
    ExitEditHint,
    UnsavedChanges,
    Saved,
    SaveFailed,
    ExitCancelled,
    ExitEditAgain,
    PreviewShown,
    PreviewHidden,
    WordWrapOn,
    WordWrapOff,
    AlertNote,
    AlertTip,
    AlertImportant,
    AlertWarning,
    AlertCaution,
    ImagePlaceholder,
    YamlErrorPrefix,
    EmptyFrontMatter,
    FileAssociationTitle,
    FileAssociationQuestion,
    FileAssociationAddedFailed,
    RegisteredInstructions,
    AlmostDone,
    RegisterFailed,
    ErrorTitle,
    Direct2DInitFailed,
    RenderTargetFailed,
    DocumentDescription,
    ApplicationDescription,
};

// Uses the Windows user UI language. Chinese Windows installations receive
// Simplified Chinese; every other language falls back to English.
void initializeLocalization();
UiLanguage currentUiLanguage();
UiLanguage uiLanguageForWindowsLangId(unsigned short languageId);
bool isChineseUi();
const wchar_t* uiText(UiText id);
const wchar_t* uiLocaleName();
const wchar_t* localizedThemeName(int themeIndex);
const char* localizedSampleMarkdown();
std::wstring localizeFrontMatterError(const std::string& error);

#endif // INKDOT_LOCALIZATION_H
