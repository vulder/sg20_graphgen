#include "sg20_graphgen/modules.h"
#include "sg20_graphgen/util.h"

#include "yaml-cpp/emitter.h"
#include "yaml-cpp/emittermanip.h"
#include "yaml-cpp/node/convert.h"
#include "yaml-cpp/yaml.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>

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

Topic *Module::getTopicByName(const std::string_view topicName) const {
  for (auto &topic : topics()) {
    if (topic->getName().compare(0, topic->getName().size(), topicName) == 0) {
      return topic.get();
    }
  }

  return nullptr;
}

Topic *Module::getTopicByID(int topicID) const {
  for (auto &topic : topics()) {
    if (topic->getID() == topicID) {
      return topic.get();
    }
  }

  return nullptr;
}

void Module::removeTopic(const std::string_view topicName) {
  auto delTopicIter = std::find_if(
      topics_list.begin(), topics_list.end(),
      [topicName](auto &topic) { return topic->getName() == topicName; });
  if (delTopicIter != topics_list.end()) {
    topics_list.erase(delTopicIter);
  }
}

void Module::dump(std::ostream &out) {
  out << "ModuleName: " << getModuleName() << " ID: " << getModuleID() << "\n";
  out << "  Topics: \n";
  for (auto &topic : topics()) {
    out << "    - ";
    topic->dump(out);
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

      for (auto &topic : module->topics()) {
        YAMLMap yamlTopicMap(yamlOut);

        yamlOut << "name" << topic->getName();
        yamlOut << "tid" << topic->getID();

        if (topic->numDependencies()) {
          yamlOut << "dep";

          yamlOut << YAML::BeginSeq;
          for (auto dep : topic->dependencies()) {
            yamlOut << dep;
          }
          yamlOut << YAML::EndSeq;
        }

        if (topic->numSoftDependencies()) {
          yamlOut << "softdep";
          yamlOut << YAML::BeginSeq;
          for (auto dep : topic->softDependencies()) {
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

Module *ModuleCollection::getModuleFromName(std::string_view moduleName) const {
  for (auto &module : modules()) {
    if (module->getModuleName().compare(0, moduleName.length(), moduleName) ==
        0) {
      return module.get();
    }
  }
  return nullptr;
}

Module *ModuleCollection::getModuleFromID(int moduleID) const {
  for (auto &module : modules()) {
    if (module->getModuleID() == moduleID) {
      return module.get();
    }
  }
  return nullptr;
}

Module &ModuleCollection::addModule(std::string moduleName) {
  modules_storage.push_back(
      std::make_unique<Module>(std::move(moduleName), getNextFreeModuleID()));
  return *modules_storage.back().get();
}

void ModuleCollection::deleteModule(int moduleID) {
  auto delModuleIter = std::find_if(
      modules_storage.begin(), modules_storage.end(),
      [moduleID](auto &module) { return module->getModuleID() == moduleID; });
  if (delModuleIter != modules_storage.end()) {
    modules_storage.erase(delModuleIter);
  }
}

Topic *ModuleCollection::addTopicToModule(std::string topicName,
                                          const std::string_view moduleName) {
  std::cout << moduleName << "\n";
  auto *module = getModuleFromName(moduleName);
  if (module) {
    return addTopicToModule(std::move(topicName), *module);
  }
  return nullptr;
}

Topic *ModuleCollection::addTopicToModule(std::string topicName,
                                          Module &module) {
  return &module.addTopic(std::move(topicName), getNextFreeTopicID());
}

int ModuleCollection::getNextFreeModuleID() const {
  if (modules_storage.size() == 0) {
    return 1;
  }
  auto &module = *std::max_element(
      modules_begin(), modules_end(), [](auto &module1, auto &module2) {
        return module1->getModuleID() < module2->getModuleID();
      });
  return module->getModuleID() + 1;
}

int ModuleCollection::getNextFreeTopicID() const {
  int maxID = 0;

  for (auto &module : modules()) {
    if (module->numTopics() == 0) {
      continue;
    }
    auto &localMaxTopic =
        *std::max_element(module->topics_begin(), module->topics_end(),
                          [](auto &topic1, auto &topic2) {
                            return topic1->getID() < topic2->getID();
                          });
    if (localMaxTopic->getID() > maxID) {
      maxID = localMaxTopic->getID();
    }
  }

  return maxID + 1;
}

} // namespace sg20
