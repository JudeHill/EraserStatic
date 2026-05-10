# CMake generated Testfile for 
# Source directory: /home/jude/EraserStatic/build/_deps/json-schema-validator-src/test/issue-54
# Build directory: /home/jude/EraserStatic/build/_deps/json-schema-validator-build/test/issue-54
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(Issue::54 "/home/jude/EraserStatic/build/_deps/json-schema-validator-src/test/test-pipe-in.sh" "/home/jude/EraserStatic/build/_deps/json-schema-validator-build/test/json-schema-validate" "/home/jude/EraserStatic/build/_deps/json-schema-validator-src/test/issue-54/schema.json" "/home/jude/EraserStatic/build/_deps/json-schema-validator-src/test/issue-54/instance.json")
set_tests_properties(Issue::54 PROPERTIES  WORKING_DIRECTORY "/home/jude/EraserStatic/build/_deps/json-schema-validator-src/test/issue-54" _BACKTRACE_TRIPLES "/home/jude/EraserStatic/build/_deps/json-schema-validator-src/test/CMakeLists.txt;8;add_test;/home/jude/EraserStatic/build/_deps/json-schema-validator-src/test/issue-54/CMakeLists.txt;1;add_test_simple_schema;/home/jude/EraserStatic/build/_deps/json-schema-validator-src/test/issue-54/CMakeLists.txt;0;")
