#include "image_path.h"

#include <iostream>
#include <string>

namespace {
int failures = 0;

void check(bool condition, const char* message) {
    if (condition) return;
    std::cerr << "FAIL: " << message << '\n';
    ++failures;
}

bool endsWith(const std::wstring& value, const std::wstring& suffix) {
    return value.size() >= suffix.size() &&
        value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}
} // namespace

int main() {
    std::wstring path;

    check(resolveLocalImagePath(
              u8R"(C:\书籍\说明\文档.md)", u8"./图片/概念图.png", path) &&
              endsWith(path, LR"(书籍\说明\图片\概念图.png)"),
          "direct UTF-8 CJK document and image paths resolve as UTF-16");

    check(resolveLocalImagePath(
              u8R"(C:\docs\readme.md)",
              "./%E4%B8%8D%E5%AD%98%E5%9C%A8.png", path) &&
              endsWith(path, LR"(docs\不存在.png)"),
          "percent-encoded UTF-8 image paths are decoded");

    check(resolveLocalImagePath(
              u8R"(C:\文档\章节\readme.md)", u8R"(..\资源\封面.png)", path) &&
              endsWith(path, LR"(文档\资源\封面.png)"),
          "parent traversal remains relative to the document directory");

    check(resolveLocalImagePath({}, u8R"(D:\图片\封面.png)", path) &&
              path == LR"(D:\图片\封面.png)",
          "absolute Unicode paths remain absolute");

    std::string invalidUtf8("./bad-\xFF.png", 11);
    check(!resolveLocalImagePath(R"(C:\docs\readme.md)", invalidUtf8, path) && path.empty(),
          "invalid UTF-8 fails without throwing");

    check(!resolveLocalImagePath(R"(C:\docs\readme.md)", "./bad-%ZZ.png", path) && path.empty(),
          "malformed percent escapes fail without throwing");

    if (failures) {
        std::cerr << failures << " image path test(s) failed\n";
        return 1;
    }
    std::cout << "All image path tests passed\n";
    return 0;
}
