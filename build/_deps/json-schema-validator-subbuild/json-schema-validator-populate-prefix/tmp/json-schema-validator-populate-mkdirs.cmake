# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/jude/EraserStatic/build/_deps/json-schema-validator-src"
  "/home/jude/EraserStatic/build/_deps/json-schema-validator-build"
  "/home/jude/EraserStatic/build/_deps/json-schema-validator-subbuild/json-schema-validator-populate-prefix"
  "/home/jude/EraserStatic/build/_deps/json-schema-validator-subbuild/json-schema-validator-populate-prefix/tmp"
  "/home/jude/EraserStatic/build/_deps/json-schema-validator-subbuild/json-schema-validator-populate-prefix/src/json-schema-validator-populate-stamp"
  "/home/jude/EraserStatic/build/_deps/json-schema-validator-subbuild/json-schema-validator-populate-prefix/src"
  "/home/jude/EraserStatic/build/_deps/json-schema-validator-subbuild/json-schema-validator-populate-prefix/src/json-schema-validator-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/jude/EraserStatic/build/_deps/json-schema-validator-subbuild/json-schema-validator-populate-prefix/src/json-schema-validator-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/jude/EraserStatic/build/_deps/json-schema-validator-subbuild/json-schema-validator-populate-prefix/src/json-schema-validator-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
