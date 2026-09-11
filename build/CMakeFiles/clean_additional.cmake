# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  "CMakeFiles\\gscoop_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\gscoop_autogen.dir\\ParseCache.txt"
  "gscoop_autogen"
  )
endif()
