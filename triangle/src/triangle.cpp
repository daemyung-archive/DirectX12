//
// This file is part of the "DirectX12" project
// See "LICENSE" for license information.
//

#include <common/window.h>
#include <common/example.h>
#include <common/uploader.h>
#include <iostream>
#include <memory>

using namespace DirectX;

//----------------------------------------------------------------------------------------------------------------------

struct Vertex {
    XMFLOAT3 position;
    XMFLOAT3 color;
};

//----------------------------------------------------------------------------------------------------------------------

struct Options {
    bool use_staging_buffer = true;
};

//----------------------------------------------------------------------------------------------------------------------

const std::unordered_map<D3D12_DESCRIPTOR_HEAP_TYPE, UINT> kDescriptorCount = {
        {D3D12_DESCRIPTOR_HEAP_TYPE_RTV,         kSwapChainBufferCount},
        {D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kImGuiFontBufferCount}};

//----------------------------------------------------------------------------------------------------------------------

inline auto BuildFilePath(const std::string &file_name) {
    std::filesystem::path file_path;

    file_path = fmt::format("{}/{}", TRIANGLE_ASSET_DIR, file_name);
    if (file_path.has_filename()) {
        return file_path;
    }

    throw std::runtime_error(fmt::format("File isn't exist: {}.", file_name));
}

//----------------------------------------------------------------------------------------------------------------------

class Triangle : public Example {
public:
    Triangle() : Example("Triangle", kDescriptorCount) {
        InitResources();
        InitPipelines();
    }

protected:
    void OnBuildDescriptors() override {
        // Set the first CPU descriptor handle of RTV heap.
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(
                _descriptor_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]->GetCPUDescriptorHandleForHeapStart());

        // Initialize swap chain views.
        for (auto i = 0; i != kSwapChainBufferCount; ++i) {
            _device->CreateRenderTargetView(_swap_chain_buffers[i].Get(), nullptr, rtv);
            swap_chain_views[i] = rtv;
            rtv.Offset(_descriptor_heap_sizes[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]);
        }
    }

    void OnUpdateUniforms(UINT index) override {
    }

    void OnUpdateImGui() override {
        if (ImGui::CollapsingHeader("Options", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::Checkbox("Use staging buffer", &_options.use_staging_buffer)) {
                WaitCommandQueueIdle();
                InitResources();
            }
        }
    }

    void OnBuildCommands(UINT index) override {
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
        clear_color[0] = 0.0f;
        clear_color[1] = 0.0f;
        clear_color[2] = 0.2f;
        clear_color[3] = 1.0f;

        // Record clearing render target view command.
        _command_list->ClearRenderTargetView(swap_chain_views[index], clear_color, 0, nullptr);

        // Define a viewport.
        viewport.Width = static_cast<float>(GetWidth(_resolution));
        viewport.Height = static_cast<float>(GetHeight(_resolution));

        // Record to set a viewport.
        _command_list->RSSetViewports(1, &viewport);

        // Define a scissor rect.
        scissor_rect.right = GetWidth(_resolution);
        scissor_rect.bottom = GetHeight(_resolution);

        // Record to set a scissor rect
        _command_list->RSSetScissorRects(1, &scissor_rect);

        // Record commands to draw a triangle.
        _command_list->OMSetRenderTargets(1, &swap_chain_views[index], true, nullptr);
        _command_list->SetGraphicsRootSignature(_root_signature.Get());
        _command_list->SetPipelineState(_pipeline_state.Get());
        _command_list->IASetVertexBuffers(0, 1, &_vertex_buffer_view);
        _command_list->IASetIndexBuffer(&_index_buffer_view);
        _command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        _command_list->DrawIndexedInstanced(3, 1, 0, 0, 0);

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
        Vertex vertices[3] = {{XMFLOAT3(1.0f, -1.0f, 0.0f),  XMFLOAT3(Colors::Red)},
                              {XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT3(Colors::Lime)},
                              {XMFLOAT3(0.0f, 1.0f, 0.0f),   XMFLOAT3(Colors::Blue)}};

        // Device indices.
        UINT16 indices[3] = {0, 1, 2};

        if (_options.use_staging_buffer) {
            Uploader uploader(_device.Get());

            // Initialize a vertex buffer.
            ThrowIfFailed(CreateDefaultBuffer(_device.Get(), sizeof(vertices), &_vertex_buffer));
            uploader.RecordCopyData(_vertex_buffer.Get(), vertices, sizeof(vertices));

            // Initialize an index buffer.
            ThrowIfFailed(CreateDefaultBuffer(_device.Get(), sizeof(indices), &_index_buffer));
            uploader.RecordCopyData(_index_buffer.Get(), indices, sizeof(indices));

            uploader.Execute();
        } else {
            // Initialize a vertex buffer.
            ThrowIfFailed(CreateUploadBuffer(_device.Get(), sizeof(vertices), &_vertex_buffer));
            UpdateBuffer(_vertex_buffer.Get(), vertices, sizeof(vertices));

            // Initialize an index buffer.
            ThrowIfFailed(CreateUploadBuffer(_device.Get(), sizeof(indices), &_index_buffer));
            UpdateBuffer(_index_buffer.Get(), indices, sizeof(indices));
        }

        // Initialize a vertex buffer view.
        _vertex_buffer_view.BufferLocation = _vertex_buffer->GetGPUVirtualAddress();
        _vertex_buffer_view.SizeInBytes = sizeof(vertices);
        _vertex_buffer_view.StrideInBytes = sizeof(Vertex);

        // Initialize an index buffer view.
        _index_buffer_view.BufferLocation = _index_buffer->GetGPUVirtualAddress();
        _index_buffer_view.SizeInBytes = sizeof(indices);
        _index_buffer_view.Format = DXGI_FORMAT_R16_UINT;
    }

    void InitPipelines() {
        // Define an input layout.
        std::vector<D3D12_INPUT_ELEMENT_DESC> input_layout = {
                {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

        // Define a root signature.
        CD3DX12_ROOT_SIGNATURE_DESC root_signature_desc(0, nullptr, 0, nullptr,
                                                        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        // Create a root signature.
        ThrowIfFailed(CreateRootSignature(_device.Get(), &root_signature_desc, &_root_signature));

        // Compile a vertex shader.
        ComPtr<ID3DBlob> vertex_shader;
        ThrowIfFailed(CompileShader(BuildFilePath("pass_through.hlsl"), "VSMain", "vs_5_0", &vertex_shader));

        // Compile a pixel shader.
        ComPtr<ID3DBlob> pixel_shader;
        ThrowIfFailed(CompileShader(BuildFilePath("pass_through.hlsl"), "PSMain", "ps_5_0", &pixel_shader));

        // Define a graphics pipeline state.
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
        desc.InputLayout = {input_layout.data(), static_cast<UINT>(input_layout.size())};
        desc.pRootSignature = _root_signature.Get();
        desc.VS = {reinterpret_cast<BYTE *>(vertex_shader->GetBufferPointer()), vertex_shader->GetBufferSize()};
        desc.PS = {reinterpret_cast<BYTE *>(pixel_shader->GetBufferPointer()), pixel_shader->GetBufferSize()};
        desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
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
    D3D12_VERTEX_BUFFER_VIEW _vertex_buffer_view = {};
    D3D12_INDEX_BUFFER_VIEW _index_buffer_view = {};
    ComPtr<ID3D12RootSignature> _root_signature;
    ComPtr<ID3D12PipelineState> _pipeline_state;
    D3D12_CPU_DESCRIPTOR_HANDLE swap_chain_views[kSwapChainBufferCount] = {};
};

//----------------------------------------------------------------------------------------------------------------------

int main(int argc, char *argv[]) {
    try {
        auto example = std::make_unique<Triangle>();
        Window().MainLoop(example.get());
    }
    catch (const std::exception &exception) {
        std::cerr << exception.what() << std::endl;
    }
    return 0;
}

//----------------------------------------------------------------------------------------------------------------------