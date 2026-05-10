# CMake generated Testfile for 
# Source directory: /home/jude/EraserStatic/build/_deps/json-schema-validator-src/test/issue-96
# Build directory: /home/jude/EraserStatic/build/_deps/json-schema-validator-build/test/issue-96
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(Issue::96 "/home/jude/EraserStatic/build/_deps/json-schema-validator-src/test/test-pipe-in.sh" "/home/jude/EraserStatic/build/_deps/json-schema-validator-build/test/json-schema-validate" "/home/jude/EraserStatic/build/_deps/json-schema-validator-src/test/issue-96/schema.json" "/home/jude/EraserStatic/build/_deps/json-schema-validator-src/test/issue-96/instance.json")
set_tests_properties(Issue::96 PROPERTIES  WILL_FAIL "1" WORKING_DIRECTORY "/home/jude/EraserStatic/build/_deps/json-schema-validator-src/test/issue-96" _BACKTRACE_TRIPLES "/home/jude/EraserStatic/build/_deps/json-schema-validator-src/test/CMakeLists.txt;8;add_test;/home/jude/EraserStatic/build/_deps/json-schema-validator-src/test/issue-96/CMakeLists.txt;1;add_test_simple_schema;/home/jude/EraserStatic/build/_deps/json-schema-validator-src/test/issue-96/CMakeLists.txt;0;")
