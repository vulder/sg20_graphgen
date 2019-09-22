#include "sg20_graphgen/modules.h"

#include "yaml-cpp/yaml.h"

#include <cassert>
#include <iostream>

namespace sg20 {

void Topic::dump(std::ostream &out) {
  out << "Name: " << name << " ID: " << ID << " Deps: [";
  std::string sep;
  for (auto dep : deps) {
    out << sep << dep;
    sep = ", ";
  }
  out << "] - SoftDeps";
  sep = "";
  for (auto dep : softDeps) {
    out << sep << dep;
    sep = ", ";
  }
  out << "]\n";
}

void Module::dump(std::ostream &out) {
  out << "ModuleName: " << getModuleName() << " ID: " << getModuleID() << "\n";
  out << "  Topics: \n";
  for (auto &topic : topics()) {
    out << "    - ";
    topic.dump(out);
  }
  out << "\n";
}

ModuleCollection
ModuleCollection::loadModulesFromFile(std::filesystem::path filepath) {
  ModuleCollection newMCollection;
  YAML::Node file = YAML::LoadFile(filepath);

  auto yamlModules = file["Modules"];
  assert(yamlModules.IsSequence() &&
         "YAML file brocken, modules was not a sequence");

  int topicID = 0;
  for (auto yamlModule : yamlModules) {
    newMCollection.modules_storage.push_back(std::make_unique<Module>(
        yamlModule["name"].as<std::string>(), yamlModule["mid"].as<int>()));
    auto module = newMCollection.modules_storage.back().get();

    auto sub = yamlModule["sub"];
    for (auto subval : sub) {
      Topic &newTopic =
          module->addTopic(subval["name"].as<std::string>(), topicID++);
      if (subval["dep"]) {
        for (auto yamldepID : subval["dep"]) {
          newTopic.addDependency(yamldepID.as<int>());
        }
      }
      if (subval["softdep"]) {
        for (auto yamldepID : subval["softdep"]) {
          newTopic.addSoftDependency(yamldepID.as<int>());
        }
      }
    }
  }

  return newMCollection;
}

} // namespace sg20
