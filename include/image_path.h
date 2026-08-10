#ifndef INKDOT_IMAGE_PATH_H
#define INKDOT_IMAGE_PATH_H

#include <string>
#include <string_view>

// Resolves a local Markdown image source against the UTF-8 document path.
// Windows filesystem paths are constructed only from UTF-16 strings so CJK
// and other Unicode filenames never pass through the active ANSI code page.
// Returns false for invalid UTF-8, malformed percent escapes, or path errors.
bool resolveLocalImagePath(std::string_view documentPathUtf8,
                           std::string_view imageSourceUtf8,
                           std::wstring& resolvedPath) noexcept;

#endif // INKDOT_IMAGE_PATH_H
