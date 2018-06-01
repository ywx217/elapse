cmake_minimum_required(VERSION 2.8.8)
set(PROJECT_NAME_STR elapse)

project(${PROJECT_NAME_STR} CXX C)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(ENABLE_ASAN "Enable address sanitizer in clang env." OFF)
set(BOOST_ROOT "c:/code/libs/cpp/boost_1_67_0" CACHE FILEPATH "boost library root path")
set(BOOST_LIBRARYDIR "${BOOST_ROOT}/stage/lib")

find_package(Threads REQUIRED)

if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    add_definitions(-Wall -ansi -Wno-deprecated -pthread -fPIC -std=c++11)
    add_compile_options(-std=c++11)
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    add_definitions(-Wall -ansi -Wno-deprecated -pthread -fPIC -std=c++11)
    add_compile_options(-std=c++11)
    if(ENABLE_ASAN)
        message(Address sanitizer enabled.)
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fsanitize=address")
        set (CMAKE_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} -fno-omit-frame-pointer -fsanitize=address")
    endif(ENABLE_ASAN)
else()
    message('other compiler: ${CMAKE_CXX_COMPILER_ID}')
endif()

if(MSVC)
    #vc 2012 fix for vararg templates
    set(MSVC_COMPILER_DEFS "-D_VARIADIC_MAX=10")
endif()

#-------------------
# Boost library
#-------------------
set(Boost_USE_STATIC_LIBS OFF CACHE BOOL "use static libraries from Boost")
set(Boost_USE_MULTITHREADED ON)
find_package(Boost REQUIRED COMPONENTS system)
message("boost found: include=${Boost_INCLUDE_DIRS} lib=${Boost_LIBRARIES}")

#-------------------
# set common include folder for module
#-------------------
set(COMMON_INCLUDES
    ${PROJECT_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
)
set(COMMON_LIBRARY
    ${PYTHON_LIBRARIES}
    ${Boost_LIBRARIES}
)
if (WIN32)
    # disable autolinking in boost
    add_definitions(-DBOOST_ALL_NO_LIB)

    # force all boost libraries to dynamic link (we already disabled
    # autolinking, so I don't know why we need this, but we do!)
    add_definitions(-DBOOST_ALL_DYN_LINK)
endif()

set(EXT_PROJECTS_DIR ${PROJECT_SOURCE_DIR}/ext)

#-------------------
# Module source
#-------------------
include_directories(${COMMON_INCLUDES})
file(GLOB LIB_HEADER_FILES
    ${PROJECT_SOURCE_DIR}/include/*.hpp
    )
file(GLOB LIB_SRC_FILES
    ${PROJECT_SOURCE_DIR}/src/*.cpp
    )
add_library(${PROJECT_NAME_STR} STATIC ${LIB_HEADER_FILES} ${LIB_SRC_FILES})
target_link_libraries(${PROJECT_NAME_STR} ${COMMON_LIBRARY})


#-------------------
# Test
#-------------------
if (ELAPSE_SKIP_TEST)
    message("Elapse test skipped.")
else (ELAPSE_SKIP_TEST)
    add_subdirectory(${EXT_PROJECTS_DIR}/gtest)

    add_definitions(${MSVC_COMPILER_DEFS})
    enable_testing()
    set(PROJECT_TEST_NAME ${PROJECT_NAME_STR}_test)
    include_directories(${GTEST_INCLUDE_DIRS})

    file(GLOB TEST_SRC_FILES ${PROJECT_SOURCE_DIR}/test/*.cpp)
    add_executable(${PROJECT_TEST_NAME} ${TEST_SRC_FILES})
    add_dependencies(${PROJECT_TEST_NAME} ${PROJECT_NAME_STR} googletest)


    if(WIN32 OR XCODE)
        if (XCODE)
            set(GTEST_FIND_LIB_SUFFIXES .a)
            set(GTEST_DEBUG_SUFFIX "d")
        else(XCODE)
            set(GTEST_FIND_LIB_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
            set(GTEST_DEBUG_SUFFIX "")
        endif(XCODE)
        target_link_libraries(${PROJECT_TEST_NAME}
            debug ${GTEST_LIBS_DIR}/DebugLibs/${CMAKE_FIND_LIBRARY_PREFIXES}gtest${GTEST_DEBUG_SUFFIX}${GTEST_FIND_LIB_SUFFIXES}
            optimized ${GTEST_LIBS_DIR}/ReleaseLibs/${CMAKE_FIND_LIBRARY_PREFIXES}gtest${GTEST_FIND_LIB_SUFFIXES}
            )
        target_link_libraries(${PROJECT_TEST_NAME}
            debug ${GTEST_LIBS_DIR}/DebugLibs/${CMAKE_FIND_LIBRARY_PREFIXES}gtest_main${GTEST_DEBUG_SUFFIX}${GTEST_FIND_LIB_SUFFIXES}
            optimized ${GTEST_LIBS_DIR}/ReleaseLibs/${CMAKE_FIND_LIBRARY_PREFIXES}gtest_main${GTEST_FIND_LIB_SUFFIXES}
            )
    else()
        target_link_libraries(${PROJECT_TEST_NAME}
            ${GTEST_LIBS_DIR}/libgtest.a
            ${GTEST_LIBS_DIR}/libgtest_main.a
            )
    endif()

    message("common library:::${COMMON_LIBRARY}")
    target_link_libraries(${PROJECT_TEST_NAME} ${CMAKE_THREAD_LIBS_INIT} ${PROJECT_NAME_STR})

    add_test(test1 ${PROJECT_TEST_NAME})
endif (ELAPSE_SKIP_TEST)
