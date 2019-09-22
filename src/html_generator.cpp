#include "sg20_graphgen/html_generator.h"

#include "HTML/Element.h"

#include <utility>

using HTML::Bold;
using HTML::Col;
using HTML::List;
using HTML::ListItem;
using HTML::Row;
using HTML::Table;

using std::move;

namespace sg20 {

Table generateHTMLTable(const ModuleCollection &moduleCollection, int maxRows) {
  Table newTable;

  Row row;
  int moduleCounter = 0;
  for (auto &module : moduleCollection.modules()) {
    moduleCounter += 1;

    row << generateHTMLCol(*module);
    if (moduleCounter % maxRows == 0) { // persist in table and create next row
      newTable << move(row);
      row = Row();
    }
  }
  if (moduleCounter % maxRows != 0) {
    newTable << move(row);
  }

  return newTable;
}

Col generateHTMLCol(const Module &module) {
  Col newColumn;
  newColumn << Bold(module.getModuleName());

  {
    List topicList;
    for (auto topic : module.topics()) {
      topicList << ListItem(topic.getName());
    }
    newColumn << move(topicList);
  }

  return newColumn;
}

} // namespace sg20
