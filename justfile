# -*-Makefile-*-

# Run all tests in hand-written loop: more informative and colourful than ctest
test-all *FLAGS:
	just test '' {{FLAGS}}

# Need to run each test in a separate process, otherwise the monolithic and
# persistent Geant4 Run and Kernel managers will make things go wrong.

# Run a selection of tests
test PATTERN *FLAGS: build
	#!/usr/bin/env bash
	cd abracadabra/build
	NPASSED=0
	NFAILED=0
	FAILED=
	while read -r testname
	do
		if ! ./tests-trial "$testname" {{FLAGS}}; then
			FAILED=$FAILED"$testname"\\n
			NFAILED=$((NFAILED+1))
		else
			NPASSED=$((NPASSED+1))
		fi
	done < <(./tests-trial {{PATTERN}} --list-test-names-only)
	if ! [ -z "$FAILED" ]; then
		printf "\\033[91m===========================================================================\n"
		printf "\\033[32m Passed $NPASSED tests, \\033[91m Failed $NFAILED\n\n"
		printf "\\033[91m Failures: \n\n$FAILED\n"
		printf "\\033[91m===========================================================================\n"
		printf "\\033[91mOVERALL: ============================== FAIL ==============================\n"
		printf "\\033[91m===========================================================================\n"
		printf "\\033[0m"
		exit 1
	else
		printf "\\033[32m Ran $NPASSED tests\n\n"
		printf "\\033[32m===========================================================================\n"
		printf "\\033[32mOVERALL: ============================== PASS ==============================\n"
		printf "\\033[32m===========================================================================\n"
		printf "\\033[0m"
	fi

# TODO Add mechanism for overriding setting in macros, on the `just` CLI

# Load macs/<model>.mac and run macs/<run>.mac in batch mode
run model='model' run='run': build
	#!/usr/bin/env sh
	cd abracadabra/build
	./abracadabra macs/{{model}}.mac macs/{{run}}.mac

# Like `run` but without fully explicit macro file names
run-full-path model run: build
	#!/usr/bin/env sh
	cd abracadabra/build
	./abracadabra {{model}} {{run}}

# Load <model> in interactive mode
interact model='model': build
	#!/usr/bin/env sh
	cd abracadabra/build
	./abracadabra macs/{{model}}.mac

# Load <model> and run <run> in batch mode
run-debug model='model' run='run': build
	#!/usr/bin/env sh
	cd abracadabra/build
	gdb --args ./abracadabra macs/{{model}}.mac macs/{{run}}.mac

# Load <model> in interactive mode
interact-debug model='model': build
	#!/usr/bin/env sh
	cd abracadabra/build
	gdb --args ./abracadabra macs/{{model}}.mac

# List available model configurations
list-models:
	#!/usr/bin/env sh
	cd abracadabra/macs
	ls -1 *model.mac | sed 's/.mac//g'

# List available run configurations
list-runs:
	#!/usr/bin/env sh
	cd abracadabra/macs
	ls -1 *run.mac | sed 's/.mac//g'

# List available model and run configurations
list-macros: list-models list-runs

build: cmake
	#!/usr/bin/env sh
	cd abracadabra/build && cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo . && make -j

clean:
	rm abracadabra/build -rf

cmake:
	#!/usr/bin/env sh
	if ! [ -d abracadabra/build ]; then
		mkdir abracadabra/build
		cd    abracadabra/build
		cmake ..
	fi

# Test with ctest
ctest: build
	#!/usr/bin/env sh
	cd abracadabra/build
	ctest

list-tests: build
	#!/usr/bin/env sh
	cd abracadabra/build
	./tests-trial --list-test-names-only
