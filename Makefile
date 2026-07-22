.PHONY: build test lint bench odin odin-test clean

build:
	pip install .

test:
	python -B tests/test_correctness.py

lint:
	ruff check .

bench:
	python bench.py

odin:
	cd odin && ./build.sh

odin-test: odin
	cd odin && ./sf test

clean:
	rm -rf build dist wheelhouse *.egg-info .ruff_cache .pytest_cache .mypy_cache
	rm -f fastbrute/_native*.so fastbrute/_native*.pyd odin/sf odin/sf.exe
	find . -name '__pycache__' -type d -prune -exec rm -rf {} +
