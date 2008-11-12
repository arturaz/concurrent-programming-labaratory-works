run: nieko viska kazka tusti

tusti: build
	dist/app SlajusA_tusti.txt

nieko: build
	dist/app SlajusA_nieko.txt

viska: build
	dist/app SlajusA_viska.txt

kazka: build
	dist/app SlajusA_kazka.txt

build:
	mkdir -p dist
	g++ -o dist/app -fopenmp SlajusA_L3b.cpp

clean:
	rm -rfv dist
