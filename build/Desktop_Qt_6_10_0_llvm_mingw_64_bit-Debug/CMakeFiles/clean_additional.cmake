# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\PipMatrixResolverQt_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\PipMatrixResolverQt_autogen.dir\\ParseCache.txt"
  "PipMatrixResolverQt_autogen"
  )
endif()
