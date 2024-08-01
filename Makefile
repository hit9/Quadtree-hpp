cmake:
	cmake -S . -B build \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_CXX_STANDARD=20 \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=1

lint:
	cppcheck *.h --enable=warning,style,performance,portability --inline-suppr --language=c++
