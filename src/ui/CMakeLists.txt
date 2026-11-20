add_library(mouse_ui STATIC razermousewindow.cpp razermousewindow.h translations.cpp)
target_include_directories(mouse_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mouse_ui PUBLIC mouse_device Qt6::Widgets Qt6::Concurrent Qt6::Network)
