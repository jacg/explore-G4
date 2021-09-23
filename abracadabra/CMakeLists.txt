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
  src/geometries/imas.hh
  src/geometries/nema.hh
  src/geometries/sipm.hh
  src/geometries/samples.hh
  src/geometries/inspect.hh
  src/materials/LXe.hh
  src/messengers/abracadabra.hh
  src/messengers/generator.hh
  src/random/random.hh
  src/utils/enumerate.hh
  src/utils/interpolate.hh
  src/utils/map_set.hh
  src/io/hdf5.hh
)

set(ABRACADABRA_SOURCES
  src/geometries/imas.cc
  src/geometries/nema.cc
  src/geometries/sipm.cc
  src/geometries/samples.cc
  src/geometries/inspect.cc
  src/messengers/abracadabra.cc
  src/messengers/generator.cc
  src/materials/LXe.cc
  src/random/random.cc
  src/io/hdf5.cc
)

set(ABRACADABRA_TESTS
  test/test-nain4.cc
  src/geometries/imas-test.cc
  src/geometries/nema-test.cc
  src/geometries/sipm_hamamatsu_blue-test.cc
  src/geometries/inspect-test.cc
  test/trivial-full-app-test.cc
  src/materials/LXe-test.cc
  test/nema-phantom-generator-test.cc
  src/random/random-test.cc
  src/utils/enumerate-test.cc
)

find_package(HighFive REQUIRED)
find_package(Poco REQUIRED COMPONENTS Foundation)

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
target_link_libraries(
  tests-trial
  PUBLIC
  Catch2::Catch2
  ${Geant4_LIBRARIES}
  Nain4
  hdf5
  HighFive
  PocoFoundation)
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
target_link_libraries(
  abracadabra
  ${Geant4_LIBRARIES}
  Nain4
  hdf5
  HighFive
  PocoFoundation)
target_include_directories(
  abracadabra PUBLIC
  ${ABRACADABRA_HEADERS}
  "${PROJECT_SOURCE_DIR}/nain4"
)

#----------------------------------------------------------------------------
# Link macro files directory to the build directory, i.e. the directory in which
# we build abracadabra. This is so that we can run the executable directly
# because it relies on these macros being in the current working directory.
#
add_custom_command(
  TARGET abracadabra POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E create_symlink
  ${CMAKE_SOURCE_DIR}/macs $<TARGET_FILE_DIR:abracadabra>/macs
  COMMENT "Symlinking G4 macros directory" VERBATIM
  )


#----------------------------------------------------------------------------
# Install the executable to 'bin' directory under CMAKE_INSTALL_PREFIX
#
install(TARGETS abracadabra DESTINATION bin)
