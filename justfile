# Test with hand-written loop: more informative and colourful than ctest
test: build
	#!/usr/bin/env bash
	cd abracadabra/build
	FAILED=
	while read -r testname
	do
		if ! ./tests-trial "$testname"; then
			FAILED=$FAILED"$testname"\\n
		fi
	done < <(./tests-trial --list-test-names-only)
	if ! [ -z "$FAILED" ]; then
		printf "\\033[91m Failures: \n\n$FAILED\n"
		echo "==========================================================================="
		echo "OVERALL: ============================== FAIL =============================="
		echo "==========================================================================="
		printf "\\033[0m"
		exit 1
	else
		printf "\\033[32m"
		echo "==========================================================================="
		echo "OVERALL: ============================== PASS =============================="
		echo "==========================================================================="
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
