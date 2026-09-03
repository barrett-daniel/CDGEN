#include "io.hpp"

#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

std::vector<std::string> get_lang_handles(SupportedLanguages lang) {
    switch (lang) {
        case SupportedLanguages::CPP: return {
            ".cpp", ".cxx", ".c++",         // code files
            ".hpp", ".hxx", ".h++", ".h"    // header files
        };
        default: throw std::exception();
    }
}

namespace io {
    std::vector<std::string> get_fnames(const std::string& dir, const SupportedLanguages target_lang) {
        std::vector<std::string> fnames;
        std::vector<std::string> handles = get_lang_handles(target_lang);

        constexpr auto str_ends_with = [](const std::string& str, const std::string& suffix) {
            if (suffix.size() > str.size()) return false;
            return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
        };

        std::filesystem::directory_iterator dir_it(dir);
        for (const auto& entry : dir_it) {
            std::string path = entry.path().string();

            // recursively search the directories
            if (entry.is_directory()) {
                std::vector<std::string> recursive_fnames = get_fnames(path, target_lang);
                fnames.insert(fnames.end(), recursive_fnames.begin(), recursive_fnames.end());
            }

            // check if the file is a c++/header file and add to the list
            for (const std::string& handle : handles) {
                if (str_ends_with(path, handle)) {
                    fnames.push_back(path);
                    break;
                }
            }
        }

        return fnames;
    }

    std::string read_contents(const std::string& fname) {
        std::ifstream fstream = std::ifstream(fname);
        std::string line;
        std::stringstream buffer;
        while (std::getline(fstream, line)) {
            buffer << line << std::endl;
        }

        return buffer.str();
    }

    void write_file(const std::string& contents, const std::string& out_fname) {
        std::ofstream out_file = std::ofstream(out_fname);
        out_file << contents;
    }

    RunConfiguration parse_user_input(int argc, char* argv[]) {
        const std::string help_msg =
            "Usage: diagram_gen -i=<input_dir> -o=<output_fname> [options]\n"
            "\n"
            "Automatically generates class diagrams from source code.\n"
            "\n"
            "Required arguments:\n"
            "  -i=<PATH>               Input directory to scan for source files\n"
            "  -o=<PATH>               Output filename for the generated diagram\n"
            "\n"
            "Optional arguments (true/false, default = true):\n"
            "  -verbose_debug=<BOOL>                Print detailed debug output during generation\n"
            "  -remove_undefined_types=<BOOL>       Omit types that could not be resolved\n"
            "  -display_functions_first=<BOOL>      List functions before fields in each class\n"
            "  -hide_empty_diagram_sections=<BOOL>  Hide function/field sections with no entries in the output\n"
            "  -show_members=<BOOL>                 Show members of classes. Takes precedence over 'show_only_public_members'\n"
            "  -show_non_public_members<BOOL>       Show members which are not public i.e. private or protected."
            "  -help                                Displays this message, and exits the program."
            "\n"
            "Example:\n"
            "  diagram_gen -i=./src -o=diagram.gv -target_language=c++ -verbose_debug=false -hide_empty_diagram_sections=false\n";

        if (argc == 1) {
            std::cerr << help_msg << std::endl;
            throw std::exception();
        }

        auto parse_bool = [&help_msg](const std::string& value, const std::string& arg_name) {
            if (value != "true" && value != "false") {
                std::cerr << "ERR: invalid input argument: \"" << arg_name << "\" with value: \"" <<  value << "\"" << std::endl << help_msg << std::endl;
                throw std::exception();
            }
            return value == "true";
        };

        // A struct containing:
        // - program run options (prefixes)
        // - a function which handles those prefixes given a user-provided argument
        struct ArgSpec {
            std::string prefix;
            std::function<void(const std::string&)> handler;
        };

        // vector of what are the valid program arugment prefixes and how they should be
        // handled i.e. parsed in to the RunConfiguration struct.
        RunConfiguration run_config{};
        const std::vector<ArgSpec> arg_specs = {
            {"-i=", [&](const std::string& param) {
                run_config.input_dir = param;
            }},
            {"-o=", [&](const std::string& param) {
                run_config.output_fname = param;
            }},
            {"-target_language=", [&](const std::string& param) {
                if (param == "c++") {
                    run_config.target_lang = SupportedLanguages::CPP;
                    return;
                }

                std::cerr << "ERR: Malformed target language provided. Valid options are: [c++]" << std::endl;
                throw std::exception();
            }},
            {"-verbose_debug=", [&](const std::string& param) {
                run_config.verbose_debug = parse_bool(param, "-verbose_debug=");
            }},
            {"-remove_undefined_types=", [&](const std::string& param) {
                run_config.remove_undefined_types = parse_bool(param, "-remove_undefined_types=");
            }},
            {"-display_functions_first=", [&](const std::string& param) {
                run_config.display_functions_first = parse_bool(param, "-display_functions_first=");
            }},
            {"-hide_empty_diagram_sections=", [&](const std::string& param) {
                run_config.hide_empty_diagram_sections = parse_bool(param, "-hide_empty_diagram_sections=");
            }},
            {"-show_members=", [&](const std::string& param) {
               run_config.show_members = parse_bool(param, "-show_members="); 
            }},
            {"-show_private_members=", [&](const std::string& param) {
                run_config.show_non_public_members = parse_bool(param, "-show_only_public_members=");
            }},
            {"-help", [&](const std::string& _) {
                std::cerr << help_msg << std::endl;
                throw std::exception();
            }}
        };

        for (int i = 1; i < argc; i++) {  // start at 1 - skip program name
            std::string user_arg = argv[i];

            bool match_found = false;
            for (const auto& spec : arg_specs) {
                if (user_arg.compare(0, spec.prefix.size(), spec.prefix) == 0) {
                    spec.handler(user_arg.substr(spec.prefix.size())); // pass the user parameter to the argument specification handler
                    match_found = true;
                    break;  // stop checking other prefixes once matched
                }
            }

            if (!match_found) {
                std::cerr << "ERR: Unknown argument: " << user_arg << std::endl << help_msg << std::endl;
                throw std::exception();
            }
        }

        // final necessary sanity checks
        if (run_config.input_dir.empty()) {
            std::cerr << "ERR: No input directory specified" << std::endl << help_msg << std::endl;
            throw std::exception();
        }

        if (run_config.output_fname.empty()) {
            std::cerr << "ERR: No output directory specified" << std::endl << help_msg << std::endl;
            throw std::exception();
        }

        if (run_config.target_lang == SupportedLanguages::NA) {
            // for now we default to C++
            run_config.target_lang = SupportedLanguages::CPP;
        }

        return run_config;
    }
}