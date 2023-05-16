#include <windows.h>
#include <d3d11.h>
#include <DirectXMath.h>
using namespace DirectX;
#include <d3dcompiler.h>

#include "log.h"
#include "common.cpp"
#include "platform_win32.cpp"
#include "morpheus.cpp"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ASSERT(X) ASSERT(X)
#include "ext/stb_image.h"

#define WIDTH 1366
#define HEIGHT 768
bool window_should_close = false;

IDXGISwapChain *swapchain;
ID3D11Device *d3d_device;
ID3D11DeviceContext *d3d_context;
ID3D11RenderTargetView *render_target;

struct Vertex {
    Vertex() {}
    Vertex(float x, float y, float z, float u, float v)
            : pos(x, y, z), tex_coord(u, v) {}

    XMFLOAT3 pos;
    XMFLOAT2 tex_coord;
};

struct CB_Per_Obj {
    XMMATRIX WVP;
};

LARGE_INTEGER performance_frequency;
inline float win32_get_seconds_elapsed(LARGE_INTEGER start, LARGE_INTEGER end) {
    float Result = (float)(end.QuadPart - start.QuadPart) / (float)performance_frequency.QuadPart;
    return Result;
}
inline LARGE_INTEGER win32_get_wall_clock() {
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return result;
}


LRESULT CALLBACK main_window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    LRESULT result = 0;

    switch (message) {
    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        result = DefWindowProcA(hwnd, message, wparam, lparam);
    }
    return result;
}



int main(int argc, char **argv) {
    timeBeginPeriod(1);
    QueryPerformanceFrequency(&performance_frequency);
    const int target_fps = 30;
    const DWORD target_ms_per_frame = (DWORD)(1000.0f * (1.0f / target_fps));
    
#define HWND_CLASS_NAME "win32_class_mania"
    HRESULT hr = S_OK;
    HINSTANCE hInstance = GetModuleHandle(NULL);
    WNDCLASSA window_class{};
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.hInstance = hInstance;
    window_class.lpfnWndProc = main_window_proc;
    window_class.lpszClassName = HWND_CLASS_NAME;
    window_class.hCursor = LoadCursorA(NULL, IDC_ARROW);
    if (!RegisterClassA(&window_class)) {
        MessageBoxA(NULL, "Error registering window classs!", "Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    HWND hwnd = NULL;
    {
        RECT rc{};
        rc.right = WIDTH;
        rc.bottom = HEIGHT;
        if (AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE)) {
            hwnd = CreateWindowA(HWND_CLASS_NAME, "Mania", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance, NULL);
        }
    }
    
    if (!hwnd) {
        DWORD err = GetLastError();
        MessageBoxA(NULL, "Error creating window!", "Error", MB_OK | MB_ICONERROR);
        return -1;
    }
    
    DXGI_SWAP_CHAIN_DESC swapchain_desc{};
    {
        DXGI_MODE_DESC buffer_desc{};
        buffer_desc.Width = WIDTH;
        buffer_desc.Height = HEIGHT;
        buffer_desc.RefreshRate.Numerator = 60;
        buffer_desc.RefreshRate.Denominator = 1;
        buffer_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        buffer_desc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        buffer_desc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

        swapchain_desc.BufferDesc = buffer_desc;
        swapchain_desc.SampleDesc.Count = 1;
        swapchain_desc.SampleDesc.Quality = 0;
        swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapchain_desc.BufferCount = 1;
        swapchain_desc.OutputWindow = hwnd;
        swapchain_desc.Windowed = TRUE;
        swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        
    }

    UINT flags = 0;
#ifdef DEVELOPER
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    
    hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flags, NULL, NULL, D3D11_SDK_VERSION, &swapchain_desc, &swapchain, &d3d_device, NULL, &d3d_context);


    ID3D11Texture2D *backbuffer;
    hr = swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&backbuffer);
    
    hr = d3d_device->CreateRenderTargetView(backbuffer, NULL, &render_target);
    backbuffer->Release();

    D3D11_VIEWPORT viewport{};
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = WIDTH;
    viewport.Height = HEIGHT;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    d3d_context->RSSetViewports(1, &viewport);

    ID3D11DepthStencilView *depth_stencil_view = nullptr;;
    ID3D11Texture2D *depth_stencil_buffer = nullptr;
    
    D3D11_TEXTURE2D_DESC depth_stencil_desc{};
    depth_stencil_desc.Width = WIDTH;
    depth_stencil_desc.Height = HEIGHT;
    depth_stencil_desc.MipLevels = 1;
    depth_stencil_desc.ArraySize = 1;
    depth_stencil_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depth_stencil_desc.SampleDesc.Count = 1;
    depth_stencil_desc.SampleDesc.Quality = 0;
    depth_stencil_desc.Usage = D3D11_USAGE_DEFAULT;
    depth_stencil_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depth_stencil_desc.CPUAccessFlags = 0;
    depth_stencil_desc.MiscFlags = 0;

    hr = d3d_device->CreateTexture2D(&depth_stencil_desc, NULL, &depth_stencil_buffer);
    hr = d3d_device->CreateDepthStencilView(depth_stencil_buffer, NULL, &depth_stencil_view);

    d3d_context->OMSetRenderTargets(1, &render_target, depth_stencil_view);


    flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef DEVELOPER
    flags |= D3DCOMPILE_DEBUG;
#endif
    
    ID3DBlob *model_vblob = nullptr;
    ID3DBlob *model_pblob = nullptr;
    ID3DBlob *error_blob = nullptr;
    hr = D3DCompileFromFile(L"model.hlsl", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "vs_main", "vs_5_0", flags, 0, &model_vblob, &error_blob);
    if (FAILED(hr)) {
        ALERT("Error compiling shader %s", "basic.hlsl");
        if (error_blob) {
            ALERT("%s", (char *)error_blob->GetBufferPointer());
            error_blob->Release();
        }
        if (model_vblob) {
            model_vblob->Release();
        }
        ASSERT(false);
    }
    hr = D3DCompileFromFile(L"model.hlsl", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "ps_main", "ps_5_0", flags, 0, &model_pblob, &error_blob);
    if (FAILED(hr)) {
        ALERT("Error compiling shader %s", "model.hlsl");
        if (error_blob) {
            ALERT("%s", (char *)error_blob->GetBufferPointer());
            error_blob->Release();
        }
        if (model_pblob) {
            model_pblob->Release();
        }
        ASSERT(false);
    }
    
    ID3D11VertexShader *model_vshader = nullptr;
    ID3D11PixelShader *model_pshader = nullptr;
    hr = d3d_device->CreateVertexShader(model_vblob->GetBufferPointer(), model_vblob->GetBufferSize(), NULL, &model_vshader);
    hr = d3d_device->CreatePixelShader(model_pblob->GetBufferPointer(), model_pblob->GetBufferSize(), NULL, &model_pshader);

    
    int width, height, n_channel;
    ID3D11Texture2D *tex_2d = nullptr;
    uint8 *data = stbi_load("cage.png", &width, &height, &n_channel, 4);
    ASSERT(data);
    {
        D3D11_TEXTURE2D_DESC tex_desc{};
        tex_desc.Width = width;
        tex_desc.Height = height;
        tex_desc.MipLevels = 1;
        tex_desc.ArraySize = 1;
        tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        tex_desc.SampleDesc.Count = 1;
        tex_desc.SampleDesc.Quality = 0;
        tex_desc.Usage = D3D11_USAGE_IMMUTABLE;
        tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        tex_desc.CPUAccessFlags = 0;
        tex_desc.MiscFlags = 0;
        D3D11_SUBRESOURCE_DATA sr_data{};
        sr_data.pSysMem = data;
        sr_data.SysMemPitch = width * sizeof(uint32);
        sr_data.SysMemSlicePitch = 0;
        hr = d3d_device->CreateTexture2D(&tex_desc, &sr_data, &tex_2d);
        stbi_image_free(data);
        ASSERT(SUCCEEDED(hr));
    }
    ID3D11ShaderResourceView *shader_resource = nullptr;
    hr = d3d_device->CreateShaderResourceView(tex_2d, NULL, &shader_resource);
        
    ID3D11SamplerState *sampler_state = nullptr;
    {
        D3D11_SAMPLER_DESC sampler_desc{};
        sampler_desc.Filter = D3D11_FILTER_ANISOTROPIC;
        sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        sampler_desc.MinLOD = 0;
        sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
        hr = d3d_device->CreateSamplerState(&sampler_desc, &sampler_state);
    }
    


    ID3D11Buffer *object_cb = nullptr;
    ID3D11Buffer *frame_cb = nullptr;
    
    CB_Per_Obj object{};
    {
        D3D11_BUFFER_DESC desc{};
        desc.ByteWidth = sizeof(CB_Per_Obj);
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        D3D11_SUBRESOURCE_DATA sr{};
        sr.pSysMem = &object;
        hr = d3d_device->CreateBuffer(&desc, &sr, &object_cb);
        ASSERT(SUCCEEDED(hr));
    }

    d3d_context->VSSetConstantBuffers(0, 1, &object_cb);

    
    ID3D11RasterizerState *wire_frame = nullptr;
    ID3D11RasterizerState *cw_cull_mode = nullptr;
    ID3D11RasterizerState *ccw_cull_mode = nullptr;
    {
        D3D11_RASTERIZER_DESC rdesc{};
        rdesc.FillMode = D3D11_FILL_WIREFRAME;
        rdesc.CullMode = D3D11_CULL_NONE;
        hr = d3d_device->CreateRasterizerState(&rdesc, &wire_frame);

        rdesc.FillMode = D3D11_FILL_SOLID;
        rdesc.CullMode = D3D11_CULL_BACK;
        
        rdesc.FrontCounterClockwise = true;
        hr = d3d_device->CreateRasterizerState(&rdesc, &cw_cull_mode);
        rdesc.FrontCounterClockwise = false;
        hr = d3d_device->CreateRasterizerState(&rdesc, &ccw_cull_mode);
    }

    OBJ_Model model = load_obj_model("untitled.obj");

    ID3D11Buffer *model_vbuffer = nullptr;
    {
        D3D11_BUFFER_DESC desc{};
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.ByteWidth = (uint32)(model.vertices.size() * sizeof(HMM_Vec4));
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        D3D11_SUBRESOURCE_DATA sr_data{};
        sr_data.pSysMem = &model.vertices[0];
        hr = d3d_device->CreateBuffer(&desc, &sr_data, &model_vbuffer);
    }
    ID3D11Buffer *model_ibuffer = nullptr;
    {
        D3D11_BUFFER_DESC desc{};
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.ByteWidth = (uint32)(model.indices.size() * sizeof(uint32));
        desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        D3D11_SUBRESOURCE_DATA sr_data{};
        sr_data.pSysMem = model.indices.data();
        hr = d3d_device->CreateBuffer(&desc, &sr_data, &model_ibuffer);
    }    

    D3D11_INPUT_ELEMENT_DESC layout_desc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    UINT model_stride = sizeof(HMM_Vec4);
    UINT model_offset = 0;
    d3d_context->IASetVertexBuffers(0, 1, &model_vbuffer, &model_stride, &model_offset);
    d3d_context->IASetIndexBuffer(model_ibuffer, DXGI_FORMAT_R32_UINT, 0);

    ID3D11InputLayout *model_layout = nullptr;
    hr = d3d_device->CreateInputLayout(layout_desc, ARRAYSIZE(layout_desc), model_vblob->GetBufferPointer(), model_vblob->GetBufferSize(), &model_layout);

    
    LARGE_INTEGER start_counter = win32_get_wall_clock();
    LARGE_INTEGER last_counter = start_counter;
    while (!window_should_close) {
        MSG message{};
        while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) {
                window_should_close = true;
                break;
            }
            
            TranslateMessage(&message);    
            DispatchMessageA(&message);
        }

        float global_time = win32_get_seconds_elapsed(start_counter, win32_get_wall_clock());
        
        FLOAT bg_color[4] = {0.14f, 0.22f, 0.34f, 1.0f};
        d3d_context->ClearRenderTargetView(render_target, bg_color);

        d3d_context->ClearDepthStencilView(depth_stencil_view, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

        XMMATRIX world = XMMatrixIdentity();
        XMVECTOR axis = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        
        XMMATRIX rotation, translation, scale;
        rotation = XMMatrixRotationNormal(axis, global_time);
        translation = XMMatrixTranslation(3.0f, 0.0f, 0.0f);
        rotation = XMMatrixRotationAxis(axis, -global_time);
        
        XMVECTOR camera_pos = XMVectorSet(1.0f, 3.0f, -5.0f, 0.0f );
        XMVECTOR camera_target = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
        XMVECTOR camera_up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        XMMATRIX view = XMMatrixLookAtLH(camera_pos, camera_target, camera_up);
        XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PI * 0.4, (float)WIDTH/(float)HEIGHT, 1.0f, 1000.0f);
        

        d3d_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        d3d_context->IASetInputLayout(model_layout);
        d3d_context->IASetVertexBuffers(0, 1, &model_vbuffer, &model_stride, &model_offset);
        d3d_context->IASetIndexBuffer(model_ibuffer, DXGI_FORMAT_R32_UINT, 0);
        d3d_context->OMSetBlendState(NULL, NULL, 0xffffffff);

        d3d_context->VSSetShader(model_vshader, 0, 0);
        
        object.WVP = world * view * proj;
        D3D11_MAPPED_SUBRESOURCE mapped_resource{};
        d3d_context->Map(object_cb, 0, D3D11_MAP_WRITE_DISCARD, NULL, &mapped_resource);
        CopyMemory(mapped_resource.pData, &object, sizeof(CB_Per_Obj));
        d3d_context->Unmap(object_cb, 0);
        d3d_context->VSSetConstantBuffers(0, 1, &object_cb);

        // d3d_context->PSSetShaderResources(0, 1, &shader_resource);
        // d3d_context->PSSetSamplers(0, 1, &sampler_state);
        d3d_context->PSSetShader(model_pshader, 0, 0);
        d3d_context->RSSetState(nullptr);

        d3d_context->DrawIndexed((uint32)model.indices.size(), 0, 0);
    
        swapchain->Present(0, 0);
        
        float work_seconds_elapsed = win32_get_seconds_elapsed(last_counter, win32_get_wall_clock());
        DWORD work_ms = (DWORD)(1000.0f * work_seconds_elapsed);
        if (work_ms < target_ms_per_frame) {
            DWORD sleep_ms = target_ms_per_frame - work_ms;
            Sleep(sleep_ms);
        }

        LARGE_INTEGER end_counter = win32_get_wall_clock();
#if 0
        float seconds_elapsed = 1000.0f * win32_get_seconds_elapsed(last_counter, end_counter);
        LOG("seconds: %f", seconds_elapsed);
#endif
        
        last_counter = end_counter;
    }
    
    return 0;
}
