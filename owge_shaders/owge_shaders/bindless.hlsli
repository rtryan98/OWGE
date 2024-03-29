#ifndef OWGE_BINDLESS
#define OWGE_BINDLESS

static const uint bindless_max_bindset_size = 16 * sizeof(uint);

// D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE = 2048
sampler samplers[2048] : register(s0);

template<typename T>
T read_bindset_uniform(uint bindset_buffer, uint bindset_offset)
{
    ByteAddressBuffer buffer = ResourceDescriptorHeap[bindset_buffer];
    return buffer.Load<T>(bindset_offset);
}

struct Resource_Handle
{
    uint index;

    uint read_index()
    {
        return this.index;
    }

    uint write_index()
    {
        return this.index + 1;
    }
};

struct Array_Buffer
{
    Resource_Handle handle;

    template<typename T>
    T load(uint index)
    {
        ByteAddressBuffer buffer = ResourceDescriptorHeap[NonUniformResourceIndex(handle.read_index())];
        return buffer.Load<T>(index * sizeof(T));
    }

    template<typename T>
    T load_uniform(uint index)
    {
        ByteAddressBuffer buffer = ResourceDescriptorHeap[handle.read_index()];
        return buffer.Load<T>(index * sizeof(T));
    }
};

struct RW_Array_Buffer
{
    Resource_Handle handle;

    template<typename T>
    T load(uint index)
    {
        ByteAddressBuffer buffer = ResourceDescriptorHeap[NonUniformResourceIndex(handle.read_index())];
        return buffer.Load<T>(index * sizeof(T));
    }

    template<typename T>
    T load_uniform(uint index)
    {
        ByteAddressBuffer buffer = ResourceDescriptorHeap[handle.read_index()];
        return buffer.Load<T>(index * sizeof(T));
    }

    template<typename T>
    void store(uint index, T value)
    {
        RWByteAddressBuffer buffer = ResourceDescriptorHeap[NonUniformResourceIndex(handle.write_index())];
        buffer.Store(index * sizeof(T), value);
    }

    template<typename T>
    void store_uniform(uint index, T value)
    {
        RWByteAddressBuffer buffer = ResourceDescriptorHeap[handle.write_index()];
        buffer.Store(index * sizeof(T), value);
    }
};

struct Raw_Buffer
{
    Resource_Handle handle;

    template<typename T>
    T load()
    {
        ByteAddressBuffer buffer = ResourceDescriptorHeap[NonUniformResourceIndex(handle.read_index())];
        return buffer.Load<T>(0);
    }

    template<typename T>
    T load_uniform()
    {
        ByteAddressBuffer buffer = ResourceDescriptorHeap[handle.read_index()];
        return buffer.Load<T>(0);
    }
};

struct RW_Raw_Buffer
{
    Resource_Handle handle;

    template<typename T>
    T load()
    {
        ByteAddressBuffer buffer = ResourceDescriptorHeap[NonUniformResourceIndex(handle.read_index())];
        return buffer.Load<T>(0);
    }

    template<typename T>
    T load_uniform()
    {
        ByteAddressBuffer buffer = ResourceDescriptorHeap[handle.read_index()];
        return buffer.Load<T>(0);
    }

    template<typename T>
    void store(T value)
    {
        RWByteAddressBuffer buffer = ResourceDescriptorHeap[NonUniformResourceIndex(handle.write_index())];
        buffer.Store(0, value);
    }

    template<typename T>
    void store_uniform(T value)
    {
        RWByteAddressBuffer buffer = ResourceDescriptorHeap[handle.write_index()];
        buffer.Store(0, value);
    }

    template<typename T>
    uint interlocked_add(uint offset, uint value)
    {
        RWByteAddressBuffer buffer = ResourceDescriptorHeap[NonUniformResourceIndex(handle.write_index())];
        uint prev;
        buffer.InterlockedAdd(offset, value, prev);
        return prev;
    }

    template<typename T>
    uint interlocked_add_uniform(uint offset, uint value)
    {
        RWByteAddressBuffer buffer = ResourceDescriptorHeap[handle.write_index()];
        uint prev;
        buffer.InterlockedAdd(offset, value, prev);
        return prev;
    }
};

struct Sampler
{
    Resource_Handle handle;

    SamplerState as_nonuniform()
    {
        return SamplerDescriptorHeap[NonUniformResourceIndex(handle.read_index())];
    //    return samplers[NonUniformResourceIndex(handle.read_index())];
    }

    SamplerState as_uniform()
    {
        return SamplerDescriptorHeap[handle.read_index()];
    //    return samplers[handle.read_index()];
    }
};

struct Texture
{
    Resource_Handle handle;

    template<typename T>
    T load_1d(uint pos)
    {
        Texture1D<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(handle.read_index())];
        return texture.Load(uint2(pos, 0));
    }

    template<typename T>
    T load_2d(uint2 pos)
    {
        Texture2D<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(handle.read_index())];
        return texture.Load(uint3(pos, 0));
    }

    template<typename T>
    T load_3d(uint3 pos)
    {
        Texture3D<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(handle.read_index())];
        return texture.Load(uint4(pos, 0));
    }

    template<typename T>
    T sample_1d(SamplerState s, float u)
    {
        Texture1D<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(handle.read_index())];
        return texture.Sample(s, u);
    }

    template<typename T>
    T sample_2d(SamplerState s, float2 uv)
    {
        Texture2D<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(handle.read_index())];
        return texture.Sample(s, uv);
    }

    template<typename T>
    T sample_2d_array(SamplerState s, float2 uv, uint index)
    {
        Texture2DArray<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(handle.read_index())];
        return texture.Sample(s, float3(uv, index));
    }

    template<typename T>
    T sample_3d(SamplerState s, float3 uvw)
    {
        Texture3D<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(handle.read_index())];
        return texture.Sample(s, uvw);
    }

    template<typename T>
    T sample_level_1d(SamplerState s, float u, float mip)
    {
        Texture2D<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(handle.read_index())];
        return texture.SampleLevel(s, u, mip);
    }

    template<typename T>
    T sample_level_2d(SamplerState s, float2 uv, float mip)
    {
        Texture2D<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(handle.read_index())];
        return texture.SampleLevel(s, uv, mip);
    }

    template<typename T>
    T sample_level_2d_array(SamplerState s, float2 uv, float mip, uint index)
    {
        Texture2DArray<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(handle.read_index())];
        return texture.SampleLevel(s, float3(uv, index), mip);
    }

    template<typename T>
    T sample_level_3d(SamplerState s, float3 uvw, float mip)
    {
        Texture3D<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(handle.read_index())];
        return texture.SampleLevel(s, uvw, mip);
    }
};

struct RW_Texture
{
    Resource_Handle handle;

    template<typename T>
    T load_1d(uint pos)
    {
        Texture1D<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(handle.read_index())];
        return texture.Load(uint2(pos, 0));
    }

    template<typename T>
    T load_1d_array(uint2 pos)
    {
        Texture1DArray<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(handle.read_index())];
        return texture.Load(uint3(pos.x, pos.y, 0));
    }

    template<typename T>
    T load_2d(uint2 pos)
    {
        Texture2D<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(handle.read_index())];
        return texture.Load(uint3(pos, 0));
    }

    template<typename T>
    T load_2d_array(uint3 pos)
    {
        Texture2DArray<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(handle.read_index())];
        return texture.Load(uint4(pos.xyz, 0));
    }

    template<typename T>
    T load_3d(uint3 pos)
    {
        Texture3D<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(handle.read_index())];
        return texture.Load(uint4(pos, 0));
    }

    template<typename T>
    void store_1d(uint pos, T value)
    {
        RWTexture1D<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(handle.write_index())];
        texture[pos] = value;
    }

    template<typename T>
    void store_1d_array(uint2 pos, T value)
    {
        RWTexture1DArray<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(handle.write_index())];
        texture[pos] = value;
    }

    template<typename T>
    void store_2d(uint2 pos, T value)
    {
        RWTexture2D<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(handle.write_index())];
        texture[pos] = value;
    }

    template<typename T>
    void store_2d_array(uint3 pos, T value)
    {
        RWTexture2DArray<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(handle.write_index())];
        texture[pos] = value;
    }

    template<typename T>
    void store_3d(uint3 pos, T value)
    {
        RWTexture3D<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(handle.write_index())];
        texture[pos] = value;
    }

    template<typename T>
    T sample_1d(SamplerState s, float u)
    {
        Texture1D<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(handle.read_index())];
        return texture.Sample(s, u);
    }

    template<typename T>
    T sample_2d(SamplerState s, float2 uv)
    {
        Texture2D<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(handle.read_index())];
        return texture.Sample(s, uv);
    }

    template<typename T>
    T sample_3d(SamplerState s, float3 uvw)
    {
        Texture3D<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(handle.read_index())];
        return texture.Sample(s, uvw);
    }

    template<typename T>
    T sample_level_1d(SamplerState s, float u, float mip)
    {
        Texture2D<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(handle.read_index())];
        return texture.SampleLevel(s, u, mip);
    }

    template<typename T>
    T sample_level_2d(SamplerState s, float2 uv, float mip)
    {
        Texture2D<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(handle.read_index())];
        return texture.SampleLevel(s, uv, mip);
    }

    template<typename T>
    T sample_level_3d(SamplerState s, float3 uvw, float mip)
    {
        Texture3D<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(handle.read_index())];
        return texture.SampleLevel(s, uvw, mip);
    }
};

#endif // OWGE_BINDLESS
