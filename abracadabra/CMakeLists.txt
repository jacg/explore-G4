#----------------------------------------------------------------------------
# Setup the project
cmake_minimum_required(VERSION 3.19)
project(ABRACADABRA
  VERSION 0.1
  DESCRIPTION "Trying to understand Geant4"
  LANGUAGES CXX)

# ----- No point in staying in pre-history: use modern standards ------------
set(CMAKE_CXX_STANDARD 17)


add_subdirectory(nain4)

# ----- Use Catch2 as C++ testing framework ---------------------------------
find_package(Catch2 REQUIRED)
file(GLOB TEST_SOURCES
  *-test.cc
  test/*.cc
  test/geometries/*.cc
  src/*.cc
  src/geometries/*.cc
  src/g4-mandatory/*.cc
  )
add_executable(tests-trial ${TEST_SOURCES})
target_link_libraries(tests-trial PUBLIC Catch2::Catch2 ${Geant4_LIBRARIES} Nain4)
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
# Locate sources and headers for this project
# NB: headers are included so they will show up in IDEs
#
file(GLOB sources
  ${PROJECT_SOURCE_DIR}/src/*.cc
  ${PROJECT_SOURCE_DIR}/src/geometries/*.cc
  ${PROJECT_SOURCE_DIR}/src/g4-mandatory/*.cc
  )
file(GLOB headers
  ${PROJECT_SOURCE_DIR}/include/*.hh
  ${PROJECT_SOURCE_DIR}/include/geometries/*.hh
  ${PROJECT_SOURCE_DIR}/include/g4-mandatory/*.hh
  )

#----------------------------------------------------------------------------
# Add the executable, and link it to the Geant4 libraries
#
add_executable(abracadabra abracadabra.cc ${sources} ${headers})
target_link_libraries(abracadabra ${Geant4_LIBRARIES} Nain4)
target_include_directories(
  abracadabra PUBLIC
  "${PROJECT_SOURCE_DIR}/include"
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
