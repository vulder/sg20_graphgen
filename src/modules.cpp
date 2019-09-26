#include "sg20_graphgen/modules.h"
#include "sg20_graphgen/util.h"

#include "yaml-cpp/emitter.h"
#include "yaml-cpp/emittermanip.h"
#include "yaml-cpp/node/convert.h"
#include "yaml-cpp/yaml.h"

#include <cassert>
#include <fstream>
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

  for (auto yamlModule : yamlModules) {
    newMCollection.modules_storage.push_back(std::make_unique<Module>(
        yamlModule["name"].as<std::string>(), yamlModule["mid"].as<int>()));
    auto module = newMCollection.modules_storage.back().get();

    auto sub = yamlModule["sub"];
    for (auto subval : sub) {
      Topic &newTopic = module->addTopic(subval["name"].as<std::string>(),
                                         subval["tid"].as<int>());
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

void ModuleCollection::storeModulesToFile(const ModuleCollection &MC,
                                          std::filesystem::path filepath) {
  YAML::Emitter yamlOut;
  yamlOut << YAML::BeginDoc;
  {
    YAMLMap modulesMap(yamlOut);
    yamlOut << "Modules";
    yamlOut << YAML::BeginSeq;
    for (auto &module : MC.modules()) {
      YAMLMap moduleMap(yamlOut);

      yamlOut << "name" << module->getModuleName();
      yamlOut << "mid" << module->getModuleID();
      yamlOut << "sub" << YAML::BeginSeq;

      for (auto topic : module->topics()) {
        YAMLMap yamlTopicMap(yamlOut);

        yamlOut << "name" << topic.getName();
        yamlOut << "tid" << topic.getID();

        if (topic.numDependencies()) {
          yamlOut << "dep";

          yamlOut << YAML::BeginSeq;
          for (auto dep : topic.dependencies()) {
            yamlOut << dep;
          }
          yamlOut << YAML::EndSeq;
        }

        if (topic.numSoftDependencies()) {
          yamlOut << "softdep";
          yamlOut << YAML::BeginSeq;
          for (auto dep : topic.softDependencies()) {
            yamlOut << dep;
          }
          yamlOut << YAML::EndSeq;
        }
      }
      yamlOut << YAML::EndSeq;
    }
    yamlOut << YAML::EndSeq;
  }
  yamlOut << YAML::EndDoc;

  assert(yamlOut.good() && "Generated YAML was wrongly formated.");

  std::ofstream outputFile(filepath);
  outputFile << yamlOut.c_str();
}

Module *ModuleCollection::getModuleFromTopicID(int topicID) const {
  for (auto &module : modules()) {
    auto *topic = module->findTopic(topicID);
    if (topic) {
      return module.get();
    }
  }
  return nullptr;
}

} // namespace sg20
