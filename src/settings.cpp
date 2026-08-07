#include "settings.h"
#include "document.h"
#include "localization.h"

#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>
#include <appmodel.h>
#include <fstream>
#include <string>

namespace {
std::wstring settingsPathFor(const wchar_t* appFolder, bool create) {
    wchar_t appDataPath[MAX_PATH];
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, appDataPath))) return L"";
    std::wstring path = appDataPath;
    path += L"\\";
    path += appFolder;
    if (create) CreateDirectoryW(path.c_str(), nullptr);
    path += L"\\settings.ini";
    return path;
}
}

std::wstring getSettingsPath() {
    return settingsPathFor(L"Inkdot", true);
}

void saveSettings(const Settings& settings) {
    std::wstring path = getSettingsPath();
    if (path.empty()) return;

    std::ofstream file(path);
    if (!file) return;

    file << "[Settings]\n";
    file << "themeIndex=" << settings.themeIndex << "\n";
    file << "zoomFactor=" << settings.zoomFactor << "\n";
    file << "windowX=" << settings.windowX << "\n";
    file << "windowY=" << settings.windowY << "\n";
    file << "windowWidth=" << settings.windowWidth << "\n";
    file << "windowHeight=" << settings.windowHeight << "\n";
    file << "windowMaximized=" << (settings.windowMaximized ? 1 : 0) << "\n";
    file << "hasAskedFileAssociation=" << (settings.hasAskedFileAssociation ? 1 : 0) << "\n";
    file << "editorShowPreview=" << (settings.editorShowPreview ? 1 : 0) << "\n";
    file << "editorWordWrap=" << (settings.editorWordWrap ? 1 : 0) << "\n";
}

Settings loadSettings() {
    Settings settings;
    std::wstring path = getSettingsPath();
    if (path.empty()) return settings;

    std::ifstream file(path);
    if (!file) {
        // One-time, read-only compatibility with the upstream Tinta settings.
        // The new file is written under %APPDATA%\Inkdot.
        std::wstring legacyPath = settingsPathFor(L"Tinta", false);
        file.clear();
        file.open(legacyPath);
        if (!file) return settings;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '[') continue;

        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = line.substr(0, eq);
        std::string value = line.substr(eq + 1);

        if (key == "themeIndex") {
            int idx = std::stoi(value);
            if (idx >= 0 && idx < THEME_COUNT) settings.themeIndex = idx;
        } else if (key == "zoomFactor") {
            float z = std::stof(value);
            if (z >= 0.5f && z <= 3.0f) settings.zoomFactor = z;
        } else if (key == "windowX") {
            settings.windowX = std::stoi(value);
        } else if (key == "windowY") {
            settings.windowY = std::stoi(value);
        } else if (key == "windowWidth") {
            int w = std::stoi(value);
            if (w >= 200) settings.windowWidth = w;
        } else if (key == "windowHeight") {
            int h = std::stoi(value);
            if (h >= 200) settings.windowHeight = h;
        } else if (key == "windowMaximized") {
            settings.windowMaximized = (value == "1");
        } else if (key == "hasAskedFileAssociation") {
            settings.hasAskedFileAssociation = (value == "1");
        } else if (key == "editorShowPreview") {
            settings.editorShowPreview = (value == "1");
        } else if (key == "editorWordWrap") {
            settings.editorWordWrap = (value == "1");
        }
    }
    return settings;
}

// Packaged (MSIX/Store) installs declare file associations in AppxManifest.xml;
// runtime registry writes would only land in the package's virtual registry.
static bool isRunningPackaged() {
    UINT32 length = 0;
    return GetCurrentPackageFullName(&length, nullptr) != APPMODEL_ERROR_NO_PACKAGE;
}

static bool hasRegisteredFileAssociation(std::wstring_view extension) {
    HKEY hKey;
    LONG result = RegOpenKeyExW(
        HKEY_CURRENT_USER,
        L"Software\\Inkdot\\Capabilities\\FileAssociations",
        0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS) return false;

    wchar_t value[64] = {};
    DWORD type = 0;
    DWORD size = sizeof(value);
    result = RegQueryValueExW(
        hKey, extension.data(), nullptr, &type,
        reinterpret_cast<BYTE*>(value), &size);
    RegCloseKey(hKey);
    return result == ERROR_SUCCESS &&
        type == REG_SZ &&
        wcscmp(value, L"Inkdot.MarkdownFile") == 0;
}

bool registerFileAssociation() {
    // Get the path to the current executable
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);

    HKEY hKey;
    LONG result;
    const wchar_t* progId = L"Inkdot.MarkdownFile";

    // Create ProgID entry in Classes
    result = RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Classes\\Inkdot.MarkdownFile", 0, nullptr,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
    if (result != ERROR_SUCCESS) return false;
    const wchar_t* desc = uiText(UiText::DocumentDescription);
    RegSetValueExW(hKey, nullptr, 0, REG_SZ, (BYTE*)desc, (DWORD)((wcslen(desc) + 1) * sizeof(wchar_t)));
    RegCloseKey(hKey);

    // Create DefaultIcon entry
    result = RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Classes\\Inkdot.MarkdownFile\\DefaultIcon", 0, nullptr,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
    if (result != ERROR_SUCCESS) return false;
    std::wstring iconPath = exePath;
    iconPath += L",0";
    RegSetValueExW(hKey, nullptr, 0, REG_SZ, (BYTE*)iconPath.c_str(), (DWORD)((iconPath.length() + 1) * sizeof(wchar_t)));
    RegCloseKey(hKey);

    // Create shell\open\command entry
    result = RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Classes\\Inkdot.MarkdownFile\\shell\\open\\command", 0, nullptr,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
    if (result != ERROR_SUCCESS) return false;
    std::wstring command = L"\"";
    command += exePath;
    command += L"\" \"%1\"";
    RegSetValueExW(hKey, nullptr, 0, REG_SZ, (BYTE*)command.c_str(), (DWORD)((command.length() + 1) * sizeof(wchar_t)));
    RegCloseKey(hKey);

    // Register app capabilities (required for Windows 10/11)
    result = RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Inkdot\\Capabilities", 0, nullptr,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
    if (result != ERROR_SUCCESS) return false;
    const wchar_t* appName = L"墨点 Inkdot";
    const wchar_t* appDesc = uiText(UiText::ApplicationDescription);
    RegSetValueExW(hKey, L"ApplicationName", 0, REG_SZ, (BYTE*)appName, (DWORD)((wcslen(appName) + 1) * sizeof(wchar_t)));
    RegSetValueExW(hKey, L"ApplicationDescription", 0, REG_SZ, (BYTE*)appDesc, (DWORD)((wcslen(appDesc) + 1) * sizeof(wchar_t)));
    RegCloseKey(hKey);

    // Register file associations in capabilities
    result = RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Inkdot\\Capabilities\\FileAssociations", 0, nullptr,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
    if (result != ERROR_SUCCESS) return false;
    for (std::wstring_view extension : DOCUMENT_FILE_EXTENSIONS) {
        result = RegSetValueExW(
            hKey, extension.data(), 0, REG_SZ, (BYTE*)progId,
            (DWORD)((wcslen(progId) + 1) * sizeof(wchar_t)));
        if (result != ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return false;
        }
    }
    RegCloseKey(hKey);

    // Add to RegisteredApplications
    result = RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\RegisteredApplications", 0, nullptr,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
    if (result != ERROR_SUCCESS) return false;
    const wchar_t* capPath = L"Software\\Inkdot\\Capabilities";
    RegSetValueExW(hKey, L"Inkdot", 0, REG_SZ, (BYTE*)capPath, (DWORD)((wcslen(capPath) + 1) * sizeof(wchar_t)));
    RegCloseKey(hKey);

    for (std::wstring_view extension : DOCUMENT_FILE_EXTENSIONS) {
        std::wstring keyPath = L"Software\\Classes\\";
        keyPath.append(extension);
        keyPath += L"\\OpenWithProgids";
        result = RegCreateKeyExW(
            HKEY_CURRENT_USER, keyPath.c_str(), 0, nullptr,
            REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
        if (result != ERROR_SUCCESS) return false;
        result = RegSetValueExW(hKey, progId, 0, REG_NONE, nullptr, 0);
        RegCloseKey(hKey);
        if (result != ERROR_SUCCESS) return false;
    }

    // Notify shell of the change
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);

    return true;
}

void openDefaultAppsSettings() {
    ShellExecuteW(nullptr, L"open", L"ms-settings:defaultapps", nullptr, nullptr, SW_SHOWNORMAL);
}

void askAndRegisterFileAssociation(Settings& settings) {
    if (isRunningPackaged()) return;
    if (settings.hasAskedFileAssociation) {
        if (hasRegisteredFileAssociation(L".md") &&
            !hasRegisteredFileAssociation(L".mmd") &&
            !registerFileAssociation()) {
            MessageBoxW(
                nullptr,
                uiText(UiText::FileAssociationAddedFailed),
                uiText(UiText::FileAssociationTitle),
                MB_OK | MB_ICONWARNING);
        }
        return;
    }

    int result = MessageBoxW(
        nullptr,
        uiText(UiText::FileAssociationQuestion),
        uiText(UiText::FileAssociationTitle),
        MB_YESNO | MB_ICONQUESTION
    );

    if (result == IDYES) {
        if (registerFileAssociation()) {
            MessageBoxW(nullptr,
                       uiText(UiText::RegisteredInstructions),
                       uiText(UiText::AlmostDone), MB_OK | MB_ICONINFORMATION);
            openDefaultAppsSettings();
        } else {
            MessageBoxW(nullptr, uiText(UiText::RegisterFailed),
                       uiText(UiText::ErrorTitle), MB_OK | MB_ICONWARNING);
        }
    }

    settings.hasAskedFileAssociation = true;
    saveSettings(settings);
}
