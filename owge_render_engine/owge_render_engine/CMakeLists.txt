target_sources(
    owge_render_engine PUBLIC
    command_allocator.cpp
    command_allocator.hpp
    render_engine.cpp
    render_engine.hpp
    resource.hpp
    resource_allocator.hpp
)
add_subdirectory(render_procedure)
