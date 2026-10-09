cmake_minimum_required(VERSION 3.20)
project(OVWatchPC LANGUAGES C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(WATCH_DIR "${CMAKE_CURRENT_SOURCE_DIR}/simulator")
set(SDL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/SDL2-2.30.12")
if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "This configuration requires Windows x64 and Visual Studio C++ tools.")
endif()
file(GLOB_RECURSE WATCH_SOURCES CONFIGURE_DEPENDS
    "${WATCH_DIR}/lvgl/src/*.c" "${WATCH_DIR}/user_test/*.c")
add_library(watch_core STATIC src/watch_model.c src/watch_storage.c)
target_include_directories(watch_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_definitions(watch_core PRIVATE _CRT_SECURE_NO_WARNINGS)
target_compile_options(watch_core PRIVATE /utf-8 /W4)
add_executable(watch_core_tests tests/test_watch_core.c)
target_link_libraries(watch_core_tests PRIVATE watch_core)
target_compile_definitions(watch_core_tests PRIVATE _CRT_SECURE_NO_WARNINGS)
target_compile_options(watch_core_tests PRIVATE /utf-8 /W4)
add_executable(ov_watch ${WATCH_SOURCES}
    src/watch_app.c
    src/watch_skin.c
    src/watch_ui_probe.c
    "${WATCH_DIR}/main.c" "${WATCH_DIR}/mouse_cursor_icon.c"
    "${WATCH_DIR}/lv_drivers/sdl/sdl.c")
target_include_directories(ov_watch PRIVATE "${WATCH_DIR}" "${SDL_DIR}/include")
target_compile_definitions(ov_watch PRIVATE LV_CONF_INCLUDE_SIMPLE LV_DRV_CONF_INCLUDE_SIMPLE
    _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES USE_KEYBOARD=0 USE_MOUSE=0 USE_MOUSEWHEEL=0)
target_compile_options(ov_watch PRIVATE /utf-8 /MP)
target_link_libraries(ov_watch PRIVATE watch_core "${SDL_DIR}/lib/x64/SDL2.lib")
set_target_properties(ov_watch PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
foreach(CONFIG Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER "${CONFIG}" CONFIG_UPPER)
    set_target_properties(ov_watch PROPERTIES
        "RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER}" "${CMAKE_BINARY_DIR}/bin")
endforeach()
add_custom_command(TARGET ov_watch POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${SDL_DIR}/lib/x64/SDL2.dll" "$<TARGET_FILE_DIR:ov_watch>/SDL2.dll")
