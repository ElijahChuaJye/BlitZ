# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "C:/SVN/e.chua SVN_BENCH/trunk/a. Personal Projects/Blitz/build3D/_deps/vma-src")
  file(MAKE_DIRECTORY "C:/SVN/e.chua SVN_BENCH/trunk/a. Personal Projects/Blitz/build3D/_deps/vma-src")
endif()
file(MAKE_DIRECTORY
  "C:/SVN/e.chua SVN_BENCH/trunk/a. Personal Projects/Blitz/build3D/_deps/vma-build"
  "C:/SVN/e.chua SVN_BENCH/trunk/a. Personal Projects/Blitz/build3D/_deps/vma-subbuild/vma-populate-prefix"
  "C:/SVN/e.chua SVN_BENCH/trunk/a. Personal Projects/Blitz/build3D/_deps/vma-subbuild/vma-populate-prefix/tmp"
  "C:/SVN/e.chua SVN_BENCH/trunk/a. Personal Projects/Blitz/build3D/_deps/vma-subbuild/vma-populate-prefix/src/vma-populate-stamp"
  "C:/SVN/e.chua SVN_BENCH/trunk/a. Personal Projects/Blitz/build3D/_deps/vma-subbuild/vma-populate-prefix/src"
  "C:/SVN/e.chua SVN_BENCH/trunk/a. Personal Projects/Blitz/build3D/_deps/vma-subbuild/vma-populate-prefix/src/vma-populate-stamp"
)

set(configSubDirs Debug)
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/SVN/e.chua SVN_BENCH/trunk/a. Personal Projects/Blitz/build3D/_deps/vma-subbuild/vma-populate-prefix/src/vma-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/SVN/e.chua SVN_BENCH/trunk/a. Personal Projects/Blitz/build3D/_deps/vma-subbuild/vma-populate-prefix/src/vma-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
