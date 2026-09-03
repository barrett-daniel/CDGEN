#include <format>
#include <string>

#include <doctest/doctest.h>
#include "../src/util/io.hpp"
#include "../src/util/run_configuration.hpp"

TEST_SUITE("Run Configuration Parsing Tests") {
    std::string program_name = "./cd_gen.exe";
    std::string input_dir_arg = "-i=input_dir";
    std::string output_fname_arg = "-o=out.gv";
    std::string target_language_arg = "-target_language=c++";

    TEST_CASE("001 Required Argument Parsing") {
        int argc = 4;
        char *argv[] = {program_name.data(), input_dir_arg.data(), output_fname_arg.data(), target_language_arg.data()};
        RunConfiguration result = io::parse_user_input(argc, argv);

        SUBCASE("Input Directory") { CHECK_EQ(result.input_dir, "input_dir"); }
        SUBCASE("Output Filename") { CHECK_EQ(result.output_fname, "out.gv"); }
        SUBCASE("Target Language") { CHECK_EQ(result.target_lang, SupportedLanguages::CPP); }
    }

    TEST_CASE("002 Optional Arugment Parsing") {
        using Member = bool RunConfiguration::*;

        auto [flag_name, member] = GENERATE(
            std::make_tuple(std::string("-verbose_debug="), (Member)&RunConfiguration::verbose_debug),
            std::make_tuple(std::string("-remove_undefined_types="), (Member)&RunConfiguration::remove_undefined_types),
            std::make_tuple(std::string("-display_functions_first="), (Member)&RunConfiguration::display_functions_first),
            std::make_tuple(std::string("-hide_empty_diagram_sections="), (Member)&RunConfiguration::hide_empty_diagram_sections)
        );
        bool value = GENERATE(true, false);

        std::string optional_arg_str = flag_name + (value ? "true" : "false");
        SUBCASE(optional_arg_str) {
            int argc = 5;
            char* argv[] = {program_name.data(), input_dir_arg.data(), target_language_arg.data(), output_fname_arg.data(), optional_arg_str.data()};

            RunConfiguration result = io::parse_user_input(argc, argv);
            CHECK_EQ(result.*member, value);
        }
    }

    TEST_CASE("003 Optional Argument Combinations") {
        bool verbose_debug_val               = GENERATE(true, false);
        bool remove_undefined_types_val      = GENERATE(true, false);
        bool display_functions_first_val     = GENERATE(true, false);
        bool hide_empty_diagram_sections_val = GENERATE(true, false);

        std::string verbose_debug_arg = "-verbose_debug=" + std::string(verbose_debug_val ? "true" : "false");
        std::string remove_undefined_types_arg = "-remove_undefined_types=" + std::string(remove_undefined_types_val ? "true" : "false");
        std::string display_functions_first_arg = "-display_functions_first=" + std::string(display_functions_first_val ? "true" : "false");
        std::string hide_empty_diagram_sections_arg = "-hide_empty_diagram_sections=" + std::string(hide_empty_diagram_sections_val ? "true" : "false");

        std::string subcase_label = std::format("{} {} {} {}",
            verbose_debug_arg,
            remove_undefined_types_arg,
            hide_empty_diagram_sections_arg,
            display_functions_first_arg
        );
        SUBCASE(subcase_label) {
            CAPTURE(verbose_debug_arg);
            CAPTURE(remove_undefined_types_arg);
            CAPTURE(display_functions_first_arg);
            CAPTURE(hide_empty_diagram_sections_arg);

            int argc = 8;
            char* argv[] = {
                program_name.data(),
                input_dir_arg.data(),
                output_fname_arg.data(),
                target_language_arg.data(),
                verbose_debug_arg.data(),
                remove_undefined_types_arg.data(),
                display_functions_first_arg.data(),
                hide_empty_diagram_sections_arg.data()
            };

            RunConfiguration result = io::parse_user_input(argc, argv);

            CHECK_EQ(result.verbose_debug, verbose_debug_val);
            CHECK_EQ(result.remove_undefined_types, remove_undefined_types_val);
            CHECK_EQ(result.display_functions_first, display_functions_first_val);
            CHECK_EQ(result.hide_empty_diagram_sections, hide_empty_diagram_sections_val);
        }
    }
}