#include "sg20_graphgen/html_generator.h"
#include "sg20_graphgen/modules.h"

#include "yaml-cpp/exceptions.h"

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/strings/str_cat.h"

#include <filesystem>
#include <fstream>
#include <iostream>

ABSL_FLAG(std::string, graph_yaml, "sg20_graph.yaml",
          "path to the yaml specification file.");
ABSL_FLAG(std::string, output, "sg20_modules.html",
          "filename for the generated dot file.");

int main(int argc, char *argv[]) {
  absl::SetProgramUsageMessage(
      absl::StrCat("Generate HTML table from the SG20 module yaml.\n\n",
                   "Example usage: ", argv[0], " --output sg20_modules.html"));
  absl::ParseCommandLine(argc, argv);

  auto yamlInputFile = std::filesystem::path(absl::GetFlag(FLAGS_graph_yaml));
  if (!std::filesystem::exists(yamlInputFile)) {
    std::cerr << "Yaml input file does not exist."
              << "\n";
    return 1;
  }

  try {
    auto MC = sg20::ModuleCollection::loadModulesFromFile(yamlInputFile);

    std::ofstream outputFile(absl::GetFlag(FLAGS_output));
    outputFile << sg20::generateHTMLTable(MC);
  } catch (YAML::Exception &e) {
    std::cerr << "Syntax error in YAML " << yamlInputFile << std::endl;
    std::cerr << "Got: " << e.what() << std::endl;
  }

  return 0;
}
