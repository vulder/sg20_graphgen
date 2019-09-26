#ifndef SG20_GRAPHGEN_UTIL_H
#define SG20_GRAPHGEN_UTIL_H

#include "yaml-cpp/emitter.h"
#include "yaml-cpp/emittermanip.h"
#include "yaml-cpp/yaml.h"

#include <iterator>
#include <type_traits>

namespace sg20 {

template <typename IteratorType> class util_range {
public:
  util_range(IteratorType begin, IteratorType end)
      : begin_iter(std::move(begin)), end_iter(std::move(end)) {}

  IteratorType begin() const { return begin_iter; }

  IteratorType end() const { return end_iter; }

private:
  IteratorType begin_iter, end_iter;
};

template <typename IteratorType>
auto make_range(IteratorType begin, IteratorType end) {
  return util_range<IteratorType>(std::move(begin), std::move(end));
}

class YAMLMap {
public:
  YAMLMap(YAML::Emitter &emitter) : outputEmitter(emitter) {
    outputEmitter << YAML::BeginMap;
  }
  ~YAMLMap() { outputEmitter << YAML::EndMap; }

private:
  YAML::Emitter &outputEmitter;
};

} // namespace sg20

#endif // SG20_GRAPHGEN_UTIL_H
