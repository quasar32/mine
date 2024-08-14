INC := -isystem lib/glad/include \
       -isystem lib/stb/ \
       -isystem lib/glfw/include \
       -isystem lib/cglm/include \
       -isystem lib/perlin-noise/src

LIB := lib/glad/src/gl.o \
       lib/stb/stb_image.o \
       lib/glfw/src/libglfw3.a \
       lib/perlin-noise/src/noise1234.o

NODEP := clean clean-lib clean-all 

.PHONY: $(NODEP)

SRC := $(wildcard src/*.c)
OBJ := $(SRC:.c=.o)
DEP := $(SRC:.c=.d)

mine: $(OBJ) $(LIB) 
	gcc src/*.o $(LIB) -lm -o $@

ifeq ($(words $(findstring $(MAKECMDGOALS), $(NODEP))), 0)
-include $(DEP)
endif

src/%.d: src/%.c
	gcc $< $(INC) -MM -MT $(@:.d=.o) -o $@ 

src/%.o: src/%.c src/%.d
	gcc $< -o $@ $(INC) -O3 -c -Wall -DNDEBUG

-include $(DEP)

lib/glad/src/gl.o:
	gcc $(@:.o=.c) -o $@ -Ilib/glad/include -c -O2

lib/stb/stb_image.o:
	gcc -x c $(@:.o=.h) -o $@ -DSTB_IMAGE_IMPLEMENTATION -c -O2

lib/glfw/src/libglfw3.a: lib/glfw/Makefile
	make -C lib/glfw

lib/glfw/Makefile:
	cd lib/glfw && cmake . 

lib/perlin-noise/src/noise1234.o:
	gcc $(@:.o=.c) -o $@ -c -O2

clean:
	rm $(OBJ) $(DEP) -f

clean-lib:
	make -C lib/glfw clean || true 
	rm $(LIB) lib/glfw/Makefile mine -f 

clean-all: clean clean-lib
