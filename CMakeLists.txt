
CMAKE_MINIMUM_REQUIRED(VERSION 3.12.1)
PROJECT(OpenMiniServer)


MESSAGE(STATUS "CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    MESSAGE(STATUS "current platform: Linux")

elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
    # 导入windows相关的静态库
    link_libraries(ws2_32)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wall")
    MESSAGE(STATUS "current platform: Windows")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
    add_definitions(-D_AMD64_)

elseif(CMAKE_SYSTEM_NAME MATCHES "Android")
    MESSAGE(STATUS "current platform: Android") 

elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    MESSAGE(STATUS "current platform: Apple")
else()  
    MESSAGE(STATUS "other platform: ${CMAKE_SYSTEM_NAME}")  
endif()


set(OPEN_FILE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/open)
include_directories(${OPEN_FILE_PATH})

set(DEMO_FILE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/demo)
include_directories(${DEMO_FILE_PATH})

file(GLOB_RECURSE SRC_LIST 
	"${OPEN_FILE_PATH}/*.cpp" 
	"${OPEN_FILE_PATH}/*.cc" 
	"${OPEN_FILE_PATH}/*.h" 
	"${OPEN_FILE_PATH}/*.hpp"  
	"${OPEN_FILE_PATH}/*.c" 

    "${DEMO_FILE_PATH}/*.cpp" 
    "${DEMO_FILE_PATH}/*.cc" 
    "${DEMO_FILE_PATH}/*.h" 
    "${DEMO_FILE_PATH}/*.hpp"  
    "${DEMO_FILE_PATH}/*.c" 
)

# group open
file(GLOB_RECURSE GROUP_FILES 
    "${OPEN_FILE_PATH}/*.cpp" 
    "${OPEN_FILE_PATH}/*.cc" 
    "${OPEN_FILE_PATH}/*.h" 
    "${OPEN_FILE_PATH}/*.hpp"  
    "${OPEN_FILE_PATH}/*.c" 
    )
source_group(open FILES ${GROUP_FILES})


# make group function
function(group_file name)
    file(GLOB_RECURSE GROUP_FILES 
    "${DEMO_FILE_PATH}/${name}/*.cpp" 
    "${DEMO_FILE_PATH}/${name}/*.cc" 
    "${DEMO_FILE_PATH}/${name}/*.h" 
    "${DEMO_FILE_PATH}/${name}/*.hpp"  
    "${DEMO_FILE_PATH}/${name}/*.c" 
    )
source_group(${name} FILES ${GROUP_FILES})
endfunction()


#group 
group_file(msg)
group_file(util)
group_file(server/stock)
group_file(server/httpd)
group_file(server/centor)


add_executable(OpenMiniServer ${SRC_LIST})


#external
set(EXTERNAL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/external)

#protobuf
#include_directories(${EXTERNAL_PATH}/protobuf/src)
#target_link_libraries(OpenMiniServer ${EXTERNAL_PATH}/protobuf/vsprojects/x64/Debug/libprotobuf.lib)

#leveldb
#include_directories(${EXTERNAL_PATH}/leveldb/include)
#include_directories(${EXTERNAL_PATH}/leveldb/build/include)
#target_link_libraries(OpenMiniServer ${EXTERNAL_PATH}/leveldb/build/Debug/leveldb.lib)

#add_definitions(-DUSE_OPEN_SSL)
#include_directories(${EXTERNAL_PATH}/openssl/include)
#target_link_libraries(OpenMiniServer ${EXTERNAL_PATH}/openssl/libcrypto.lib)
#target_link_libraries(OpenMiniServer ${EXTERNAL_PATH}/openssl/libssl.lib)

#MariadDB
#include_directories(C:/Program\ Files/MariaDB\ 11.0/include/)
#target_link_libraries(OpenMiniServer C:/Program\ Files/MariaDB\ 11.0/lib/libmariadb.lib)