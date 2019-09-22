set(GRAPHGEN_SRC
  graph_generator.cpp
  graphgen.cpp
  html_generator.cpp
  modules.cpp
  )

add_executable(graphgen ${GRAPHGEN_SRC})
target_link_libraries(graphgen
  absl::flags
  absl::flags_parse
  boost_graph
  yaml-cpp
)
