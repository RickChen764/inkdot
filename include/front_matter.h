#ifndef INKDOT_FRONT_MATTER_H
#define INKDOT_FRONT_MATTER_H

#include <cstddef>
#include <string>
#include <vector>

namespace qmd {

struct FrontMatterField {
    std::string key;
    std::string value;
};

struct FrontMatterResult {
    bool present = false;
    size_t bodyOffset = 0;
    std::vector<FrontMatterField> fields;
    std::string raw;
    std::string error;
};

// Parses the leading YAML Front Matter block used by Markdown tools. The
// built-in parser deliberately targets front-matter data rather than the full
// YAML document model: mappings, sequences, nesting, flow collections,
// quoted scalars, comments, and |/> block scalars are supported without
// adding a heavyweight runtime dependency.
FrontMatterResult parseFrontMatter(const std::string& markdown);

} // namespace qmd

#endif // INKDOT_FRONT_MATTER_H
