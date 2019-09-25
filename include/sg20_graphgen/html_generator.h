#ifndef SG20_GRAPHGEN_HTMLGENERATOR_H
#define SG20_GRAPHGEN_HTMLGENERATOR_H

#include "sg20_graphgen/modules.h"

#include "HTML/HTML.h"

namespace sg20 {

//===----------------------------------------------------------------------===//
// HTML generator functions

HTML::Table generateHTMLTable(const ModuleCollection &moduleCollection,
                              int maxRows = 3);
HTML::Col generateHTMLCol(const Module &module);

//===----------------------------------------------------------------------===//
// Dot HTML generator functions

HTML::Table generateDotHTMLTable(const Module &module);

} // namespace sg20

#endif // SG20_GRAPHGEN_HTMLGENERATOR_H
