
cmake_minimum_required(VERSION 3.18)
project(satsuma C CXX)

option(CLIQUES "Build with clique support" OFF)
option(SATSUMA_BUILD_CLI "Build the standalone Satsuma executable" ON)
option(SATSUMA_BUILD_TESTING "Test the CNF output API against the CLI" OFF)

include(FetchContent)
# Install the header API and its dependency as one package. Do not configure
# Dejavu's executable or let it change this project's compiler flags.
set(DEJAVU_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/include/dejavu")
FetchContent_Declare(dejavu
    GIT_REPOSITORY https://github.com/markusa4/dejavu.git
    GIT_TAG 4c275e9ebac4fe51a26aac406682b9bcbfd03a9d
    SOURCE_DIR "${DEJAVU_LOCATION}"
    SOURCE_SUBDIR satsuma-headers-only)
FetchContent_MakeAvailable(dejavu)
# An explicit source override need not be named "dejavu"; Satsuma's includes
# use that name, so stage an override in the build tree when necessary.
if(NOT dejavu_SOURCE_DIR STREQUAL DEJAVU_LOCATION)
    file(COPY "${dejavu_SOURCE_DIR}/" DESTINATION "${DEJAVU_LOCATION}"
         FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")
endif()

add_library(satsuma_headers INTERFACE)
add_library(Satsuma::headers ALIAS satsuma_headers)
target_include_directories(satsuma_headers INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/src" "${CMAKE_CURRENT_BINARY_DIR}/include")
target_compile_features(satsuma_headers INTERFACE cxx_std_20)

if(SATSUMA_BUILD_CLI)
    add_executable(satsuma src/satsuma.cpp)
    target_link_libraries(satsuma PRIVATE Satsuma::headers)
    # Preserve the CLI's assertion policy without imposing it on API clients.
    target_compile_definitions(satsuma PRIVATE NDEBUG)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(satsuma PRIVATE -Wall)
    endif()
    if(CLIQUES)
        add_library(cliquer_c STATIC
            src/cliquer/cliquer.c src/cliquer/graph.c src/cliquer/reorder.c)
        target_include_directories(cliquer_c PUBLIC src/cliquer)
        target_compile_definitions(satsuma PRIVATE CLIQUES=1)
        target_link_libraries(satsuma PRIVATE cliquer_c)
    else()
        target_compile_definitions(satsuma PRIVATE CLIQUES=0)
    endif()
    install(TARGETS satsuma RUNTIME DESTINATION bin)
endif()

install(DIRECTORY src/ DESTINATION include/satsuma
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")
install(DIRECTORY "${DEJAVU_LOCATION}/" DESTINATION include/satsuma/dejavu
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")
install(FILES LICENSE DESTINATION share/licenses/satsuma)
install(FILES "${dejavu_SOURCE_DIR}/LICENSE"
        DESTINATION share/licenses/satsuma RENAME LICENSE.dejavu)

if(SATSUMA_BUILD_TESTING)
    if(NOT SATSUMA_BUILD_CLI OR CLIQUES)
        message(FATAL_ERROR "The CNF output tests require SATSUMA_BUILD_CLI=ON and CLIQUES=OFF")
    endif()
    enable_testing()
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_executable(satsuma-cnf-output tests/cnf_output.cpp)
    target_link_libraries(satsuma-cnf-output PRIVATE Satsuma::headers)
    target_compile_definitions(satsuma-cnf-output PRIVATE CLIQUES=0 NDEBUG)
    add_test(NAME cnf-output
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/cnf_output.py"
                $<TARGET_FILE:satsuma-cnf-output> $<TARGET_FILE:satsuma>
                "${CMAKE_CURRENT_SOURCE_DIR}")
    set_tests_properties(cnf-output PROPERTIES TIMEOUT 180)
endif()
