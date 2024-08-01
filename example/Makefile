defalut: build

cmake:
	cmake -S  . -B build \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=1

build: cmake
	cd build && make

run:
	./build/quadtree_example

clean:
	make -C build clean

.PHONY: build
