# Test with hand-written loop: more informative and colourful than ctest
test *FLAGS: build
# Need to run each test in a separate process, otherwise the monolithic and
# persistent Geant4 Run and Kernel managers will make things go wrong.

	#!/usr/bin/env bash
	cd abracadabra/build
	NPASSED=0
	NFAILED=0
	FAILED=
	while read -r testname
	do
		if ! ./tests-trial {{FLAGS}} "$testname"; then
			FAILED=$FAILED"$testname"\\n
			NFAILED=$((NFAILED+1))
		else
			NPASSED=$((NPASSED+1))
		fi
	done < <(./tests-trial --list-test-names-only)
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

# Load <model> and run <run> in batch mode
run model='model' run='run': build
	#!/usr/bin/env sh
	cd abracadabra/build
	./abracadabra macs/{{model}}.mac macs/{{run}}.mac

# Load <model> in interactive mode
interact model='model': build
	#!/usr/bin/env sh
	cd abracadabra/build
	./abracadabra macs/{{model}}.mac

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
	cd abracadabra/build && make -j

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
