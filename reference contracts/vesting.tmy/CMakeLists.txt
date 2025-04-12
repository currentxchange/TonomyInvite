add_contract(vesting.tmy vesting.tmy ${CMAKE_CURRENT_SOURCE_DIR}/src/vesting.tmy.cpp)

target_include_directories(vesting.tmy
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(vesting.tmy
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/vesting.tmy.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/vesting.tmy.contracts.md @ONLY )

target_compile_options( vesting.tmy PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
