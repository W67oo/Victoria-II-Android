# CMakeList.txt : CMake project for DataContainer_OOS_Reporter, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (DataContainer_OOS_Reporter "DataContainer_OOS_Reporter.cpp" "Parsing/parsing.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET DataContainer_OOS_Reporter PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
