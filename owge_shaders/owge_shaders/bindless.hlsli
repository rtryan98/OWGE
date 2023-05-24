#ifndef OWGE_BINDLESS
#define OWGE_BINDLESS

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
        RWByteAddressBuffer buffer = ResourceDescriptorHeap[handle.write_index()];
        buffer.store(index * sizeof(T), value);
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
        ByteAddressBuffer buffer = ResourceDescriptorHeap[NonUniformResourceIndex(handle.store_index())];
        buffer.store<T>(0, value);
    }

    template<typename T>
    void store_uniform(T value)
    {
        ByteAddressBuffer buffer = ResourceDescriptorHeap[handle.store_index()];
        buffer.store<T>(0, value);
    }
};

struct Sampler
{
    Resource_Handle handle;

    SamplerState sampler_state()
    {
        return SamplerDescriptorHeap[NonUniformResourceIndex(handle.read_index())];
    }
    
    SamplerState sampler_state_uniform()
    {
        return SamplerDescriptorHeap[handle.read_index()];
    }
};

struct Texture
{
    Resource_Handle handle;

    template<typename T>
    T load_1d(uint pos)
    {
        Texture1D<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(handle.read_index)];
        return texture.Load(uint2(pos, 0));
    }

    template<typename T>
    T load_2d(uint2 pos)
    {
        Texture2D<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(handle.read_index)];
        return texture.Load(uint3(pos, 0));
    }

    template<typename T>
    T load_3d(uint3 pos)
    {
        Texture3D<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(handle.read_index)];
        return texture.Load(uint4(pos, 0));
    }

    template<typename T>
    T sample_1d(SamplerState s, float u)
    {
        Texture1D<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(handle.read_index)];
        return texture.Sample(s, u);
    }

    template<typename T>
    T sample_2d(SamplerState s, float2 uv)
    {
        Texture2D<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(handle.read_index)];
        return texture.Sample(s, uv);
    }

    template<typename T>
    T sample_3d(SamplerState s, float3 uvw)
    {
        Texture3D<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(handle.read_index)];
        return texture.Sample(s, uvw);
    }

    template<typename T>
    T sample_level_1d(SamplerState s, float u, float mip)
    {
        Texture2D<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(handle.read_index)];
        return texture.SampleLevel(s, u, mip);
    }

    template<typename T>
    T sample_level_2d(SamplerState s, float2 uv, float mip)
    {
        Texture2D<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(handle.read_index)];
        return texture.SampleLevel(s, uv, mip);
    }

    template<typename T>
    T sample_level_3d(SamplerState s, float3 uvw, float mip)
    {
        Texture3D<T> texture = ResourceDescriptorHeap[NonUniformResourceIndex(handle.read_index)];
        return texture.SampleLevel(s, uvw, mip);
    }
}

struct RW_Texture
{
    Resource_Handle handle;

}

#endif // OWGE_BINDLESS
