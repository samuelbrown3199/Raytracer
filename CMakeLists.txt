cmake_minimum_required(VERSION 3.14)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
project(Raytracer)

include_directories (
	src
	contrib/Generic/include
	externals/SDL/include
	externals/vkbootstrap/src
	externals/vma/include
	externals/glm
)

link_directories(
	${PROJECT_SOURCE_DIR}/contrib/Generic/lib
)

add_definitions(-DSDL_MAIN_HANDLED)
set(SDL_DISABLE_SDL3MAIN ON)
set(SDL_SHARED OFF)
set(SDL_STATIC ON)
set(SDL_TEST OFF)
set(SDL_SUBPROJECT ON)
set(SDL_DISABLE_UNINSTALL ON)
set(DSDL_EXAMPLES OFF)
add_subdirectory("externals/SDL")
set_target_properties(SDL_uclibc SDL3_test SDL3-static PROPERTIES FOLDER Externals)

add_subdirectory("externals/vma")
add_subdirectory("externals/vk-bootstrap")
set_target_properties(vk-bootstrap PROPERTIES FOLDER Externals)

add_subdirectory("src")