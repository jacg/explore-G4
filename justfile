build: cmake
	#!/usr/bin/env sh
	cd abracadabra/build && make -j

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


list-tests: build
	#!/usr/bin/env sh
	cd abracadabra/build
	./tests-trial --list-test-names-only
