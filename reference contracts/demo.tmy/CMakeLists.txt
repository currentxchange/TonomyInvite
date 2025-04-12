add_contract(demo.tmy demo.tmy ${CMAKE_CURRENT_SOURCE_DIR}/src/demo.tmy.cpp)

target_include_directories(demo.tmy
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(demo.tmy
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/demo.tmy.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/demo.tmy.contracts.md @ONLY )

target_compile_options( demo.tmy PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
