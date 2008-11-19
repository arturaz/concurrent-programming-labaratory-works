single: build
	dist/app

run: build
	mpirun --hostfile hosts -np 5 dist/app

run-sc: build
	mpirun --hostfile hosts-sc -np 5 dist/app

build:
	mkdir -p dist
	mpic++ -o dist/app SlajusA_nd.cpp

clean:
	rm -rfv dist
