cmake_minimum_required(VERSION 3.21)

project(toolbox VERSION 1.0)

if (APPLE)
	set(CMAKE_OSX_DEPLOYMENT_TARGET "13.3")
endif() 

add_subdirectory(source)

add_executable(pokus pokus.cpp)
target_link_libraries(pokus multicurl)

#add_executable(coroutine coroutine.cpp)
#target_link_libraries(coroutine multicurl)

add_executable(coroutine2 coroutine2.cpp)
target_link_libraries(coroutine2 multicurl)

add_executable(task task.cpp)
target_link_libraries(task multicurl)