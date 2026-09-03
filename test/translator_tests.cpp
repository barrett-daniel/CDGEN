#include <doctest/doctest.h>
#include "intermediate_representation.hpp"
#include "../src/util/run_configuration.hpp"
#include "../src/translator/translator.hpp"


// Minimal, deterministic RunConfiguration builder. input_dir/output_fname
// are required fields but unused by the translator, so any value is fine.
RunConfiguration make_config(bool hide_empty_diagram_sections = true, bool display_functions_first = true) {
    RunConfiguration cfg;
    cfg.input_dir = "in";
    cfg.output_fname = "out";
    cfg.hide_empty_diagram_sections = hide_empty_diagram_sections;
    cfg.display_functions_first = display_functions_first;
    return cfg;
}

TypeInfo make_type(std::string name, bool primitive = true) {
    TypeInfo t;
    t.name = std::move(name);
    t.primitive = primitive;
    return t;
}

ClassInfo make_empty_class(std::string name) {
    ClassInfo c;
    c.name = std::move(name);
    c.type = ClassType::CLASS;
    return c;
}

// ---------------------------------------------------------------------

TEST_SUITE("Translator Tests - Overall Structure") {
    TEST_CASE("001 Empty Class Produces Empty Diagram") {
        auto result = translator::to_gviz_file({}, make_config());

        CHECK_NE(result.find("digraph {"), std::string::npos);

        // The closing brace should come after the opening one.
        auto open_pos = result.find("digraph {");
        auto close_pos = result.find('}');
        REQUIRE_NE(close_pos, std::string::npos);
        CHECK_GT(close_pos, open_pos);

        // Nothing class-related should be present.
        CHECK_EQ(result.find("// Class:"), std::string::npos);
    }

    TEST_CASE("002 Class, Relation, and Node Specification Comment Blocks Are Emitted") {
        auto result = translator::to_gviz_file({make_empty_class("Foo")}, make_config());

        CHECK_NE(result.find("// Class: Foo"), std::string::npos);
        CHECK_NE(result.find("// Relations"), std::string::npos);
        CHECK_NE(result.find("// Node Specification"), std::string::npos);
    }

    TEST_CASE("003 Class Name is Bold") {
        auto result = translator::to_gviz_file({make_empty_class("Widget")}, make_config());

        CHECK_NE(result.find("Widget ["), std::string::npos);
        CHECK_NE(result.find("shape=plain"), std::string::npos);
        CHECK_NE(result.find("<b> Widget </b>"), std::string::npos);
    }

    TEST_CASE("004 Multiple Classes are Parsed in Translation") {
        std::vector<ClassInfo> classes{make_empty_class("Alpha"), make_empty_class("Beta")};
        auto result = translator::to_gviz_file(classes, make_config());

        auto alpha_pos = result.find("// Class: Alpha");
        auto beta_pos = result.find("// Class: Beta");
        REQUIRE_NE(alpha_pos, std::string::npos);
        REQUIRE_NE(beta_pos, std::string::npos);
        CHECK_LT(alpha_pos, beta_pos); // alpha comes before beta
    }
}

// ---------------------------------------------------------------------

TEST_SUITE("Translator Tests - Run Configuration Changes") {

    // test cases for these two parameters are combined as they have interacting logic and
    // we can generate the necessary parameterised tests to check for all interactions/flag options
    TEST_CASE("001 - show_members and show_non_public_members") {
        VisibilityType method_visibility = GENERATE(VisibilityType::PUBLIC, VisibilityType::PRIVATE);
        bool show_members = GENERATE(true, false);
        bool show_non_public_members = GENERATE(true, false);

        // construct data class and member data structures
        ClassInfo c = make_empty_class("Foo");
        MethodInfo m;
        m.name = "doStuff";
        m.type = make_type("void");
        m.visibility = method_visibility; // variable method visibility
        c.methods.push_back(m);
        c.variables.push_back(VariableInfo{"field", make_type("int"), VisibilityType::PUBLIC});

        RunConfiguration config = make_config();
        config.show_members = show_members;
        config.show_non_public_members = show_non_public_members;

        auto result = translator::to_gviz_file({c}, config);

        bool method_expected_visible = show_members && (method_visibility == VisibilityType::PUBLIC || show_non_public_members);
        bool var_expected_visible = show_members;

        std::string subcase_label = std::string() +
            "visibility=" + (method_visibility == VisibilityType::PUBLIC ? "PUBLIC" : "PRIVATE") +
            ", show_members=" + (show_members ? "TRUE" : "FALSE") +
            ", show_non_public_members=" + (show_non_public_members ? "TRUE" : "FALSE");

        SUBCASE(subcase_label) {
            auto method_pos = result.find("doStuff(");
            auto var_pos = result.find(": field");

            if (method_expected_visible) CHECK_NE(method_pos, std::string::npos);
            else CHECK_EQ(method_pos, std::string::npos);

            if (var_expected_visible) CHECK_NE(var_pos, std::string::npos);
            else CHECK_EQ(var_pos, std::string::npos);
        }
    }

    TEST_CASE("002 - display_functions_first") {
        ClassInfo c = make_empty_class("Foo");
        MethodInfo m;
        m.name = "doStuff";
        m.type = make_type("void");
        m.visibility = VisibilityType::PUBLIC;
        c.methods.push_back(m);
        c.variables.push_back(VariableInfo{"field", make_type("int"), VisibilityType::PUBLIC});

        bool functions_first = GENERATE(true, false);
        auto result = translator::to_gviz_file({c}, make_config(true, functions_first));

        std::string subcase_label = std::string("Case: ") + (functions_first ? "TRUE" : "FALSE");
        SUBCASE(subcase_label) {
            auto method_pos = result.find("doStuff(");
            auto var_pos = result.find(": field");
            REQUIRE_NE(method_pos, std::string::npos);
            REQUIRE_NE(var_pos, std::string::npos);

            if (functions_first) { CHECK_LT(method_pos, var_pos); }
            else                 { CHECK_GT(method_pos, var_pos); }
        }
    }

    TEST_CASE("003 - hide_empty_diagram_sections") {
        bool hide_empty_diagram_sections = GENERATE(true, false);
        auto result = translator::to_gviz_file({make_empty_class("Foo")}, make_config(hide_empty_diagram_sections));

        std::string subcase_label = std::string("Case: ") + (hide_empty_diagram_sections ? "TRUE" : "FALSE");
        SUBCASE(subcase_label) {

            if (hide_empty_diagram_sections) {
                // No property-table wrapper and no placeholder row should appear at all.
                CHECK_EQ(result.find("cellborder=\"0\""), std::string::npos);
                CHECK_EQ(result.find("<tr> <td> </td> </tr>"), std::string::npos);
            } else {
                // Find each placeholder row
                uint8_t count = 0;
                size_t pos = 0;
                while ((pos = result.find("<tr> <td> </td> </tr>", pos)) != std::string::npos) {
                    ++count;
                    pos += 1;
                }

                // check only two were found
                CHECK_EQ(count, 2u);
            }
        }
    }
}

// ---------------------------------------------------------------------

TEST_SUITE("Translator Tests - Variable Formatting") {
    TEST_CASE("001 Variable visibility decorators: + (public) - (private) # (protected)") {
        ClassInfo c = make_empty_class("Foo");
        c.variables.push_back(VariableInfo{"pubVar", make_type("int"), VisibilityType::PUBLIC});
        c.variables.push_back(VariableInfo{"privVar", make_type("double"), VisibilityType::PRIVATE});
        c.variables.push_back(VariableInfo{"protVar", make_type("float"), VisibilityType::PROTECTED});
        auto result = translator::to_gviz_file({c}, make_config());

        CHECK_NE(result.find("+ <b>int</b> : pubVar"), std::string::npos);
        CHECK_NE(result.find("- <b>double</b> : privVar"), std::string::npos);
        CHECK_NE(result.find("# <b>float</b> : protVar"), std::string::npos);
    }

    TEST_CASE("002 Variables of Like-Type and Like-Visibility are Grouped") {
        ClassInfo c = make_empty_class("Foo");
        c.variables.push_back(VariableInfo{"a", make_type("int"), VisibilityType::PUBLIC});
        c.variables.push_back(VariableInfo{"b", make_type("int"), VisibilityType::PUBLIC});
        auto result = translator::to_gviz_file({c}, make_config());

        CHECK_NE(result.find("+ <b>int</b> : a, b"), std::string::npos);
        // Should NOT appear as two separate rows.
        CHECK_EQ(result.find("+ <b>int</b> : a</td>"), std::string::npos);
    }

    TEST_CASE("003 Variables of Like-Type but Different Visibility are Not Grouped") {
        ClassInfo c = make_empty_class("Foo");
        c.variables.push_back(VariableInfo{"a", make_type("int"), VisibilityType::PUBLIC});
        c.variables.push_back(VariableInfo{"b", make_type("int"), VisibilityType::PRIVATE});
        auto result = translator::to_gviz_file({c}, make_config());

        CHECK_NE(result.find("+ <b>int</b> : a"), std::string::npos);
        CHECK_NE(result.find("- <b>int</b> : b"), std::string::npos);
        CHECK_EQ(result.find("a, b"), std::string::npos);
        CHECK_EQ(result.find("b, a"), std::string::npos);
    }
}

// ---------------------------------------------------------------------

TEST_SUITE("Translator Tests - Method Formatting") {
    TEST_CASE("001 Method with No Parameters") {
        ClassInfo c = make_empty_class("Foo");
        MethodInfo m;
        m.name = "doStuff";
        m.type = make_type("void");
        m.visibility = VisibilityType::PUBLIC;
        c.methods.push_back(m);

        auto result = translator::to_gviz_file({c}, make_config());

        CHECK_NE(result.find("+ <b>void</b> doStuff("), std::string::npos);
        // empty parameter list -> nothing between the parens
        CHECK_NE(result.find("doStuff()"), std::string::npos);
    }

    TEST_CASE("002 Method Parameters are Bold") {
        ClassInfo c = make_empty_class("Foo");
        MethodInfo m;
        m.name = "add";
        m.type = make_type("int");
        m.visibility = VisibilityType::PRIVATE;
        m.parameters.push_back(IdentifierInfo{"x", make_type("int")});
        m.parameters.push_back(IdentifierInfo{"y", make_type("int")});
        c.methods.push_back(m);

        auto result = translator::to_gviz_file({c}, make_config());

        CHECK_NE(result.find("- <b>int</b> add("), std::string::npos);
        CHECK_NE(result.find("<b>int</b> x, <b>int</b> y"), std::string::npos);
    }

    TEST_CASE("003 Prefix and Postfix Qualifiers Correctly Positioned") {
        ClassInfo c = make_empty_class("Foo");
        MethodInfo m;
        m.name = "getCount";
        m.type = make_type("int");
        m.type.qualifiers.push_back(QualifierInfo{QualifierPosition::PREFIX, "static"});
        m.type.qualifiers.push_back(QualifierInfo{QualifierPosition::POSTFIX, "const"});
        m.visibility = VisibilityType::PUBLIC;
        c.methods.push_back(m);

        auto result = translator::to_gviz_file({c}, make_config());

        // Prefix qualifier is folded into the same bold block as the return type.
        CHECK_NE(result.find("<b>static int</b> getCount"), std::string::npos);

        // Postfix qualifier appears after the closing paren, as its own bold block.
        auto paren_pos = result.find("getCount()");
        auto const_pos = result.find("<b>const", paren_pos == std::string::npos ? 0 : paren_pos);
        REQUIRE_NE(paren_pos, std::string::npos);
        REQUIRE_NE(const_pos, std::string::npos);
        CHECK_GT(const_pos, paren_pos);
    }
}

// ---------------------------------------------------------------------

TEST_SUITE("Translator Tests - Type Formatting") {
    TEST_CASE("001 Template Parameters are Included and XML-Escaped") {
        ClassInfo c = make_empty_class("Foo");
        TypeInfo vec_of_int = make_type("vector", false);
        vec_of_int.template_parameters.push_back(make_type("int"));
        c.variables.push_back(VariableInfo{"data", vec_of_int, VisibilityType::PUBLIC});
        auto result = translator::to_gviz_file({c}, make_config());

        // "<" / ">" from the template syntax must be escaped for the XML label.
        CHECK_NE(result.find("vector&lt;int&gt;"), std::string::npos);
        CHECK_EQ(result.find("vector<int>"), std::string::npos);
    }

    TEST_CASE("002 Nested Template Parameters") {
        ClassInfo c = make_empty_class("Foo");
        TypeInfo inner = make_type("vector", false);
        inner.template_parameters.push_back(make_type("int"));
        TypeInfo outer = make_type("map", false);
        outer.template_parameters.push_back(make_type("string", false));
        outer.template_parameters.push_back(inner);

        c.variables.push_back(VariableInfo{"data", outer, VisibilityType::PUBLIC});

        auto result = translator::to_gviz_file({c}, make_config());

        CHECK_NE(result.find("map&lt;string, vector&lt;int&gt;"),  std::string::npos);
    }
}

// ---------------------------------------------------------------------

TEST_SUITE("Translator Tests - Relationships") {
    TEST_CASE("001 INHERITANCE Relation Correctly Translated") {
        ClassInfo c = make_empty_class("Derived");
        c.relationships["Base"] = RelationshipType::INHERITANCE;

        auto result = translator::to_gviz_file({c}, make_config());

        CHECK_NE(result.find("edge [dir=forward arrowtail=vee]"), std::string::npos);
        CHECK_NE(result.find("\"Derived\" -> \"Base\""), std::string::npos);
    }

    TEST_CASE("002 DEPENDENCY Relation Correctly Translated") {
        ClassInfo c = make_empty_class("A");
        c.relationships["B"] = RelationshipType::DEPENDENCY;

        auto result = translator::to_gviz_file({c}, make_config());

        CHECK_NE(result.find("edge [dir=none]"), std::string::npos);
        CHECK_NE(result.find("\"A\" -> \"B\""), std::string::npos);
    }

    TEST_CASE("003 AGGREGATION Relation Correctly Translated") {
        ClassInfo c = make_empty_class("Whole");
        c.relationships["Part"] = RelationshipType::AGGREGATION;

        auto result = translator::to_gviz_file({c}, make_config());

        CHECK_NE(result.find("edge [dir=back arrowtail=odiamond]"), std::string::npos);
        CHECK_NE(result.find("\"Whole\" -> \"Part\""), std::string::npos);
    }

    TEST_CASE("004 COMPOSITION Relation Correctly Translated") {
        ClassInfo c = make_empty_class("Whole");
        c.relationships["Part"] = RelationshipType::COMPOSITION;

        auto result = translator::to_gviz_file({c}, make_config());

        CHECK_NE(result.find("edge [dir=back arrowtail=diamond]"), std::string::npos);
        CHECK_NE(result.find("\"Whole\" -> \"Part\""), std::string::npos);
    }

    TEST_CASE("005 No Relations is Translated with No Edges") {
        auto result = translator::to_gviz_file({make_empty_class("Lonely")}, make_config());

        CHECK_EQ(result.find("edge ["), std::string::npos);
        CHECK_EQ(result.find("->"), std::string::npos);
    }
}

