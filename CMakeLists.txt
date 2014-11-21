cmake_minimum_required(VERSION 2.8.4)
project(odr)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -std=gnu89")

set(SOURCE_FILES_CLIENT
    api.c
    api.h
    client.c
    client.h
    debug.h)

set(SOURCE_FILES_SERVER
    api.c
    api.h
    common.c
    common.h
    debug.h
    server.c
    server.h)

set(SOURCE_FILES_ODR
    common.c
    common.h
    debug.h
    get_hw_addrs.c
    get_hw_addrs.h
    ODR.c
    ODR.h)


add_executable(client_cse533-14 ${SOURCE_FILES_CLIENT})
add_executable(server_cse533-14 ${SOURCE_FILES_SERVER})
add_executable(ODR_cse533-14 ${SOURCE_FILES_ODR})