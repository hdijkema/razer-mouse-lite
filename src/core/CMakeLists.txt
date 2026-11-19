add_library(mouse_core STATIC profile.cpp razerprotocol.cpp hiddescriptor.cpp)
target_include_directories(mouse_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mouse_core PUBLIC Qt6::Gui)
