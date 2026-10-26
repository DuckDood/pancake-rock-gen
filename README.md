# Pancake rock generator
## Makes (or at least tries to make) sedimentary pancake rocks like this:
![colorado pancake rocks](https://www.visitcos.com/imager/files_idss_com/C88/657febf2-d6d0-4657-b10e-e072951c4c12_e45adf5f6bc0c5c2a30a39868f44eab6.jpg)

## Renders (made in blender) of the rocks it generates:
![cool shadowy view](shadowview.png)  
![front view](frontview.png)

## Trying it out
Try it out (here)[https://duckdood.github.io/terra/pancakes/]

## Building locally (Tested on linux with GCC)
```
git clone https://github.com/DuckDood/pancake-rock-gen.git
cd pancake-rock-gen
make
```
This puts a program in build/ that will generate rocks and put it in an OBJ file named layer.obj
