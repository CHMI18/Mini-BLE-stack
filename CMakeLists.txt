cmake_minimum_required(VERSION 3.10)

project(BLEstack C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(BLUEZ REQUIRED bluez)

include_directories(
    include
    ${BLUEZ_INCLUDE_DIRS}
)


add_executable(ble_project
    src/main.c
    src/hci_handler.c
    src/att_handler.c
    src/acl_handler.c
)

target_link_libraries(ble_project
    ${BLUEZ_LIBRARIES}
)
