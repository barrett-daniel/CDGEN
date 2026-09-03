#pragma once

#include <string>
#include <vector>

#include "run_configuration.hpp"

namespace io {
    /**
     * Fetches the file paths for source code files in the given directory.
     *
     * @param dir the directory to search
     * @param target_lang the language for which code files are to be search for
     * @return the list of file paths/names for source code files of the target language in the given directory.
     */
    std::vector<std::string> get_fnames(const std::string& dir, SupportedLanguages target_lang);

    /**
     * Reads the contetns of a file and encodes such as a string
     *
     * @param fname file path to be read
     * @return the encoded string contents of the file
     */
    std::string read_contents(const std::string& fname);

    /**
     * Writes the given string to a file.
     *
     * @param contents the contents of the file to be written
     * @param out_fname the name of the file to write to
     */
    void write_file(const std::string& contents, const std::string& out_fname);

    /**
     * Parses program arguments into a run configuration struct.
     */
    RunConfiguration parse_user_input(int argc, char* argv[]);
}

