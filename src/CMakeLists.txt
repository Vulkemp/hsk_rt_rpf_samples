cmake_minimum_required(VERSION 3.18)

project("hsk_sponza_sample_app")

MESSAGE("--- << CMAKE of ${PROJECT_NAME} >> --- ")
MESSAGE(STATUS "CURRENT SOURCE DIR \"${CMAKE_CURRENT_SOURCE_DIR}\"")#

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

if (WIN32)
	target_link_libraries(
		${PROJECT_NAME}
		PUBLIC "${CMAKE_SOURCE_DIR}/hsk_rt_rpf/third_party/sdl2/win/x64/SDL2main.lib"
	)
endif()

target_include_directories(
	${PROJECT_NAME}
	PUBLIC "../hsk_rt_rpf/src"
	PUBLIC "../hsk_rt_rpf/third_party"
	PUBLIC ${Vulkan_INCLUDE_DIRS}
)
