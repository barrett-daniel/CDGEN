#pragma once

#include <format>
#include <unordered_map>
#include <vector>

#include "stream_writter.hpp"
#include "util/run_configuration.hpp"
#include "intermediate_representation.hpp"

class TranslateWriter {
public:
    TranslateWriter(const RunConfiguration& run_config);

    /**
     * @return the encoded tranlsation as a string. Call `close_translator` prior to this for the final translation.
     */
    std::string str();
    void close_translator();

    void write_class_name_comment(const std::string& class_name);
    void write_relations_comment();
    void write_node_spec_comment();

    void write_relations(const std::string& class_name, const std::unordered_map<std::string, RelationshipType>& relations);

    void write_node_spec_header(const std::string& class_name);
    void write_node_spec_footer();

    void write_method_property_table(const std::vector<MethodInfo>& method_infos);
    void write_variable_property_table(const std::vector<VariableInfo>& variable_infos);

private:

    void write_property_table_header();
    void write_property_table_footer();

    StreamWriter writer;
    const RunConfiguration& run_config;
};
