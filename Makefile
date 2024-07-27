INC =-Ilib/glad/include -Ilib/stb/ -Ilib/glfw/include 
INC += -Ilib/cglm/include -Ilib/perlin-noise/src

OBJ = lib/glad/src/gl.o lib/stb/stb_image.o lib/glfw/src/libglfw3.a
OBJ += lib/perlin-noise/src/noise1234.o

mine: src/*.c src/*.h $(OBJ)
	gcc $(INC) $^ -lm -o $@ -Wall -O3 -s -DNDEBUG

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
	make -C lib/glfw clean 
	rm $(OBJ) lib/glfw/Makefile mine -f 
