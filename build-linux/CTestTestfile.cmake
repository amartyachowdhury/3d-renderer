# CMake generated Testfile for 
# Source directory: /src
# Build directory: /src/build-linux
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(math "/src/build-linux/test_math")
set_tests_properties(math PROPERTIES  _BACKTRACE_TRIPLES "/src/CMakeLists.txt;44;add_test;/src/CMakeLists.txt;0;")
add_test(raycaster_unit "/src/build-linux/test_raycaster")
set_tests_properties(raycaster_unit PROPERTIES  WORKING_DIRECTORY "/src" _BACKTRACE_TRIPLES "/src/CMakeLists.txt;48;add_test;/src/CMakeLists.txt;0;")
add_test(rasterizer_unit "/src/build-linux/test_rasterizer")
set_tests_properties(rasterizer_unit PROPERTIES  _BACKTRACE_TRIPLES "/src/CMakeLists.txt;52;add_test;/src/CMakeLists.txt;0;")
add_test(golden "/src/build-linux/test_golden" "/src/build-linux/raytracer" "/src/build-linux/rasterizer" "/src/build-linux/raycaster" "/src")
set_tests_properties(golden PROPERTIES  WORKING_DIRECTORY "/src" _BACKTRACE_TRIPLES "/src/CMakeLists.txt;55;add_test;/src/CMakeLists.txt;0;")
