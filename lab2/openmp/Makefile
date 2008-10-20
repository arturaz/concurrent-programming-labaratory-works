run: run1 run2 run3

run1: build
	dist/app SlajusA_1.txt

run2: build
	dist/app SlajusA_2.txt

run3: build
	dist/app SlajusA_3.txt

build:
	mkdir -p dist
	g++ -o dist/app -fopenmp SlajusA_L2b.cc

clean:
	rm -rfv dist
