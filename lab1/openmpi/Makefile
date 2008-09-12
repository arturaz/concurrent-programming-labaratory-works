run: build
	mpirun -np 5 dist/app

build:
	mkdir -p dist
	mpic++ -o dist/app SlajusA_L1c.cc

clean:
	rm -rfv dist
