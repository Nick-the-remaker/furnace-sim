cmake_minimum_required(VERSION 2.8)
find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )
include_directories(../src)

set(ApplicationName simulator)

# FILE(GLOB SC_FILES "src/*.cpp" "src/*.h")
# add_executable(${ApplicationName} ${SC_FILES})

add_executable(${ApplicationName} src/furnace-sim.cpp)
target_link_libraries( ${ApplicationName} ${OpenCV_LIBS})
set_property(TARGET ${ApplicationName} PROPERTY CXX_STANDARD 11)