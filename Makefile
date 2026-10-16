.PHONY: all deps test run release package clean

all:
	./scripts/build.sh

deps:
	./scripts/build.sh --deps-only

test:
	mkdir -p build
	$(CXX) -std=c++17 -O2 -Wall -Wextra -Wpedantic -Isrc src/core.cpp tests/core_tests.cpp -o build/core-tests
	./build/core-tests

run: all
	./run.sh

release:
	STATIC_RUNTIME=1 ./scripts/build.sh

package: release
	./scripts/package.sh

clean:
	rm -rf build
