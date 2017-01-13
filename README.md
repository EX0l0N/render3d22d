# Render3d22d
(read as "render 3D to 2D")

![](https://www.exoz.net/render3d22d/hello_world.jpg)

### what it is

#### Impatient? [**See it in action!**](https://www.exoz.net/render3d22d/)

- Render3d22d is a proof of concept to try out if it is possible, to render 3D content to a 2D HTML canvas,
when no hardware acceleration is available.
- Although it was developed to run in web browsers, it was not implemented in JavaScript but in plain C.
- Using the amazing [emscripten compiler suite](http://kripken.github.io/emscripten-site/)
Render3d22d was compiled to strongly typed, optimized Javascript conforming to [asm.js](http://asmjs.org/).
- Because the native [html5.h](http://kripken.github.io/emscripten-site/docs/api_reference/html5.h.html)
of emscripten is meant to develop fast [WebGL](https://www.khronos.org/webgl/) applications,
Render3d22d is using emscriptens [SDL](https://www.libsdl.org/) support,
which you would typically use to port old SDL applications.
- SDL support seems to be the only possible way to get an unaccelerated 2D html canvas,
but it offers the possibility to develop (and debug) a native C application, whilst be able to compile to HTML/JS all the time.
- To do some of the grunt work, Render3d22d relies on parts of
[coreh/gl-matrix.c](https://github.com/coreh/gl-matrix.c).

### this respository contains
- the C source code
- a binary for linux-x64 (you need to have libSDL1.2 and libSDL_image installed)
- a binary for win32 and support libs (just checkout the whole directory)
- the HTML/JS files used for the live demo

## how to compile
*No, there is no makefile!* (if anyone is willing to write one, I'll include it)

SDL based binary (provided you have dev packages for libSDL and libSDL_image installed):

    gcc -O3 -Wall -Werror -pedantic-errors -std=c99 -D_GNU_SOURCE=1 -D_REENTRANT -I/usr/include/SDL \
    render.c mat?.c quat.c vec?.c -lSDL -lSDL_image -lm -o render

Compile to HTML/JS unsing the
[Emscripten SDK](http://kripken.github.io/emscripten-site/docs/getting_started/downloads.html#sdk-download-and-install):

    emcc -O3 -Wall -Werror -pedantic-errors -std=c99 \
    -D_GNU_SOURCE=1 -D_REENTRANT -s TOTAL_MEMORY=32000000 \
    render.c mat?.c quat.c vec?.c -o index.html \
    --use-preload-plugins --preload-file "./color_map_2048.jpg"
