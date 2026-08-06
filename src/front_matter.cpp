#include "front_matter.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string_view>
#include <utility>

namespace qmd {
namespace {

struct SourceLine {
    std::string text;
    size_t offset = 0;
    size_t nextOffset = 0;
};

enum class NodeKind { Scalar, Mapping, Sequence };

struct Node {
    NodeKind kind = NodeKind::Scalar;
    std::string scalar;
    std::vector<std::pair<std::string, Node>> mapping;
    std::vector<Node> sequence;
};

struct ParsedLine {
    int indent = 0;
    size_t number = 0;
    std::string content;
};

std::string trim(std::string_view text) {
    size_t first = 0;
    while (first < text.size() &&
           std::isspace(static_cast<unsigned char>(text[first]))) first++;
    size_t last = text.size();
    while (last > first &&
           std::isspace(static_cast<unsigned char>(text[last - 1]))) last--;
    return std::string(text.substr(first, last - first));
}

std::vector<SourceLine> splitSourceLines(const std::string& source) {
    std::vector<SourceLine> lines;
    size_t offset = 0;
    while (offset < source.size()) {
        size_t end = source.find('\n', offset);
        size_t next = end == std::string::npos ? source.size() : end + 1;
        size_t textEnd = end == std::string::npos ? source.size() : end;
        if (textEnd > offset && source[textEnd - 1] == '\r') textEnd--;
        lines.push_back({source.substr(offset, textEnd - offset), offset, next});
        offset = next;
    }
    if (source.empty()) lines.push_back({});
    return lines;
}

size_t findOutside(std::string_view text, char wanted) {
    char quote = 0;
    bool escaped = false;
    int square = 0;
    int curly = 0;
    for (size_t i = 0; i < text.size(); i++) {
        char c = text[i];
        if (quote) {
            if (quote == '"' && escaped) {
                escaped = false;
            } else if (quote == '"' && c == '\\') {
                escaped = true;
            } else if (c == quote) {
                // YAML single quotes escape themselves by doubling.
                if (quote == '\'' && i + 1 < text.size() && text[i + 1] == '\'') i++;
                else quote = 0;
            }
            continue;
        }
        if (c == '\'' || c == '"') quote = c;
        else if (c == '[') square++;
        else if (c == ']') square = std::max(0, square - 1);
        else if (c == '{') curly++;
        else if (c == '}') curly = std::max(0, curly - 1);
        else if (c == wanted && square == 0 && curly == 0) return i;
    }
    return std::string_view::npos;
}

size_t findMappingColon(std::string_view text) {
    size_t start = 0;
    while (start < text.size()) {
        size_t relative = findOutside(text.substr(start), ':');
        if (relative == std::string_view::npos) return relative;
        size_t colon = start + relative;
        if (colon + 1 == text.size() ||
            std::isspace(static_cast<unsigned char>(text[colon + 1]))) {
            return colon;
        }
        start = colon + 1;
    }
    return std::string_view::npos;
}

std::string stripComment(std::string_view text) {
    char quote = 0;
    bool escaped = false;
    int square = 0;
    int curly = 0;
    for (size_t i = 0; i < text.size(); i++) {
        char c = text[i];
        if (quote) {
            if (quote == '"' && escaped) escaped = false;
            else if (quote == '"' && c == '\\') escaped = true;
            else if (c == quote) {
                if (quote == '\'' && i + 1 < text.size() && text[i + 1] == '\'') i++;
                else quote = 0;
            }
            continue;
        }
        if (c == '\'' || c == '"') quote = c;
        else if (c == '[') square++;
        else if (c == ']') square = std::max(0, square - 1);
        else if (c == '{') curly++;
        else if (c == '}') curly = std::max(0, curly - 1);
        else if (c == '#' && square == 0 && curly == 0 &&
                 (i == 0 || std::isspace(static_cast<unsigned char>(text[i - 1])))) {
            return trim(text.substr(0, i));
        }
    }
    return trim(text);
}

std::string unquote(std::string text) {
    if (text.size() < 2) return text;
    char quote = text.front();
    if ((quote != '\'' && quote != '"') || text.back() != quote) return text;
    std::string out;
    out.reserve(text.size() - 2);
    for (size_t i = 1; i + 1 < text.size(); i++) {
        char c = text[i];
        if (quote == '\'' && c == '\'' && i + 2 < text.size() && text[i + 1] == '\'') {
            out.push_back('\'');
            i++;
            continue;
        }
        if (quote == '"' && c == '\\' && i + 2 < text.size()) {
            char next = text[++i];
            switch (next) {
                case 'n': out.push_back('\n'); break;
                case 'r': out.push_back('\r'); break;
                case 't': out.push_back('\t'); break;
                case '"': out.push_back('"'); break;
                case '\\': out.push_back('\\'); break;
                default: out.push_back('\\'); out.push_back(next); break;
            }
            continue;
        }
        out.push_back(c);
    }
    return out;
}

std::vector<std::string> splitFlow(std::string_view text) {
    std::vector<std::string> parts;
    size_t start = 0;
    while (start <= text.size()) {
        size_t comma = findOutside(text.substr(start), ',');
        if (comma == std::string_view::npos) {
            parts.push_back(trim(text.substr(start)));
            break;
        }
        parts.push_back(trim(text.substr(start, comma)));
        start += comma + 1;
    }
    return parts;
}

Node parseInline(std::string value, std::string& error, size_t lineNumber);

Node parseFlowSequence(std::string_view inner, std::string& error, size_t lineNumber) {
    Node node;
    node.kind = NodeKind::Sequence;
    for (const auto& part : splitFlow(inner)) {
        if (!part.empty()) node.sequence.push_back(parseInline(part, error, lineNumber));
    }
    return node;
}

Node parseFlowMapping(std::string_view inner, std::string& error, size_t lineNumber) {
    Node node;
    node.kind = NodeKind::Mapping;
    for (const auto& part : splitFlow(inner)) {
        if (part.empty()) continue;
        size_t colon = findMappingColon(part);
        if (colon == std::string_view::npos) {
            error = "line " + std::to_string(lineNumber) + ": expected ':' in flow mapping";
            return node;
        }
        std::string key = unquote(trim(std::string_view(part).substr(0, colon)));
        node.mapping.push_back({key, parseInline(trim(std::string_view(part).substr(colon + 1)),
                                                 error, lineNumber)});
        if (!error.empty()) return node;
    }
    return node;
}

Node parseInline(std::string value, std::string& error, size_t lineNumber) {
    Node node;
    value = stripComment(value);
    if (!value.empty() && (value.front() == '\'' || value.front() == '"') &&
        (value.size() < 2 || value.back() != value.front())) {
        error = "line " + std::to_string(lineNumber) + ": unterminated quoted scalar";
        return node;
    }
    if (value.size() >= 2 && value.front() == '[' && value.back() == ']') {
        return parseFlowSequence(std::string_view(value).substr(1, value.size() - 2),
                                 error, lineNumber);
    }
    if (value.size() >= 2 && value.front() == '{' && value.back() == '}') {
        return parseFlowMapping(std::string_view(value).substr(1, value.size() - 2),
                                error, lineNumber);
    }
    if ((!value.empty() && (value.front() == '[' || value.front() == '{')) ||
        (!value.empty() && (value.back() == ']' || value.back() == '}'))) {
        error = "line " + std::to_string(lineNumber) + ": unbalanced flow collection";
        return node;
    }
    node.scalar = unquote(value);
    return node;
}

bool isSequenceLine(std::string_view content) {
    return content == "-" ||
        (content.size() > 1 && content[0] == '-' &&
         std::isspace(static_cast<unsigned char>(content[1])));
}

class Parser {
public:
    explicit Parser(std::vector<ParsedLine> lines) : lines_(std::move(lines)) {}

    Node parse(std::string& error) {
        skipIgnorable();
        if (index_ >= lines_.size()) {
            Node root;
            root.kind = NodeKind::Mapping;
            return root;
        }
        if (lines_[index_].indent != 0) {
            error = "line " + std::to_string(lines_[index_].number) +
                ": top-level content must not be indented";
            return {};
        }
        Node root = parseBlock(0, error);
        skipIgnorable();
        if (error.empty() && index_ != lines_.size()) {
            error = "line " + std::to_string(lines_[index_].number) + ": unexpected indentation";
        }
        return root;
    }

private:
    bool isIgnorable(const ParsedLine& line) const {
        std::string value = trim(line.content);
        return value.empty() || value.front() == '#';
    }

    void skipIgnorable() {
        while (index_ < lines_.size() && isIgnorable(lines_[index_])) index_++;
    }

    Node parseBlock(int indent, std::string& error) {
        skipIgnorable();
        if (index_ >= lines_.size()) return {};
        if (lines_[index_].indent != indent) {
            error = "line " + std::to_string(lines_[index_].number) + ": inconsistent indentation";
            return {};
        }
        return isSequenceLine(lines_[index_].content)
            ? parseSequence(indent, error)
            : parseMapping(indent, error);
    }

    Node parseMapping(int indent, std::string& error) {
        Node node;
        node.kind = NodeKind::Mapping;
        while (index_ < lines_.size()) {
            skipIgnorable();
            if (index_ >= lines_.size() || lines_[index_].indent != indent ||
                isSequenceLine(lines_[index_].content)) break;
            ParsedLine line = lines_[index_++];
            parseMappingEntry(node, line.content, indent, line.number, error);
            if (!error.empty()) break;
        }
        return node;
    }

    void parseMappingEntry(Node& node, std::string_view content, int indent,
                           size_t lineNumber, std::string& error) {
        size_t colon = findMappingColon(content);
        if (colon == std::string_view::npos) {
            error = "line " + std::to_string(lineNumber) + ": expected 'key: value'";
            return;
        }
        std::string key = unquote(trim(content.substr(0, colon)));
        if (key.empty()) {
            error = "line " + std::to_string(lineNumber) + ": mapping key is empty";
            return;
        }
        std::string value = stripComment(content.substr(colon + 1));
        Node child;
        if (value == "|" || value == ">" || value == "|-" || value == ">-" ||
            value == "|+" || value == ">+") {
            child.scalar = parseBlockScalar(indent, value[0] == '>', value.size() > 1 ? value[1] : 0);
        } else if (!value.empty()) {
            child = parseInline(value, error, lineNumber);
        } else {
            skipIgnorable();
            if (index_ < lines_.size() && lines_[index_].indent > indent) {
                child = parseBlock(lines_[index_].indent, error);
            }
        }
        node.mapping.push_back({std::move(key), std::move(child)});
    }

    Node parseSequence(int indent, std::string& error) {
        Node node;
        node.kind = NodeKind::Sequence;
        while (index_ < lines_.size()) {
            skipIgnorable();
            if (index_ >= lines_.size() || lines_[index_].indent != indent ||
                !isSequenceLine(lines_[index_].content)) break;
            ParsedLine line = lines_[index_++];
            std::string value = trim(std::string_view(line.content).substr(1));
            if (value.empty()) {
                skipIgnorable();
                if (index_ < lines_.size() && lines_[index_].indent > indent) {
                    node.sequence.push_back(parseBlock(lines_[index_].indent, error));
                } else {
                    node.sequence.push_back({});
                }
            } else {
                size_t colon = findMappingColon(value);
                if (colon != std::string_view::npos) {
                    Node item;
                    item.kind = NodeKind::Mapping;
                    // A mapping that starts after "- " has a virtual indent
                    // two columns past the sequence. Deeper following lines
                    // can then correctly become children of its first key.
                    int itemIndent = indent + 2;
                    parseMappingEntry(item, value, itemIndent, line.number, error);
                    while (error.empty() && index_ < lines_.size()) {
                        skipIgnorable();
                        if (index_ >= lines_.size() || lines_[index_].indent != itemIndent ||
                            isSequenceLine(lines_[index_].content)) break;
                        ParsedLine continuation = lines_[index_++];
                        parseMappingEntry(item, continuation.content, itemIndent,
                                          continuation.number, error);
                    }
                    node.sequence.push_back(std::move(item));
                } else {
                    node.sequence.push_back(parseInline(value, error, line.number));
                }
            }
            if (!error.empty()) break;
            skipIgnorable();
            if (index_ < lines_.size() && lines_[index_].indent > indent) {
                error = "line " + std::to_string(lines_[index_].number) +
                    ": unexpected indentation after sequence item";
                break;
            }
        }
        return node;
    }

    std::string parseBlockScalar(int parentIndent, bool folded, char chomping) {
        size_t firstContent = index_;
        while (firstContent < lines_.size() && trim(lines_[firstContent].content).empty()) {
            firstContent++;
        }
        if (firstContent >= lines_.size() || lines_[firstContent].indent <= parentIndent) return {};
        int contentIndent = lines_[firstContent].indent;
        std::string out;
        bool previousBlank = false;
        bool wroteLine = false;
        while (index_ < lines_.size()) {
            bool blank = trim(lines_[index_].content).empty();
            if (!blank && lines_[index_].indent <= parentIndent) break;
            const ParsedLine& line = lines_[index_++];
            std::string text = line.content;
            if (line.indent > contentIndent) {
                text.insert(0, static_cast<size_t>(line.indent - contentIndent), ' ');
            }
            if (folded && wroteLine && !previousBlank && !blank) out.push_back(' ');
            else if (wroteLine) out.push_back('\n');
            out += text;
            previousBlank = blank;
            wroteLine = true;
        }
        if (chomping != '-' && !out.empty()) out.push_back('\n');
        return out;
    }

    std::vector<ParsedLine> lines_;
    size_t index_ = 0;
};

void appendIndented(std::string& out, std::string_view text, int spaces) {
    size_t start = 0;
    while (start <= text.size()) {
        size_t end = text.find('\n', start);
        if (end == std::string_view::npos) end = text.size();
        out.append(static_cast<size_t>(spaces), ' ');
        out.append(text.substr(start, end - start));
        if (end == text.size()) break;
        out.push_back('\n');
        start = end + 1;
    }
}

std::string displayNode(const Node& node, int depth = 0) {
    if (node.kind == NodeKind::Scalar) return node.scalar;
    std::string out;
    if (node.kind == NodeKind::Sequence) {
        for (size_t i = 0; i < node.sequence.size(); i++) {
            if (i) out.push_back('\n');
            std::string value = displayNode(node.sequence[i], depth + 1);
            out += "\xE2\x80\xA2 "; // bullet
            if (node.sequence[i].kind == NodeKind::Scalar) out += value;
            else {
                out.push_back('\n');
                appendIndented(out, value, 2);
            }
        }
        return out;
    }
    for (size_t i = 0; i < node.mapping.size(); i++) {
        if (i) out.push_back('\n');
        out += node.mapping[i].first;
        out += ':';
        std::string value = displayNode(node.mapping[i].second, depth + 1);
        if (!value.empty()) {
            if (node.mapping[i].second.kind == NodeKind::Scalar) {
                out.push_back(' ');
                out += value;
            } else {
                out.push_back('\n');
                appendIndented(out, value, 2);
            }
        }
    }
    return out;
}

} // namespace

FrontMatterResult parseFrontMatter(const std::string& markdown) {
    FrontMatterResult result;
    auto sourceLines = splitSourceLines(markdown);
    if (sourceLines.empty()) return result;

    std::string first = sourceLines[0].text;
    if (first.size() >= 3 &&
        static_cast<unsigned char>(first[0]) == 0xEF &&
        static_cast<unsigned char>(first[1]) == 0xBB &&
        static_cast<unsigned char>(first[2]) == 0xBF) {
        first.erase(0, 3);
    }
    if (trim(first) != "---") return result;

    size_t closeLine = sourceLines.size();
    for (size_t i = 1; i < sourceLines.size(); i++) {
        std::string marker = trim(sourceLines[i].text);
        if (marker == "---" || marker == "...") {
            closeLine = i;
            break;
        }
    }
    // An unmatched opening rule remains ordinary Markdown.
    if (closeLine == sourceLines.size()) return result;

    result.present = true;
    result.bodyOffset = sourceLines[closeLine].nextOffset;
    size_t rawStart = sourceLines[0].nextOffset;
    size_t rawEnd = sourceLines[closeLine].offset;
    result.raw = markdown.substr(rawStart, rawEnd - rawStart);
    while (!result.raw.empty() && (result.raw.back() == '\n' || result.raw.back() == '\r')) {
        result.raw.pop_back();
    }

    std::vector<ParsedLine> parsedLines;
    for (size_t i = 1; i < closeLine; i++) {
        const std::string& raw = sourceLines[i].text;
        int indent = 0;
        while (indent < static_cast<int>(raw.size()) && raw[static_cast<size_t>(indent)] == ' ') indent++;
        if (indent < static_cast<int>(raw.size()) && raw[static_cast<size_t>(indent)] == '\t') {
            result.error = "line " + std::to_string(i + 1) + ": tabs cannot be used for indentation";
            break;
        }
        std::string content = raw.substr(static_cast<size_t>(indent));
        parsedLines.push_back({indent, i + 1, std::move(content)});
    }

    Node root;
    if (result.error.empty()) {
        Parser parser(std::move(parsedLines));
        root = parser.parse(result.error);
    }
    if (!result.error.empty()) return result;
    if (root.kind != NodeKind::Mapping) {
        result.error = "top-level Front Matter must be a YAML mapping";
        return result;
    }
    for (const auto& entry : root.mapping) {
        result.fields.push_back({entry.first, displayNode(entry.second)});
    }
    return result;
}

} // namespace qmd
