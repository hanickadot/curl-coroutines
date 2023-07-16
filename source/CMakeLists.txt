add_library(utility INTERFACE utility/format.hpp)
target_include_directories(utility INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(utility INTERFACE cxx_std_23)

find_package(CURL REQUIRED)

add_library(multicurl STATIC multicurl/multicurl.cpp)
target_include_directories(multicurl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(multicurl PUBLIC cxx_std_23)
target_link_libraries(multicurl PRIVATE CURL::libcurl)