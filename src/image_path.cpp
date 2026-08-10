#include "image_path.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <filesystem>
#include <climits>
#include <string>

namespace {

int hexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

bool percentDecode(std::string_view input, std::string& output) {
    output.clear();
    output.reserve(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] != '%') {
            output.push_back(input[i]);
            continue;
        }
        if (i + 2 >= input.size()) return false;
        int high = hexValue(input[i + 1]);
        int low = hexValue(input[i + 2]);
        if (high < 0 || low < 0) return false;
        output.push_back(static_cast<char>((high << 4) | low));
        i += 2;
    }
    return true;
}

bool utf8ToWideStrict(std::string_view input, std::wstring& output) {
    output.clear();
    if (input.empty()) return true;
    if (input.size() > static_cast<size_t>(INT_MAX)) return false;

    int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
        input.data(), static_cast<int>(input.size()), nullptr, 0);
    if (length <= 0) return false;

    output.resize(static_cast<size_t>(length));
    int converted = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
        input.data(), static_cast<int>(input.size()), output.data(), length);
    if (converted != length) {
        output.clear();
        return false;
    }
    return true;
}

} // namespace

bool resolveLocalImagePath(std::string_view documentPathUtf8,
                           std::string_view imageSourceUtf8,
                           std::wstring& resolvedPath) noexcept {
    resolvedPath.clear();
    try {
        std::string decodedSource;
        if (!percentDecode(imageSourceUtf8, decodedSource)) return false;

        std::wstring wideSource;
        if (!utf8ToWideStrict(decodedSource, wideSource) || wideSource.empty()) return false;

        std::filesystem::path sourcePath(wideSource);
        std::filesystem::path result;
        if (sourcePath.is_absolute() || documentPathUtf8.empty()) {
            result = sourcePath;
        } else {
            std::wstring wideDocumentPath;
            if (!utf8ToWideStrict(documentPathUtf8, wideDocumentPath) ||
                wideDocumentPath.empty()) {
                return false;
            }
            std::filesystem::path documentPath(wideDocumentPath);
            result = documentPath.parent_path() / sourcePath;
        }

        resolvedPath = result.lexically_normal().wstring();
        return !resolvedPath.empty();
    } catch (const std::filesystem::filesystem_error&) {
        resolvedPath.clear();
        return false;
    } catch (...) {
        // A malformed image reference must never take down the document.
        resolvedPath.clear();
        return false;
    }
}
