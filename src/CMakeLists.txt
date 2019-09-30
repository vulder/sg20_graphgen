set(GRAPHGEN_LIB_SRC
  graph_generator.cpp
  html_generator.cpp
  modules.cpp
)

add_library(sg20_graphgen
  ${GRAPHGEN_LIB_SRC}
)
target_link_libraries(sg20_graphgen
  LINK_PUBLIC
  absl::flags
  absl::flags_parse
  boost_graph
  yaml-cpp
)

add_executable(graphgen
  graphgen.cpp
)
target_link_libraries(graphgen
  sg20_graphgen
)

add_executable(yamlEditor 
  yamlEditor.cpp
)
target_link_libraries(yamlEditor
  sg20_graphgen
)
