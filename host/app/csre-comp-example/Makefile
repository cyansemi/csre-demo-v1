.PHONY: all

csre-comp-example: src/csre-comp-example.cpp
	g++ $< -I./include -L./lib -lcsre-comp -lrecomp -o $@

clean:
	rm csre-comp-example
