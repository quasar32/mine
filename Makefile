INC=-Ilib/glad/include -Ilib/stb/

all: lib/glad/src/gl.o lib/stb/stb_image.o
	gcc src/*.c $(INC) -lglfw $^ -lm -o mine -Wall

lib/glad/src/gl.o:
	gcc $(@:.o=.c) -o $@ -Ilib/glad/include -c

lib/stb/stb_image.o:
	gcc -x c $(@:.o=.h) -o $@ -DSTB_IMAGE_IMPLEMENTATION -c 

clean:
	rm lib/glad/src/gl.o lib/stb_image.o mine 
