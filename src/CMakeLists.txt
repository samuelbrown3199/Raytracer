set (
	program_source

	main.cpp

	Renderers/Software/Material.h
	Renderers/Software/Hittable.h
	Renderers/Software/Common.h
	Renderers/Software/Maths/Vector.h
	Renderers/Software/Maths/Ray.h
	Renderers/Software/Maths/Interval.h
	Renderers/Software/Maths/Interval.cpp

	Renderers/Software/SoftwareRenderer.h
	Renderers/Software/SoftwareRenderer.cpp
)

add_executable(Raytracer
	${program_source}
)

foreach(source IN LISTS program_source)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "/" "\\" source_path_msvc "${source_path}")
    source_group("${source_path_msvc}" FILES "${source}")
endforeach()

#target_link_libraries(Raytracer SDL3-static vulkan-1 vk-bootstrap)
set_target_properties(Raytracer PROPERTIES RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_SOURCE_DIR}/Image>)
set_property(TARGET Raytracer PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/Image")
target_compile_features(Raytracer PRIVATE cxx_std_20)