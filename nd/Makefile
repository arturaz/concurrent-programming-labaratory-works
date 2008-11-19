single: build
	mpirun -np 10 dist/app

run: build
	mpirun --hostfile hosts -np 10 dist/app

run-sc: build
	mpirun --hostfile hosts-sc -np 10 dist/app

build:
	mkdir -p dist
	mpic++ -o dist/app SlajusA_nd.cpp

clean:
	rm -rfv dist
