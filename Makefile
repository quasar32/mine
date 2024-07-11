INC =-Ilib/glad/include -Ilib/stb/ -Ilib/glfw/include 
INC += -Ilib/cglm/include -Ilib/perlin-noise/src

OBJ = lib/glad/src/gl.o lib/stb/stb_image.o lib/glfw/src/libglfw3.a
OBJ += lib/perlin-noise/src/noise1234.o

mine: src/*.c $(OBJ)
	gcc $(INC) $^ -lm -o $@ -Wall -O2 

lib/glad/src/gl.o:
	gcc $(@:.o=.c) -o $@ -Ilib/glad/include -c

lib/stb/stb_image.o:
	gcc -x c $(@:.o=.h) -o $@ -DSTB_IMAGE_IMPLEMENTATION -c 

lib/glfw/src/libglfw3.a: lib/glfw/Makefile
	make -C lib/glfw

lib/glfw/Makefile:
	cd lib/glfw && cmake . 

lib/perlin-noise/src/noise1234.o:
	gcc $(@:.o=.c) -o $@ -c

clean:
	make -C lib/glfw clean 
	rm $(OBJ) lib/glfw/Makefile mine 
