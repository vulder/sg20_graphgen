#ifndef SG20_GRAPHGEN_MODULES_H
#define SG20_GRAPHGEN_MODULES_H

#include "sg20_graphgen/util.h"

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

namespace sg20 {

class Topic {
public:
  Topic(const std::string name, int ID) : name(name), ID(ID) {}

  std::string getName() const { return name; }
  int getID() const { return ID; }

  auto deps_begin() { return deps.begin(); }
  auto deps_end() { return deps.end(); }
  auto deps_begin() const { return deps.begin(); }
  auto deps_end() const { return deps.end(); }
  auto soft_begin() { return softDeps.begin(); }
  auto soft_end() { return softDeps.end(); }
  auto soft_begin() const { return softDeps.begin(); }
  auto soft_end() const { return softDeps.end(); }

  auto dependencies() { return make_range(deps_begin(), deps_end()); }
  auto softDependencies() { return make_range(soft_begin(), soft_end()); }
  auto dependencies() const { return make_range(deps_begin(), deps_end()); }
  auto softDependencies() const { return make_range(soft_begin(), soft_end()); }

  void addDependency(int TID) { deps.push_back(TID); }
  void addSoftDependency(int TID) { softDeps.push_back(TID); }

  void dump(std::ostream &out);

private:
  const std::string name;
  const int ID;
  std::vector<int> deps;
  std::vector<int> softDeps;
};

class Module {
public:
  Module(const std::string moduleName, int moduleID)
      : moduleName(std::move(moduleName)), moduleID(moduleID) {}

  std::string getModuleName() const { return moduleName; }
  int getModuleID() const { return moduleID; }
  size_t numTopics() const { return topics_list.size(); }

  inline Topic &addTopic(const std::string name, int TID) {
    topics_list.emplace_back(std::move(name), TID);
    return topics_list.back();
  }

  const Topic *findTopic(int TID) {
    auto find = std::find_if(topics_list.begin(), topics_list.end(),
                             [TID](const Topic t) { return t.getID() == TID; });
    if (find != topics_list.end()) {
      return &(*find);
    }

    return nullptr;
  }

  auto topics_begin() { return topics_list.begin(); }
  auto topics_end() { return topics_list.end(); }
  auto topics_begin() const { return topics_list.begin(); }
  auto topics_end() const { return topics_list.end(); }

  auto topics() { return make_range(topics_begin(), topics_end()); }
  auto topics() const { return make_range(topics_begin(), topics_end()); }

  void dump(std::ostream &out);

private:
  const std::string moduleName;
  const int moduleID;
  std::vector<Topic> topics_list;
};

class ModuleCollection {
public:
  using ModulesStorageTy = std::vector<std::unique_ptr<Module>>;

  static ModuleCollection loadModulesFromFile(std::filesystem::path filepath);

public:
  auto modules_begin() { return modules_storage.begin(); }
  auto modules_end() { return modules_storage.end(); }
  auto modules_begin() const { return modules_storage.begin(); }
  auto modules_end() const { return modules_storage.end(); }

  auto modules() { return make_range(modules_begin(), modules_end()); }
  auto modules() const { return make_range(modules_begin(), modules_end()); }

  size_t numModules() const { return modules_storage.size(); }
  size_t numTopics() const {
    size_t topics = 0;
    for (auto &module : modules()) {
      topics += module->numTopics();
    }
    return topics;
  }

  // Tries to find the module containing the specified topicID.
  // If found returns the module, otherwise, nullptr.
  Module *getModuleFromTopicID(int topicID) const;

private:
  ModuleCollection() = default;
  ModulesStorageTy modules_storage;
};

} // namespace sg20

#endif // SG20_GRAPHGEN_MODULES_H