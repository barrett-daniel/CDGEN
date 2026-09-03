#include "optimiser.hpp"

#include <string>
#include <vector>
#include <algorithm>
#include <iterator>

#include "intermediate_representation.hpp"

namespace optimiser {
    std::vector<std::string> flatten_nested_classes(std::vector<ClassInfo>& classes) {
        std::vector<std::string> flattened_classes;

        for (size_t i = 0; i < classes.size(); ++i) {
            std::vector<ClassInfo> nested = std::move(classes[i].nested_classes);

            for (const ClassInfo& elem : nested) {
                flattened_classes.push_back(elem.name); // copy
            }

            classes.insert(
                classes.end(),
                std::make_move_iterator(nested.begin()),
                std::make_move_iterator(nested.end())
            );
        }

        return flattened_classes;
    }

    std::vector<std::string> strip_undefined_relations(std::vector<ClassInfo>& classes) {
        std::vector<std::string> stripped_classes;

        for (ClassInfo& class_info : classes) {
            // lambda which returns true if the other element in a relation is
            // not defined in the set of classes
            auto relation_not_defined = [&](const std::pair<std::string, RelationshipType>& relation) {
                const auto& [other_class_name, _] = relation;

                auto it = std::ranges::find_if(classes, [&other_class_name](const ClassInfo& elem) {
                    return elem.name == other_class_name;
                });
                const bool defined = it != classes.end();

                if (!defined) {
                    stripped_classes.push_back(other_class_name); // copy
                }

                return !defined || class_info.name == other_class_name; // or self-referencing but don't report it
            };

            std::erase_if(class_info.relationships, relation_not_defined);
        }

        return stripped_classes;
    }
};