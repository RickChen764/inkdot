#include "localization.h"

#include <iostream>
#include <string>

namespace {
int failures = 0;
void check(bool condition, const char* message) {
    if (condition) return;
    std::cerr << "FAIL: " << message << '\n';
    failures++;
}
}

int main() {
    check(uiLanguageForWindowsLangId(0x0804) == UiLanguage::SimplifiedChinese,
          "Simplified Chinese LANGID selects Chinese UI");
    check(uiLanguageForWindowsLangId(0x0404) == UiLanguage::SimplifiedChinese,
          "Traditional Chinese LANGID also receives the Chinese UI");
    check(uiLanguageForWindowsLangId(0x0409) == UiLanguage::English,
          "English LANGID selects English UI");
    check(uiLanguageForWindowsLangId(0x0411) == UiLanguage::English,
          "unsupported languages fall back to English");
    initializeLocalization();
    check(std::wstring(uiText(UiText::SearchPlaceholder)).size() > 0,
          "the detected language returns a translation");
    check(std::wstring(localizedThemeName(0)).size() > 0,
          "theme names are localized");
    check(std::wstring(uiLocaleName()).size() > 0,
          "DirectWrite receives a locale matching the selected UI");

    if (failures) {
        std::cerr << failures << " localization test(s) failed\n";
        return 1;
    }
    std::cout << "All localization tests passed\n";
    return 0;
}
