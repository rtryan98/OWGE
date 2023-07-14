target_sources(
    owge_render_engine PRIVATE
    bindless.cpp
    bindless.hpp
    camera.cpp
    camera.hpp
    command_allocator.cpp
    command_allocator.hpp
    command_list.cpp
    command_list.hpp
    render_engine.cpp
    render_engine.hpp
    resource.hpp
    resource_allocator.hpp
    staging_buffer_allocator.cpp
    staging_buffer_allocator.hpp
)
add_subdirectory(render_procedure)
