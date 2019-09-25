#include "sg20_graphgen/graph_generator.h"
#include "sg20_graphgen/html_generator.h"
#include "sg20_graphgen/modules.h"

#include "boost/graph/adjacency_list.hpp"
#include "boost/graph/graph_selectors.hpp"
#include "boost/graph/graphviz.hpp"
#include "boost/graph/properties.hpp"
#include "boost/graph/subgraph.hpp"
#include "boost/pending/property.hpp"

#include <fstream>

using boost::adjacency_list;
using boost::directedS;
using boost::edge_attribute;
using boost::edge_attribute_t;
using boost::edge_index_t;
using boost::graph_edge_attribute_t;
using boost::graph_graph_attribute;
using boost::graph_graph_attribute_t;
using boost::graph_name;
using boost::graph_name_t;
using boost::graph_vertex_attribute;
using boost::graph_vertex_attribute_t;
using boost::property;
using boost::vecS;
using boost::vertex_attribute_t;

namespace sg20 {

void emitFullDotGraph(const ModuleCollection &moduleCollection,
                      std::filesystem::path outputFilename) {
  using GraphvizAttributes = std::map<std::string, std::string>;
  using Graph = adjacency_list<
      boost::vecS, boost::vecS, directedS,
      property<vertex_attribute_t, GraphvizAttributes>,
      property<edge_index_t, int,
               property<edge_attribute_t, GraphvizAttributes>>,
      property<graph_name_t, std::string,
               property<graph_graph_attribute_t, GraphvizAttributes,
                        property<graph_vertex_attribute_t, GraphvizAttributes,
                                 property<graph_edge_attribute_t,
                                          GraphvizAttributes>>>>>;

  if (outputFilename.extension() != "dot" &&
      outputFilename.extension() != "gv") {
    std::cerr
        << "Warning: Output filename does not have a graphviz extension!\n";
  }

  boost::subgraph<Graph> graph(moduleCollection.numTopics());

  // set graph properties
  boost::get_property(graph, graph_name) = "main";
  boost::get_property(graph, graph_graph_attribute)["pack"] = "true";

  for (auto &module : moduleCollection.modules()) {
    boost::subgraph<Graph> &sub_graph = graph.create_subgraph();

    // set sub_graph properties
    boost::get_property(sub_graph, graph_name) =
        "cluster_" + module->getModuleName();
    boost::get_property(sub_graph, graph_graph_attribute)["label"] =
        module->getModuleName();
    get_property(sub_graph, graph_vertex_attribute)["shape"] = "Mrecord";

    for (auto topic : module->topics()) {
      int TID = topic.getID();
      add_vertex(TID, sub_graph);
      get(vertex_attribute_t(), graph)[TID]["label"] = topic.getName();

      for (auto dep : topic.dependencies()) {
        boost::add_edge(TID, dep, graph);
      }

      for (auto dep : topic.softDependencies()) {
        auto newInstEdge = boost::add_edge(TID, dep, graph);
        get(edge_attribute, graph)[newInstEdge.first]["style"] = "dotted";
      }
    }
  }

  std::cout << "Storing graph into " << outputFilename << "\n";
  std::ofstream outputFile(outputFilename);
  write_graphviz(outputFile, graph);
}

void generateDependencies(const ModuleCollection &moduleCollection,
                          std::ofstream &out) {
  for (auto &module : moduleCollection.modules()) {
    for (auto topic : module->topics()) {
      for (auto dep : topic.dependencies()) {
        Module *depModule = moduleCollection.getModuleFromTopicID(dep);
        if (depModule) {
          out << module->getModuleID() << ":" << topic.getID() << " -> "
              << depModule->getModuleID() << ":" << dep << ";\n";
        }
      }

      for (auto dep : topic.softDependencies()) {
        Module *depModule = moduleCollection.getModuleFromTopicID(dep);
        if (depModule) {
          out << module->getModuleID() << ":" << topic.getID() << " -> "
              << depModule->getModuleID() << ":" << dep << "[style=\""
              << "dotted"
              << "\"]"
              << ";\n";
        }
      }
    }
  }
}

void emitHTMLDotGraph(const ModuleCollection &moduleCollection,
                      std::filesystem::path outputFilename,
                      bool includeDependecies) {
  std::cout << "Storing graph into " << outputFilename << "\n";
  std::ofstream outputFile(outputFilename);
  outputFile << "digraph main {\n";

  for (auto &module : moduleCollection.modules()) {
    outputFile << module->getModuleID() << "[shape=box"
               << ", label=<" << generateDotHTMLTable(*(module.get()))
               << ">];\n";
  }

  if (includeDependecies) {
    generateDependencies(moduleCollection, outputFile);
  }

  outputFile << "}";
}

} // namespace sg20
