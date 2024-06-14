all: lib/glad/src/gl.o
	gcc src/*.c -Ilib/glad/include -lglfw $^ -lm -o mine

lib/glad/src/gl.o:
	gcc $(@:.o=.c) -o $@ -Ilib/glad/include -c

clean:
	rm lib/glad/src/gl.o mine 
