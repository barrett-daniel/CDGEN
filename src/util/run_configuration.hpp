#pragma once
#include <string>
#include <cstdint>

enum class SupportedLanguages : uint8_t {
    NA,
    CPP
};

struct RunConfiguration {
    // required
    std::string input_dir;
    std::string output_fname;

    // optional i.e. they have default values
    SupportedLanguages target_lang;
    bool verbose_debug = true;
    bool remove_undefined_types = true;
    bool display_functions_first = true;
    bool hide_empty_diagram_sections = true;
    bool show_members = true;
    bool show_non_public_members = true;
};
