#ifndef SG20_GRAPHGEN_GRAPHGENERATOR_H
#define SG20_GRAPHGEN_GRAPHGENERATOR_H

#include "sg20_graphgen/modules.h"

#include <filesystem>

namespace sg20 {

void emitFullDotGraph(const ModuleCollection &moduleCollection,
                      std::filesystem::path outputFilename);

void emitHTMLDotGraph(const ModuleCollection &moduleCollection,
                      std::filesystem::path outputFilename,
                      bool includeDependecies = false);

} // namespace sg20

#endif // SG20_GRAPHGEN_GRAPHGENERATOR_H
