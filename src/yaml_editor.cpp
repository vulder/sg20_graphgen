#include "sg20_graphgen/modules.h"

#include "yaml-cpp/exceptions.h"

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/strings/ascii.h"
#include "absl/strings/internal/str_format/arg.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "absl/utility/utility.h"

#include <cctype>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <regex>
#include <string>
#include <string_view>
#include <type_traits>

using std::cerr;
using std::cin;
using std::cout;

ABSL_FLAG(std::string, graph_yaml, "sg20_graph.yaml",
          "path to the yaml specification file.");
ABSL_FLAG(std::string, output, "sg20_graph.yaml",
          "filename for the generated yaml file.");

void printHelp() {
  cout << "How to modify module/topic structure?";
  cout << R"(
1) listModules
2) listTopics   MODULE_NAME
3) listDeps     MODULE_NAME:TOPIC_NAME
4) addModule    MODULE_NAME
5) delModule    MODULE_NAME
6) addTopic     MODULE_NAME:TOPIC_NAME
7) delTopic     MODULE_NAME:TOPIC_NAME
8) addDep       MODULE_NAME:TOPIC_NAME -> MODULE_NAME:TOPIC_NAME
9) delDep       MODULE_NAME:TOPIC_NAME -> MODULE_NAME:TOPIC_NAME
q) quit
h) help

Hints:
  - every NAME can always be replaced by the corresponding ID
  - dependencies arrows(->) can be replaced with ~> to indicate soft dependencies
)";
}

enum class CommandType {
  LIST_MODULES = 1,
  LIST_TOPICS,
  LIST_DEPENDENCIES,
  ADD_MODULE,
  DELETE_MODULE,
  ADD_TOPIC,
  DELETE_TOPIC,
  ADD_DEPENDENCY,
  DELETE_DEPENDENCY,
  HELP,
  QUIT,
  ERROR
};
CommandType convertToCommandType(const std::string_view rawCmd);
constexpr auto commandToInt(CommandType cmd) {
  return static_cast<std::underlying_type<CommandType>::type>(cmd);
}

bool isNumber(const std::string_view str) {
  return !str.empty() && std::find_if(str.begin(), str.end(), [](char c) {
                           return !std::isdigit(c);
                         }) == str.end();
}

// This function parses the rest of the user input and returns a module. The
// leftover user input on the line should be formatted like this:
//
// MODULE_NAME
// where every direct name can be replaced with the corresponding ID.
//
// If the module is not found a nullptr is returned instead.
sg20::Module *getModuleFromUser(const sg20::ModuleCollection &MC) {
  std::string rawInput;
  std::getline(cin, rawInput);
  rawInput = absl::StripLeadingAsciiWhitespace(
      absl::StripTrailingAsciiWhitespace(rawInput));

  sg20::Module *reqModule = isNumber(rawInput)
                                ? MC.getModuleFromID(std::stoi(rawInput))
                                : MC.getModuleFromName(rawInput);
  if (!reqModule) {
    cerr << "Could not find module \"" << rawInput << "\"\n";
    return nullptr;
  }

  return reqModule;
}

struct ModuleTopicTuple : std::pair<sg20::Module *, sg20::Topic *> {
  ModuleTopicTuple(sg20::Module *module, sg20::Topic *topic)
      : std::pair<sg20::Module *, sg20::Topic *>(module, topic) {}
  sg20::Module *getModule() { return first; }
  sg20::Module *getModule() const { return first; }
  sg20::Topic *getTopic() { return second; }
  sg20::Topic *getTopic() const { return second; }
  bool isValid() const { return getModule() && getTopic(); }
};

// This function parses the rest of the user input and returns a pair module and
// topic. The leftover user input on the line should be formatted like this:
//
// MODULE_NAME:TOPIC_NAME
// where every direct name can be replaced with the corresponding ID.
//
// If the module or topic is not found a nullptr is returned instead.
ModuleTopicTuple getModuleAndTopicFromUser(const sg20::ModuleCollection &MC) {
  std::string rawInput;
  std::getline(cin, rawInput);
  rawInput = absl::StripLeadingAsciiWhitespace(
      absl::StripTrailingAsciiWhitespace(rawInput));

  std::vector<std::string> splitInput = absl::StrSplit(rawInput, ":");
  sg20::Module *reqModule = isNumber(splitInput[0])
                                ? MC.getModuleFromID(std::stoi(splitInput[0]))
                                : MC.getModuleFromName(splitInput[0]);
  if (!reqModule) {
    cerr << "Could not find module \"" << splitInput[0] << "\"\n";
    return {nullptr, nullptr};
  }
  if (splitInput.size() < 2) {
    cerr << "Could not find topic name.";
    return {nullptr, nullptr};
  }
  sg20::Topic *reqTopic =
      isNumber(splitInput[1])
          ? reqModule->getTopicByID(std::stoi(splitInput[1]))
          : reqModule->getTopicByName(splitInput[1]);
  if (!reqTopic) {
    cerr << "Could not find topic \"" << splitInput[1] << "\" in module \""
         << splitInput[0] << "\"\n";
    return {nullptr, nullptr};
  }
  return {reqModule, reqTopic};
}

struct SourceTargetDependency : std::pair<ModuleTopicTuple, ModuleTopicTuple> {
  SourceTargetDependency(ModuleTopicTuple source, ModuleTopicTuple target,
                         std::string depSpecifier)
      : std::pair<ModuleTopicTuple, ModuleTopicTuple>(source, target),
        depSpecifier(std::move(depSpecifier)) {}
  ModuleTopicTuple getSource() { return first; }
  ModuleTopicTuple getSource() const { return first; }
  ModuleTopicTuple getTarget() { return second; }
  ModuleTopicTuple getTarget() const { return second; }
  bool isValid() const {
    return getSource().isValid() && getTarget().isValid();
  }
  std::string_view getDependencyTypeSpecifier() const { return depSpecifier; }

private:
  std::string depSpecifier;
};

// This function parses the rest of the user input and returns a source and a
// target tuple with and additional dependency specifier. The leftover user
// input on the line should be formatted like this:
//
// MODULE_NAME:TOPIC_NAME -> MODULE_NAME:TOPIC_NAME
// where every direct name can be replaced with the corresponding ID.
//
// If the modules or topics are not found a nullptr is returned instead.
SourceTargetDependency
getSourceTargetDepFromUser(const sg20::ModuleCollection &MC) {
  std::string rawInput;
  std::getline(cin, rawInput);

  const std::regex cmdInputRgx(R"((.*):(.*) (->|~>) (.*):(.*))");
  std::smatch matches;
  std::regex_search(rawInput, matches, cmdInputRgx);
  if (matches.size() != 6) {
    cerr << "Command input was wrongly formatted.";
    return {{nullptr, nullptr}, {nullptr, nullptr}, matches.str(3)};
  }
  //===--------------------------------------------------------------------===//
  // Source module handling
  std::string sourceModuleRef = std::string(absl::StripLeadingAsciiWhitespace(
      absl::StripTrailingAsciiWhitespace(matches.str(1))));
  sg20::Module *reqSourceModule =
      isNumber(sourceModuleRef) ? MC.getModuleFromID(std::stoi(sourceModuleRef))
                                : MC.getModuleFromName(sourceModuleRef);
  if (!reqSourceModule) {
    cerr << "Could not find source module \"" << sourceModuleRef << "\"\n";
    return {{nullptr, nullptr}, {nullptr, nullptr}, matches.str(3)};
  }

  //===--------------------------------------------------------------------===//
  // Source topic handling
  std::string sourceTopicRef = std::string(absl::StripLeadingAsciiWhitespace(
      absl::StripTrailingAsciiWhitespace(matches.str(2))));
  sg20::Topic *reqSourceTopic =
      isNumber(sourceTopicRef)
          ? reqSourceModule->getTopicByID(std::stoi(sourceTopicRef))
          : reqSourceModule->getTopicByName(sourceTopicRef);
  if (!reqSourceTopic) {
    cerr << "Could not find source topic \"" << sourceTopicRef
         << "\" in module \"" << sourceModuleRef << "\"\n";
    return {{reqSourceModule, nullptr}, {nullptr, nullptr}, matches.str(3)};
  }

  //===--------------------------------------------------------------------===//
  // Target module handling
  std::string targetModuleRef = std::string(absl::StripLeadingAsciiWhitespace(
      absl::StripTrailingAsciiWhitespace(matches.str(4))));
  sg20::Module *reqTargetModule =
      isNumber(targetModuleRef) ? MC.getModuleFromID(std::stoi(targetModuleRef))
                                : MC.getModuleFromName(targetModuleRef);
  if (!reqTargetModule) {
    cerr << "Could not find target module \"" << targetModuleRef << "\"\n";
    return {
        {reqSourceModule, reqSourceTopic}, {nullptr, nullptr}, matches.str(3)};
  }

  //===--------------------------------------------------------------------===//
  // Target topic handling
  std::string targetTopicRef = std::string(absl::StripLeadingAsciiWhitespace(
      absl::StripTrailingAsciiWhitespace(matches.str(5))));
  sg20::Topic *reqTargetTopic =
      isNumber(targetTopicRef)
          ? reqTargetModule->getTopicByID(std::stoi(targetTopicRef))
          : reqTargetModule->getTopicByName(targetTopicRef);
  if (!reqTargetTopic) {
    cerr << "Could not find target topic \"" << targetTopicRef
         << "\" in module \"" << targetModuleRef << "\"\n";
    return {{reqSourceModule, reqSourceTopic},
            {reqTargetModule, nullptr},
            matches.str(3)};
  }

  return {{reqSourceModule, reqSourceTopic},
          {reqTargetModule, reqTargetTopic},
          matches.str(3)};
}

void handleListModules(sg20::ModuleCollection &MC) {
  cout << "Found the following modules:\n";
  for (auto &module : MC.modules()) {
    cout << "Name: " << module->getModuleName()
         << "  (ID: " << module->getModuleID() << ")"
         << "\n";
  }
}

void handleAddModule(sg20::ModuleCollection &MC) {
  std::string newModuleName;
  std::getline(cin, newModuleName);
  newModuleName = absl::StripLeadingAsciiWhitespace(
      absl::StripTrailingAsciiWhitespace(newModuleName));
  sg20::Module &newModule = MC.addModule(newModuleName);
  cout << "Create new module: " << newModule.getModuleName()
       << "  (ID: " << newModule.getModuleID() << ")"
       << "\n";
}

void handleDeleteModule(sg20::ModuleCollection &MC) {
  sg20::Module *reqModule = getModuleFromUser(MC);
  if (!reqModule) {
    return; // if user input was wrong return to main menu
  }
  std::string deletedModuleName = reqModule->getModuleName();
  int deletedModuleID = reqModule->getModuleID();
  MC.deleteModule(reqModule->getModuleID());
  cout << "Deleted module: " << deletedModuleName
       << "  (ID: " << deletedModuleID << ")"
       << "\n";
}

void handleListTopics(sg20::ModuleCollection &MC) {
  sg20::Module *reqModule = getModuleFromUser(MC);
  if (!reqModule) {
    return; // if user input was wrong return to main menu
  }

  cout << "Found the following topics for " << reqModule->getModuleName()
       << ":\n";
  // TODO: align fmt
  for (auto &topic : reqModule->topics()) {
    cout << "Name: " << topic->getName() << "  (ID: " << topic->getID() << ")"
         << "\n";
  }
}

void handleAddTopic(sg20::ModuleCollection &MC) {
  std::string rawInput;
  std::getline(cin, rawInput);
  std::vector<std::string> splitInput = absl::StrSplit(rawInput, ":");

  if (splitInput.size() < 2) {
    cerr << "Input was wrongly formatted.";
    return;
  }

  splitInput[0] = absl::StripLeadingAsciiWhitespace(
      absl::StripTrailingAsciiWhitespace(splitInput[0]));
  splitInput[1] = absl::StripLeadingAsciiWhitespace(
      absl::StripTrailingAsciiWhitespace(splitInput[1]));

  sg20::Module *reqModule = isNumber(splitInput[0])
                                ? MC.getModuleFromID(std::stoi(splitInput[0]))
                                : MC.getModuleFromName(splitInput[0]);
  if (!reqModule) {
    cerr << "Could not find module \"" << splitInput[0] << "\"\n";
    return;
  }
  sg20::Topic *newTopic =
      MC.addTopicToModule(std::move(splitInput[1]), *reqModule);
  cout << "Created new topic: " << newTopic->getName()
       << "  (ID: " << newTopic->getID() << ")"
       << " in module " << reqModule->getModuleName() << "\n";
}

void handleDeleteTopic(sg20::ModuleCollection &MC) {
  auto [reqModule, reqTopic] = getModuleAndTopicFromUser(MC);
  if (!reqModule || !reqTopic) {
    return; // if user input was wrong return to main menu
  }

  std::string deletedTopicName = reqTopic->getName();
  int deletedTopicID = reqTopic->getID();
  reqModule->removeTopic(reqTopic->getName());

  cout << "Deleted topic: " << deletedTopicName << "  (ID: " << deletedTopicID
       << ") out of module " << reqModule->getModuleName() << "\n";
}

void handleAddDependency(sg20::ModuleCollection &MC) {
  auto sourceTargetDep = getSourceTargetDepFromUser(MC);
  if (!sourceTargetDep.isValid()) {
    return; // if user input was wrong return to main menu
  }

  if (sourceTargetDep.getDependencyTypeSpecifier().compare(0, 2, "->") == 0) {
    sourceTargetDep.getSource().getTopic()->addDependency(
        sourceTargetDep.getTarget().getTopic()->getID());
    cout << "Added dependency from "
         << sourceTargetDep.getSource().getTopic()->getName() << " -> "
         << sourceTargetDep.getTarget().getTopic()->getName() << "\n";
  } else if (sourceTargetDep.getDependencyTypeSpecifier().compare(0, 2, "~>") ==
             0) {
    sourceTargetDep.getSource().getTopic()->addSoftDependency(
        sourceTargetDep.getTarget().getTopic()->getID());
    cout << "Added soft dependency from "
         << sourceTargetDep.getSource().getTopic()->getName() << " ~> "
         << sourceTargetDep.getTarget().getTopic()->getName() << "\n";
  } else {
    cerr << "Did not understand dependency specifier "
         << sourceTargetDep.getDependencyTypeSpecifier() << "\n";
  }
}

void handleDeleteDependency(sg20::ModuleCollection &MC) {
  auto sourceTargetDep = getSourceTargetDepFromUser(MC);
  if (!sourceTargetDep.isValid()) {
    return; // if user input was wrong return to main menu
  }

  std::string deletedSrcTopicName =
      sourceTargetDep.getSource().getTopic()->getName();
  std::string deletedTargetTopicName =
      sourceTargetDep.getTarget().getTopic()->getName();

  if (sourceTargetDep.getDependencyTypeSpecifier().compare(0, 2, "->") == 0) {
    sourceTargetDep.getSource().getTopic()->removeDependency(
        sourceTargetDep.getTarget().getTopic()->getID());

    cout << "Removed dependency from " << deletedSrcTopicName << " -> "
         << deletedTargetTopicName << "\n";
  } else if (sourceTargetDep.getDependencyTypeSpecifier().compare(0, 2, "~>") ==
             0) {
    sourceTargetDep.getSource().getTopic()->removeSoftDependency(
        sourceTargetDep.getTarget().getTopic()->getID());

    cout << "Removed soft dependency from " << deletedSrcTopicName << " ~> "
         << deletedTargetTopicName << "\n";
  } else {
    cerr << "Did not understand dependency specifier "
         << sourceTargetDep.getDependencyTypeSpecifier() << "\n";
  }
}

void handleListDependencies(sg20::ModuleCollection &MC) {
  cout << "For which topic should we print all dependencies?\n";
  auto [reqModule, reqTopic] = getModuleAndTopicFromUser(MC);
  if (!reqModule || !reqTopic) {
    return; // if user input was wrong return to main menu
  }

  cout << "Found the following dependencies for " << reqTopic->getName()
       << "\n";
  for (auto dep : reqTopic->dependencies()) {
    // TODO: write dependency name look up
    cout << "-> " << dep << "\n";
  }
  cout << "Found the following soft dependencies for " << reqTopic->getName()
       << "\n";
  for (auto softDep : reqTopic->softDependencies()) {
    // TODO: write dependency name look up
    cout << "~> " << softDep << "\n";
  }
}

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
    printHelp();
    while (keepRunning) {
      cout << "\n\n--------------------\n";
      cout << "Enter command:\n";
      std::string cmd;
      cin >> cmd;
      switch (convertToCommandType(cmd)) {
      case CommandType::LIST_MODULES:
        handleListModules(MC);
        break;
      case CommandType::ADD_MODULE:
        handleAddModule(MC);
        break;
      case CommandType::DELETE_MODULE:
        handleDeleteModule(MC);
        break;
      case CommandType::LIST_TOPICS:
        handleListTopics(MC);
        break;
      case CommandType::ADD_TOPIC:
        handleAddTopic(MC);
        break;
      case CommandType::DELETE_TOPIC:
        handleDeleteTopic(MC);
        break;
      case CommandType::ADD_DEPENDENCY:
        handleAddDependency(MC);
        break;
      case CommandType::DELETE_DEPENDENCY:
        handleDeleteDependency(MC);
        break;
      case CommandType::LIST_DEPENDENCIES:
        handleListDependencies(MC);
        break;
      case CommandType::HELP:
        printHelp();
        break;
      case CommandType::QUIT:
        keepRunning = false;
        break;
      case CommandType::ERROR:
        cout << "Did not understand command: " << cmd << "\n\n";
        std::string cleanInput;
        getline(cin, cleanInput);
        printHelp();
        break;
      }
    }
    cout << "Save to output file (yes/no)?\n";
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

bool isCommand(const std::string_view rawCmd, CommandType cmdType,
               const std::string_view cmdName) {
  return absl::StartsWith(rawCmd, std::to_string(commandToInt(cmdType))) ||
         absl::StartsWith(rawCmd, cmdName);
}

CommandType convertToCommandType(const std::string_view rawCmd) {
  // Listing commands
  if (isCommand(rawCmd, CommandType::LIST_MODULES, "listModules")) {
    return CommandType::LIST_MODULES;
  }
  if (isCommand(rawCmd, CommandType::LIST_TOPICS, "listTopic")) {
    return CommandType::LIST_TOPICS;
  }
  if (isCommand(rawCmd, CommandType::LIST_DEPENDENCIES, "listDeps")) {
    return CommandType::LIST_DEPENDENCIES;
  }

  // Edit module commands
  if (isCommand(rawCmd, CommandType::ADD_MODULE, "addModule")) {
    return CommandType::ADD_MODULE;
  }
  if (isCommand(rawCmd, CommandType::DELETE_MODULE, "delModule")) {
    return CommandType::DELETE_MODULE;
  }

  // Edit topic commands
  if (isCommand(rawCmd, CommandType::ADD_TOPIC, "addTopic")) {
    return CommandType::ADD_TOPIC;
  }
  if (isCommand(rawCmd, CommandType::DELETE_TOPIC, "delTopic")) {
    return CommandType::DELETE_TOPIC;
  }

  // Edit dependencie commands
  if (isCommand(rawCmd, CommandType::ADD_DEPENDENCY, "addDep")) {
    return CommandType::ADD_DEPENDENCY;
  }
  if (isCommand(rawCmd, CommandType::DELETE_DEPENDENCY, "delDep")) {
    return CommandType::DELETE_DEPENDENCY;
  }

  if (absl::StartsWith(rawCmd, "h") || absl::StartsWith(rawCmd, "help")) {
    return CommandType::HELP;
  }
  if (absl::StartsWith(rawCmd, "q") || absl::StartsWith(rawCmd, "quit")) {
    return CommandType::QUIT;
  }

  return CommandType::ERROR;
}
