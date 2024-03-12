#include "owge_render_engine/render_engine.hpp"

#include <algorithm>
#include <utility>
#include <fstream>
#include <ranges>

#include "owge_render_engine/command_list.hpp"

#include <owge_common/file_util.hpp>

#if OWGE_USE_NVPERF
#pragma warning(push, 3) // NvPerf sample code does not compile under W4
#include <nvperf_host_impl.h>
#include <NvPerfD3D12.h>
#pragma warning(pop)
#endif // OWGE_USE_NVPERF

#if OWGE_USE_WIN_PIX_EVENT_RUNTIME
#include <WinPixEventRuntime/pix3.h>
#endif // OWGE_USE_WIN_PIX_EVENT_RUNTIME

namespace owge
{
static constexpr uint32_t NO_RTV_DSV = 0x1FFFFF;

Render_Engine::Render_Engine(HWND hwnd,
    const D3D12_Context_Settings& d3d12_context_settings,
    const Render_Engine_Settings& render_engine_settings)
    : m_ctx()
    , m_settings(render_engine_settings)
    , m_swapchain()
{
    m_ctx = create_d3d12_context(&d3d12_context_settings);
    m_swapchain = std::make_unique<D3D12_Swapchain>(
        m_ctx.factory, m_ctx.device, m_ctx.direct_queue,
        hwnd, MAX_SWAPCHAIN_BUFFERS);

    for (auto i = 0; i < MAX_CONCURRENT_GPU_FRAMES; ++i)
    {
        auto& frame_ctx = m_frame_contexts[i];
        frame_ctx.direct_queue_cmd_alloc = std::make_unique<Command_Allocator>(
            m_ctx.device, D3D12_COMMAND_LIST_TYPE_DIRECT);
        m_ctx.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&frame_ctx.direct_queue_fence));
        frame_ctx.frame_number = 0;
        frame_ctx.staging_buffer_allocator = std::make_unique<Staging_Buffer_Allocator>(
            m_ctx.device, this);
    }

    m_resource_manager = std::make_unique<Resource_Manager>(&m_ctx);

    m_bindset_allocator = std::make_unique<Bindset_Allocator>(this);
    m_bindset_stager = std::make_unique<Bindset_Stager>();

    if (render_engine_settings.nvperf_enabled)
    {
#if OWGE_USE_NVPERF
        if (d3d12_context_settings.enable_validation ||
            d3d12_context_settings.enable_gpu_based_validation)
        {
            // TODO: nvperf does not support validation. Post warning.
            printf("nvperf cannot be enabled at the same time as validation layers.\n");
            return;
        }
        if (!nv::perf::InitializeNvPerf())
        {
            // TODO: log failed init.
            printf("nvperf failed to initialize.\n");
            return;
        }
        if (!nv::perf::D3D12LoadDriver())
        {
            // TODO: log no driver
            printf("nvperf driver load failed.\n");
            return;
        }
        if (!nv::perf::D3D12IsNvidiaDevice(m_ctx.device))
        {
            // TODO: log no nv device
            printf("nvperf enabled but device is not a nvidia device.\n");
            return;
        }
        if (!nv::perf::profiler::D3D12IsGpuSupported(m_ctx.device))
        {
            // TODO: log unsupported nv device
            printf("nvperf enabled but gpu is unsupported.\n");
            return;
        }
        if (render_engine_settings.nvperf_lock_clocks_to_rated_tdp)
        {
            nv::perf::D3D12SetDeviceClockState(m_ctx.device, NVPW_DEVICE_CLOCK_SETTING_LOCK_TO_RATED_TDP);
        }
        // TODO: implement nvperf
        m_nvperf_active = true;
        printf("nvperf was loaded successfully.\n");
#endif
    }
}

Render_Engine::~Render_Engine()
{
    d3d12_context_wait_idle(&m_ctx);

    m_bindset_allocator->release_resources();
    for (auto& frame_ctx : m_frame_contexts)
    {
        frame_ctx.staging_buffer_allocator->reset();
    }
    empty_deletion_queues(~0ull);

#if OWGE_USE_NVPERF
    if (m_nvperf_active &&
        m_settings.nvperf_lock_clocks_to_rated_tdp)
    {
        nv::perf::D3D12SetDeviceClockState(m_ctx.device, NVPW_DEVICE_CLOCK_SETTING_DEFAULT);
    }
#endif

    m_bindset_allocator = nullptr;
    m_frame_contexts = {};
    m_bindset_stager = nullptr;
    m_swapchain = nullptr;

    destroy_d3d12_context(&m_ctx);
}

void Render_Engine::add_procedure(Render_Procedure* proc)
{
    m_procedures.push_back(proc);
}

void Render_Engine::render(float delta_time)
{
    auto& frame_ctx = m_frame_contexts[m_current_frame_index];

    if (wait_for_d3d12_fence(frame_ctx.direct_queue_fence.Get(), frame_ctx.frame_number, INFINITE) != WAIT_OBJECT_0)
    {
        // TODO: wait error. Warn about possible desync?
    }

    frame_ctx.direct_queue_cmd_alloc->reset();
    frame_ctx.staging_buffer_allocator->reset();
    empty_deletion_queues(m_current_frame);
    auto procedure_cmd = frame_ctx.direct_queue_cmd_alloc->get_or_allocate().cmd;
    frame_ctx.upload_cmd = frame_ctx.direct_queue_cmd_alloc->get_or_allocate().cmd;

    if (m_swapchain->try_resize())
    {
        // TODO: client window area dependent resources
    }
    m_swapchain->acquire_next_image();

    auto procedure_cmd_list = Command_List(this, procedure_cmd);
    auto procedure_cmd_global_barrier_builder = procedure_cmd_list.acquire_barrier_builder();

    Render_Procedure_Payload proc_payload = {
        .render_engine = this,
        .cmd = &procedure_cmd_list,
        .barrier_builder = &procedure_cmd_global_barrier_builder,
        .swapchain = m_swapchain.get(),
        .delta_time = delta_time
    };
    procedure_cmd->SetComputeRootSignature(m_ctx.global_rootsig);
    procedure_cmd->SetGraphicsRootSignature(m_ctx.global_rootsig);
    auto descriptor_heaps = std::to_array({
        m_ctx.cbv_srv_uav_descriptor_heap, m_ctx.sampler_descriptor_heap
        });
    procedure_cmd->SetDescriptorHeaps(uint32_t(descriptor_heaps.size()), descriptor_heaps.data());
    // procedure_cmd->SetComputeRootDescriptorTable(1, m_ctx.sampler_descriptor_heap->GetGPUDescriptorHandleForHeapStart());
    // procedure_cmd->SetGraphicsRootDescriptorTable(1, m_ctx.sampler_descriptor_heap->GetGPUDescriptorHandleForHeapStart());

    for (auto procedure : m_procedures)
    {
        procedure_cmd_list.begin_event(procedure->get_name());
        procedure->process(proc_payload);
        procedure_cmd_list.end_event();
    }

    m_bindset_stager->process(procedure_cmd);

    for (const auto& staged_upload : m_staged_uploads)
    {
        frame_ctx.upload_cmd->CopyBufferRegion(
            staged_upload.dst,
            staged_upload.dst_offset,
            staged_upload.src,
            staged_upload.src_offset,
            staged_upload.size);
    }
    m_staged_uploads.clear();

    D3D12_GLOBAL_BARRIER global_upload_barrier = {
        .SyncBefore = D3D12_BARRIER_SYNC_COPY,
        .SyncAfter = D3D12_BARRIER_SYNC_ALL,
        .AccessBefore = D3D12_BARRIER_ACCESS_COPY_DEST,
        .AccessAfter = D3D12_BARRIER_ACCESS_COMMON
    };
    D3D12_BARRIER_GROUP global_upload_barrier_group = {
        .Type = D3D12_BARRIER_TYPE_GLOBAL,
        .pGlobalBarriers = &global_upload_barrier
    };
    frame_ctx.upload_cmd->Barrier(1, &global_upload_barrier_group);

    frame_ctx.upload_cmd->Close();
    procedure_cmd->Close();
    auto cmds = std::to_array({
        static_cast<ID3D12CommandList*>(frame_ctx.upload_cmd),
        static_cast<ID3D12CommandList*>(procedure_cmd) });
    m_ctx.direct_queue->ExecuteCommandLists(uint32_t(cmds.size()), cmds.data());

    auto swapchain = m_swapchain->get_swapchain();
    DXGI_SWAP_CHAIN_DESC1 swapchain_desc = {};
    swapchain->GetDesc1(&swapchain_desc);
    auto allow_tearing = swapchain_desc.Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING
        ? DXGI_PRESENT_ALLOW_TEARING
        : 0u;
    swapchain->Present(0, allow_tearing);

    m_current_frame += 1;
    m_current_frame_index = m_current_frame % MAX_CONCURRENT_GPU_FRAMES;
    frame_ctx.frame_number += 1;

    m_ctx.direct_queue->Signal(frame_ctx.direct_queue_fence.Get(), frame_ctx.frame_number);
}

void* Render_Engine::upload_data(uint64_t size, uint64_t align, Buffer_Handle dst, uint64_t dst_offset)
{
    auto& frame_ctx = m_frame_contexts[m_current_frame_index];
    auto allocation = frame_ctx.staging_buffer_allocator->allocate(size, align);
    auto& buffer = get_buffer(dst);
    m_staged_uploads.push_back({
        .src = allocation.resource,
        .src_offset = allocation.offset,
        .dst = buffer.resource,
        .dst_offset = dst_offset,
        .size = size
        });
    return &static_cast<uint8_t*>(allocation.data)[allocation.offset];
}

void Render_Engine::copy_and_upload_data(uint64_t size, uint64_t align, Buffer_Handle dst, uint64_t dst_offset, void* data)
{
    auto& frame_ctx = m_frame_contexts[m_current_frame_index];
    auto allocation = frame_ctx.staging_buffer_allocator->allocate(size, align);
    memcpy(&static_cast<char*>(allocation.data)[allocation.offset], data, size);
    auto& buffer = get_buffer(dst);
    m_staged_uploads.push_back({
        .src = allocation.resource,
        .src_offset = allocation.offset,
        .dst = buffer.resource,
        .dst_offset = dst_offset,
        .size = size
        });
}

void Render_Engine::update_bindings(const Bindset& bindset)
{
    auto& frame_ctx = m_frame_contexts[m_current_frame_index];
    m_bindset_stager->stage_bindset(this, bindset, frame_ctx.staging_buffer_allocator.get());
}

Buffer_Handle Render_Engine::create_buffer(const Buffer_Desc& desc, const wchar_t* name)
{
    return m_resource_manager->create_buffer(desc, name);
}

Texture_Handle Render_Engine::create_texture(const Texture_Desc& desc, const wchar_t* name)
{
    return m_resource_manager->create_texture(desc, name);
}

Shader_Handle Render_Engine::create_shader(const Shader_Desc& desc)
{
    return m_resource_manager->create_shader(desc);
}

Pipeline_Handle Render_Engine::create_pipeline(const Graphics_Pipeline_Desc& desc, const wchar_t* name)
{
    return m_resource_manager->create_pipeline(desc, name);
}

Pipeline_Handle Render_Engine::create_pipeline(const Compute_Pipeline_Desc& desc, const wchar_t* name)
{
    return m_resource_manager->create_pipeline(desc, name);
}

Sampler_Handle Render_Engine::create_sampler(const Sampler_Desc& desc)
{
    return m_resource_manager->create_sampler(desc);
}

Bindset Render_Engine::create_bindset()
{
    return m_bindset_allocator->allocate_bindset();
}

void Render_Engine::destroy_buffer(Buffer_Handle handle)
{
    m_resource_manager->destroy_buffer(handle, m_current_frame + MAX_CONCURRENT_GPU_FRAMES);
}

void Render_Engine::destroy_texture(Texture_Handle handle)
{
    m_resource_manager->destroy_texture(handle, m_current_frame + MAX_CONCURRENT_GPU_FRAMES);
}

void Render_Engine::destroy_shader(Shader_Handle handle)
{
    m_resource_manager->destroy_shader(handle);
}

void Render_Engine::destroy_pipeline(Pipeline_Handle handle)
{
    m_resource_manager->destroy_pipeline(handle, m_current_frame + MAX_CONCURRENT_GPU_FRAMES);
}

void Render_Engine::destroy_sampler(Sampler_Handle handle)
{
    m_resource_manager->destroy_sampler(handle, m_current_frame + MAX_CONCURRENT_GPU_FRAMES);
}

void Render_Engine::destroy_bindset(const Bindset& bindset)
{
    m_bindset_deletion_queue.push_back({ bindset, m_current_frame + MAX_CONCURRENT_GPU_FRAMES });
}

void Render_Engine::destroy_d3d12_resource_deferred(ID3D12Resource* resource)
{
    m_resource_manager->destroy_d3d12_resource_deferred(resource, m_current_frame + MAX_CONCURRENT_GPU_FRAMES);
}

const Buffer& Render_Engine::get_buffer(Buffer_Handle handle) const
{
    return m_resource_manager->get_buffer(handle);
}

Buffer& Render_Engine::get_buffer(Buffer_Handle handle)
{
    return m_resource_manager->get_buffer(handle);
}

const Texture& Render_Engine::get_texture(Texture_Handle handle) const
{
    return m_resource_manager->get_texture(handle);
}

Texture& Render_Engine::get_texture(Texture_Handle handle)
{
    return m_resource_manager->get_texture(handle);
}

const Pipeline& Render_Engine::get_pipeline(Pipeline_Handle handle) const
{
    return m_resource_manager->get_pipeline(handle);
}

Pipeline& Render_Engine::get_pipeline(Pipeline_Handle handle)
{
    return m_resource_manager->get_pipeline(handle);
}

const Shader& Render_Engine::get_shader(Shader_Handle handle) const
{
    return m_resource_manager->get_shader(handle);
}

Shader& Render_Engine::get_shader(Shader_Handle handle)
{
    return m_resource_manager->get_shader(handle);
}

D3D12_CPU_DESCRIPTOR_HANDLE Render_Engine::get_cpu_descriptor_from_texture(Texture_Handle handle) const
{
    D3D12_CPU_DESCRIPTOR_HANDLE result = {};
    auto& texture = get_texture(handle);
    if (texture.rtv != NO_RTV_DSV)
    {
        auto increment = m_ctx.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        auto start = m_ctx.rtv_descriptor_heap->GetCPUDescriptorHandleForHeapStart();
        result.ptr = start.ptr + texture.rtv * increment;
    }
    else if (texture.dsv != NO_RTV_DSV)
    {
        auto increment = m_ctx.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        auto start = m_ctx.dsv_descriptor_heap->GetCPUDescriptorHandleForHeapStart();
        result.ptr = start.ptr + texture.dsv * increment;
    }
    return result;
}

void Render_Engine::empty_deletion_queues(uint64_t frame)
{
    m_resource_manager->empty_deletion_queues(frame);

    auto bindset_range = std::ranges::remove_if(m_bindset_deletion_queue, [this, frame](auto& element) {
        if (frame >= element.frame)
        {
            m_bindset_allocator->delete_bindset(element.resource);
            return true;
        }
        return false;
        });
    m_bindset_deletion_queue.erase(bindset_range.begin(), bindset_range.end());
}
}
