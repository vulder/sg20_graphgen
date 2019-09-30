# SG20 teaching graph generator

Small graph generator to visualize the SG20 teaching module dependency graph.

## Build
```bash
> git submodule init && git submodule update
> mkdir build && cd build
> cmake ..
> make
```

## Example usage:
### Step 1: generate graphviz dot graph
```bash
bin/graphgen --graph_yaml d1725.yaml
```

### Step 2: convert graphviz dot file format of choice
```bash
dot -Tpng sg20_graph.dot -o sg20_graph.png
```
Depending on the generated graph and its dependencies, different graphviz layouting algorithms are needed to make the generated drawing visually appealing. Try: `dot, neato, twopi, circo, fdp, sfdp, patchwork, osage`

### Step 3: visualize
```bash
feh sg20_graph.png
```

## Editing yaml files
A simple yaml file is the base for specifying modules, topics, and dependencies between them.
To allow for easier creation and editing of these file, we provide a small yaml-editor.
Create a new yaml file `newFile` with:
```bash
bin/yamlEditor --output newFile.yaml
```
or edit an existing file with:
```bash
bin/yamlEditor --graph_yaml inputFile.yaml --output newFile.yaml
```
