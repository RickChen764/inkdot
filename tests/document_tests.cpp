#include "document.h"

#include <iostream>

namespace {

int failures = 0;

void check(bool condition, const char* message) {
    if (condition) return;
    std::cerr << "FAIL: " << message << '\n';
    failures++;
}

} // namespace

int main() {
    check(isSupportedDocumentPath("notes.md"), ".md is supported");
    check(isSupportedDocumentPath("notes.MARKDOWN"), ".markdown is case-insensitive");
    check(isSupportedDocumentPath(L"diagram.MMD"), ".mmd is case-insensitive");
    check(isMermaidDocumentPath("diagram.mmd"), ".mmd is detected as Mermaid");
    check(!isMermaidDocumentPath("notes.md"), ".md is not detected as Mermaid");
    check(!isSupportedDocumentPath("diagram.mmdd"), "similar extensions are rejected");
    check(!isSupportedDocumentPath("notes.txt"), ".txt is not shown as a document");
    check(isSupportedDropPath(L"notes.txt"), "existing .txt drag-and-drop remains supported");

    qmd::MarkdownParser parser;
    auto mermaid = parseDocument(
        parser, "flowchart LR\nA --> B\n", "diagram.mmd");
    check(mermaid.success, "Mermaid document is created");
    check(mermaid.root && mermaid.root->children.size() == 1,
          "Mermaid document has one diagram element");
    if (mermaid.root && mermaid.root->children.size() == 1) {
        check(mermaid.root->children[0]->type == qmd::ElementType::MermaidDiagram,
              ".mmd content becomes a Mermaid diagram element");
    }

    // Obsidian/Typora inline extensions
    auto ext = parseDocument(parser,
        "before ==mark 中文== mid x^2^ and H~2~O ~~gone~~ `==not this==`\n", "notes.md");
    check(ext.success, "extension test parses");
    if (ext.success && !ext.root->children.empty()) {
        const auto& para = ext.root->children[0];
        int highlights = 0, sups = 0, subs = 0, codeIntact = 0, strikes = 0;
        for (const auto& child : para->children) {
            if (child->type == qmd::ElementType::Highlight) {
                highlights++;
                check(!child->children.empty() &&
                      child->children[0]->text == "mark \xe4\xb8\xad\xe6\x96\x87",
                      "highlight content preserved incl. CJK");
            }
            if (child->type == qmd::ElementType::Superscript) {
                sups++;
                check(!child->children.empty() && child->children[0]->text == "2",
                      "superscript content preserved");
            }
            if (child->type == qmd::ElementType::Subscript) subs++;
            if (child->type == qmd::ElementType::Strikethrough) {
                strikes++;
                check(!child->children.empty() && child->children[0]->text == "gone",
                      "strikethrough content preserved");
            }
            if (child->type == qmd::ElementType::Code) {
                codeIntact++;
                check(!child->children.empty() &&
                      child->children[0]->text == "==not this==",
                      "code spans are not transformed");
            }
        }
        check(highlights == 1, "one ==highlight== parsed");
        check(sups == 1, "one ^sup^ parsed");
        check(subs == 1, "one ~sub~ parsed");
        check(strikes == 1, "one ~~strike~~ parsed");
        check(codeIntact == 1, "inline code untouched");
    }

    // GitHub alerts
    auto alert = parseDocument(parser, "> [!NOTE]\n> Body text here.\n", "notes.md");
    check(alert.success, "alert blockquote parses");
    if (alert.success && !alert.root->children.empty()) {
        const auto& quote = alert.root->children[0];
        check(quote->type == qmd::ElementType::BlockQuote, "alert stays a blockquote");
        check(quote->alertKind == 1, "[!NOTE] detected as alert kind 1");
        check(!quote->children.empty() &&
              !quote->children[0]->children.empty() &&
              quote->children[0]->children[0]->text == "Body text here.",
              "alert marker stripped, body preserved");
    }
    auto caution = parseDocument(parser, "> [!caution]\n> Careful.\n", "notes.md");
    check(caution.success && !caution.root->children.empty() &&
          caution.root->children[0]->alertKind == 5,
          "alert markers match case-insensitively");
    auto notAlert = parseDocument(parser, "> [!NOTE] trailing words\n", "notes.md");
    check(notAlert.success && !notAlert.root->children.empty() &&
          notAlert.root->children[0]->alertKind == 0,
          "marker with trailing text on the same line stays a plain quote");

    // Inline <br> becomes a hard break (#45)
    auto br = parseDocument(parser, "line one<br>line two<BR/>line three<br />end\n", "notes.md");
    check(br.success, "inline br document parses");
    if (br.success && !br.root->children.empty()) {
        int hardBreaks = 0;
        int literalBr = 0;
        for (const auto& child : br.root->children[0]->children) {
            if (child->type == qmd::ElementType::HardBreak) hardBreaks++;
            if (child->type == qmd::ElementType::Text &&
                child->text.find("<br") != std::string::npos) literalBr++;
        }
        check(hardBreaks == 3, "all three <br> variants become hard breaks");
        check(literalBr == 0, "no literal <br> text remains");
    }

    auto markdown = parseDocument(parser, "# Heading\n", "notes.md");
    check(markdown.success, "Markdown document still parses");
    check(markdown.root && !markdown.root->children.empty() &&
          markdown.root->children[0]->type == qmd::ElementType::Heading,
          ".md content keeps Markdown parsing");

    // YAML Front Matter becomes a metadata card and does not leak into the
    // Markdown body. Source offsets remain relative to the original file so
    // editor/preview scroll sync continues to work.
    std::string yamlSource =
        "---\n"
        "title: Test document\n"
        "tags: [one, two]\n"
        "---\n"
        "# Body heading\n";
    auto yaml = parseDocument(parser, yamlSource, "notes.md");
    check(yaml.success && yaml.root && yaml.root->children.size() == 2,
          "front matter card and Markdown body are both created");
    if (yaml.success && yaml.root && yaml.root->children.size() == 2) {
        const auto& card = yaml.root->children[0];
        const auto& body = yaml.root->children[1];
        check(card->type == qmd::ElementType::FrontMatter,
              "front matter becomes its own renderable element");
        check(card->metadata.size() == 2 && card->metadata[0].first == "title",
              "parsed metadata fields are attached to the card");
        check(card->text == "title: Test document\ntags: [one, two]",
              "the card also retains original YAML formatting for display and copy");
        check(body->type == qmd::ElementType::Heading,
              "body starts with the real Markdown heading");
        check(body->sourceOffset >= yamlSource.find("Body heading"),
              "body source offsets are shifted to the original document");
    }

    auto brokenYaml = parseDocument(parser,
        "---\ntags: [one, two\n---\n# Body\n", "notes.md");
    check(brokenYaml.success && brokenYaml.root &&
          brokenYaml.root->children[0]->type == qmd::ElementType::FrontMatter &&
          !brokenYaml.root->children[0]->error.empty(),
          "invalid YAML is a visible card error, not a document load failure");

    if (failures != 0) {
        std::cerr << failures << " test(s) failed\n";
        return 1;
    }
    std::cout << "All document tests passed\n";
    return 0;
}
