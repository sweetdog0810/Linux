.PHONY: configure build run test debug clean tree

configure:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

build: configure
	cmake --build build

run: build
	./build/petlink_l00

test: build
	bash scripts/self_check.sh

debug: build
	gdb ./build/petlink_l00

clean:
	rm -rf build
	rm -f logs/*.log

tree:
	find . -maxdepth 3 -not -path './.git/*' -not -path './build/*' | sort
