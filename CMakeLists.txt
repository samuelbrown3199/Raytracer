cmake_minimum_required(VERSION 3.14)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
project(Raytracer)

include_directories (
        src
)

set (
	program_source

	src/main.cpp
)

add_executable(Raytracer
	${program_source}
)

foreach(source IN LISTS program_source)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "/" "\\" source_path_msvc "${source_path}")
    source_group("${source_path_msvc}" FILES "${source}")
endforeach()

set_target_properties(Raytracer PROPERTIES RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_SOURCE_DIR}/Image>)
set_property(TARGET Raytracer PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/Image")
target_compile_features(Raytracer PRIVATE cxx_std_20)