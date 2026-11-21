add_library(mouse_ui STATIC razermousewindow.cpp widgets.cpp appearance.cpp)
target_include_directories(mouse_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} PRIVATE "${PROJECT_BINARY_DIR}/generated")
target_link_libraries(mouse_ui PUBLIC mouse_device fltk-css::fltk-css fltk::images)
