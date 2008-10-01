run: build
	mpirun --hostfile hosts -np 5 dist/app

run-sc: build
	mpirun --hostfile hosts-sc -np 5 dist/app

build:
	mkdir -p dist
	mpic++ -o dist/app SlajusA_L1c.cc

clean:
	rm -rfv dist
