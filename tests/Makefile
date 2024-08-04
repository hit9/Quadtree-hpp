defalut: build

install:
	conan install . --output-folder=build --build=missing -s compiler.cppstd=20 -s build_type=Debug

cmake:
	cd build && cmake .. \
		-DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake \
		-DCMAKE_BUILD_TYPE=Debug \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=1

build: cmake
	cd build && make

run:
	ctest --test-dir ./build --output-on-failure

clean:
	make -C build clean

.PHONY: build
