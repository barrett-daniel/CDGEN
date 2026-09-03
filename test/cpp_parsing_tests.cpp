#include <string>
#include <vector>

#include <doctest/doctest.h>
#include "intermediate_representation.hpp"
#include "../src/parser/parser.hpp"

TEST_SUITE("C++ Parsing Tests - Class Identification") {
    TEST_CASE("001 Empty Class") {
        std::vector<std::string> input = {R"(
            class Foo {};
        )"};
        std::vector<ClassInfo> result = parser::parse_cpp(input);

        REQUIRE_EQ(result.size(), 1ull);
        CHECK_EQ(result[0].name, "Foo");
        CHECK_EQ(result[0].type, ClassType::CLASS);
        CHECK(result[0].methods.empty());
        CHECK(result[0].variables.empty());
        CHECK(result[0].relationships.empty());
        CHECK(result[0].nested_classes.empty());
    }

    TEST_CASE("002 Multiple Classes Per File") {
        std::vector<std::string> input = {R"(
            class Foo {};
            class Bar {};
        )"};
        std::vector<ClassInfo> result = parser::parse_cpp(input);

        REQUIRE_EQ(result.size(), 2ull);
        CHECK_EQ(result[0].name, "Foo");
        CHECK_EQ(result[1].name, "Bar");
    }

    TEST_CASE("003 Multiple Classes Across Files") {
        std::vector<std::string> input = {R"(
            class Foo {};
        )", R"(
            class Bar {};
        )"};
        std::vector<ClassInfo> result = parser::parse_cpp(input);

        REQUIRE_EQ(result.size(), 2ull);
        CHECK_EQ(result[0].name, "Foo");
        CHECK_EQ(result[1].name, "Bar");
    }

    TEST_CASE("004 Struct") {
        std::vector<std::string> input = {R"(
            struct Foo {};
        )"};
        std::vector<ClassInfo> result = parser::parse_cpp(input);

        REQUIRE_EQ(result.size(), 1ull);
        CHECK_EQ(result[0].name, "Foo");
        CHECK_EQ(result[0].type, ClassType::STRUCT);
    }
}

TEST_SUITE("C++ Parsing Tests - Methods and Variables") {
    TEST_CASE("001 Class With Methods") {
        std::vector<std::string> input = {R"(
            class Foo {
                void do_nothing();
                void do_nothing_again() {
                    std::count << "nothing";
                }
            };
        )"};
        std::vector<ClassInfo> result = parser::parse_cpp(input);

        REQUIRE_EQ(result.size(), 1ull);
        REQUIRE_EQ(result[0].methods.size(), 2ull);
        CHECK_EQ(result[0].methods[0].name, "do_nothing");
        CHECK_EQ(result[0].methods[1].name, "do_nothing_again");
    }

    TEST_CASE("002 Class With Variables") {
        std::vector<std::string> input = {R"(
            class Foo {
                int i, j;
            };
        )"};
        std::vector<ClassInfo> result = parser::parse_cpp(input);

        REQUIRE_EQ(result.size(), 1ull);
        REQUIRE_EQ(result[0].variables.size(), 2ull);

        VariableInfo& var1 = result[0].variables[0];
        VariableInfo& var2 = result[0].variables[1];
        CHECK_EQ(var1.name, "i");
        CHECK_EQ(var2.name, "j");
        CHECK_EQ(var1.type.name, "int");
        CHECK_EQ(var2.type.name, "int");
    }

    TEST_CASE("003 Class With Variables And Methods") {
        std::vector<std::string> input = {R"(
            class Foo {
                void do_nothing();
                int i;
            };
        )"};
        std::vector<ClassInfo> result = parser::parse_cpp(input);

        REQUIRE_EQ(result.size(), 1ull);
        REQUIRE_EQ(result[0].variables.size(), 1ull);
        REQUIRE_EQ(result[0].methods.size(), 1ull);

        VariableInfo& var1 = result[0].variables[0];
        CHECK_EQ(var1.name, "i");
        CHECK_EQ(var1.type.name, "int");
        CHECK_EQ(result[0].methods[0].name, "do_nothing");
    }

    TEST_CASE("004 Method Return Types") {
        std::vector<std::string> input = {R"(
            class Foo {
                int get_int();
            };
        )"};
        std::vector<ClassInfo> result = parser::parse_cpp(input);

        REQUIRE_EQ(result.size(), 1ull);
        REQUIRE_EQ(result[0].methods.size(), 1ull);
        CHECK_EQ(result[0].methods[0].type.name, "int");
    }

    TEST_CASE("005 Method Parameters") {
        std::vector<std::string> input = {R"(
            class Foo {
                void do_nothing(int i);
            };
        )"};
        std::vector<ClassInfo> result = parser::parse_cpp(input);

        REQUIRE_EQ(result.size(), 1ull);
        REQUIRE_EQ(result[0].methods.size(), 1ull);
        REQUIRE_EQ(result[0].methods[0].parameters.size(), 1ull);

        CHECK_EQ(result[0].methods[0].parameters[0].type.name, "int");
    }

    TEST_CASE("006 Template Return Value") {
        std::vector<std::string> input = {R"(
            class Foo {
                std::vector<int> get_vector();
            };
        )"};
        std::vector<ClassInfo> result = parser::parse_cpp(input);

        REQUIRE_EQ(result.size(), 1ull);
        REQUIRE_EQ(result[0].methods.size(), 1ull);

        TypeInfo& return_type = result[0].methods[0].type;
        CHECK_EQ(return_type.name, "std::vector");
        REQUIRE_EQ(return_type.template_parameters.size(), 1ull);
        CHECK_EQ(return_type.template_parameters[0].name, "int");
    }

    TEST_CASE("007 Template Parameter Value") {
        std::vector<std::string> input = {R"(
            class Foo {
                void do_nothing(std::vector<float> foo);
            };
        )"};
        std::vector<ClassInfo> result = parser::parse_cpp(input);

        REQUIRE_EQ(result.size(), 1ull);
        REQUIRE_EQ(result[0].methods.size(), 1ull);
        REQUIRE_EQ(result[0].methods[0].parameters.size(), 1ull);

        TypeInfo& parameter_type = result[0].methods[0].parameters[0].type;
        CHECK_EQ(parameter_type.name, "std::vector");
        REQUIRE_EQ(parameter_type.template_parameters.size(), 1ull);
        CHECK_EQ(parameter_type.template_parameters[0].name, "float");
    }

    TEST_CASE("008 Pointer Return Value") {
        std::vector<std::string> input = {R"(
            class Foo {
                int* get_int_ptr();
            };
        )"};
        std::vector<ClassInfo> result = parser::parse_cpp(input);

        REQUIRE_EQ(result.size(), 1ull);
        REQUIRE_EQ(result[0].methods.size(), 1ull);
        CHECK_EQ(result[0].methods[0].type.name, "int*");
    }

    TEST_CASE("009 Pointer Parameter") {
        std::vector<std::string> input = {R"(
            class Foo {
                void go_nothing(int* i);
            };
        )"};
        std::vector<ClassInfo> result = parser::parse_cpp(input);

        REQUIRE_EQ(result.size(), 1ull);
        REQUIRE_EQ(result[0].methods.size(), 1ull);
        REQUIRE_EQ(result[0].methods[0].parameters.size(), 1ull);
        CHECK_EQ(result[0].methods[0].parameters[0].type.name, "int*");
    }

    TEST_CASE("010 Pointer Member Variable") {
        std::vector<std::string> input = {R"(
            class Foo {
                int i, *j, k;
                float* l, m;
            };
        )"};
        std::vector<ClassInfo> result = parser::parse_cpp(input);

        REQUIRE_EQ(result.size(), 1ull);
        REQUIRE_EQ(result[0].variables.size(), 5ull);

        CHECK_EQ(result[0].variables[0].type.name, "int");
        CHECK_EQ(result[0].variables[1].type.name, "int*");
        CHECK_EQ(result[0].variables[2].type.name, "int");
        CHECK_EQ(result[0].variables[3].type.name, "float*");
        CHECK_EQ(result[0].variables[4].type.name, "float");
    }

    TEST_CASE("011 Double/Triple/etc. Pointers") {
        std::vector<std::string> input = {R"(
            class Foo {
                int*** get_int_ptrptr(float**** i);
                int i, **j, k;
            };
        )"};
        std::vector<ClassInfo> result = parser::parse_cpp(input);

        REQUIRE_EQ(result.size(), 1ull);

        // variables
        REQUIRE_EQ(result[0].variables.size(), 3ull);
        CHECK_EQ(result[0].variables[0].type.name, "int");
        CHECK_EQ(result[0].variables[1].type.name, "int**");
        CHECK_EQ(result[0].variables[2].type.name, "int");

        // return value
        REQUIRE_EQ(result[0].methods.size(), 1ull);
        CHECK_EQ(result[0].methods[0].type.name, "int***");

        // parameter
        REQUIRE_EQ(result[0].methods[0].parameters.size(), 1ull);
        CHECK_EQ(result[0].methods[0].parameters[0].type.name, "float****");
    }

    TEST_CASE("012 Reference Return Value") {
        std::vector<std::string> input = {R"(
            class Foo {
                int& get_int_ref();
            };
        )"};
        std::vector<ClassInfo> result = parser::parse_cpp(input);

        REQUIRE_EQ(result.size(), 1ull);
        REQUIRE_EQ(result[0].methods.size(), 1ull);
        CHECK_EQ(result[0].methods[0].type.name, "int&");
    }

    TEST_CASE("013 Reference Parameters Value") {
        std::vector<std::string> input = {R"(
            class Foo {
                void do_nothing(int& i);
            };
        )"};
        std::vector<ClassInfo> result = parser::parse_cpp(input);

        REQUIRE_EQ(result.size(), 1ull);
        REQUIRE_EQ(result[0].methods.size(), 1ull);
        REQUIRE_EQ(result[0].methods[0].parameters.size(), 1ull);
        CHECK_EQ(result[0].methods[0].parameters[0].type.name, "int&");
    }

    TEST_CASE("014 Reference Variables") {
        std::vector<std::string> input = {R"(
            class Foo {
                int i, &j, k;
                float& l, m;
            };
        )"};
        std::vector<ClassInfo> result = parser::parse_cpp(input);

        REQUIRE_EQ(result.size(), 1ull);
        REQUIRE_EQ(result[0].variables.size(), 5ull);

        CHECK_EQ(result[0].variables[0].type.name, "int");
        CHECK_EQ(result[0].variables[1].type.name, "int&");
        CHECK_EQ(result[0].variables[2].type.name, "int");
        CHECK_EQ(result[0].variables[3].type.name, "float&");
        CHECK_EQ(result[0].variables[4].type.name, "float");
    }

    TEST_CASE("015 R-value References") {
        std::vector<std::string> input = {R"(
            class Foo {
                int&& get_int_refref(float&& i);
            };
        )"};
        std::vector<ClassInfo> result = parser::parse_cpp(input);

        REQUIRE_EQ(result.size(), 1ull);

        // return value
        REQUIRE_EQ(result[0].methods.size(), 1ull);
        CHECK_EQ(result[0].methods[0].type.name, "int&&");

        // parameter
        REQUIRE_EQ(result[0].methods[0].parameters.size(), 1ull);
        CHECK_EQ(result[0].methods[0].parameters[0].type.name, "float&&");
    }

    TEST_CASE("016 Method Qualifiers") {
        struct QualifierCase {
            std::string member_decl;
            std::string qualifier_name;
            QualifierPosition position;
        };

        QualifierCase constexpr_case = {"constexpr void do_nothing();", "constexpr", QualifierPosition::PREFIX};
        QualifierCase static_case    = {"static void do_nothing();",    "static",    QualifierPosition::PREFIX};
        QualifierCase virtual_case   = {"virtual void do_nothing();",   "virtual",   QualifierPosition::PREFIX};
        QualifierCase override_case  = {"void do_nothing() override;",  "override",  QualifierPosition::POSTFIX};
        QualifierCase const_case     = {"void do_nothing() const;",     "const",     QualifierPosition::POSTFIX};
        auto cases = GENERATE(constexpr_case, static_case, virtual_case, override_case, const_case);

        SUBCASE(cases.qualifier_name) {
            std::vector<std::string> input = {
                "class Foo {\n    " + cases.member_decl + "\n};\n"
            };

            std::vector<ClassInfo> result = parser::parse_cpp(input);

            REQUIRE_EQ(result.size(), 1ull);
            REQUIRE_EQ(result[0].methods.size(), 1ull);
            REQUIRE_EQ(result[0].methods[0].type.qualifiers.size(), 1ull);

            QualifierInfo& method_qualifier = result[0].methods[0].type.qualifiers[0];
            CHECK_EQ(method_qualifier.name, cases.qualifier_name);
            CHECK_EQ(method_qualifier.position, cases.position);
        }
    }

    TEST_CASE("017 Const Parameter") {
        std::vector<std::string> input = {R"(
            class Foo {
                void do_nothing(const int i);
            };
        )"};
        std::vector<ClassInfo> result = parser::parse_cpp(input);

        REQUIRE_EQ(result.size(), 1ull);
        REQUIRE_EQ(result[0].methods.size(), 1ull);
        REQUIRE_EQ(result[0].methods[0].parameters.size(), 1ull);
        REQUIRE_EQ(result[0].methods[0].parameters[0].type.qualifiers.size(), 1ull);

        QualifierInfo& parameter_qualifier = result[0].methods[0].parameters[0].type.qualifiers[0];
        CHECK_EQ(parameter_qualifier.name, "const");
        CHECK_EQ(parameter_qualifier.position, QualifierPosition::PREFIX);
    }

    TEST_CASE("018 Const Reference Parameter") {
        std::vector<std::string> input = {R"(
            class Foo {
                void do_nothing(const int& i);
            };
        )"};
        std::vector<ClassInfo> result = parser::parse_cpp(input);

        REQUIRE_EQ(result.size(), 1ull);
        REQUIRE_EQ(result[0].methods.size(), 1ull);
        REQUIRE_EQ(result[0].methods[0].parameters.size(), 1ull);
        REQUIRE_EQ(result[0].methods[0].parameters[0].type.qualifiers.size(), 1ull);

        QualifierInfo& parameter_qualifier = result[0].methods[0].parameters[0].type.qualifiers[0];
        CHECK_EQ(parameter_qualifier.name, "const");
        CHECK_EQ(parameter_qualifier.position, QualifierPosition::PREFIX);

        CHECK_EQ(result[0].methods[0].parameters[0].type.name, "int&");
    }

    TEST_CASE("019 Nameless Parameter in Method Declaration") {
        std::vector<std::string> input = {R"(
            class Foo {
                void do_nothing(int);
            };
        )"};
        std::vector<ClassInfo> result = parser::parse_cpp(input);

        REQUIRE_EQ(result.size(), 1ull);
        REQUIRE_EQ(result[0].methods.size(), 1ull);
        REQUIRE_EQ(result[0].methods[0].parameters.size(), 1ull);

        CHECK_EQ(result[0].methods[0].parameters[0].type.name, "int");
    }

    TEST_CASE("020 Constructors") {
        std::vector<std::string> input = {R"(
            class Foo {
                Foo(int _x) : x(_x) {}
                int x;
            };

            class Bar {
                Bar(int x) {
                    i = x * SCALE;
                }

                int i;
            };
        )"};
        std::vector<ClassInfo> result = parser::parse_cpp(input);

        REQUIRE_EQ(result.size(), 2ull);

        REQUIRE_EQ(result[0].methods.size(), 1ull);
        REQUIRE_EQ(result[0].methods[0].parameters.size(), 1ull);
        CHECK_EQ(result[0].methods[0].name, "");
        CHECK_EQ(result[0].methods[0].type.name, "Foo");
        CHECK_EQ(result[0].methods[0].parameters[0].type.name, "int");

        REQUIRE_EQ(result[1].methods.size(), 1ull);
        REQUIRE_EQ(result[1].methods[0].parameters.size(), 1ull);
        CHECK_EQ(result[1].methods[0].name, "");
        CHECK_EQ(result[1].methods[0].type.name, "Bar");
        CHECK_EQ(result[1].methods[0].parameters[0].type.name, "int");
    }

    TEST_CASE("021 Destructors") {
        std::vector<std::string> input = {R"(
            class Foo {
                ~Foo();
            };
        )"};
        std::vector<ClassInfo> result = parser::parse_cpp(input);

        REQUIRE_EQ(result.size(), 1ull);
        REQUIRE_EQ(result[0].methods.size(), 1ull);
        CHECK_EQ(result[0].methods[0].name, "");
        CHECK_EQ(result[0].methods[0].type.name, "~Foo");
    }

    TEST_CASE("022 Namespace Types") {
        std::vector<std::string> input = {R"(
            class Foo {
                std::string to_string();
            };
        )"};
        std::vector<ClassInfo> result = parser::parse_cpp(input);

        REQUIRE_EQ(result.size(), 1ull);
        REQUIRE_EQ(result[0].methods.size(), 1ull);

        CHECK_EQ(result[0].methods[0].type.name, "std::string");
    }

    TEST_CASE("023 Operators") {
        std::vector<std::string> input = {R"(
            class Foo {
                Foo operator+(Foo a);
            };
        )"};
        std::vector<ClassInfo> result = parser::parse_cpp(input);

        REQUIRE_EQ(result.size(), 1ull);
        REQUIRE_EQ(result[0].methods.size(), 1ull);
        REQUIRE_EQ(result[0].methods[0].parameters.size(), 1ull);

        CHECK_EQ(result[0].methods[0].parameters[0].type.name, "Foo");
        CHECK_EQ(result[0].methods[0].type.name, "Foo");
        CHECK_EQ(result[0].methods[0].name, "operator+");
    }
}

TEST_SUITE("C++ Parsing Tests - Visibility Tests") {
    TEST_CASE("001 Default Class Visibility") {
        std::vector<std::string> input = {R"(
            class Foo {
                int i;
            };
        )"};
        std::vector<ClassInfo> result = parser::parse_cpp(input);

        REQUIRE_EQ(result.size(), 1ull);
        REQUIRE_EQ(result[0].variables.size(), 1ull);
        CHECK_EQ(result[0].variables[0].visibility, VisibilityType::PRIVATE);
    }

    TEST_CASE("002 Default Struct Visibility") {
        std::vector<std::string> input = {R"(
            struct Foo {
                int i;
            };
        )"};
        std::vector<ClassInfo> result = parser::parse_cpp(input);

        REQUIRE_EQ(result.size(), 1ull);
        REQUIRE_EQ(result[0].variables.size(), 1ull);
        CHECK_EQ(result[0].variables[0].visibility, VisibilityType::PUBLIC);
    }

    TEST_CASE("003 Mixed Visibility Types") {
        std::vector<std::string> input = {R"(
            class Foo {
                public:
                    int i;
                private:
                    int j;
                protected:
                    int k;
            };
        )"};
        std::vector<ClassInfo> result = parser::parse_cpp(input);

        REQUIRE_EQ(result.size(), 1ull);
        REQUIRE_EQ(result[0].variables.size(), 3ull);
        CHECK_EQ(result[0].variables[0].visibility, VisibilityType::PUBLIC);
        CHECK_EQ(result[0].variables[1].visibility, VisibilityType::PRIVATE);
        CHECK_EQ(result[0].variables[2].visibility, VisibilityType::PROTECTED);
    }
}

TEST_SUITE("C++ Parsing Tests - Relationship Tests") {

    void relation_test_checks(const std::vector<ClassInfo>& result, RelationshipType expected_relation) {
        REQUIRE_EQ(result.size(), 1ull);
        REQUIRE_EQ(result[0].relationships.size(), 1ull);
        REQUIRE(result[0].relationships.contains("Bar"));
        CHECK_EQ(result[0].relationships.at("Bar"), expected_relation);
    }

    TEST_CASE("001 INHERITANCE Relations") {
        std::vector<std::string> input = {R"(
            class Foo : Bar {};
        )"};
        std::vector<ClassInfo> result = parser::parse_cpp(input);
        relation_test_checks(result, RelationshipType::INHERITANCE);
    }

    TEST_CASE("002 Method Return Values cause DEPENDENCY Relations") {
        std::vector<std::string> input = {R"(
            class Foo {
                Bar do_nothing();
            };
        )"};
        std::vector<ClassInfo> result = parser::parse_cpp(input);
        relation_test_checks(result, RelationshipType::DEPENDENCY);
    }

    TEST_CASE("003 Parameters cause DEPENDENCY Relations") {
        std::vector<std::string> input = {R"(
            class Foo {
                void do_nothing(Bar a);
            };
        )"};
        std::vector<ClassInfo> result = parser::parse_cpp(input);
        relation_test_checks(result, RelationshipType::DEPENDENCY);
    }

    TEST_CASE("004 Direct Member Variables cause COMPOSITION Relations") {
        std::vector<std::string> input = {R"(
            class Foo {
                Bar a;
            };
        )"};
        std::vector<ClassInfo> result = parser::parse_cpp(input);
        relation_test_checks(result, RelationshipType::COMPOSITION);
    }

    TEST_CASE("005 Reference Member Variables cause AGGREGATION Relations") {
        std::vector<std::string> input = {R"(
            class Foo {
                Bar& a;
            };
        )"};
        std::vector<ClassInfo> result = parser::parse_cpp(input);
        relation_test_checks(result, RelationshipType::AGGREGATION);
    }

    TEST_CASE("006 Pointer Member Variables cause AGGREGATION Relations") {
        std::vector<std::string> input = {R"(
            class Foo {
                Bar* a;
            };
        )"};
        std::vector<ClassInfo> result = parser::parse_cpp(input);
        relation_test_checks(result, RelationshipType::AGGREGATION);
    }

    TEST_CASE("00X Primitives do not cause Relations") {
        std::vector<std::string> input = {R"(
            class Foo {
                int do_nothing(int a);

                int a;
                int* b;
                int& c;
            };
        )"};
        std::vector<ClassInfo> result = parser::parse_cpp(input);

        REQUIRE_EQ(result.size(), 1ull);
        CHECK(result[0].relationships.empty());
    }
}