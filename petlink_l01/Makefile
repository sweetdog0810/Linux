.PHONY: configure build run run0 run1 list sim test test-l00 test-l01 debug clean tree

configure:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

build: configure
	cmake --build build

run: run1

run0: build
	./build/petlink_l00

run1: build
	./build/petlink_l01

list: build
	./build/petlink_l01 --list

sim: build
	python3 tools/stm32_text_simulator.py

test: test-l00 test-l01

# L00 回归测试：升级到 L01 后，旧功能仍然要能通过。
test-l00: build
	bash scripts/self_check.sh

# L01 静态检查 + 伪串口集成检查。
test-l01: build
	bash scripts/l01_static_check.sh
	python3 scripts/l01_integration_check.py
	python3 scripts/l01_disconnect_check.py

debug: build
	gdb ./build/petlink_l01

clean:
	rm -rf build
	rm -f logs/*.log
	rm -f reports/*.txt

tree:
	find . -maxdepth 3 -not -path './.git/*' -not -path './build/*' | sort
