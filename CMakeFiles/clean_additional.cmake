# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  "CMakeFiles/MyQt6App_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/MyQt6App_autogen.dir/ParseCache.txt"
  "MyQt6App_autogen"
  )
endif()
