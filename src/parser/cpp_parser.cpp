#include <cassert>
#include <cstring>
#include <stdexcept>

#include "parser.hpp"

#include <tree_sitter/api.h>
extern "C" const TSLanguage *tree_sitter_cpp();

// utility functions
std::string wts_node_lexeme(TSNode node, const std::string& text);
VisibilityType encode_access_specifier(const std::string& lexeme);
std::vector<ClassInfo> get_class_info(TSNode root, const std::string& contents);
void log_tree(TSNode node, const std::string& contents, std::string indent_buff = "\t");
void max_relation_emplace(std::unordered_map<std::string, RelationshipType>& relations, std::string key, RelationshipType value);
void insert_type_qualifiers(TSNode node, const std::string& contents, std::vector<QualifierInfo>& qualifiers, QualifierPosition qualifier_position);
std::pair<TypeInfo, TSNode> deduce_type(TSNode node, const std::string& contents, TypeInfo* type_info = nullptr);


// visitors
void visit_template_argument_list(TSNode node, const std::string& contents, TypeInfo* type_info);
void visit_template_type         (TSNode node, const std::string& contents, TypeInfo* type_info);
void visit_parameter_declaration (TSNode node, const std::string& contents, ClassInfo* class_info, MethodInfo* method_info);
void visit_parameter_list        (TSNode node, const std::string& contents, ClassInfo* class_info, MethodInfo* method_info);
void visit_function_declarator   (TSNode node, const std::string& contents, ClassInfo* class_info, MethodInfo* method_info);
void visit_function_definition   (TSNode node, const std::string& contents, ClassInfo* class_info, VisibilityType visibility);
void visit_field_declaration     (TSNode node, const std::string& contents, ClassInfo* class_info, VisibilityType visibility);
void visit_field_declaration_list(TSNode node, const std::string& contents, ClassInfo* class_info);
void visit_base_class_clause     (TSNode node, const std::string& contents, ClassInfo* class_info);
ClassInfo visit_class_specifier  (TSNode node, const std::string& contents);

namespace parser {
    std::vector<ClassInfo> parse_cpp(const std::vector<std::string>& file_contents) {
        std::vector<ClassInfo> classes;

        // configure tree-sitter parser
        TSParser* parser = ts_parser_new();
        ts_parser_set_language(parser, tree_sitter_cpp());

        for (const std::string& contents : file_contents) {
            TSTree *tree = ts_parser_parse_string(
                parser,
                NULL,
                contents.c_str(),
                contents.size()
            );

            TSNode root = ts_tree_root_node(tree);
            std::vector<ClassInfo> file_classes = get_class_info(root, contents);
            classes.insert(classes.end(), file_classes.begin(), file_classes.end());
            ts_tree_delete(tree);
        }

        ts_parser_delete(parser);
        return classes;
    }
};


// ----------------------------------------------------------------------------
// Utility
// ----------------------------------------------------------------------------

/**
 * Wrapper function for extracting the lexeme from a given `TSNode` (hence
 * the name `wts_node_lexeme` w/ "w" standing for "wrapper")
 *
 * @param node
 * @param text
 * @return
 */
std::string wts_node_lexeme(TSNode node, const std::string& text) {
    uint32_t start = ts_node_start_byte(node);
    uint32_t end = ts_node_end_byte(node);
    return std::string(text, start, end - start);
}

/**
 * Wrapper tree-sitter function to encode the lexeme of an access specifier
 * as a `VisibilityType` enum value;
 *
 * @param lexeme of the specifier
 * @return the encoded `VisibilityType` value
 */
VisibilityType encode_access_specifier(const std::string& lexeme) {
    if (strcmp(lexeme.c_str(), "public") == 0) return VisibilityType::PUBLIC;
    if (strcmp(lexeme.c_str(), "private") == 0) return VisibilityType::PRIVATE;
    if (strcmp(lexeme.c_str(), "protected") == 0) return VisibilityType::PROTECTED;

    throw std::runtime_error("Err in \"w_ts_encode_access_specifier\" - unable to parse lexeme as a valid access specifier");
}

/**
 * Identifies all classes in a file and returns their info.
 *
 * @param root node of parse tree
 * @param contents the contents of the file
 * @return the list of classes within the tree descending from the root
 */
std::vector<ClassInfo> get_class_info(TSNode root, const std::string& contents) {
    std::vector<ClassInfo> classes;

    const char* root_type = ts_node_type(root);
    assert(strcmp(root_type, "translation_unit") == 0);

    uint32_t children_count = ts_node_child_count(root);
    for (uint32_t i = 0; i < children_count; i++) {
        TSNode child = ts_node_child(root, i);
        const char* child_type = ts_node_type(child);

        if (strcmp(child_type, "class_specifier") == 0 || strcmp(child_type, "struct_specifier") == 0) {
            ClassInfo class_info = visit_class_specifier(child, contents);
            classes.push_back(class_info);
        }
    }

    return classes;
}

/**
 * Logs the given tree recursively
 *
 * @param node the root node of the subtree
 * @param contents the contents of the file
 */
void log_tree(TSNode node, const std::string& contents, std::string indent_buff) {
    printf("%snode type: \"%s\" - %d\n", indent_buff.c_str(), ts_node_type(node), ts_node_symbol(node));
    printf("%s\n", wts_node_lexeme(node, contents).c_str());

    uint32_t count = ts_node_child_count(node);
    for (uint32_t i = 0; i < count; i++) {
        TSNode child = ts_node_child(node, i);
        log_tree(child, contents, indent_buff + '\t');
    }
}

/**
 * Wrapper around emplacing into the relation map to only overwrite existing relations
 * if the given relation is stronger than the current one.
 *
 * @param relations the set of relations to emplace into
 * @param key the given class which the relation is to
 * @param value the proposed new value of the relation
 */
void max_relation_emplace(std::unordered_map<std::string, RelationshipType>& relations, std::string key, RelationshipType value) {
    if (key.empty()) {
        return;
    }

    // strip pointer/reference type indicators as they're not relevant to relation associations
    while (key.back() == '*' || key.back() == '&') {
        key.pop_back();
    }

    // if the given relation does not already exist -> emplace it
    if (!relations.contains(key)) {
        relations.emplace(key, value);
        return;
    }

    // if the new relation is stronger than the existing one -> replace it
    auto relation_stronger = [](const RelationshipType a, const RelationshipType b) {
        return static_cast<uint8_t>(a) < static_cast<uint8_t>(b);
    } ;
    RelationshipType curr_relation = relations.at(key);
    if (relation_stronger(value, curr_relation)) {
        relations[key] = value;
    }

    // do nothing
}

/**
 * Parses the given qualifier node and inserts the appropriate encoded qualifier into the given list of qualifiers.
 *
 * @param node which contains qualifier info to encode
 * @param contents contents of the parsed file
 * @param qualifiers the qualifiers to add to
 * @param qualifier_position where the qualifier is placed relative to the identifier
 */
void insert_type_qualifiers(TSNode node, const std::string& contents, std::vector<QualifierInfo>& qualifiers, QualifierPosition qualifier_position) {
    const char* node_type = ts_node_type(node);

    if (strcmp(node_type, "storage_class_specifier") == 0) {
        TSNode storage_specifier_node = ts_node_child(node, 0);
        qualifiers.emplace_back(qualifier_position, wts_node_lexeme(storage_specifier_node, contents));
    }

    if (strcmp(node_type, "type_qualifier") == 0) {
        TSNode type_qualifier_node = ts_node_child(node, 0);
        qualifiers.emplace_back(qualifier_position, wts_node_lexeme(type_qualifier_node, contents));
    }

    if (strcmp(node_type, "virtual_specifier") == 0) {
        TSNode type_qualifier_node = ts_node_child(node, 0);
        qualifiers.emplace_back(qualifier_position, wts_node_lexeme(type_qualifier_node, contents));
    }

    if (strcmp(node_type, "virtual") == 0) {
        qualifiers.emplace_back(qualifier_position, wts_node_lexeme(node, contents));
    }
}

/**
 * Deduces the C++ type of a given identifier node, unwrapping pointers as needed.
 *
 * @param node the node whose type is to be deduced
 * @param contents the contents of the parsed file
 * @param type_info curried/recursively built up type information. If not specified, one is automatically created upon the inital call.
 *
 * @return the deduced type info and the node at end of the unwrapped tree of nested pointer types (if present) as a pair.
 */
std::pair<TypeInfo, TSNode> deduce_type(TSNode node, const std::string& contents, TypeInfo* type_info) {
    TypeInfo local_type_info;
    if (!type_info) {
        type_info = &local_type_info;
    }

    uint32_t child_count = ts_node_child_count(node);
    for (uint32_t i = 0; i < child_count; i++) {
        TSNode child = ts_node_child(node, i);
        const char* child_type = ts_node_type(child);

        insert_type_qualifiers(child, contents, type_info->qualifiers, QualifierPosition::PREFIX);

        if (strcmp(child_type, "namespace_identifier") == 0) {
            type_info->name.append(wts_node_lexeme(child, contents) + "::");
        }

        // the wrapper of a namespace'd type -> search recursively to deduce the type
        if (strcmp(child_type, "qualified_identifier") == 0) {
            auto [tinfo, _] = deduce_type(child, contents, type_info);
            return {tinfo, node};
        }

        if (strcmp(child_type, "type_identifier") == 0) {
            type_info->name.append(wts_node_lexeme(child, contents));
            type_info->primitive = false;
        }

        if (strcmp(child_type, "primitive_type") == 0) {
            type_info->name.append(wts_node_lexeme(child, contents));
            type_info->primitive = true;
        }

        if (strcmp(child_type, "template_type") == 0) {
            type_info->primitive = false;
            visit_template_type(child, contents, type_info);
        }

        if (strcmp(child_type, "pointer_declarator") == 0) {
            type_info->name.append("*");
            return deduce_type(child, contents, type_info);
        }

        if (strcmp(child_type, "reference_declarator") == 0) {
            // the 1st node is the reference declarator chars i.e. '&', '&&'
            TSNode ref_node = ts_node_child(child, 0);
            type_info->name.append(wts_node_lexeme(ref_node, contents));

            // 2nd node is the continuation of the parse tree so return that node
            return {*type_info, child};
        }
    }

    return {*type_info, node};
}

// ----------------------------------------------------------------------------
// Visitors
// ----------------------------------------------------------------------------

void visit_template_argument_list(TSNode node, const std::string& contents, TypeInfo* type_info) {
    uint32_t child_count = ts_node_child_count(node);
    for (uint32_t i = 0; i < child_count; i++) {
        TSNode child = ts_node_child(node, i);
        const char* child_type = ts_node_type(child);

        if (strcmp(child_type, "type_descriptor") == 0) {
            TypeInfo child_type_info;
            auto [deduced_child_type, _] = deduce_type(child, contents, &child_type_info);
            type_info->template_parameters.push_back(std::move(deduced_child_type));
            continue;
        }
    }
}

void visit_template_type(TSNode node, const std::string& contents, TypeInfo* type_info) {
    uint32_t child_count = ts_node_child_count(node);
    for (uint32_t i = 0; i < child_count; i++) {
        TSNode child = ts_node_child(node, i);
        const char* child_type = ts_node_type(child);

        if (strcmp(child_type, "type_identifier") == 0) {
            type_info->name.append(wts_node_lexeme(child, contents));
            continue;
        }

        if (strcmp(child_type, "template_argument_list") == 0) {
            visit_template_argument_list(child, contents, type_info);
            continue;
        }
    }
}

void visit_parameter_declaration(TSNode node, const std::string& contents, ClassInfo* class_info, MethodInfo* method_info) {
    auto [type_info, unwrapped_node] = deduce_type(node, contents);

    // any non-primitive types included as a parameter are encoded as a dependency relation to the class.
    if (!type_info.primitive) {
        max_relation_emplace(class_info->relationships, type_info.name, RelationshipType::DEPENDENCY);
    }
    for (const TypeInfo& template_tinfo : type_info.template_parameters) {
        max_relation_emplace(class_info->relationships, template_tinfo.name, RelationshipType::DEPENDENCY);
    }

    // we init the name as an empty string and update it when an 'identifier' node is found amongst the children.
    // the name is then used to add to the parameter list of the method at the end of this function. This allows
    // for named, and not-named parameters (in a function declaration naming is optional) to be handled the same.
    std::string name;

    uint32_t child_count = ts_node_child_count(unwrapped_node);
    for (uint32_t i = 0; i < child_count; i++) {
        TSNode child = ts_node_child(unwrapped_node, i);
        const char* child_type = ts_node_type(child);

        if (strcmp(child_type, "identifier") == 0) {
            name = wts_node_lexeme(child, contents);
            continue;
        }

        // accounts for parameter function pointers
        // its a bit tricky to properly encode the types and names
        // but this just encodes the pointer return type as the type
        // and the function declaration as the name of the parameter
        if (strcmp(child_type, "function_declarator") == 0) {
            name = wts_node_lexeme(child, contents);
            continue;
        }
    }

    method_info->parameters.emplace_back(std::move(name), std::move(type_info));
}

void visit_parameter_list(TSNode node, const std::string& contents, ClassInfo* class_info, MethodInfo* method_info) {

    uint32_t child_count = ts_node_child_count(node);
    for (uint32_t i = 0; i < child_count; i++) {
        TSNode child = ts_node_child(node, i);
        const char* child_type = ts_node_type(child);

        if (strcmp(child_type, "parameter_declaration") == 0) {
            visit_parameter_declaration(child, contents, class_info, method_info);
            continue;
        }
    }
}

void visit_function_declarator(TSNode node, const std::string& contents, ClassInfo* class_info, MethodInfo* method_info) {
    uint32_t child_count = ts_node_child_count(node);
    for (uint32_t i = 0; i < child_count; i++) {
        TSNode child = ts_node_child(node, i);
        const char* child_type = ts_node_type(child);

        // at the function declarator node, any type qualifiers will be postfix e.g. const methods
        insert_type_qualifiers(child, contents, method_info->type.qualifiers, QualifierPosition::POSTFIX);

        // nameless methods
        if (strcmp(child_type, "destructor_name") == 0 || strcmp(child_type, "identifier") == 0) {
            // note: the "identifier" case above is how constructors are parsed
            method_info->type.name = wts_node_lexeme(child, contents);
        }

        // function names or operator names
        if (strcmp(child_type, "field_identifier" ) == 0 || strcmp(child_type, "operator_name") == 0) {
            method_info->name = wts_node_lexeme(child, contents);
        }

        if (strcmp(child_type, "parameter_list") == 0) {
            visit_parameter_list(child, contents, class_info, method_info);
        }

        // handles function pointers
        if (strcmp(child_type, "parenthesized_declarator") == 0) {
            TSNode paren_child = ts_node_child(child, 1);
            const char* paren_child_type = ts_node_type(paren_child);
            assert(strcmp(paren_child_type, "pointer_declarator") == 0);

            TSNode identifier_node = ts_node_child(paren_child, 1);
            const char* identifier_type = ts_node_type(identifier_node);
            assert(strcmp(identifier_type, "field_identifier") == 0);

            method_info->name = wts_node_lexeme(identifier_node, contents);
            continue;
        }
    }
}

void visit_function_definition(TSNode node, const std::string& contents, ClassInfo* class_info, VisibilityType visibility) {
    auto [type_info, unwrapped_node] = deduce_type(node, contents);

    // return type of function is mapped to a dependency relation for the given class
    if (!type_info.primitive) {
        max_relation_emplace(class_info->relationships, type_info.name, RelationshipType::DEPENDENCY);
    }
    for (const TypeInfo& template_tinfo : type_info.template_parameters) {
        max_relation_emplace(class_info->relationships, template_tinfo.name, RelationshipType::DEPENDENCY);
    }

    MethodInfo method_info = {
        .name = "",
        .type = std::move(type_info),
        .visibility = visibility,
        .parameters = {}
    };

    uint32_t child_count = ts_node_child_count(unwrapped_node);
    for (uint32_t i = 0; i < child_count; i++) {
        TSNode child = ts_node_child(unwrapped_node, i);
        const char* child_type = ts_node_type(child);

        if (strcmp(child_type, "function_declarator") == 0) {
            visit_function_declarator(child, contents, class_info, &method_info);
            class_info->methods.push_back(method_info);
            continue;
        }
    }
}

void visit_field_declaration(TSNode node, const std::string& contents, ClassInfo* class_info, VisibilityType visibility) {
    // can probably clean this function up a bit by extracting the pattern of cases in the nested pointer type and non-pointer types
    std::vector<VariableInfo> variables;

    // lambda to remove pointer and reference decorators from the end of a type. Mutates the type name in place
    auto strip_decorator_chars = [](std::string& type_name) {
        while (type_name.back() == '*' || type_name.back() == '&') {
            type_name.pop_back();
        }
    };

    auto [type_info, _] = deduce_type(node, contents);

    // now we need to identify what relation this type has to the class (assuming its a member variable and not the
    // start of a function declaration). We use the below lambda to do so, store the relationship type, and continue the
    // parse to identify if this field is a function or a member. If the field is a member, we use the stored type to
    // insert a new relation, if its a function the implied relation is insert as per the subsequent visitor tree.
    // template parameters of members are also accounted for later, but we don't need to store their relationship types
    // as only the outer deduced type is manipulated in this function.

    // lambda for inserting a new relation into the given set of relations for a member variable based on its type
    // i.e. normal member, reference, or raw pointer.
    auto deduce_member_relation_type = [](const std::string& tinfo_name) {
        // default relationship mapping for basic members (e.g. Foo a) -> COMPOSITION (lifetime tied to outer class)
        RelationshipType relationship = RelationshipType::COMPOSITION;

        // if the member is either a pointer or a reference -> AGGREGATION (lifetime not tied to outer class)
        // note: raw pointers can be owning (implying COMPOSITION) or non-owning (implying AGGREGATION).
        //          determining if a pointer is owning/non-owning requires a more sophisticated parse with
        //          implicit understanding of the lifetime of objects in a project. Pointers are assumed
        //          to be non-owning for simplicity - it may not also be feasible to automate determination of
        //          owning/non-owning pointers.
        if (tinfo_name.back() == '&' || tinfo_name.back() == '*') {
            relationship = RelationshipType::AGGREGATION;
        }

        return relationship;
    };
    RelationshipType member_relation_type = deduce_member_relation_type(type_info.name);

    strip_decorator_chars(type_info.name);
    bool was_function = false;

    uint32_t child_count = ts_node_child_count(node);
    for (uint32_t i = 0; i < child_count; i++) {
        TSNode child = ts_node_child(node, i);
        const char* child_type = ts_node_type(child);

        // nested classes/structs
        if (strcmp(child_type, "class_specifier") == 0 || strcmp(child_type, "struct_specifier") == 0) {
            ClassInfo nested_class = visit_class_specifier(child, contents);
            max_relation_emplace(class_info->relationships, nested_class.name, RelationshipType::COMPOSITION);
            class_info->nested_classes.push_back(std::move(nested_class));
        }

        // variable names
        if (strcmp(child_type, "field_identifier") == 0) {
            variables.emplace_back(wts_node_lexeme(child, contents), type_info, visibility);
            continue;
        }

        // a function declaration is initially parsed as a "field declaration" but
        // can be parsed in the same way as a function definition, hence transition
        // visiting the current node as if it was a function definition;
        if (strcmp(child_type, "function_declarator") == 0) {
            visit_function_definition(node, contents, class_info, visibility);
            was_function = true;
            continue;
        }

        // explore nested pointers
        if (strcmp(child_type, "pointer_declarator") == 0) {
            type_info.name.append("*");
            auto [ptr_type_info, unwrapped_node] = deduce_type(child, contents, &type_info); // handle recursive pointers

            uint32_t unwrapped_node_child_count = ts_node_child_count(unwrapped_node);
            for (uint32_t i = 0; i < unwrapped_node_child_count; i++) {
                TSNode unwrapped_child = ts_node_child(unwrapped_node, i);
                const char* unwrapped_child_type = ts_node_type(unwrapped_child);

                if (strcmp(unwrapped_child_type, "field_identifier") == 0) {
                    variables.emplace_back(wts_node_lexeme(unwrapped_child, contents), type_info, visibility);
                    continue;
                }

                if (strcmp(unwrapped_child_type, "function_declarator") == 0) {
                    // a function declaration is initially parsed as a "field declaration" but
                    // can be parsed in the same way as a function definition, hence transition
                    // visiting the current node as if it was a function definition;
                    visit_function_definition(node, contents, class_info, visibility);
                    was_function = true;
                    continue;
                }
            }
            strip_decorator_chars(type_info.name);
            continue;
        }

        // explore nested references
        if (strcmp(child_type, "reference_declarator") == 0) {
            TSNode identifier_node = ts_node_child(child, 1);
            const char* identifier_type = ts_node_type(identifier_node);

            // case where the reference for a variable member field of the class
            if (strcmp(identifier_type, "field_identifier") == 0) {
                TSNode ref_node = ts_node_child(child, 0);
                type_info.name.append(wts_node_lexeme(ref_node, contents));
                variables.emplace_back(wts_node_lexeme(identifier_node, contents), type_info, visibility);
                strip_decorator_chars(type_info.name);
            }

            // case where the reference is for a method of the class
            if (strcmp(identifier_type, "function_declarator") == 0) {
                visit_function_definition(node, contents, class_info, visibility);
                was_function = true;
                continue;
            }
        }
    }

    // if the field was not a function, use the previously stored relation type as a relation to the given class.
    if (!was_function) {
        // insert relationships for all non-primitive types and any template parameters
        if (!type_info.primitive) {
            max_relation_emplace(class_info->relationships, type_info.name, member_relation_type);
        }
        for (const TypeInfo& template_tinfo : type_info.template_parameters) {
            RelationshipType template_relationship = deduce_member_relation_type(template_tinfo.name);
            max_relation_emplace(class_info->relationships, template_tinfo.name, template_relationship);
        }
    }
    class_info->variables.insert(class_info->variables.end(), variables.begin(), variables.end());
}

void visit_field_declaration_list(TSNode node, const std::string& contents, ClassInfo* class_info) {
    // init the current visibility to the C++ default for structs/classes i.e. private for classes and public for structs
    VisibilityType current_visibility = class_info->type == ClassType::STRUCT ? VisibilityType::PUBLIC : VisibilityType::PRIVATE;

    uint32_t child_count = ts_node_child_count(node);
    for (uint32_t i = 0; i < child_count; i++) {
        TSNode child = ts_node_child(node, i);
        const char* child_type = ts_node_type(child);

        // visibility specifiers e.g. PUBLIC, PRIVATE, PROTECTED
        if (strcmp(child_type, "access_specifier") == 0) {
            std::string specifier = wts_node_lexeme(child, contents);
            current_visibility = encode_access_specifier(specifier);
            continue;
        }

        if (strcmp(child_type, "function_definition") == 0) {
            visit_function_definition(child, contents, class_info, current_visibility);
            continue;
        }

        if (strcmp(child_type, "field_declaration") == 0 || strcmp(child_type, "declaration") == 0) {
            // note: I think the "declaration" node_type case only occurs preceding/wrapping a destructor declaration
            visit_field_declaration(child, contents, class_info, current_visibility);
            continue;
        }
    }
}

void visit_base_class_clause(TSNode node, const std::string& contents, ClassInfo* class_info) {
    uint32_t child_count = ts_node_child_count(node);
    for (uint32_t i = 0; i < child_count; i++) {
        TSNode child = ts_node_child(node, i);
        const char* child_type = ts_node_type(child);

        if (strcmp(child_type, "type_identifier") == 0) {
            std::string parent_class_name = wts_node_lexeme(child, contents);
            max_relation_emplace(class_info->relationships, parent_class_name, RelationshipType::INHERITANCE);
        }
    }
}

ClassInfo visit_class_specifier(TSNode node, const std::string& contents) {
    ClassInfo class_info;

    const char* node_type = ts_node_type(node);
    if (strcmp(node_type, "class_specifier") == 0) {
        class_info.type = ClassType::CLASS;
    }
    if (strcmp(node_type, "struct_specifier") == 0) {
        class_info.type = ClassType::STRUCT;
    }

    uint32_t child_count = ts_node_child_count(node);
    for (uint32_t i = 0; i < child_count; i++) {
        TSNode child = ts_node_child(node, i);
        const char* child_type = ts_node_type(child);

        // class name
        if (strcmp(child_type, "type_identifier") == 0) {
            std::string name = wts_node_lexeme(child, contents);
            class_info.name = std::move(name);
            continue;
        }

        if (strcmp(child_type, "base_class_clause") == 0) {
            visit_base_class_clause(child, contents, &class_info);
            continue;
        }

        // the set of function/variable members of the class
        if (strcmp(child_type, "field_declaration_list") == 0) {
            visit_field_declaration_list(child, contents, &class_info);
            continue;
        }
    }
    return class_info;
}