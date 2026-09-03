#include <algorithm>
#include <set>
#include <vector>

#include <doctest/doctest.h>
#include "intermediate_representation.hpp"
#include "../src/optimiser/optimiser.hpp"

// builder helper for constructing a ClassInfo
ClassInfo make_class(std::string name, std::vector<ClassInfo> nested = {}) {
    ClassInfo c{};
    c.name = std::move(name);
    c.nested_classes = std::move(nested);
    c.type = ClassType::CLASS;
    return c;
}

TEST_SUITE("Optimiser Tests - flatten_nested_classes") {

    // returns the vector of strings of class names from the vectors of classes
    std::vector<std::string> names_of(const std::vector<ClassInfo>& classes) {
        std::vector<std::string> out;
        out.reserve(classes.size());
        for (const auto& c : classes) out.push_back(c.name);
        return out;
    }

    TEST_CASE("001 Empty input stays empty") {
        std::vector<ClassInfo> classes;
        auto result = optimiser::flatten_nested_classes(classes);
        CHECK(result.empty());
        CHECK(classes.empty());
    }

    TEST_CASE("002 Classes with no nesting are untouched") {
        std::vector<ClassInfo> classes = {make_class("A"), make_class("B")};
        auto result = optimiser::flatten_nested_classes(classes);

        CHECK(result.empty());
        REQUIRE_EQ(classes.size(), 2ull);
        CHECK_EQ(names_of(classes), std::vector<std::string>{"A", "B"});
    }

    TEST_CASE("003 Nested class is hoisted") {
        std::vector<ClassInfo> classes = {
            make_class("Outer", {make_class("Inner1")})
        };

        auto result = optimiser::flatten_nested_classes(classes);

        CHECK_EQ(result, std::vector<std::string>{"Inner1"});
        REQUIRE_EQ(classes.size(), 2ull);
        CHECK_EQ(names_of(classes), std::vector<std::string>{"Outer", "Inner1"});
    }

    TEST_CASE("004 Nested classes are empty after flattening") {
        std::vector<ClassInfo> classes = {
            make_class("Outer", {make_class("Inner")})
        };

        optimiser::flatten_nested_classes(classes);

        CHECK(classes[0].name == "Outer");
        CHECK(classes[0].nested_classes.empty());
    }

    TEST_CASE("005 Multi-level nesting") {
        // Outer -> Mid -> Leaf
        std::vector<ClassInfo> classes = {
            make_class("Outer", {
                make_class("Mid", {make_class("Leaf")})
            })
        };

        auto result = optimiser::flatten_nested_classes(classes);

        CHECK_EQ(result, std::vector<std::string>{"Mid", "Leaf"});
        REQUIRE_EQ(classes.size(), 3ull);
        CHECK_EQ(names_of(classes), std::vector<std::string>{"Outer", "Mid", "Leaf"});
        CHECK(classes[1].nested_classes.empty()); // Mid's nested vector was moved
    }

    TEST_CASE("006 Preserves fields of hoisted classes") {
        ClassInfo inner = make_class("Inner");
        inner.type = ClassType::INTERFACE;
        inner.relationships["Outer"] = RelationshipType::DEPENDENCY;

        std::vector<ClassInfo> classes = {make_class("Outer", {inner})};

        optimiser::flatten_nested_classes(classes);

        REQUIRE_EQ(classes.size(), 2ull);
        CHECK_EQ(classes[1].name, "Inner");
        CHECK_EQ(classes[1].type, ClassType::INTERFACE);
        REQUIRE(classes[1].relationships.contains("Outer"));
        CHECK_EQ(classes[1].relationships.at("Outer"), RelationshipType::DEPENDENCY);
    }
}

TEST_SUITE("Optimiser Tests - strip_undefined_relations") {
    TEST_CASE("001 Empty input stays empty") {
        std::vector<ClassInfo> classes;
        auto result = optimiser::strip_undefined_relations(classes);
        CHECK(result.empty());
    }

    TEST_CASE("002 Relation to a defined class is kept") {
        std::vector<ClassInfo> classes = {make_class("A"), make_class("B")};
        classes[0].relationships["B"] = RelationshipType::AGGREGATION;
        auto result = optimiser::strip_undefined_relations(classes);

        CHECK(result.empty());
        REQUIRE_EQ(classes[0].relationships.size(), 1ull);
        CHECK_EQ(classes[0].relationships.at("B"), RelationshipType::AGGREGATION);
    }

    TEST_CASE("003 Relation to an undefined class is removed") {
        std::vector<ClassInfo> classes = {make_class("A")};
        classes[0].relationships["Ghost"] = RelationshipType::DEPENDENCY;
        auto result = optimiser::strip_undefined_relations(classes);

        CHECK_EQ(result, std::vector<std::string>{"Ghost"});
        CHECK(classes[0].relationships.empty());
    }

    TEST_CASE("004 Mixed defined and undefined relations on one class") {
        std::vector<ClassInfo> classes = {make_class("A"), make_class("B")};
        classes[0].relationships["B"] = RelationshipType::COMPOSITION;
        classes[0].relationships["Ghost"] = RelationshipType::DEPENDENCY;
        auto result = optimiser::strip_undefined_relations(classes);

        CHECK_EQ(result, std::vector<std::string>{"Ghost"});
        REQUIRE_EQ(classes[0].relationships.size(), 1ull);
        CHECK(classes[0].relationships.contains("B"));
    }

    TEST_CASE("005 Multiple undefined relations are all removed") {
        std::vector<ClassInfo> classes = {make_class("A")};
        classes[0].relationships["Ghost1"] = RelationshipType::DEPENDENCY;
        classes[0].relationships["Ghost2"] = RelationshipType::INHERITANCE;
        auto result = optimiser::strip_undefined_relations(classes);

        CHECK(std::find(result.begin(), result.end(), "Ghost1") != result.end());
        CHECK(std::find(result.begin(), result.end(), "Ghost2") != result.end());
        CHECK(classes[0].relationships.empty());
    }

    TEST_CASE("006 Undefined reference is removed from all referencing class relations") {
        std::vector<ClassInfo> classes = {make_class("A"), make_class("B")};
        classes[0].relationships["Ghost"] = RelationshipType::DEPENDENCY;
        classes[1].relationships["Ghost"] = RelationshipType::AGGREGATION;
        auto result = optimiser::strip_undefined_relations(classes);

        CHECK_EQ(result, std::vector<std::string>{"Ghost", "Ghost"});
        CHECK(classes[0].relationships.empty());
        CHECK(classes[1].relationships.empty());
    }

    TEST_CASE("007 Class with no relationships is untouched") {
        std::vector<ClassInfo> classes = {make_class("A")};
        auto result = optimiser::strip_undefined_relations(classes);

        CHECK(result.empty());
        CHECK(classes[0].relationships.empty());
    }

    TEST_CASE("008 Self-References are Removed") {
        std::vector<ClassInfo> classes = {make_class("A")};
        classes[0].relationships["A"] = RelationshipType::DEPENDENCY;
        auto result = optimiser::strip_undefined_relations(classes);

        REQUIRE_EQ(classes.size(), 1ull);
        CHECK(classes[0].relationships.empty());
    }
}
