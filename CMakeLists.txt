cmake_minimum_required(VERSION 3.15)
project(mysylar)

set(CMAKE_CXX_STANDARD 14)

add_subdirectory(thread)
add_subdirectory(common)
find_package (glog 0.3.5 REQUIRED)

link_directories(/home/l1nkkk/share/lib/netbase)
include_directories(/home/l1nkkk/share/include/netbase)


add_executable(mysylar main.cpp io/iomanager.cpp io/iomanager.h)
# 玄学
target_link_libraries(mysylar l1nkkkthread)
target_link_libraries(mysylar  glog::glog l1nkkkcommon -lpthread)
target_link_libraries(mysylar  netbase )
