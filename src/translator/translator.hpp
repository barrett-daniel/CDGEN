#pragma once

#include <vector>
#include <string>
#include <sstream>

#include "../intermediate_representation.hpp"
#include "../util/run_configuration.hpp"

namespace translator {
    std::string to_gviz_file(const std::vector<ClassInfo>& classes, const RunConfiguration& run_config);
};