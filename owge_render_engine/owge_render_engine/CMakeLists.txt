target_sources(
    owge_render_engine PUBLIC
    render_engine.cpp
    render_engine.hpp
    resource.hpp
    resource_allocator.hpp
)
add_subdirectory(render_procedure)
