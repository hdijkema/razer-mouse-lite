add_library(mouse_device STATIC mousebackend.cpp)
target_include_directories(mouse_device PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mouse_device PUBLIC mouse_platform)
