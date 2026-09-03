#include "translator.hpp"

#include <format>
#include <vector>

#include "intermediate_representation.hpp"
#include "translate_writer.hpp"

namespace translator {
    std::string to_gviz_file(const std::vector<ClassInfo>& classes, const RunConfiguration& run_config) {
        TranslateWriter trans_writer(run_config);

        for (const ClassInfo& class_info : classes) {
            trans_writer.write_class_name_comment(class_info.name);

            trans_writer.write_relations_comment();
            trans_writer.write_relations(class_info.name, class_info.relationships);

            trans_writer.write_node_spec_comment();
            trans_writer.write_node_spec_header(class_info.name);

            if (run_config.show_members) {
                if (run_config.display_functions_first) {
                    trans_writer.write_method_property_table(class_info.methods);
                    trans_writer.write_variable_property_table(class_info.variables);
                } else {
                    trans_writer.write_variable_property_table(class_info.variables);
                    trans_writer.write_method_property_table(class_info.methods);
                }
            }
            
            trans_writer.write_node_spec_footer();
        }
        trans_writer.close_translator();
        return trans_writer.str();
    }
};