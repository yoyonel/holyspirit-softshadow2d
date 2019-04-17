# Packages

## Fedora release 29 (Twenty Nine)

```sh
dnf install
	openal-soft-devel.x86_64
	libsndfile-devel
 	glew-devel
	libXrandr-devel
```

# Build

```sh
╰─ mkdir build/
╰─ cd build

╰─ cmake ..
[...]
-- Configuring done
-- Generating done
-- Build files have been written to: /home/atty/Prog/holyspirit-softshadow2d/build


╰─ make -j[nb_core - 1]
[...]
[100%] Linking CXX shared library ../../../lib/libsfml-graphics.so
[100%] Built target sfml-graphics

╰─ sudo make install
[...]
-- Installing: /usr/local/lib/libsfml-graphics.so.2.0.0
-- Installing: /usr/local/lib/libsfml-audio.so.2.0.0
