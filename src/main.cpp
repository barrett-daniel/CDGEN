#include <filesystem>
#include <iostream>
#include <vector>
#include <fstream>

#include "intermediate_representation.hpp"
#include "util/io.hpp"
#include "optimiser/optimiser.hpp"
#include "parser/parser.hpp"
#include "util/run_configuration.hpp"
#include "translator/translator.hpp"


int main(int argc, char* argv[]) {
    // get the user supplied run configuration
    RunConfiguration run_config = io::parse_user_input(argc, argv);
    if (run_config.verbose_debug) {
        std::cout << "Running class diagram generator...\n\n"
            "Run options: \n"
            " - input directory: " << run_config.input_dir << std::endl <<
            " - output file name: " << run_config.output_fname << std::endl <<
            " - verbose debug: " << run_config.verbose_debug << std::endl <<
            " - removed undefined types: " << run_config.remove_undefined_types << std::endl <<
            " - display functions first: " << run_config.display_functions_first << std::endl <<
            " - hide empty diagram sections: " << run_config.hide_empty_diagram_sections << std::endl << std::endl;
    }

    // get the source code files from the directory and read their contents
    std::vector<std::string> fnames = io::get_fnames(run_config.input_dir, run_config.target_lang);
    std::vector<std::string> file_contents;
    file_contents.reserve(fnames.size());
    for (const std::string& fname : fnames) {
        file_contents.push_back(io::read_contents(fname));
    }

    // parse the files based on the given language
    std::vector<ClassInfo> classes;
    switch (run_config.target_lang) {
        case SupportedLanguages::CPP: { classes = parser::parse_cpp(file_contents); break; }
        default: throw std::exception();
    };
    if (run_config.verbose_debug) {
        std::cout << "The follow classes were identified:" << std::endl;
        for (const ClassInfo& class_info : classes) {
            std::cout << " - " << class_info.name << std::endl;
        }
        std::cout << std::endl;
    }

    // optimiser logic to prepare for translation
    std::vector<std::string> flattened_classes = optimiser::flatten_nested_classes(classes);
    std::vector<std::string> stripped_classes;
    if (run_config.remove_undefined_types) {
        stripped_classes = optimiser::strip_undefined_relations(classes);
    }

    // print optimiser changes:
    if (run_config.verbose_debug) {
        if (!flattened_classes.empty()) {
            std::cout << "The following inner/nested class definitions were identified:" << std::endl;
            for (const std::string& flattened_class_name : flattened_classes) {
                std::cout << " - " << flattened_class_name << std::endl;
            }
            std::cout << std::endl;
        }
        if (!stripped_classes.empty()) {
            std::cout << "The following are classes identified for which no class definition could be found within the given directory. "
                         "These identifiers may be from the stdlib, some external library/API, etc. "
                         "They have been removed from the class diagram. "
                         "If you intended for any of these classes to be included in the class diagram, ensure they are defined within the given directories. "
                         "If you believe a class was omitted in error and is a bug, please report such. "
                         "In the meantime, you are able to manually add the missed class to the diagram by altering the output `.gv` file:"
            << std::endl;

            for (const std::string& stripped_class_name : stripped_classes) {
                std::cout << " - " << stripped_class_name << std::endl;
            }
            std::cout << std::endl;
        }
    }

    // translate the encoded classes to graphviz language and write such to the output file.
    std::string gviz_file = translator::to_gviz_file(classes, run_config);
    io::write_file(gviz_file, run_config.output_fname);

    std::cout << "Graphviz file successfully generated!" << std::endl;
    return 0;
}