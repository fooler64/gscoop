cmake_minimum_required(VERSION 3.16)
project(gscoop VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Windows 资源（exe 图标 + 版本信息）
if(WIN32)
    enable_language(RC)
    set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -I${CMAKE_CURRENT_SOURCE_DIR}/resources")
endif()

# 查找 Qt6
find_package(Qt6 REQUIRED COMPONENTS Widgets Network Concurrent)

set(RESOURCES
    resources/app.qrc
    resources/gscoop.rc
)

set(SOURCES
    src/main.cpp
    src/models/scoop_models.cpp
    src/core/scoop_service.cpp
    src/core/settings_store.cpp
    src/core/theme_manager.cpp
    src/ui/main_window.cpp
    src/ui/animated_stack.cpp
    src/ui/activity_bar.cpp
    src/ui/theme.cpp
    src/ui/search_page.cpp
    src/ui/installed_page.cpp
    src/ui/bucket_page.cpp
    src/ui/settings_page.cpp
    src/ui/package_info_dialog.cpp
    src/ui/icon_painter.cpp
    src/ui/add_bucket_dialog.cpp
    src/ui/add_buckets_dialog.cpp
    src/ui/explore_bucket_dialog.cpp
    src/ui/bucket_info_dialog.cpp
    src/ui/doctor_page.cpp
    src/ui/modern_style.cpp
    src/ui/window_button.cpp
)

set(HEADERS
    src/models/scoop_models.h
    src/core/scoop_service.h
    src/core/settings_store.h
    src/core/theme_manager.h
    src/ui/main_window.h
    src/ui/animated_stack.h
    src/ui/activity_bar.h
    src/ui/theme.h
    src/ui/search_page.h
    src/ui/installed_page.h
    src/ui/bucket_page.h
    src/ui/settings_page.h
    src/ui/package_info_dialog.h
    src/ui/icon_painter.h
    src/ui/add_bucket_dialog.h
    src/ui/add_buckets_dialog.h
    src/ui/explore_bucket_dialog.h
    src/ui/bucket_info_dialog.h
    src/ui/doctor_page.h
    src/ui/modern_style.h
    src/ui/window_button.h
)

qt_add_executable(gscoop
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

target_include_directories(gscoop PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(gscoop PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::Concurrent
)

set_target_properties(gscoop PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

# 部署 themes/ 到可执行目录
add_custom_command(TARGET gscoop POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/themes
        $<TARGET_FILE_DIR:gscoop>/themes
)

# Windows 图标资源（如果存在）
if(WIN32)
    target_compile_definitions(gscoop PRIVATE
        _WIN32_WINNT=0x0601
        NOMINMAX
    )
    # dwmapi：Windows 标题栏深色
    target_link_libraries(gscoop PRIVATE dwmapi)
endif()
