# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\PocketLibApp_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\PocketLibApp_autogen.dir\\ParseCache.txt"
  "PocketLibApp_autogen"
  )
endif()
