INC=-Ilib/glad/include -Ilib/stb/ -Ilib/glfw/include -Ilib/cglm/include

OBJ=lib/glad/src/gl.o lib/stb/stb_image.o lib/glfw/src/libglfw3.a

all: $(OBJ)
	gcc src/*.c $(INC) $^ -lm -o mine -Wall -O2

lib/glad/src/gl.o:
	gcc $(@:.o=.c) -o $@ -Ilib/glad/include -c

lib/stb/stb_image.o:
	gcc -x c $(@:.o=.h) -o $@ -DSTB_IMAGE_IMPLEMENTATION -c 

lib/glfw/src/libglfw3.a: lib/glfw/Makefile
	make -C lib/glfw

lib/glfw/Makefile:
	cmake lib/glfw

clean:
	make -C lib/glfw clean 
	rm $(OBJ) lib/glfw/Makefile mine 
