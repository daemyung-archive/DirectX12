//
// This file is part of the "DirectX12" project
// See "LICENSE" for license information.
//

#include <common/window.h>
#include <common/example.h>
#include <common/resource_uploader.h>
#include <common/image_loader.h>
#include <memory>
#include <vector>
#include <array>

using namespace DirectX;

//----------------------------------------------------------------------------------------------------------------------

struct Vertex {
    XMFLOAT3 position;
    XMFLOAT2 uv;
};

//----------------------------------------------------------------------------------------------------------------------

struct Transforms {
    XMFLOAT4X4 projection;
    XMFLOAT4X4 view;
    XMFLOAT4X4 model;
    XMFLOAT4X4 uv_transform;
};

//----------------------------------------------------------------------------------------------------------------------

struct Options {
    std::array<float, 2> uv_translation = {-0.5f, -0.5f};
    float uv_rotation = 0.0f;
    std::array<float, 2> uv_scale = {4.0f, 4.0f};
    INT sampler_filter = 0;
    INT sampler_address_u = 0;
    INT sampler_address_v = 0;
    INT sampler_max_anisotropy = 1;
    std::array<float, 4> sampler_border_color = {0.0f, 0.0f, 0.0f, 1.0f};
};

//----------------------------------------------------------------------------------------------------------------------

inline auto BuildFilePath(const std::string &file_name) {
    std::filesystem::path file_path;

    file_path = fmt::format("{}/{}", SAMPLER_ASSET_DIR, file_name);
    if (file_path.has_filename()) {
        return file_path;
    }

    throw std::runtime_error(fmt::format("File isn't exist: {}.", file_name));
}

//----------------------------------------------------------------------------------------------------------------------

const std::unordered_map<D3D12_DESCRIPTOR_HEAP_TYPE, UINT> kDescriptorCount = {
        {D3D12_DESCRIPTOR_HEAP_TYPE_RTV,         kSwapChainBufferCount},
        {D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kImGuiFontBufferCount + 1},
        {D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,     1}};
const std::vector<const char *> kFilterNames = {"MIN_MAG_MIP_POINT",
                                                "MIN_MAG_POINT_MIP_LINEAR",
                                                "MIN_POINT_MAG_LINEAR_MIP_POINT",
                                                "MIN_POINT_MAG_MIP_LINEAR",
                                                "MIN_LINEAR_MAG_MIP_POINT",
                                                "MIN_LINEAR_MAG_POINT_MIP_LINEAR",
                                                "MIN_MAG_LINEAR_MIP_POINT",
                                                "MIN_MAG_MIP_LINEAR",
                                                "ANISOTROPIC",
                                                "MINIMUM_MIN_MAG_MIP_POINT",
                                                "MINIMUM_MIN_MAG_POINT_MIP_LINEAR",
                                                "MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT",
                                                "MINIMUM_MIN_POINT_MAG_MIP_LINEAR",
                                                "MINIMUM_MIN_LINEAR_MAG_MIP_POINT",
                                                "MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR",
                                                "MINIMUM_MIN_MAG_LINEAR_MIP_POINT",
                                                "MINIMUM_MIN_MAG_MIP_LINEAR",
                                                "MINIMUM_ANISOTROPIC",
                                                "MAXIMUM_MIN_MAG_MIP_POINT",
                                                "MAXIMUM_MIN_MAG_POINT_MIP_LINEAR",
                                                "MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT",
                                                "MAXIMUM_MIN_POINT_MAG_MIP_LINEAR",
                                                "MAXIMUM_MIN_LINEAR_MAG_MIP_POINT",
                                                "MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR",
                                                "MAXIMUM_MIN_MAG_LINEAR_MIP_POINT",
                                                "MAXIMUM_MIN_MAG_MIP_LINEAR",
                                                "MAXIMUM_ANISOTROPIC"};
const std::vector<const char *> kTextureAddressModeNames = {"TEXTURE_ADDRESS_MODE_WRAP",
                                                            "TEXTURE_ADDRESS_MODE_MIRROR",
                                                            "TEXTURE_ADDRESS_MODE_CLAMP",
                                                            "TEXTURE_ADDRESS_MODE_BORDER",
                                                            "TEXTURE_ADDRESS_MODE_MIRROR_ONCE"};
const std::vector<D3D12_FILTER> kFilters = {D3D12_FILTER_MIN_MAG_MIP_POINT,
                                            D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR,
                                            D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT,
                                            D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR,
                                            D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT,
                                            D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
                                            D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT,
                                            D3D12_FILTER_MIN_MAG_MIP_LINEAR,
                                            D3D12_FILTER_ANISOTROPIC,
                                            D3D12_FILTER_MINIMUM_MIN_MAG_MIP_POINT,
                                            D3D12_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR,
                                            D3D12_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT,
                                            D3D12_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR,
                                            D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT,
                                            D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
                                            D3D12_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT,
                                            D3D12_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR,
                                            D3D12_FILTER_MINIMUM_ANISOTROPIC,
                                            D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_POINT,
                                            D3D12_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR,
                                            D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT,
                                            D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR,
                                            D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT,
                                            D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
                                            D3D12_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT,
                                            D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR,
                                            D3D12_FILTER_MAXIMUM_ANISOTROPIC};
const std::vector<D3D12_TEXTURE_ADDRESS_MODE> kTextureAddressModes = {D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                                                                      D3D12_TEXTURE_ADDRESS_MODE_MIRROR,
                                                                      D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                                                                      D3D12_TEXTURE_ADDRESS_MODE_BORDER,
                                                                      D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE};

//----------------------------------------------------------------------------------------------------------------------

class Sampler : public Example {
public:
    Sampler() : Example("Sampler", kDescriptorCount) {
        InitResources();
        InitSampler();
        InitPipelines();
    }

protected:
    void OnInit() override {
        _camera.SetRadius(2.0f);
    }

    void OnTerm() override {
    }

    void OnResize(const Resolution &resolution) override {
        // Update a viewport.
        _viewport.Width = static_cast<float>(GetWidth(resolution));
        _viewport.Height = static_cast<float>(GetHeight(resolution));

        // Update a scissor rect.
        _scissor_rect.right = GetWidth(resolution);
        _scissor_rect.bottom = GetHeight(resolution);
    }

    void OnUpdate(UINT index) override {
        // Update ImGui.
        if (ImGui::CollapsingHeader("Options", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderFloat2("UV translation", _options.uv_translation.data(), -5.0f, 5.0f);
            ImGui::SliderFloat("UV rotation", &_options.uv_rotation, 0.0f, 360.0f);
            ImGui::SliderFloat2("UV scale", _options.uv_scale.data(), 1.0f, 6.0f);

            ImGui::Separator();

            if (ImGui::Combo("Sampler filter", &_options.sampler_filter, kFilterNames.data(),
                             static_cast<INT>(kFilterNames.size()))) {
                WaitCommandQueueIdle();
                InitSampler();
            }

            if (ImGui::Combo("Sampler address U", &_options.sampler_address_u, kTextureAddressModeNames.data(),
                             static_cast<INT>(kTextureAddressModeNames.size()))) {
                WaitCommandQueueIdle();
                InitSampler();
            }

            if (ImGui::Combo("Sampler address V", &_options.sampler_address_v, kTextureAddressModeNames.data(),
                             static_cast<INT>(kTextureAddressModeNames.size()))) {
                WaitCommandQueueIdle();
                InitSampler();
            }

            if (ImGui::SliderInt("Sampler max anisotropy", &_options.sampler_max_anisotropy, 1, 16)) {
                WaitCommandQueueIdle();
                InitSampler();
            }

            if (ImGui::ColorPicker4("Sampler border color", _options.sampler_border_color.data(),
                                    ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_DisplayRGB)) {
                WaitCommandQueueIdle();
                InitSampler();
            }
        }

        // Define transforms.
        Transforms transforms;
        transforms.projection = _camera.GetProjection();
        transforms.view = _camera.GetView();
        XMStoreFloat4x4(&transforms.model, XMMatrixRotationY(XM_PI));
        auto T = XMMatrixTranslation(_options.uv_translation[0], _options.uv_translation[1], 0.0f);
        auto R = XMMatrixRotationZ(XMConvertToRadians(_options.uv_rotation));
        auto S = XMMatrixScaling(_options.uv_scale[0], _options.uv_scale[1], 1.0f);
        XMStoreFloat4x4(&transforms.uv_transform, XMMatrixMultiply(T, XMMatrixMultiply(R, S)));

        // Update transformation.
        UpdateBuffer(_constant_buffers[index].Get(), &transforms, sizeof(Transforms));
    }

    void OnRender(UINT index) override {
        FLOAT clear_color[4] = {};
        D3D12_VIEWPORT viewport = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
        D3D12_RECT scissor_rect = {};
        D3D12_RESOURCE_BARRIER resource_barrier = {};

        // Define a transition of a resource from PRESENT to RENDER_TARGET.
        resource_barrier = CD3DX12_RESOURCE_BARRIER::Transition(_swap_chain_buffers[index].Get(),
                                                                D3D12_RESOURCE_STATE_PRESENT,
                                                                D3D12_RESOURCE_STATE_RENDER_TARGET);

        // Record a resource transition command.
        _command_list->ResourceBarrier(1, &resource_barrier);

        // Define a clear color.
        clear_color[0] = 0.025f;
        clear_color[1] = 0.025f;
        clear_color[2] = 0.025f;
        clear_color[3] = 1.0f;

        // Record clearing render target view command.
        _command_list->ClearRenderTargetView(_swap_chain_views[index], clear_color, 0, nullptr);

        // Record commands to draw a triangle.
        _command_list->OMSetRenderTargets(1, &_swap_chain_views[index], true, nullptr);
        _command_list->RSSetViewports(1, &_viewport);
        _command_list->RSSetScissorRects(1, &_scissor_rect);
        _command_list->SetGraphicsRootSignature(_root_signature.Get());

        ID3D12DescriptorHeap *heaps[2] = {_descriptor_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].Get(),
                                          _descriptor_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER].Get()};
        _command_list->SetDescriptorHeaps(2, heaps);
        _command_list->SetGraphicsRootConstantBufferView(0, _constant_buffers[index]->GetGPUVirtualAddress());
        _command_list->SetGraphicsRootDescriptorTable(1,
                                                      _descriptor_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->GetGPUDescriptorHandleForHeapStart());
        _command_list->SetGraphicsRootDescriptorTable(2,
                                                      _descriptor_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER]->GetGPUDescriptorHandleForHeapStart());
        _command_list->SetPipelineState(_pipeline_state.Get());
        _command_list->IASetVertexBuffers(0, 1, &_vertex_buffer_view);
        _command_list->IASetIndexBuffer(&_index_buffer_view);
        _command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        _command_list->DrawIndexedInstanced(6, 1, 0, 0, 0);

        RecordDrawImGuiCommands();

        // Define a transition of a resource from RENDER_TARGET to PRESENT.
        resource_barrier = CD3DX12_RESOURCE_BARRIER::Transition(_swap_chain_buffers[index].Get(),
                                                                D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                                D3D12_RESOURCE_STATE_PRESENT);

        // Record a resource transition command.
        _command_list->ResourceBarrier(1, &resource_barrier);
    }

private:
    void InitResources() {
        // Define vertices.
        Vertex vertices[4] = {{XMFLOAT3(1.0f, -1.0f, 0.0f),  XMFLOAT2(0.0f, 1.0f)},
                              {XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f)},
                              {XMFLOAT3(-1.0f, 1.0f, 0.0f),  XMFLOAT2(1.0f, 0.0f)},
                              {XMFLOAT3(1.0f, 1.0f, 0.0f),   XMFLOAT2(0.0f, 0.0f)}};

        // Device indices.
        UINT16 indices[6] = {1, 0, 3, 1, 3, 2};

        ResourceUploader uploader(_device.Get());

        // Initialize a vertex buffer.
        ThrowIfFailed(CreateDefaultBuffer(_device.Get(), sizeof(vertices), &_vertex_buffer));
        uploader.RecordCopyData(_vertex_buffer.Get(), vertices, sizeof(vertices));

        // Initialize an index buffer.
        ThrowIfFailed(CreateDefaultBuffer(_device.Get(), sizeof(indices), &_index_buffer));
        uploader.RecordCopyData(_index_buffer.Get(), indices, sizeof(indices));

        // Read an image.
        ImageLoader image_loader;
        auto image = image_loader.LoadFile(BuildFilePath("uv_test_pattern.dds"));

        // Initialize a texture.
        ThrowIfFailed(CreateDefaultTexture2D(_device.Get(), image.width, image.height,
                                             image.mip_levels, image.format, &_texture));
        for (auto i = 0; i != image.mip_levels; ++i) {
            auto &subresource = image.subresources[i];
            uploader.RecordCopyData(_texture.Get(), i, subresource.data, subresource.row_pitch * subresource.height);
        }

        uploader.Execute();

        // Initialize constant buffers.
        for (auto i = 0; i != kSwapChainBufferCount; ++i) {
            ThrowIfFailed(CreateConstantBuffer(_device.Get(), sizeof(Transforms), &_constant_buffers[i]));
        }

        // Initialize a vertex buffer view.
        _vertex_buffer_view.BufferLocation = _vertex_buffer->GetGPUVirtualAddress();
        _vertex_buffer_view.SizeInBytes = sizeof(vertices);
        _vertex_buffer_view.StrideInBytes = sizeof(Vertex);

        // Initialize an index buffer view.
        _index_buffer_view.BufferLocation = _index_buffer->GetGPUVirtualAddress();
        _index_buffer_view.SizeInBytes = sizeof(indices);
        _index_buffer_view.Format = DXGI_FORMAT_R16_UINT;

        // Initialize a texture view.
        _device->CreateShaderResourceView(_texture.Get(), nullptr,
                                          _descriptor_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->GetCPUDescriptorHandleForHeapStart());
    }

    void InitSampler() {
        D3D12_SAMPLER_DESC desc = {};
        desc.Filter = kFilters[_options.sampler_filter];
        desc.AddressU = kTextureAddressModes[_options.sampler_address_u];
        desc.AddressV = kTextureAddressModes[_options.sampler_address_v];
        desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        desc.MaxAnisotropy = _options.sampler_max_anisotropy;
        memcpy(desc.BorderColor, _options.sampler_border_color.data(), sizeof(float) * 4);
        desc.MaxLOD = D3D12_FLOAT32_MAX;

        CD3DX12_CPU_DESCRIPTOR_HANDLE cpu_handle(
                _descriptor_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER]->GetCPUDescriptorHandleForHeapStart());
        _device->CreateSampler(&desc, cpu_handle);
    }

    void InitPipelines() {
        // Define an input layout.
        std::vector<D3D12_INPUT_ELEMENT_DESC> input_layout = {
                {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

        D3D12_DESCRIPTOR_RANGE descriptor_ranges[2] = {};
        descriptor_ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        descriptor_ranges[0].NumDescriptors = 1;
        descriptor_ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
        descriptor_ranges[1].NumDescriptors = 1;

        // Define a root parameter.
        CD3DX12_ROOT_PARAMETER root_parameters[3];
        root_parameters[0].InitAsConstantBufferView(0);
        root_parameters[1].InitAsDescriptorTable(1, &descriptor_ranges[0]);
        root_parameters[2].InitAsDescriptorTable(1, &descriptor_ranges[1]);

        // Define a root signature.
        CD3DX12_ROOT_SIGNATURE_DESC root_signature_desc(3, root_parameters, 0, nullptr,
                                                        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        // Create a root signature.
        ThrowIfFailed(CreateRootSignature(_device.Get(), &root_signature_desc, &_root_signature));

        // Compile a vertex shader.
        ComPtr<ID3DBlob> vertex_shader;
        ThrowIfFailed(CompileShader(BuildFilePath("unlit.hlsl"), "VSMain", "vs_5_0", &vertex_shader));

        // Compile a pixel shader.
        ComPtr<ID3DBlob> pixel_shader;
        ThrowIfFailed(CompileShader(BuildFilePath("unlit.hlsl"), "PSMain", "ps_5_0", &pixel_shader));

        // Define a graphics pipeline state.
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
        desc.InputLayout = {input_layout.data(), static_cast<UINT>(input_layout.size())};
        desc.pRootSignature = _root_signature.Get();
        desc.VS = {reinterpret_cast<BYTE *>(vertex_shader->GetBufferPointer()), vertex_shader->GetBufferSize()};
        desc.PS = {reinterpret_cast<BYTE *>(pixel_shader->GetBufferPointer()), pixel_shader->GetBufferSize()};
        desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        desc.DepthStencilState.DepthEnable = false;
        desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        desc.SampleMask = UINT_MAX;
        desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc.NumRenderTargets = 1;
        desc.RTVFormats[0] = kSwapChainFormat;
        desc.SampleDesc = {1, 0};

        // Create a graphics pipeline state.
        ThrowIfFailed(_device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&_pipeline_state)));
    }

private:
    Options _options;
    ComPtr<ID3D12Resource> _vertex_buffer;
    ComPtr<ID3D12Resource> _index_buffer;
    ComPtr<ID3D12Resource> _texture;
    FrameResource<ID3D12Resource> _constant_buffers;
    D3D12_VERTEX_BUFFER_VIEW _vertex_buffer_view = {};
    D3D12_INDEX_BUFFER_VIEW _index_buffer_view = {};
    ComPtr<ID3D12RootSignature> _root_signature;
    ComPtr<ID3D12PipelineState> _pipeline_state;
    D3D12_VIEWPORT _viewport = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    D3D12_RECT _scissor_rect = {0, 0, 0, 0};
};

//----------------------------------------------------------------------------------------------------------------------

int main(int argc, char *argv[]) {
    try {
        auto example = std::make_unique<Sampler>();
        Window::GetInstance()->MainLoop(example.get());
    }
    catch (const std::exception &exception) {
        OutputDebugStringA(exception.what());
    }
    return 0;
}

//----------------------------------------------------------------------------------------------------------------------