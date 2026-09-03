#include "translate_writer.hpp"

#include <format>
#include <queue>
#include <string>

#include "intermediate_representation.hpp"

// ----------------------------------------------------------------------------
// Util Declarations
// ----------------------------------------------------------------------------

std::string format_type(const TypeInfo& tinfo);
std::string format_method(MethodInfo info);
std::string format_variable(VariableInfo info);
std::string xml_escape(const std::string& s);
void insert_type_def(std::stringstream& ss, const std::string& type);
std::string get_combined_qualifiers(const std::vector<QualifierInfo>& qualifiers, QualifierPosition desired_pos);
char visibility_to_decorator(VisibilityType visibility);

// ----------------------------------------------------------------------------
// TranslateWriter Definitions
// ----------------------------------------------------------------------------

TranslateWriter::TranslateWriter(const RunConfiguration& run_config) : run_config(run_config) {
    writer.write_line("digraph {");
    writer.indent();
}

std::string TranslateWriter::str() {
    return writer.get_code_str();
}

void TranslateWriter::close_translator() {
    writer.dedent();
    writer.write_line("}");
}

void TranslateWriter::write_class_name_comment(const std::string& class_name) {
    writer.write_line("// -----------");
    writer.write_line("// Class: %s", class_name);
    writer.write_line("// -----------");
    writer.write_line();
}

void TranslateWriter::write_relations_comment() {
    writer.write_line("// Relations");
    writer.write_line();
}

void TranslateWriter::write_node_spec_comment() {
    writer.write_line("// Node Specification");
    writer.write_line();
}

void TranslateWriter::write_relations(const std::string& class_name, const std::unordered_map<std::string, RelationshipType>& relations) {
    auto relation_to_label = [](const RelationshipType relation) {
        switch (relation) {
            case RelationshipType::INHERITANCE: return "edge [dir=forward arrowtail=vee]";
            case RelationshipType::DEPENDENCY: return "edge [dir=none]";
            case RelationshipType::AGGREGATION: return "edge [dir=back arrowtail=odiamond]";
            case RelationshipType::COMPOSITION: return "edge [dir=back arrowtail=diamond]";
        }
        throw std::runtime_error("Invalid relationship type");
    };

    for (auto& [other_name, relation_type] : relations) {
        writer.write_line(relation_to_label(relation_type));
        writer.write_line("%s -> %s", std::format("\"{}\"", class_name), std::format("\"{}\"", other_name));
        writer.write_line();
    }
}

void TranslateWriter::write_node_spec_header(const std::string& class_name) {
    writer.write_line("%s [", class_name);
    writer.indent();
    writer.write_line("shape=plain");
    writer.write_line("label=<");
    writer.indent();
    writer.write_line("<table border=\"0\" cellborder=\"1\" cellspacing=\"0\" cellpadding=\"4\">");
    writer.indent();
    writer.write_line("<tr> <td> <b> %s </b> </td> </tr>", class_name);
}

void TranslateWriter::write_node_spec_footer() {
    writer.dedent();
    writer.write_line("</table>");
    writer.dedent();
    writer.write_line(">");
    writer.dedent();
    writer.write_line("]");
}

void TranslateWriter::write_method_property_table(const std::vector<MethodInfo>& method_infos) {
    std::queue<std::string> label_buff;

    // iterate over methods, and store formatted method labels into buffer
    for (const MethodInfo& info : method_infos) {
        if (!run_config.show_non_public_members && info.visibility != VisibilityType::PUBLIC) {
            continue;
        }
        std::string fmt_line = std::format("<tr> <td align=\"left\"> {} </td> </tr>", format_method(info));
        label_buff.push(std::move(fmt_line));
    }

    // hide/show the section logic based upon run_config.
    if (label_buff.empty()) {
        if (run_config.hide_empty_diagram_sections) {
            return;
        }
        label_buff.emplace("<tr> <td> </td> </tr>"); // empty line so table is not hidden
    }

    // actually write all the data now
    write_property_table_header();
    while (!label_buff.empty()) {
        std::string& fmt_line = label_buff.front();
        writer.write_line(fmt_line);
        label_buff.pop();
    }
    write_property_table_footer();
}

void TranslateWriter::write_variable_property_table(const std::vector<VariableInfo>& variable_infos) {
    std::queue<std::string> label_buff;

    // construct a map to store and group all the class variables based on visibility and type
    std::unordered_map<VisibilityType, std::unordered_map<std::string, std::vector<const VariableInfo*>>> grouped_vars;
    for (const VariableInfo& info : variable_infos) {
        if (!run_config.show_non_public_members && info.visibility != VisibilityType::PUBLIC) {
            continue;
        }

        std::stringstream ss;
        insert_type_def(ss, get_combined_qualifiers(info.type.qualifiers, QualifierPosition::PREFIX) + format_type(info.type));
        std::string type = ss.str();

        auto& vec = grouped_vars[info.visibility][type];
        vec.emplace_back(&info);
    }

    // iterate over grouped types and push formatted description line to buffer
    for (auto& [vis_type, imap] : grouped_vars) {
        for (auto& [type_descriptor, vec] : imap) {
            std::stringstream ss;
            ss << visibility_to_decorator(vis_type) << " " << type_descriptor << ": ";

            for (size_t i = 0; i < vec.size(); i++) {
                ss << vec[i]->name;

                if (i + 1 < vec.size()) {
                    ss << ", ";
                }
            }
            std::string fmt_line = std::format("<tr> <td align=\"left\"> {} </td> </tr>", ss.str());
            label_buff.push(std::move(fmt_line));
        }
    }

    // hide/show the section logic based upon run_config.
    if (label_buff.empty()) {
        if (run_config.hide_empty_diagram_sections) {
            return;
        }
        label_buff.emplace("<tr> <td> </td> </tr>"); // empty line so table is not hidden
    }

    // actually write all the data now
    write_property_table_header();
    while (!label_buff.empty()) {
        std::string& fmt_line = label_buff.front();
        writer.write_line(fmt_line);
        label_buff.pop();
    }
    write_property_table_footer();
}

void TranslateWriter::write_property_table_header() {
    writer.write_line("<tr> <td>");
    writer.indent();
    writer.write_line("<table border=\"0\" cellborder=\"0\" cellspacing=\"0\" cellpadding=\"4\">");
    writer.indent();
}

void TranslateWriter::write_property_table_footer() {
    writer.dedent();
    writer.write_line("</table>");
    writer.dedent();
    writer.write_line("</td> </tr>");
}

// ----------------------------------------------------------------------------
// Util Definitions
// ----------------------------------------------------------------------------


std::string format_type(const TypeInfo& tinfo) {
    std::string fmt_tinfo;

    fmt_tinfo += tinfo.name;

    if (!tinfo.template_parameters.empty()) {
        fmt_tinfo += "<";

        for (int i = 0; i < tinfo.template_parameters.size(); i++) {
            fmt_tinfo += format_type(tinfo.template_parameters[i]);

            if (i + 1 < tinfo.template_parameters.size()) {
                fmt_tinfo += ", ";
            }
        }

        fmt_tinfo += "> ";
    }

    return fmt_tinfo;
}


std::string format_method(MethodInfo info) {
    std::stringstream ss;

    ss << visibility_to_decorator(info.visibility) << " ";
    std::string method_type_lex = get_combined_qualifiers(info.type.qualifiers, QualifierPosition::PREFIX) + format_type(info.type);
    insert_type_def(ss, method_type_lex);
    ss << info.name;

    ss << "(";
    for (size_t i = 0; i < info.parameters.size(); i++) {
        const IdentifierInfo& pinfo = info.parameters[i];
        insert_type_def(ss, format_type(pinfo.type));
        ss << pinfo.name;

        if (i + 1 < info.parameters.size()) {
            ss << ", ";
        }
    }

    ss << ")";
    std::string postfix_qualifiers = get_combined_qualifiers(info.type.qualifiers, QualifierPosition::POSTFIX);
    if (!postfix_qualifiers.empty()) {
        ss << " ";
        insert_type_def(ss, postfix_qualifiers);
    }
    return ss.str();
}

std::string xml_escape(const std::string& s) {
    std::string out;
    for (char c : s) {
        switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            default: out += c;
        }
    }
    return out;
}

void insert_type_def(std::stringstream& ss, const std::string& type) {
    ss << "<b>" << xml_escape(type) << "</b> ";
}

std::string get_combined_qualifiers(const std::vector<QualifierInfo>& qualifiers, QualifierPosition desired_pos) {
    std::string fmt_str;
    for (QualifierInfo qinfo : qualifiers) {
        if (qinfo.position == desired_pos) {
            fmt_str += qinfo.name + ' ';
        }
    }
    return fmt_str;
}

char visibility_to_decorator(VisibilityType visibility) {
    switch (visibility) {
        case VisibilityType::PUBLIC: return '+';
        case VisibilityType::PRIVATE: return '-';
        case VisibilityType::PROTECTED: return '#';
    }

    std::string err = std::format("Unsupported visibility type in \"visibility_to_decorator\": {}", static_cast<int>(visibility));
    throw std::runtime_error(err);
}