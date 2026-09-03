#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

enum class VisibilityType : uint8_t {
    PUBLIC,
    PRIVATE,
    PROTECTED
};

enum class QualifierPosition : uint8_t {
    PREFIX,
    POSTFIX
};

struct QualifierInfo {
    QualifierPosition position;
    std::string name;
};

struct TypeInfo {
    std::string name;
    std::vector<TypeInfo> template_parameters;
    std::vector<QualifierInfo> qualifiers;
    bool primitive;
};

struct VariableInfo {
    std::string name;
    TypeInfo type;
    VisibilityType visibility;
};

struct IdentifierInfo {
    std::string name;
    TypeInfo type;
};

struct MethodInfo {
    std::string name;
    TypeInfo type;
    VisibilityType visibility;
    std::vector<IdentifierInfo> parameters;
};

enum class RelationshipType : uint8_t {
    INHERITANCE,
    COMPOSITION,
    AGGREGATION,
    DEPENDENCY
};

enum class ClassType : uint8_t {
    CLASS,
    STRUCT,
    INTERFACE,
    ABSTRACT_CLASS
};

struct ClassInfo {
    std::string name;
    std::vector<MethodInfo> methods;
    std::vector<VariableInfo> variables;
    std::unordered_map<std::string, RelationshipType> relationships;
    std::vector<ClassInfo> nested_classes;
    ClassType type;
};