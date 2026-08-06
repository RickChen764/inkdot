#include "front_matter.h"

#include <iostream>
#include <string>

namespace {

int failures = 0;

void check(bool condition, const char* message) {
    if (condition) return;
    std::cerr << "FAIL: " << message << '\n';
    failures++;
}

const qmd::FrontMatterField* field(const qmd::FrontMatterResult& result,
                                   const std::string& key) {
    for (const auto& item : result.fields) {
        if (item.key == key) return &item;
    }
    return nullptr;
}

} // namespace

int main() {
    {
        std::string source =
            "---\n"
            "title: \"Tinta notes\"\n"
            "draft: false # publishing state\n"
            "tags: [markdown, Windows, \"C++\"]\n"
            "author:\n"
            "  name: Rick\n"
            "  links:\n"
            "    - https://example.com/profile\n"
            "    - mailto:rick@example.com\n"
            "aliases:\n"
            "  - Tiny reader\n"
            "  - Fast viewer\n"
            "contributors:\n"
            "  - name: Ada\n"
            "    contact:\n"
            "      url: https://example.com/ada#profile\n"
            "summary: >-\n"
            "  Native YAML metadata\n"
            "  without a large dependency.\n"
            "---\n"
            "# Body\n";
        auto result = qmd::parseFrontMatter(source);
        check(result.present, "front matter is detected");
        check(result.error.empty(), "common YAML parses without an error");
        check(result.bodyOffset == source.find("# Body"), "body offset points after closing delimiter");
        check(result.fields.size() == 7, "top-level mapping becomes seven fields");
        check(field(result, "title") && field(result, "title")->value == "Tinta notes",
              "quoted scalar is unquoted");
        check(field(result, "draft") && field(result, "draft")->value == "false",
              "comments are removed from plain scalars");
        check(field(result, "tags") &&
              field(result, "tags")->value == "\xE2\x80\xA2 markdown\n\xE2\x80\xA2 Windows\n\xE2\x80\xA2 C++",
              "flow sequence is rendered as a readable list");
        check(field(result, "author") &&
              field(result, "author")->value.find("name: Rick") != std::string::npos &&
              field(result, "author")->value.find("https://example.com/profile") != std::string::npos,
              "nested mappings and URL sequences are preserved");
        check(field(result, "contributors") &&
              field(result, "contributors")->value.find("name: Ada") != std::string::npos &&
              field(result, "contributors")->value.find("https://example.com/ada#profile") != std::string::npos,
              "nested values inside a sequence mapping are preserved");
        check(field(result, "summary") &&
              field(result, "summary")->value == "Native YAML metadata without a large dependency.",
              "folded block scalar is folded and stripped");
    }

    {
        auto result = qmd::parseFrontMatter(
            "\xEF\xBB\xBF---\r\n"
            "date: 2026-08-06\r\n"
            "description: |\r\n"
            "  first line\r\n"
            "  second line\r\n"
            "...\r\n"
            "text\r\n");
        check(result.present && result.error.empty(), "BOM, CRLF, and YAML end marker are supported");
        check(field(result, "description") &&
              field(result, "description")->value == "first line\nsecond line\n",
              "literal block scalar preserves newlines");
    }

    {
        auto result = qmd::parseFrontMatter("---\ntitle: test\n# no closing marker\n");
        check(!result.present, "unclosed delimiter remains ordinary Markdown");
    }

    {
        auto result = qmd::parseFrontMatter("# Heading\n---\nnot front matter\n---\n");
        check(!result.present, "a later horizontal rule is not front matter");
    }

    {
        auto result = qmd::parseFrontMatter("---\ntitle: [one, two\n---\n# Body\n");
        check(result.present && !result.error.empty(), "invalid YAML reports a non-fatal error");
        check(result.bodyOffset > 0, "invalid metadata is still stripped from the Markdown body");
    }

    {
        auto result = qmd::parseFrontMatter("---\n- one\n- two\n---\n# Body\n");
        check(result.present && !result.error.empty(), "top-level sequence is rejected for front matter");
    }

    {
        auto empty = qmd::parseFrontMatter("---\n# metadata intentionally empty\n---\n# Body\n");
        check(empty.present && empty.error.empty() && empty.fields.empty(),
              "empty and comment-only front matter is valid");

        auto quoted = qmd::parseFrontMatter("---\ntitle: \"missing end\n---\n# Body\n");
        check(quoted.present && !quoted.error.empty(),
              "unterminated quoted scalar reports an error");
    }

    if (failures != 0) {
        std::cerr << failures << " front matter test(s) failed\n";
        return 1;
    }
    std::cout << "All front matter tests passed\n";
    return 0;
}
