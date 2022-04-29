cmake_minimum_required(VERSION 3.18)

project("hsk_sponza_sample_app")

MESSAGE("--- << CMAKE of ${PROJECT_NAME} >> --- ")
MESSAGE(STATUS "CURRENT SOURCE DIR \"${CMAKE_CURRENT_SOURCE_DIR}\"")#

# Compiler flags
set(BASE_FLAGS "${CMAKE_CXX_FLAGS} -fconcepts-ts")
set(NONSTRICT_FLAGS ${BASE_FLAGS})
set(STRICT_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

# Enable strict mode for own code
SET(CMAKE_CXX_FLAGS ${STRICT_FLAGS})

# collect sources
file(GLOB_RECURSE src "*.cpp")
add_executable(${PROJECT_NAME} ${src})

# find vulkan
find_package(Vulkan REQUIRED)
MESSAGE(STATUS "Vulkan inc dir ${Vulkan_INCLUDE_DIRS}")

target_link_libraries(
	${PROJECT_NAME}
	PUBLIC rtrpf
)

target_include_directories(
	${PROJECT_NAME}
	PUBLIC "../hsk_rt_rpf/src"
	PUBLIC "../hsk_rt_rpf/third_party"
	PUBLIC ${Vulkan_INCLUDE_DIRS}
)
