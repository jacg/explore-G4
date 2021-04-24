add_library(Nain4 nain4.cc g4-mandatory.cc)

find_package(Geant4 REQUIRED)

include(${Geant4_USE_FILE})

target_link_libraries(Nain4 PUBLIC ${Geant4_LIBRARIES})
