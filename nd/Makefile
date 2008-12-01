build: gen
	mkdir -p dist
	mpic++ -o dist/app SlajusA_nd.cpp

single: build
	mpirun -np 10 dist/app dataA dataB

run: build
	mpirun --hostfile hosts -np 10 dist/app

run-sc: build
	mpirun --hostfile hosts-sc -np 10 dist/app

clean:
	rm -rfv dist

gen:
	g++ -o generator generator.cpp
