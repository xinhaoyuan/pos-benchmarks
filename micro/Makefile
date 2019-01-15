.PHONY: all clean

all:
	mkdir -p build
	cd build; cmake .. -DCMAKE_BUILD_TYPE=Release; make

clean:
	rm -r build paper-micro-bench
	rm *.tex *.html *.csv
