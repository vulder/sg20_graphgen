#include "sg20_graphgen/modules.h"

#include "yaml-cpp/exceptions.h"

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/strings/str_cat.h"

#include <filesystem>
#include <iostream>

ABSL_FLAG(std::string, graph_yaml, "sg20_graph.yaml",
          "path to the yaml specification file.");

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

  try {
    auto MC = sg20::ModuleCollection::loadModulesFromFile(yamlInputFile);

    sg20::ModuleCollection::storeModulesToFile(
        MC, std::filesystem::path("newout.yaml"));
  } catch (YAML::Exception &e) {
    std::cerr << "Syntax error in YAML " << yamlInputFile << std::endl;
    std::cerr << "reason: " << e.what() << std::endl;
  }

  return 0;
}
