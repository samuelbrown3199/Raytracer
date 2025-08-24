set (
	program_source

	main.cpp
	
	Useful/UsefulFiles.h
	Useful/UsefulFiles.cpp
	Useful/UsefulStrings.h
	Useful/UsefulStrings.cpp
	Useful/Useful.h

	Renderers/ModelLoader.h
	Renderers/ModelLoader.cpp

	#CPU Renderer

	Renderers/Software/Common.h

	Renderers/Software/Material.h
	Renderers/Software/Hittable.h
	Renderers/Software/Texture.h
	Renderers/Software/Texture.cpp

	Renderers/Software/Maths/AABB.h
	Renderers/Software/Maths/AABB.cpp
	Renderers/Software/Maths/BVH.h
	Renderers/Software/Maths/Vector.h
	Renderers/Software/Maths/Ray.h
	Renderers/Software/Maths/Interval.h
	Renderers/Software/Maths/Interval.cpp

	Renderers/Software/SoftwareRenderer.h
	Renderers/Software/SoftwareRenderer.cpp
	
	#GPU Renderer

	Renderers/Hardware/Vulkan/VulkanDescriptors.h
	Renderers/Hardware/Vulkan/VulkanDescriptors.cpp
	Renderers/Hardware/Vulkan/VulkanImages.h
	Renderers/Hardware/Vulkan/VulkanImages.cpp
	Renderers/Hardware/Vulkan/VulkanInitialisers.h
	Renderers/Hardware/Vulkan/VulkanInitialisers.cpp
	Renderers/Hardware/Vulkan/VulkanPipelines.h
	Renderers/Hardware/Vulkan/VulkanPipelines.cpp
	Renderers/Hardware/Vulkan/VulkanTypes.h
	
	Renderers/Hardware/Imgui/imconfig.h
	Renderers/Hardware/Imgui/imgui.h
	Renderers/Hardware/Imgui/imgui.cpp
	Renderers/Hardware/Imgui/imgui_demo.cpp
	Renderers/Hardware/Imgui/imgui_draw.cpp
	Renderers/Hardware/Imgui/imgui_internal.h
	Renderers/Hardware/Imgui/imgui_tables.cpp
	Renderers/Hardware/Imgui/imgui_widgets.cpp
	Renderers/Hardware/Imgui/imstb_rectpack.h
	Renderers/Hardware/Imgui/imstb_textedit.h
	Renderers/Hardware/Imgui/imstb_truetype.h
	Renderers/Hardware/Imgui/imgui_stdlib.h
	Renderers/Hardware/Imgui/imgui_stdlib.cpp

	Renderers/Hardware/Imgui/imgui.h
	Renderers/Hardware/Imgui/imgui.cpp
	Renderers/Hardware/Imgui/backends/imgui_impl_vulkan.h
	Renderers/Hardware/Imgui/backends/imgui_impl_vulkan.cpp

	Renderers/Hardware/Imgui/implot.h
	Renderers/Hardware/Imgui/implot.cpp
	Renderers/Hardware/Imgui/implot_demo.cpp
	Renderers/Hardware/Imgui/implot_internal.h
	Renderers/Hardware/Imgui/implot_items.cpp

	Renderers/Hardware/Imgui/backends/imgui_impl_sdl3.h
	Renderers/Hardware/Imgui/backends/imgui_impl_sdl3.cpp

	Renderers/Hardware/Imgui/ImGuiFileDialog.h
	Renderers/Hardware/Imgui/ImGuiFileDialog.cpp
	Renderers/Hardware/Imgui/ImGuiFileDialogConfig.h

	Renderers/Hardware/CameraController.h
	Renderers/Hardware/CameraController.cpp
	Renderers/Hardware/PerformanceStats.h
	Renderers/Hardware/PerformanceStats.cpp
	Renderers/Hardware/InputManager.h
	Renderers/Hardware/InputManager.cpp
	Renderers/Hardware/Window.h
	Renderers/Hardware/Window.cpp
	Renderers/Hardware/RaytracerTypes.h
	Renderers/Hardware/HardwareRenderer.h
	Renderers/Hardware/HardwareRenderer.cpp
)

set (

	shader_source
	
	Shaders/Compute/sky.comp
	Shaders/Compute/raytrace.comp
	Shaders/Compute/Random.glsl
)

add_executable(Raytracer
	${program_source}
	${shader_source}
)

foreach(source IN LISTS program_source shader_source)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "/" "\\" source_path_msvc "${source_path}")
    source_group("${source_path_msvc}" FILES "${source}")
endforeach()

target_link_libraries(Raytracer SDL3-static vulkan-1 vk-bootstrap)
set_target_properties(Raytracer PROPERTIES RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_SOURCE_DIR}/Image>)
set_property(TARGET Raytracer PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/Image")
target_compile_features(Raytracer PRIVATE cxx_std_20)