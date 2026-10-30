all: obj/ build/ obj/main.c.o build/rock_gen 
OSMODE := l

obj/: 
ifeq (${OSMODE}, l)
	mkdir -p obj
else
	mkdir obj
endif

build/: 
ifeq (${OSMODE}, l)
	mkdir -p build
else
	mkdir build
endif

obj/main.c.o: src/main.c 
ifeq (${OSMODE}, l)
	${CC} src/main.c -c -o obj/main.c.o -O3 -fopenmp -g3
else
	${CC} src/main.c -c -o obj/main.c.o -O3 -fopenmp -g3
endif


build/rock_gen: obj/main.c.o 
ifeq (${OSMODE}, l)
	${CC} obj/main.c.o -o build/rock_gen -fopenmp -g3 -lm
else
	${CC} obj/main.c.o -o build/rock_gen -fopenmp -g3 -lm
endif


clean:
	rm -r obj
	rm -r build
.PHONY: clean


gencommands:
	mkdir emmgtemp
ifeq (${OSMODE}, l)
	clang src/main.c  -O3 -fopenmp -g3 -MJ emmgtemp/0.json -fsyntax-only
else
	clang src/main.c  -O3 -fopenmp -g3 -MJ emmgtemp/0.json -fsyntax-only
endif
# not cross platform here sad i think
	echo [ > emmgtemp/[
	echo ] > emmgtemp/]
	cat emmgtemp/[ emmgtemp/*.json emmgtemp/] > compile_commands.json
	rm -r emmgtemp
