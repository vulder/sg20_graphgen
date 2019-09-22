#include "sg20_graphgen/graph_generator.h"
#include "sg20_graphgen/html_generator.h"
#include "sg20_graphgen/modules.h"

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/strings/str_cat.h"

#include <filesystem>
#include <iostream>

ABSL_FLAG(std::string, graph_yaml, "sg20_graph.yaml",
          "path to the yaml specification file.");
ABSL_FLAG(std::string, output, "sg20_graph.dot",
          "filename for the generated dot file.");

int main(int argc, char *argv[]) {
  absl::SetProgramUsageMessage(
      absl::StrCat("Generate dot files for the SG20 module graph.\n\n",
                   "Example usage: ", argv[0], " --output fullgraph.dot"));
  absl::ParseCommandLine(argc, argv);

  auto yamlInputFile = std::filesystem::path(absl::GetFlag(FLAGS_graph_yaml));
  if (!std::filesystem::exists(yamlInputFile)) {
    std::cerr << "Yaml input file does not exist."
              << "\n";
    return 1;
  }

  auto MC = sg20::ModuleCollection::loadModulesFromFile(yamlInputFile);

  sg20::emitFullDotGraph(MC,
                         std::filesystem::path(absl::GetFlag(FLAGS_output)));
  return 0;
}
