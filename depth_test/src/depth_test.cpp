//
// This file is part of the "DirectX12" project
// See "LICENSE" for license information.
//

#include <generator/generator.hpp>
#include <common/window.h>
#include <common/example.h>
#include <common/resource_uploader.h>
#include <memory>

using namespace DirectX;

//----------------------------------------------------------------------------------------------------------------------

struct Vertex {
    XMFLOAT3 position;
    XMFLOAT3 normal;
};

//----------------------------------------------------------------------------------------------------------------------

struct Constants {
    XMFLOAT4X4 projection;
    XMFLOAT4X4 view;
    XMFLOAT4X4 model;
    XMFLOAT3X4 normal;
};

//----------------------------------------------------------------------------------------------------------------------

struct Options {
    float camera_near = 1.0f;
    float camera_far = 10.0f;
    std::array<float, 3> light_position = {0.0f, 0.0f, -5.0f};
    bool use_depth_test = true;
    INT depth_write_mask = D3D12_DEPTH_WRITE_MASK_ALL;
    INT depth_function = D3D12_COMPARISON_FUNC_LESS - 1;
    float clear_depth_value = 1.0f;
};

//----------------------------------------------------------------------------------------------------------------------

const std::unordered_map<D3D12_DESCRIPTOR_HEAP_TYPE, UINT> kDescriptorCount = {
        {D3D12_DESCRIPTOR_HEAP_TYPE_RTV,         kSwapChainBufferCount},
        {D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kImGuiFontBufferCount + 1},
        {D3D12_DESCRIPTOR_HEAP_TYPE_DSV,         1}};
const std::vector<const char *> kDepthWriteMaskNames = {"ZERO", "ALL"};
const std::vector<const char *> kDepthFunctionNames = {"NEVER", "LESS", "EQUAL", "LESS_EQUAL", "GREATER", "NOT_EQUAL",
                                                       "GREATER_EQUAL", "ALWAYS"};
const std::vector<D3D12_COMPARISON_FUNC> kDepthFunctions = {D3D12_COMPARISON_FUNC_NEVER,
                                                            D3D12_COMPARISON_FUNC_LESS,
                                                            D3D12_COMPARISON_FUNC_EQUAL,
                                                            D3D12_COMPARISON_FUNC_LESS_EQUAL,
                                                            D3D12_COMPARISON_FUNC_GREATER,
                                                            D3D12_COMPARISON_FUNC_NOT_EQUAL,
                                                            D3D12_COMPARISON_FUNC_GREATER_EQUAL,
                                                            D3D12_COMPARISON_FUNC_ALWAYS};

//----------------------------------------------------------------------------------------------------------------------

class DepthTest : public Example {
public:
    DepthTest() : Example("Depth test", kDescriptorCount) {
        FileSystem::GetInstance()->AddDirectory(DEPTH_TEST_ASSET_DIR);

        InitResources();
        InitPipelines();
    }

protected:
    void OnInit() override {
        // Initialize camera properties.
        _camera.SetNear(_options.camera_near);
        _camera.SetFar(_options.camera_far);
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

        // Resize a depth buffer.
        InitDepthBuffer();
    }

    void OnUpdate(UINT index) override {
        // Update ImGui.
        if (ImGui::CollapsingHeader("Options", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::DragFloatRange2("Camera near and far", &_options.camera_near, &_options.camera_far,
                                       0.05f, 1.0f, 100.0f, "%.2f", nullptr, ImGuiSliderFlags_AlwaysClamp)) {
                _camera.SetNear(_options.camera_near);
                _camera.SetFar(_options.camera_far);
            }

            if (ImGui::Checkbox("Use depth test", &_options.use_depth_test)) {
                WaitCommandQueueIdle();
                InitPipelines();
            }

            ImGui::SliderFloat("Clear depth value", &_options.clear_depth_value, 0.0f, 1.0f);

            if (ImGui::Combo("Depth write mask", &_options.depth_write_mask, kDepthWriteMaskNames.data(),
                             static_cast<INT>(kDepthWriteMaskNames.size()))) {
                WaitCommandQueueIdle();
                InitPipelines();
            }

            if (ImGui::Combo("Depth function", &_options.depth_function, kDepthFunctionNames.data(),
                             static_cast<INT>(kDepthFunctionNames.size()))) {
                WaitCommandQueueIdle();
                InitPipelines();
            }
        }

        // Define transformation.
        Constants constants;
        constants.projection = _camera.GetProjection();
        constants.view = _camera.GetView();
        constants.model = kIdentityFloat4x4;
        constants.normal = XMMatrixInverseTranspose(constants.model);

        // Update transformation.
        UpdateBuffer(_constant_buffers[index].Get(), &constants, sizeof(Constants));
    }

    void OnRender(UINT index) override {
        FLOAT clear_color[4] = {};
        D3D12_RESOURCE_BARRIER resource_barrier = {};

        // Define a transition of a resource from PRESENT to RENDER_TARGET.
        resource_barrier = CD3DX12_RESOURCE_BARRIER::Transition(_swap_chain_buffers[index].Get(),
                                                                D3D12_RESOURCE_STATE_PRESENT,
                                                                D3D12_RESOURCE_STATE_RENDER_TARGET);

        // Record a resource transition command.
        _command_list->ResourceBarrier(1, &resource_barrier);

        // Define a clear color.
        clear_color[0] = 0.0f;
        clear_color[1] = 0.0f;
        clear_color[2] = 0.2f;
        clear_color[3] = 1.0f;

        // Record clearing render target view command.
        _command_list->ClearRenderTargetView(_swap_chain_views[index], clear_color, 0, nullptr);
        _command_list->ClearDepthStencilView(_depth_buffer_view, D3D12_CLEAR_FLAG_DEPTH, _options.clear_depth_value, 0,
                                             0, nullptr);

        // Record commands to draw a triangle.
        _command_list->OMSetRenderTargets(1, &_swap_chain_views[index], true, &_depth_buffer_view);
        _command_list->RSSetViewports(1, &_viewport);
        _command_list->RSSetScissorRects(1, &_scissor_rect);
        _command_list->SetGraphicsRootSignature(_root_signature.Get());
        _command_list->SetGraphicsRootConstantBufferView(0, _constant_buffers[index]->GetGPUVirtualAddress());
        _command_list->SetPipelineState(_pipeline_state.Get());
        _command_list->IASetVertexBuffers(0, 1, &_vertex_buffer_view);
        _command_list->IASetIndexBuffer(&_index_buffer_view);
        _command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        _command_list->DrawIndexedInstanced(_draw_count, 1, 0, 0, 0);

        RecordDrawImGuiCommands(_command_list.Get());

        // Define a transition of a resource from RENDER_TARGET to PRESENT.
        resource_barrier = CD3DX12_RESOURCE_BARRIER::Transition(_swap_chain_buffers[index].Get(),
                                                                D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                                D3D12_RESOURCE_STATE_PRESENT);

        // Record a resource transition command.
        _command_list->ResourceBarrier(1, &resource_barrier);
    }

private:
    void InitResources() {
        generator::TorusKnotMesh generator;
        std::vector<Vertex> vertices;
        std::vector<UINT16> indices;

        for (auto &vertex : generator.vertices()) {
            auto position = XMFLOAT3(static_cast<float>(vertex.position[0]),
                                     static_cast<float>(vertex.position[1]),
                                     static_cast<float>(vertex.position[2]));
            auto normal = XMFLOAT3(static_cast<float>(vertex.normal[0]),
                                   static_cast<float>(vertex.normal[1]),
                                   static_cast<float>(vertex.normal[2]));
            vertices.push_back({position, normal});
        }

        for (auto &triangle : generator.triangles()) {
            indices.push_back(static_cast<UINT16>(triangle.vertices[0]));
            indices.push_back(static_cast<UINT16>(triangle.vertices[1]));
            indices.push_back(static_cast<UINT16>(triangle.vertices[2]));
        }

        _draw_count = static_cast<UINT>(indices.size());

        auto vertex_size = static_cast<UINT>(sizeof(Vertex) * vertices.size());
        auto index_size = static_cast<UINT>(sizeof(UINT16) * indices.size());

        ResourceUploader uploader(_device.Get());

        // Initialize a vertex buffer.
        ThrowIfFailed(CreateDefaultBuffer(_device.Get(), vertex_size, &_vertex_buffer));
        uploader.RecordCopyData(_vertex_buffer.Get(), vertices.data(), vertex_size);

        // Initialize an index buffer.
        ThrowIfFailed(CreateDefaultBuffer(_device.Get(), index_size, &_index_buffer));
        uploader.RecordCopyData(_index_buffer.Get(), indices.data(), index_size);

        uploader.Execute();

        // Initialize constant buffers.
        for (auto i = 0; i != kSwapChainBufferCount; ++i) {
            ThrowIfFailed(CreateConstantBuffer(_device.Get(), sizeof(Constants), &_constant_buffers[i]));
        }

        // Initialize a vertex buffer view.
        _vertex_buffer_view.BufferLocation = _vertex_buffer->GetGPUVirtualAddress();
        _vertex_buffer_view.SizeInBytes = vertex_size;
        _vertex_buffer_view.StrideInBytes = sizeof(Vertex);

        // Initialize an index buffer view.
        _index_buffer_view.BufferLocation = _index_buffer->GetGPUVirtualAddress();
        _index_buffer_view.SizeInBytes = index_size;
        _index_buffer_view.Format = DXGI_FORMAT_R16_UINT;
    }

    void InitPipelines() {
        // Define an input layout.
        std::vector<D3D12_INPUT_ELEMENT_DESC> input_layout = {
                {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                {"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

        // Define a root parameter.
        CD3DX12_ROOT_PARAMETER root_parameter;
        root_parameter.InitAsConstantBufferView(0);

        // Define a root signature.
        CD3DX12_ROOT_SIGNATURE_DESC root_signature_desc(1, &root_parameter, 0, nullptr,
                                                        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        // Create a root signature.
        ThrowIfFailed(CreateRootSignature(_device.Get(), &root_signature_desc, &_root_signature));

        // Compile a vertex shader.
        ComPtr<IDxcBlob> vertex_shader;
        ThrowIfFailed(_compiler.CompileShader("pass_through.hlsl", L"VSMain", L"vs_6_0", &vertex_shader));

        // Compile a pixel shader.
        ComPtr<IDxcBlob> pixel_shader;
        ThrowIfFailed(_compiler.CompileShader("pass_through.hlsl", L"PSMain", L"ps_6_0", &pixel_shader));

        // Define a depth stencil state.
        D3D12_DEPTH_STENCIL_DESC depth_stencil_desc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        depth_stencil_desc.DepthEnable = _options.use_depth_test;
        depth_stencil_desc.DepthWriteMask = static_cast<D3D12_DEPTH_WRITE_MASK>(_options.depth_write_mask);
        depth_stencil_desc.DepthFunc = kDepthFunctions[_options.depth_function];

        // Define a graphics pipeline state.
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
        desc.InputLayout = {input_layout.data(), static_cast<UINT>(input_layout.size())};
        desc.pRootSignature = _root_signature.Get();
        desc.VS = {reinterpret_cast<BYTE *>(vertex_shader->GetBufferPointer()), vertex_shader->GetBufferSize()};
        desc.PS = {reinterpret_cast<BYTE *>(pixel_shader->GetBufferPointer()), pixel_shader->GetBufferSize()};
        desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        desc.DepthStencilState = depth_stencil_desc;
        desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        desc.SampleMask = UINT_MAX;
        desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc.NumRenderTargets = 1;
        desc.RTVFormats[0] = kSwapChainFormat;
        desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        desc.SampleDesc = {1, 0};

        // Create a graphics pipeline state.
        ThrowIfFailed(_device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&_pipeline_state)));
    }

    void InitDepthBuffer() {
        D3D12_CLEAR_VALUE clr;
        clr.Format = DXGI_FORMAT_D32_FLOAT;
        clr.DepthStencil.Depth = 1.0f;

        auto resolution = Window::GetInstance()->GetResolution();
        CreateDefaultTexture2D(_device.Get(), GetWidth(resolution), GetHeight(resolution), 1, DXGI_FORMAT_D32_FLOAT,
                               D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clr,
                               &_depth_buffer);

        _depth_buffer_view = _descriptor_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_DSV]->GetCPUDescriptorHandleForHeapStart();
        _device->CreateDepthStencilView(_depth_buffer.Get(), nullptr, _depth_buffer_view);
    }

private:
    Options _options;
    ComPtr<ID3D12Resource> _vertex_buffer;
    ComPtr<ID3D12Resource> _index_buffer;
    FrameResource<ID3D12Resource> _constant_buffers;
    D3D12_VERTEX_BUFFER_VIEW _vertex_buffer_view = {};
    D3D12_INDEX_BUFFER_VIEW _index_buffer_view = {};
    ComPtr<ID3D12RootSignature> _root_signature;
    ComPtr<ID3D12PipelineState> _pipeline_state;
    ComPtr<ID3D12Resource> _depth_buffer;
    D3D12_CPU_DESCRIPTOR_HANDLE _depth_buffer_view = {};
    D3D12_VIEWPORT _viewport = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    D3D12_RECT _scissor_rect = {0, 0, 0, 0};
    UINT _draw_count = 0;
};

//----------------------------------------------------------------------------------------------------------------------

int main(int argc, char *argv[]) {
    try {
        auto example = std::make_unique<DepthTest>();
        Window::GetInstance()->MainLoop(example.get());
    }
    catch (const std::exception &exception) {
        OutputDebugStringA(exception.what());
    }
    return 0;
}

//----------------------------------------------------------------------------------------------------------------------
