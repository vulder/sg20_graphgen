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
Depending on the generated graph and its dependencies, different graphviz layouting algorithms are needed to make the generated drawing visually appealing.

### Step 3: visualize
```bash
feh sg20_graph.png
```
