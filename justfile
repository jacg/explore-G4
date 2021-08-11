build: cmake
	#!/usr/bin/env zsh
	cd abracadabra/build && make -j

cmake:
	#!/usr/bin/env zsh
	if ! [ -d abracadabra/build ]; then
		mkdir abracadabra/build
		cd    abracadabra/build
		cmake ..
	fi

# Test with ctest
ctest: build
	#!/usr/bin/env zsh
	cd abracadabra/build
	ctest

# Test with hand-written loop: more informative and colourful than ctest
test: build
	#!/usr/bin/env zsh
	cd abracadabra/build
	FAILED=
	./tests-trial --list-test-names-only |
	while read testname
	do
		if ! ./tests-trial "$testname"; then
			FAILED=$FAILED"$testname"\\n
		fi
	done
	if ! [ -z "$FAILED" ]; then
		echo -e "  \\033[91m Failures:\\033[0m"
		echo $FAILED | while read testname
		do
			echo -e "     \\033[91m $testname\\033[0m"
		done
		echo -e "\\033[91m"
		echo "==========================================================================="
		echo "OVERALL: ============================== FAIL =============================="
		echo "==========================================================================="
		echo -e "\\033[0m"
		exit 1
	else
		echo -e "\\033[32m"
		echo "==========================================================================="
		echo "OVERALL: ============================== PASS =============================="
		echo "==========================================================================="
		echo -e "\\033[0m"
	fi


list-tests: build
	#!/usr/bin/env zsh
	cd abracadabra/build
	./tests-trial --list-test-names-only
