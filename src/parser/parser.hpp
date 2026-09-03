#pragma once

#include <vector>
#include <string>

#include "../intermediate_representation.hpp"

namespace parser {
    /**
     * Parses the set of C++ files into the Intermediate Representation
     *
     * @param file_contents the contents of the C++ files to parse
     * @return a list of class information encoding the C++ files
     */
    std::vector<ClassInfo> parse_cpp(const std::vector<std::string>& file_contents);

    // Currently not implemented - included to demonstrate intended extendability for project
    // whereby another parser file can be created for Java, using the same IR, the translator will
    // produce a suitable class diagram.
    //
    // /**
    //  * Parses the set of Java files into the Intermediate Representation.
    //  *
    //  * @param file_contents the contents of the Java files to parse
    //  * @return a list of class information encoding the Java files
    //  */
    // std::vector<ClassInfo> parse_java(const std::vector<std::string>& file_contents);
}
