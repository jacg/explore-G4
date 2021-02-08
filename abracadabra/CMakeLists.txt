#----------------------------------------------------------------------------
# Setup the project
cmake_minimum_required(VERSION 3.19)
project(ABRACADABRA)

# ----- No point in staying in pre-history: use modern standards ------------
set(CMAKE_CXX_STANDARD 17)

# ----- Hacking linkable nani4
add_library(trivial include/trivial.hh src/trivial.cc)
# ----- Use Catch2 as C++ testing framework ---------------------------------
find_package(Catch2 REQUIRED)
file(GLOB TEST_SOURCES test/*.cc)
add_executable(tests-trial ${TEST_SOURCES})
target_link_libraries(tests-trial PRIVATE Catch2::Catch2 trivial)
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
include_directories(${PROJECT_SOURCE_DIR}/include)

#----------------------------------------------------------------------------
# Locate sources and headers for this project
# NB: headers are included so they will show up in IDEs
#
file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)

#----------------------------------------------------------------------------
# Add the executable, and link it to the Geant4 libraries
#
add_executable(abracadabra abracadabra.cc ${sources} ${headers})
target_link_libraries(abracadabra ${Geant4_LIBRARIES})

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
