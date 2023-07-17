cmake_minimum_required(VERSION 3.21)

project(toolbox VERSION 1.0)

if (APPLE)
	set(CMAKE_OSX_DEPLOYMENT_TARGET "13.3")
endif() 

add_subdirectory(source)

add_executable(task task.cpp)
target_link_libraries(task multicurl)

add_executable(download_all download_all.cpp)
target_link_libraries(download_all multicurl)

add_executable(example example.cpp)
target_link_libraries(example multicurl)

add_executable(generator generator.cpp)
target_link_libraries(generator multicurl)
