#include "sg20_graphgen/modules.h"

#include "yaml-cpp/exceptions.h"

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/strings/internal/str_format/arg.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"

#include <filesystem>
#include <iostream>
#include <string_view>

using std::cout;
using std::cin;

ABSL_FLAG(std::string, graph_yaml, "sg20_graph.yaml",
          "path to the yaml specification file.");
ABSL_FLAG(std::string, output, "sg20_graph.yaml",
          "filename for the generated yaml file.");

void printHelp() {
  cout << "How to modify module/topic structure?";
  cout << R"(
1) editModules
2) editTopics
3) editDependencies
q) quit
)";
}

enum class CommandType {
  EDIT_MODULE,
  EDIT_TOPIC,
  EDIT_DEPENDENCY,
  QUIT,
  ERROR
};
CommandType convertToCommandType(const std::string_view rawCmd);

void editModules(sg20::ModuleCollection &MC);
void editTopics(sg20::ModuleCollection &MC);
void editDependencies();

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

    bool keepRunning = true;
    while (keepRunning) {
      cout << "\n\n--------------------\n";
      printHelp();
      cout << "Enter command:\n";
      std::string cmd;
      cin >> cmd;
      switch (convertToCommandType(cmd)) {
      case CommandType::EDIT_MODULE:
        editModules(MC);
        break;
      case CommandType::EDIT_TOPIC:
        editTopics(MC);
        break;
      case CommandType::EDIT_DEPENDENCY:
        editDependencies();
        break;
      case CommandType::QUIT:
        keepRunning = false;
        break;
      case CommandType::ERROR:
        cout << "Did not understand command: " << cmd << "\n\n";
        printHelp();
        break;
      }
    }
    cout << "Save to output file (yes/no)?";
    std::string answer;
    cin >> answer;
    if (answer.compare(0, 1, "y") == 0 || answer.compare(0, 1, "Y") == 0) {
      sg20::ModuleCollection::storeModulesToFile(
          MC, std::filesystem::path(absl::GetFlag(FLAGS_output)));
    }
  } catch (YAML::Exception &e) {
    std::cerr << "Syntax error in YAML " << yamlInputFile << std::endl;
    std::cerr << "reason: " << e.what() << std::endl;
  }

  return 0;
}

CommandType convertToCommandType(const std::string_view rawCmd) {
  if (rawCmd.compare(0, 1, "1") == 0 ||
      rawCmd.compare(0, 10, "editModule") == 0) {
    return CommandType::EDIT_MODULE;
  } else if (rawCmd.compare(0, 1, "2") == 0 ||
             rawCmd.compare(0, 9, "editTopic") == 0) {
    return CommandType::EDIT_TOPIC;
  } else if (rawCmd.compare(0, 1, "3") == 0 ||
             rawCmd.compare(0, 14, "editDependency") == 0) {
    return CommandType::EDIT_DEPENDENCY;
  } else if (rawCmd.compare(0, 1, "q") == 0 ||
             rawCmd.compare(0, 4, "quit") == 0) {
    return CommandType::QUIT;
  }

  return CommandType::ERROR;
}

void editModules(sg20::ModuleCollection &MC) {
  cout << R"(
+ Subcommands         +
│ add/del MODULE_NAME │
│ delID MODULE_ID     │
└─────────────────────┘
)";
  std::string rawCmd;
  cin >> rawCmd;

  if (rawCmd.compare(0, 3, "add") == 0) {
    cin >> rawCmd;
    auto &newModule = MC.addModule(std::move(rawCmd));
    std::cout << "New module added...\n";
    newModule.dump(std::cout);
  } else if (rawCmd.compare(0, 3, "del") == 0) {
    cin >> rawCmd;
    auto *module = MC.getModuleFromName(rawCmd);
    if (module) {
      MC.deleteModule(module->getModuleID());
      cout << "Deleted module";
    } else {
      cout << "Could not find module: " << rawCmd << "\n";
    }
  } else if (rawCmd.compare(0, 5, "delID") == 0) {
    int moduleID;
    cin >> moduleID;
    MC.deleteModule(moduleID);
    cout << "Deleted module";
  }
}

void editTopics(sg20::ModuleCollection &MC) {
  cout << R"(
+ Subcommands                    +
│ add/del MODULE_NAME:TOPIC_NAME │
│ delID MODULE_NAME:TOPIC_ID     │
└────────────────────────────────┘
)";
  std::string rawCmd;
  cin >> rawCmd;

  if (rawCmd.compare(0, 3, "add") == 0) {
    cin >> rawCmd;
    std::vector<std::string> splitInput = absl::StrSplit(rawCmd, ":");
    auto *topic = MC.addTopicToModule(splitInput[1], splitInput[0]);
    if (!topic) {
      cout << "Could not add topic: " << rawCmd << "\n";
    }
  } else if (rawCmd.compare(0, 3, "del") == 0) {
    cin >> rawCmd;
    std::vector<std::string> splitInput = absl::StrSplit(rawCmd, ":");
    auto *module = MC.getModuleFromName(splitInput[0]);
    if (module) {
      module->removeTopic(splitInput[1]);
    } else {
      cout << "Could not find module: " << rawCmd << "\n";
    }
  } else if (rawCmd.compare(0, 5, "delID") == 0) {
    int topicID;
    cin >> topicID;
    MC.getModuleFromTopicID(topicID);
  }
}

void editDependencies() {}
