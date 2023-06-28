target_sources(
    owge_shaders PRIVATE
    bindless.hlsli
    complex.hlsli
    math.hlsli
    math_constants.hlsli
    rand.hlsli
)

add_subdirectory(ocean)
