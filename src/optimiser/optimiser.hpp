#pragma once

#include <vector>
#include "../intermediate_representation.hpp"

/**
 * Collection of methods which aim to clean up the parsed class data
 * in preparation for translation to a Graphviz diagram.
 */
namespace optimiser {

    /**
     *
     * @param classes which will have any nested classes appended to the end
     *      of the top level vector
     * @return the list of flattened class_names
     */
    std::vector<std::string> flatten_nested_classes(std::vector<ClassInfo>& classes);

    /**
     * Alters the set of classes and their relations to remove relations to undefined
     * classes. If a relation is found to another class which is not defined in the
     * set, it is assumed to be an external dependency (e.g. in the stdlib, or some
     * 3rd party API). Any relation to that external dependency is removed from the set.
     *
     * <b>Pre-condition:</b> nested classes are flattened
     *
     * @param classes whose relations are to be stripped
     * @return the list of stripped classes from relations
     */
    std::vector<std::string> strip_undefined_relations(std::vector<ClassInfo>& classes);
};