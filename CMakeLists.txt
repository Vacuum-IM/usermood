cmake_minimum_required(VERSION 3.2)

project(usermood-plugin)
set(PLUGIN_NAME usermood)

set(SOURCES usermood.cpp usermooddialog.cpp)
set(HEADERS usermood.h usermooddialog.h definitions.h)
set(MOC_HEADERS usermood.h usermooddialog.h)
set(UIS usermooddialog.ui)

include_directories("${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}")

include("${CMAKE_SOURCE_DIR}/cmake/config.cmake")
include("${CMAKE_SOURCE_DIR}/translations/languages.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/plugin.cmake")

find_package(Qt5 COMPONENTS Widgets Xml REQUIRED)
target_link_libraries(${PLUGIN_NAME} Qt5::Widgets Qt5::Xml)

install(DIRECTORY "resources/menuicons" DESTINATION "${INSTALL_RESOURCES}")
install(DIRECTORY "resources/moodicons" DESTINATION "${INSTALL_RESOURCES}")
