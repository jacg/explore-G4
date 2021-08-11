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

# Test with hand-written loop: more colourful than ctest
test: build
	#!/usr/bin/env zsh
	cd abracadabra/build
	FAILED=
	./tests-trial --list-test-names-only |
	while read testname
	do
		if ! ./tests-trial "$testname"; then
			echo -e "\\033[91m THIS ONE FAILED with $?\\033[0m"
			FAILED=$FAILED"$testname"\\n
		else
			echo -e "\\033[32m THIS ONE PASSED\\033[0m"
		fi
		echo "================================"
	done
	if ! [ -z "$FAILED" ]; then
		echo -e "\\033[91m OVERALL: ==================== FAIL ====================\\033[0m"
		echo $FAILED | while read testname
		do
			echo -e "     \\033[91m $testname\\033[0m"
		done
		exit 1
	else
		echo -e "\\033[32m OVERALL: ==================== PASS ====================\\033[0m"
	fi


list-tests: build
	#!/usr/bin/env zsh
	cd abracadabra/build
	./tests-trial --list-test-names-only
