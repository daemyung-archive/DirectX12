//
// This file is part of the "DirectX12" project
// See "LICENSE" for license information.
//

#include <common/window.h>
#include <common/example.h>
#include <common/resource_uploader.h>
#include <memory>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Waddress-of-temporary"
#endif

//----------------------------------------------------------------------------------------------------------------------

using namespace DirectX;

//----------------------------------------------------------------------------------------------------------------------

struct Vertex {
    XMFLOAT3 position;
    XMFLOAT3 color;
};

//----------------------------------------------------------------------------------------------------------------------

struct Transformations {
    XMFLOAT4X4 projection;
    XMFLOAT4X4 view;
    XMFLOAT4X4 model;
};

//----------------------------------------------------------------------------------------------------------------------

struct AccelerationStructureBuffers {
    ComPtr<ID3D12Resource> scratch;
    ComPtr<ID3D12Resource> result;
    ComPtr<ID3D12Resource> instance_desc;
};

//----------------------------------------------------------------------------------------------------------------------

const std::unordered_map<D3D12_DESCRIPTOR_HEAP_TYPE, UINT> kDescriptorCount = {
        {D3D12_DESCRIPTOR_HEAP_TYPE_RTV,         kSwapChainBufferCount},
        {D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kImGuiFontBufferCount + 3 * kSwapChainBufferCount}};

//----------------------------------------------------------------------------------------------------------------------

auto kRayGeneration = L"RayGeneration";
auto kMiss = L"Miss";
auto kHitGroup = L"HitGroup";
auto kClosestHit = L"ClosestHit";

//----------------------------------------------------------------------------------------------------------------------

inline auto BuildFilePath(const std::string &file_name) {
    std::filesystem::path file_path;

    file_path = fmt::format("{}/{}", RAYTRACING_TRIANGLE_ASSET_DIR, file_name);
    if (file_path.has_filename()) {
        return file_path;
    }

    throw std::runtime_error(fmt::format("File isn't exist: {}.", file_name));
}

//----------------------------------------------------------------------------------------------------------------------

class RaytracingTriangle : public Example {
public:
    RaytracingTriangle() : Example("Raytracing triangle", kDescriptorCount) {
        CheckRaytracingSupport();
        InitResources();
        InitPipelines();
    }

protected:
    void OnInit() override {
    }

    void OnTerm() override {
    }

    void OnResize(const Resolution &resolution) override {
        // Update the width and the height.
        _width = GetWidth(resolution);
        _height = GetHeight(resolution);

        // Update offscreen buffers.
        InitOffscreenBuffers();
    }

    void OnUpdate(UINT index) override {
        // Define transformation.
        Transformations transformation;
        transformation.projection = _camera.GetInverseProjection();
        transformation.view = _camera.GetInverseView();
        transformation.model = kIdentityFloat4x4;

        // Update transformation.
        UpdateBuffer(_constant_buffers[index].Get(), &transformation, sizeof(Transformations));

        BYTE *data;
        ThrowIfFailed(_sbt_buffers[index]->Map(0, nullptr, reinterpret_cast<void **>(&data)));

        // Update the ray generation SBT.
        memcpy(data, _raytracing_pipeline_state_properties->GetShaderIdentifier(kRayGeneration),
               D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
        CD3DX12_GPU_DESCRIPTOR_HANDLE gpu_handle(
                _descriptor_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->GetGPUDescriptorHandleForHeapStart());
        gpu_handle.Offset(_descriptor_heap_sizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV], index * 3);
        *(reinterpret_cast<UINT64 *>(data + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES)) = gpu_handle.ptr;
        data += _sbt_size;

        // Update the miss SBT.
        memcpy(data, _raytracing_pipeline_state_properties->GetShaderIdentifier(kMiss),
               D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
        data += _sbt_size;

        // Update the hit group SBT.
        memcpy(data, _raytracing_pipeline_state_properties->GetShaderIdentifier(kHitGroup),
               D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
        *(reinterpret_cast<UINT64 *>(data + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES)) =
                _vertex_buffer->GetGPUVirtualAddress();

        _sbt_buffers[index]->Unmap(0, nullptr);
    }

    void OnRender(UINT index) override {
        D3D12_RESOURCE_BARRIER resource_barriers[2] = {};

        // Define a resource transition to write the result of the raytracing.
        resource_barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(_offscreen_buffers[index].Get(),
                                                                    D3D12_RESOURCE_STATE_COPY_SOURCE,
                                                                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        // Record a resource transition command.
        _command_list->ResourceBarrier(1, resource_barriers);

        // Record to set a global root signature.
        _command_list->SetComputeRootSignature(_global_root_signature.Get());

        // Record to set a raytracing pipeline command.
        _command_list->SetPipelineState1(_raytracing_pipeline_state.Get());

        // Define dispatch rays.
        D3D12_DISPATCH_RAYS_DESC rays_desc = {};
        rays_desc.RayGenerationShaderRecord.StartAddress = _sbt_buffers[index]->GetGPUVirtualAddress();
        rays_desc.RayGenerationShaderRecord.SizeInBytes = _sbt_size;
        rays_desc.MissShaderTable.StartAddress = rays_desc.RayGenerationShaderRecord.StartAddress + _sbt_size;
        rays_desc.MissShaderTable.StrideInBytes = _sbt_size;
        rays_desc.MissShaderTable.SizeInBytes = _sbt_size;
        rays_desc.HitGroupTable.StartAddress = rays_desc.MissShaderTable.StartAddress + _sbt_size;
        rays_desc.HitGroupTable.StrideInBytes = _sbt_size;
        rays_desc.HitGroupTable.SizeInBytes = _sbt_size;
        rays_desc.Width = _width;
        rays_desc.Height = _height;
        rays_desc.Depth = 1;

        // Record to dispatch rays command.
        _command_list->DispatchRays(&rays_desc);

        // Define a resource barrier to copy from the offscreen to the swap chain image.
        resource_barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(_offscreen_buffers[index].Get(),
                                                                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                                                    D3D12_RESOURCE_STATE_COPY_SOURCE);
        resource_barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(_swap_chain_buffers[index].Get(),
                                                                    D3D12_RESOURCE_STATE_PRESENT,
                                                                    D3D12_RESOURCE_STATE_COPY_DEST);

        // Record a resource transition command.
        _command_list->ResourceBarrier(2, resource_barriers);

        // Record copy from the offscreen to the swap chain image.
        _command_list->CopyResource(_swap_chain_buffers[index].Get(), _offscreen_buffers[index].Get());

        // Define a resource barrier to render to the swap chain image.
        resource_barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(_swap_chain_buffers[index].Get(),
                                                                    D3D12_RESOURCE_STATE_COPY_DEST,
                                                                    D3D12_RESOURCE_STATE_RENDER_TARGET);

        // Record a resource transition command.
        _command_list->ResourceBarrier(1, resource_barriers);

        // Record to set the swap chain image as render target command.
        _command_list->OMSetRenderTargets(1, &_swap_chain_views[index], true, nullptr);

        // Record ImGui commands.
        RecordDrawImGuiCommands(_command_list.Get());

        // Define a resource barrier to present the swap chain image.
        resource_barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(_swap_chain_buffers[index].Get(),
                                                                    D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                                    D3D12_RESOURCE_STATE_PRESENT);

        // Record a resource transition command.
        _command_list->ResourceBarrier(1, resource_barriers);
    }

private:
    void CheckRaytracingSupport() {
        D3D12_FEATURE_DATA_D3D12_OPTIONS5 options;
        ThrowIfFailed(_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options, sizeof(options)));

        if (options.RaytracingTier == D3D12_RAYTRACING_TIER_NOT_SUPPORTED) {
            throw std::runtime_error("Fail to support raytracing.");
        }
    }

    void InitResources() {
        // Define vertices.
        Vertex vertices[3] = {{XMFLOAT3(1.0f, -1.0f, 0.0f),  XMFLOAT3(Colors::Red)},
                              {XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT3(Colors::Lime)},
                              {XMFLOAT3(0.0f, 1.0f, 0.0f),   XMFLOAT3(Colors::Blue)}};

        // Device indices.
        UINT16 indices[3] = {0, 1, 2};

        ResourceUploader uploader(_device.Get());

        // Initialize a vertex buffer.
        ThrowIfFailed(CreateDefaultBuffer(_device.Get(), sizeof(vertices), &_vertex_buffer));
        uploader.RecordCopyData(_vertex_buffer.Get(), vertices, sizeof(vertices));

        // Initialize an index buffer.
        ThrowIfFailed(CreateDefaultBuffer(_device.Get(), sizeof(indices), &_index_buffer));
        uploader.RecordCopyData(_index_buffer.Get(), indices, sizeof(indices));

        uploader.Execute();

        // Initialize constant buffers.
        for (auto i = 0; i != kSwapChainBufferCount; ++i) {
            ThrowIfFailed(CreateConstantBuffer(_device.Get(), sizeof(Transformations), &_constant_buffers[i]));
        }

        // Create a command allocator for acceleration structures.
        ComPtr<ID3D12CommandAllocator> command_allocator;
        _device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&command_allocator));

        // Create a command list for acceleration structures.
        ComPtr<ID3D12GraphicsCommandList5> command_list;
        _device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, command_allocator.Get(), nullptr,
                                   IID_PPV_ARGS(&command_list));

        // Define a geometry.
        D3D12_RAYTRACING_GEOMETRY_DESC geometry_desc = {};
        geometry_desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
        geometry_desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
        geometry_desc.Triangles.VertexBuffer = {_vertex_buffer->GetGPUVirtualAddress(), sizeof(Vertex)};
        geometry_desc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
        geometry_desc.Triangles.VertexCount = _countof(vertices);

        // Define the bottom level acceleration structure inputs.
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS blas_inputs = {};
        blas_inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
        blas_inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
        blas_inputs.NumDescs = 1;
        blas_inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        blas_inputs.pGeometryDescs = &geometry_desc;

        // Retrieve a prebuild information for the bottom level acceleration structure.
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO blas_prebuild_info;
        _device->GetRaytracingAccelerationStructurePrebuildInfo(&blas_inputs, &blas_prebuild_info);

        AccelerationStructureBuffers blas_buffers;

        // Create buffers for the bottom level acceleration structure.
        ThrowIfFailed(CreateDefaultBuffer(_device.Get(), blas_prebuild_info.ScratchDataSizeInBytes,
                                          D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                          D3D12_RESOURCE_STATE_COMMON,
                                          &blas_buffers.scratch));
        ThrowIfFailed(CreateDefaultBuffer(_device.Get(), blas_prebuild_info.ResultDataMaxSizeInBytes,
                                          D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                          D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
                                          &blas_buffers.result));

        // Define a bottom level acceleration structure.
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC blas_desc = {};
        blas_desc.DestAccelerationStructureData = blas_buffers.result->GetGPUVirtualAddress();
        blas_desc.Inputs = blas_inputs;
        blas_desc.ScratchAccelerationStructureData = blas_buffers.scratch->GetGPUVirtualAddress();

        // Record a build raytracing bottom level acceleration structure command.
        command_list->BuildRaytracingAccelerationStructure(&blas_desc, 0, nullptr);

        D3D12_RESOURCE_BARRIER barrier = {};

        // Record a barrier.
        barrier = CD3DX12_RESOURCE_BARRIER::UAV(blas_buffers.result.Get());
        command_list->ResourceBarrier(1, &barrier);
        _blas_buffer = blas_buffers.result;

        // Define a raytracing instance.
        D3D12_RAYTRACING_INSTANCE_DESC instance_desc = {};
        memcpy(instance_desc.Transform, &kIdentityFloat4x4, sizeof(instance_desc.Transform));
        instance_desc.InstanceID = 0;
        instance_desc.InstanceMask = 0xFF;
        instance_desc.InstanceContributionToHitGroupIndex = 0;
        instance_desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        instance_desc.AccelerationStructure = _blas_buffer->GetGPUVirtualAddress();

        AccelerationStructureBuffers tlas_buffers;

        // Create a buffer for the instance description.
        ThrowIfFailed(CreateUploadBuffer(_device.Get(), sizeof(D3D12_RAYTRACING_INSTANCE_DESC),
                                         &tlas_buffers.instance_desc));
        UpdateBuffer(tlas_buffers.instance_desc.Get(), &instance_desc, sizeof(D3D12_RAYTRACING_INSTANCE_DESC));

        // Define the top level acceleration structure inputs.
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS tlas_inputs = {};
        tlas_inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        tlas_inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
        tlas_inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        tlas_inputs.NumDescs = 1;
        tlas_inputs.InstanceDescs = tlas_buffers.instance_desc->GetGPUVirtualAddress();

        // Retrieve a prebuilt information for the top level acceleration structure.
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO tlas_prebuid_info = {};
        _device->GetRaytracingAccelerationStructurePrebuildInfo(&tlas_inputs, &tlas_prebuid_info);

        // Create buffers for the top level acceleration structure.
        ThrowIfFailed(CreateDefaultBuffer(_device.Get(), tlas_prebuid_info.ScratchDataSizeInBytes,
                                          D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                          D3D12_RESOURCE_STATE_COMMON,
                                          &tlas_buffers.scratch));
        ThrowIfFailed(CreateDefaultBuffer(_device.Get(), tlas_prebuid_info.ResultDataMaxSizeInBytes,
                                          D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                          D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
                                          &tlas_buffers.result));

        // Define a top level acceleration structure.
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC tlas_desc = {};
        tlas_desc.DestAccelerationStructureData = tlas_buffers.result->GetGPUVirtualAddress();
        tlas_desc.Inputs = tlas_inputs;
        tlas_desc.ScratchAccelerationStructureData = tlas_buffers.scratch->GetGPUVirtualAddress();

        // Record a build raytracing bottom level acceleration structure command.
        command_list->BuildRaytracingAccelerationStructure(&tlas_desc, 0, nullptr);

        // Record a barrier.
        barrier = CD3DX12_RESOURCE_BARRIER::UAV(blas_buffers.result.Get());
        command_list->ResourceBarrier(1, &barrier);
        _tlas_buffer = tlas_buffers.result;

        // Stop recording a command list.
        command_list->Close();

        // Submit a command list.
        ID3D12CommandList *command_lists[] = {command_list.Get()};
        _command_queue->ExecuteCommandLists(_countof(command_lists), command_lists);
        WaitCommandQueueIdle();

        // Initialize descriptor sets.
        CD3DX12_CPU_DESCRIPTOR_HANDLE cpu_handle(
                _descriptor_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->GetCPUDescriptorHandleForHeapStart());
        for (auto i = 0; i != kSwapChainBufferCount; ++i) {
            // Skip the first slot because it's for the offscreen buffer.
            cpu_handle.Offset(_descriptor_heap_sizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]);

            // Define a SRV.
            D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
            srv_desc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
            srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srv_desc.RaytracingAccelerationStructure.Location = _tlas_buffer->GetGPUVirtualAddress();

            // Create a SRV.
            _device->CreateShaderResourceView(nullptr, &srv_desc, cpu_handle);
            cpu_handle.Offset(_descriptor_heap_sizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]);

            // Define a CBV.
            D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
            cbv_desc.BufferLocation = _constant_buffers[i]->GetGPUVirtualAddress();
            cbv_desc.SizeInBytes = static_cast<UINT>(AlignPow2(sizeof(Transformations), 256));

            // Create a CBV.
            _device->CreateConstantBufferView(&cbv_desc, cpu_handle);
            cpu_handle.Offset(_descriptor_heap_sizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]);
        }
    }

    void InitOffscreenBuffers() {
        // Create offscreen buffers.
        for (auto &offscreen_buffer : _offscreen_buffers) {
            ThrowIfFailed(CreateDefaultTexture2D(_device.Get(), _width, _height, 1,
                                                 DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                                 D3D12_RESOURCE_STATE_COPY_SOURCE, &offscreen_buffer));
        }

        // Initialize descriptor sets.
        CD3DX12_CPU_DESCRIPTOR_HANDLE cpu_handle(
                _descriptor_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->GetCPUDescriptorHandleForHeapStart());
        for (auto i = 0; i != kSwapChainBufferCount; ++i) {
            // Create an UAV.
            _device->CreateUnorderedAccessView(_offscreen_buffers[i].Get(), nullptr, nullptr, cpu_handle);
            cpu_handle.Offset(_descriptor_heap_sizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV], 3);
        }
    }

    void InitPipelines() {
        CD3DX12_DESCRIPTOR_RANGE descriptor_ranges[3];
        CD3DX12_ROOT_PARAMETER root_parameter;
        CD3DX12_ROOT_SIGNATURE_DESC root_signature_desc;

        // Define descriptor ranges for the ray generation root signature.
        descriptor_ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
        descriptor_ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
        descriptor_ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

        // Define a root parameter for the ray generation root signature.
        root_parameter.InitAsDescriptorTable(_countof(descriptor_ranges), descriptor_ranges);

        // Define a ray generation root signature.
        root_signature_desc = CD3DX12_ROOT_SIGNATURE_DESC(1, &root_parameter, 0, nullptr,
                                                          D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);

        // Create a ray generation root signature.
        ThrowIfFailed(CreateRootSignature(_device.Get(), &root_signature_desc, &_ray_generation_root_signature));

        // Define a miss root signature.
        root_signature_desc = CD3DX12_ROOT_SIGNATURE_DESC(0, nullptr, 0, nullptr,
                                                          D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);

        // Create a miss root signature.
        ThrowIfFailed(CreateRootSignature(_device.Get(), &root_signature_desc, &_miss_root_signature));

        // Define a root parameter for the hit group root signature.
        root_parameter.InitAsShaderResourceView(1);

        // Define a hit group root signature.
        root_signature_desc = CD3DX12_ROOT_SIGNATURE_DESC(1, &root_parameter, 0, nullptr,
                                                          D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);

        // Create a hit group root signature.
        ThrowIfFailed(CreateRootSignature(_device.Get(), &root_signature_desc, &_hit_group_root_signature));

        // Define a global root signature.
        root_signature_desc = CD3DX12_ROOT_SIGNATURE_DESC(0, nullptr, 0, nullptr);

        // Create a global root signature.
        ThrowIfFailed(CreateRootSignature(_device.Get(), &root_signature_desc, &_global_root_signature));

        // Calculate the shader binding table size.
        _sbt_size = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
        _sbt_size += 8; // This slot is used for binding a descriptor table or CBV.
        _sbt_size = AlignPow2(_sbt_size, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);

        // Create a shader binding table buffers.
        for (auto &sbt_buffer : _sbt_buffers) {
            ThrowIfFailed(CreateUploadBuffer(_device.Get(), _sbt_size * 3, &sbt_buffer));
        }

        // Use 11 subobjects to define the raytracing pipeline.
        // 1. DXIL library
        // 2. The global root signature.
        // 3. The ray generation root signature.
        // 4. The ray generation root signature to exports association.
        // 5. The miss root signature.
        // 6. The miss root signature to exports association.
        // 7. The hit group.
        // 8. The hit group root signature.
        // 9. The hit group root signature to exports association.
        // 10. The shader config.
        // 11. The pipeline config.
        std::array<D3D12_STATE_SUBOBJECT, 11> subobjects = {};
        uint32_t index = 0;

        // Compile a library.
        ComPtr<IDxcBlob> library;
        _compiler.CompileLibrary(BuildFilePath("raytracing.hlsl"), &library);

        // Define exports.
        std::array<D3D12_EXPORT_DESC, 3> exports = {};
        exports[0].Name = kRayGeneration;
        exports[0].Flags = D3D12_EXPORT_FLAG_NONE;
        exports[1].Name = kMiss;
        exports[1].Flags = D3D12_EXPORT_FLAG_NONE;
        exports[2].Name = kClosestHit;
        exports[2].Flags = D3D12_EXPORT_FLAG_NONE;

        // Define DXIL library.
        D3D12_DXIL_LIBRARY_DESC library_desc = {};
        library_desc.DXILLibrary = {library->GetBufferPointer(), library->GetBufferSize()};
        library_desc.NumExports = static_cast<UINT>(exports.size());
        library_desc.pExports = exports.data();

        // 1. Define a subobject as DXIL library.
        subobjects[index].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
        subobjects[index].pDesc = &library_desc;
        ++index;

        // 2. Define a subobject as the global root signature.
        subobjects[index].Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
        subobjects[index].pDesc = _global_root_signature.GetAddressOf();
        ++index;

        // 3. Define a subobject as the ray generation root signature.
        subobjects[index].Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
        subobjects[index].pDesc = _ray_generation_root_signature.GetAddressOf();
        ++index;

        // Define a ray generation root signature to exports association.
        D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION ray_generation_exports_association = {};
        ray_generation_exports_association.pSubobjectToAssociate = &subobjects[index - 1];
        ray_generation_exports_association.NumExports = 1;
        ray_generation_exports_association.pExports = &kRayGeneration;

        // 4. Define a subobject as a ray generation root signature to exports association.
        subobjects[index].Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
        subobjects[index].pDesc = &ray_generation_exports_association;
        ++index;

        // 5. Define a subobject as the miss root signature.
        subobjects[index].Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
        subobjects[index].pDesc = _miss_root_signature.GetAddressOf();
        ++index;

        // Define a miss root signature to exports association.
        D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION miss_exports_association = {};
        miss_exports_association.pSubobjectToAssociate = &subobjects[index - 1];
        miss_exports_association.NumExports = 1;
        miss_exports_association.pExports = &kMiss;

        // 6. Define a subobject as a miss root signature to exports association.
        subobjects[index].Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
        subobjects[index].pDesc = &miss_exports_association;
        ++index;

        // Define a hit group.
        D3D12_HIT_GROUP_DESC hit_group_desc = {};
        hit_group_desc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
        hit_group_desc.HitGroupExport = kHitGroup;
        hit_group_desc.ClosestHitShaderImport = kClosestHit;

        // 7. Define a subobject as a hit group.
        subobjects[index].Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
        subobjects[index].pDesc = &hit_group_desc;
        ++index;

        // 8. Define a subobject as a hit group root signature.
        subobjects[index].Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
        subobjects[index].pDesc = _hit_group_root_signature.GetAddressOf();
        ++index;

        // Define a hit group root signature to exports association.
        D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION hit_group_exports_association = {};
        hit_group_exports_association.pSubobjectToAssociate = &subobjects[index - 1];
        hit_group_exports_association.NumExports = 1;
        hit_group_exports_association.pExports = &kClosestHit;

        // 9. Define a subobject as a hit group root signature.
        subobjects[index].Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
        subobjects[index].pDesc = &hit_group_exports_association;
        ++index;

        // Define the shader config.
        D3D12_RAYTRACING_SHADER_CONFIG shader_config = {};
        shader_config.MaxPayloadSizeInBytes = 4 * sizeof(float);
        shader_config.MaxAttributeSizeInBytes = 2 * sizeof(float);

        // 10. Define a subobject as the shader config.
        subobjects[index].Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
        subobjects[index].pDesc = &shader_config;
        ++index;

        // Define the pipeline config.
        D3D12_RAYTRACING_PIPELINE_CONFIG pipeline_config = {};
        pipeline_config.MaxTraceRecursionDepth = 1;

        // 11. Define a subobject as the pipeline config.
        subobjects[index].Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
        subobjects[index].pDesc = &pipeline_config;
        ++index;

        // Define a raytracing pipeline.
        D3D12_STATE_OBJECT_DESC desc = {};
        desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
        desc.NumSubobjects = index;
        desc.pSubobjects = subobjects.data();

        // Create a raytracing pipeline.
        ThrowIfFailed(_device->CreateStateObject(&desc, IID_PPV_ARGS(&_raytracing_pipeline_state)));

        // Retrieve a raytracing pipeline properties.
        ThrowIfFailed(_raytracing_pipeline_state->QueryInterface(IID_PPV_ARGS(&_raytracing_pipeline_state_properties)));
    }

private:
    FrameResource<ID3D12Resource> _offscreen_buffers;
    ComPtr<ID3D12Resource> _vertex_buffer;
    ComPtr<ID3D12Resource> _index_buffer;
    FrameResource<ID3D12Resource> _constant_buffers;
    ComPtr<ID3D12Resource> _blas_buffer;
    ComPtr<ID3D12Resource> _tlas_buffer;
    ComPtr<ID3D12RootSignature> _global_root_signature;
    ComPtr<ID3D12RootSignature> _ray_generation_root_signature;
    ComPtr<ID3D12RootSignature> _miss_root_signature;
    ComPtr<ID3D12RootSignature> _hit_group_root_signature;
    UINT64 _sbt_size = 0;
    FrameResource<ID3D12Resource> _sbt_buffers;
    ComPtr<ID3D12StateObject> _raytracing_pipeline_state;
    ComPtr<ID3D12StateObjectProperties> _raytracing_pipeline_state_properties;
    UINT _width = 0;
    UINT _height = 0;
};

//----------------------------------------------------------------------------------------------------------------------

int main(int argc, char *argv[]) {
    try {
        auto example = std::make_unique<RaytracingTriangle>();
        Window::GetInstance()->MainLoop(example.get());
    }
    catch (const std::exception &exception) {
        OutputDebugStringA(exception.what());
    }
    return 0;
}

//----------------------------------------------------------------------------------------------------------------------

#ifdef __clang__
#pragma clang diagnostic pop
#endif
