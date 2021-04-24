#----------------------------------------------------------------------------
# Setup the project
cmake_minimum_required(VERSION 3.19)
project(ABRACADABRA
  VERSION 0.1
  DESCRIPTION "Trying to understand Geant4"
  LANGUAGES CXX)

# ----- Generate compile_commands.json, unless otherwise instructed on CLI --
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
# ----- Ensure that standard C++ headers are found by clangd
if(CMAKE_EXPORT_COMPILE_COMMANDS)
  set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()

# ----- No point in staying in pre-history: use modern standards ------------
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(nain4)

# The files that make up the headers/source/tests of the project
set(ABRACADABRA_HEADERS
  src/g4-mandatory/action_initialization.hh
  src/g4-mandatory/detector_construction.hh
  src/g4-mandatory/event_action.hh
  src/g4-mandatory/primary_generator_action.hh
  src/g4-mandatory/run_action.hh
  src/g4-mandatory/stepping_action.hh
  src/geometries/imas.hh
  src/geometries/nema.hh
  src/geometries/sipm.hh
  src/random/random.hh
  src/utils/enumerate.hh
  src/io/hdf5.hh
)

set(ABRACADABRA_SOURCES
  src/g4-mandatory/action_initialization.cc
  src/g4-mandatory/detector_construction.cc
  src/g4-mandatory/event_action.cc
  src/g4-mandatory/primary_generator_action.cc
  src/g4-mandatory/run_action.cc
  src/g4-mandatory/stepping_action.cc
  src/geometries/imas.cc
  src/geometries/nema.cc
  src/geometries/sipm.cc
  src/random/random.cc
  src/io/hdf5.cc
)

set(ABRACADABRA_TESTS
  test/test-nain4.cc
  src/geometries/imas-test.cc
  src/geometries/nema-test.cc
  src/geometries/sipm_hamamatsu_blue-test.cc
  test/trivial-full-app-test.cc
  test/nema-phantom-generator-test.cc
  src/random/random-test.cc
  src/utils/enumerate-test.cc
)

find_package(HighFive REQUIRED)

# ----- Use Catch2 as C++ testing framework ---------------------------------
find_package(Catch2 REQUIRED)
set(ALL_TEST_SOURCES
  catch2-main-test.cc
  ${ABRACADABRA_SOURCES}
  ${ABRACADABRA_HEADERS}
  ${ABRACADABRA_TESTS}
)

# TODO including headers in add_executable apparently makes them show up in IDEs
add_executable(tests-trial ${ALL_TEST_SOURCES})
target_link_libraries(tests-trial PUBLIC Catch2::Catch2 ${Geant4_LIBRARIES} Nain4 hdf5 HighFive)
include(CTest)
include(Catch)
catch_discover_tests(tests-trial)

#----------------------------------------------------------------------------
# Find Geant4 package, activating all available UI and Vis drivers by default
# You can set WITH_GEANT4_UIVIS to OFF via the command line or ccmake/cmake-gui
# to build a batch mode only executable
#
option(WITH_GEANT4_UIVIS "Build abracadabra with Geant4 UI and Vis drivers" ON)
if(WITH_GEANT4_UIVIS)
  find_package(Geant4 REQUIRED ui_all vis_all)
else()
  find_package(Geant4 REQUIRED)
endif()

#----------------------------------------------------------------------------
# Setup Geant4 include directories and compile definitions
# Setup include directory for this project
#
include(${Geant4_USE_FILE})
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/nain4)

#----------------------------------------------------------------------------
# Add the executable, and link it to the Geant4 libraries
#
# NB: headers are included so they will show up in IDEs
add_executable(abracadabra
  abracadabra.cc
  ${ABRACADABRA_SOURCES}
  ${ABRACADABRA_HEADERS}
)
target_link_libraries(abracadabra ${Geant4_LIBRARIES} Nain4 hdf5 HighFive)
target_include_directories(
  abracadabra PUBLIC
  ${ABRACADABRA_HEADERS}
  "${PROJECT_SOURCE_DIR}/nain4"
)

#----------------------------------------------------------------------------
# Copy all scripts to the build directory, i.e. the directory in which we
# build abracadabra. This is so that we can run the executable directly because it
# relies on these scripts being in the current working directory.
#
set(ABRACADABRA_SCRIPTS
  abracadabra.in
  abracadabra.out
  init_vis.mac
  run1.mac
  run2.mac
  vis.mac
  )

foreach(_script ${ABRACADABRA_SCRIPTS})
  configure_file(
    ${PROJECT_SOURCE_DIR}/${_script}
    ${PROJECT_BINARY_DIR}/${_script}
    COPYONLY
    )
endforeach()

#----------------------------------------------------------------------------
# Install the executable to 'bin' directory under CMAKE_INSTALL_PREFIX
#
install(TARGETS abracadabra DESTINATION bin)
