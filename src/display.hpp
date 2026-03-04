#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>

#include "color.hpp"
#include "keyboard.hpp"
#include "surface.hpp"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstring>
#include <d3d11.h>
#include <d3dcompiler.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#endif

namespace gfx {

template <typename PixelT>
class Display {
public:
    Display(std::size_t width, std::size_t height, const std::string& title = "Software Renderer")
        : width_(width), height_(height), title_(title) {
#ifdef _WIN32
        initWin32();
#else
        (void)title_;
#endif
    }

    ~Display() {
#ifdef _WIN32
        releaseD3D();
#endif
    }

    bool processEvents() {
#ifdef _WIN32
        MSG msg{};
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                open_ = false;
                return false;
            }

            if (msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN) {
                keyboard_.setKeyState(static_cast<std::uint32_t>(msg.wParam), true);
                if (msg.wParam == VK_ESCAPE && (msg.lParam & (1L << 30)) == 0) {
                    const int answer = MessageBoxW(
                        hwnd_,
                        L"Do you want to exit?",
                        L"Exit Game",
                        MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
                    if (answer == IDYES) {
                        PostQuitMessage(0);
                        open_ = false;
                        return false;
                    }
                }
            } else if (msg.message == WM_KEYUP || msg.message == WM_SYSKEYUP) {
                keyboard_.setKeyState(static_cast<std::uint32_t>(msg.wParam), false);
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        return open_;
#else
        return open_;
#endif
    }

    void present(const Surface<PixelT>& surface) {
#ifdef _WIN32
        if (!open_ || !context_ || !swapChain_) {
            return;
        }

        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (SUCCEEDED(context_->Map(texture_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
            for (std::size_t y = 0; y < height_; ++y) {
                std::uint8_t* dst = static_cast<std::uint8_t*>(mapped.pData) + y * mapped.RowPitch;
                for (std::size_t x = 0; x < width_; ++x) {
                    const auto px = surface.at(x, y).template cast<std::uint8_t>();
                    dst[x * 4 + 0] = px.b;
                    dst[x * 4 + 1] = px.g;
                    dst[x * 4 + 2] = px.r;
                    dst[x * 4 + 3] = px.a;
                }
            }
            context_->Unmap(texture_, 0);
        }

        const float clearColor[4] = {0.f, 0.f, 0.f, 1.f};
        context_->OMSetRenderTargets(1, &backBufferRTV_, nullptr);
        context_->ClearRenderTargetView(backBufferRTV_, clearColor);
        D3D11_VIEWPORT vp{};
        vp.Width = static_cast<float>(width_);
        vp.Height = static_cast<float>(height_);
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        context_->RSSetViewports(1, &vp);
        context_->VSSetShader(vertexShader_, nullptr, 0);
        context_->PSSetShader(pixelShader_, nullptr, 0);
        context_->PSSetShaderResources(0, 1, &textureSRV_);
        context_->PSSetSamplers(0, 1, &sampler_);
        context_->IASetInputLayout(nullptr);
        context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        context_->Draw(4, 0);
        swapChain_->Present(1, 0);
#else
        (void)surface;
#endif
    }

    bool isOpen() const { return open_; }

    const Keyboard& keyboard() const { return keyboard_; }

private:
#ifdef _WIN32
    static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        if (msg == WM_DESTROY) {
            PostQuitMessage(0);
            return 0;
        }
        return DefWindowProcW(hwnd, msg, wp, lp);
    }

    void initWin32() {
        HINSTANCE instance = GetModuleHandleW(nullptr);
        WNDCLASSW wc{};
        wc.lpfnWndProc = wndProc;
        wc.hInstance = instance;
        const wchar_t* className = L"SoftwareRendererWindow";
        const std::wstring windowTitle(title_.begin(), title_.end());

        wc.lpszClassName = className;
        RegisterClassW(&wc);

        hwnd_ = CreateWindowExW(
            0,
            className,
            windowTitle.c_str(),
            WS_POPUP,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            nullptr,
            nullptr,
            instance,
            nullptr);

        const int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        const int x = std::max(0, (screenWidth - static_cast<int>(width_)) / 2);
        SetWindowPos(
            hwnd_,
            HWND_TOP,
            x,
            0,
            static_cast<int>(width_),
            static_cast<int>(height_),
            SWP_SHOWWINDOW);

        ShowWindow(hwnd_, SW_SHOW);
        open_ = hwnd_ != nullptr;
        if (!open_) {
            return;
        }

        DXGI_SWAP_CHAIN_DESC sd{};
        sd.BufferDesc.Width = static_cast<UINT>(width_);
        sd.BufferDesc.Height = static_cast<UINT>(height_);
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.SampleDesc.Count = 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.BufferCount = 1;
        sd.OutputWindow = hwnd_;
        sd.Windowed = TRUE;

        const HRESULT devResult = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            0,
            nullptr,
            0,
            D3D11_SDK_VERSION,
            &sd,
            &swapChain_,
            &device_,
            nullptr,
            &context_);
        if (FAILED(devResult)) {
            open_ = false;
            return;
        }

        ID3D11Texture2D* backBuffer = nullptr;
        swapChain_->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
        device_->CreateRenderTargetView(backBuffer, nullptr, &backBufferRTV_);
        if (backBuffer) backBuffer->Release();

        D3D11_TEXTURE2D_DESC texDesc{};
        texDesc.Width = static_cast<UINT>(width_);
        texDesc.Height = static_cast<UINT>(height_);
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_DYNAMIC;
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        device_->CreateTexture2D(&texDesc, nullptr, &texture_);
        device_->CreateShaderResourceView(texture_, nullptr, &textureSRV_);



        const char* vsSrc =
            "struct VSOut { float4 pos : SV_Position; float2 uv : TEXCOORD0; };"
            "VSOut main(uint id : SV_VertexID) {"
            "  float2 pos[4] = { float2(-1, 1), float2(1, 1), float2(-1, -1), float2(1, -1) };"
            "  float2 uv[4] = { float2(0, 0), float2(1, 0), float2(0, 1), float2(1, 1) };"
            "  VSOut o; o.pos = float4(pos[id], 0, 1); o.uv = uv[id]; return o; }";
        const char* psSrc =
            "Texture2D tex0 : register(t0); SamplerState samp0 : register(s0);"
            "float4 main(float4 pos : SV_Position, float2 uv : TEXCOORD0) : SV_Target {"
            "  return tex0.Sample(samp0, uv); }";

        ID3DBlob* vsBlob = nullptr;
        ID3DBlob* psBlob = nullptr;
        ID3DBlob* errBlob = nullptr;
        if (FAILED(D3DCompile(vsSrc, strlen(vsSrc), nullptr, nullptr, nullptr, "main", "vs_4_0", 0, 0, &vsBlob, &errBlob))) {
            if (errBlob) errBlob->Release();
            open_ = false;
            return;
        }
        if (FAILED(D3DCompile(psSrc, strlen(psSrc), nullptr, nullptr, nullptr, "main", "ps_4_0", 0, 0, &psBlob, &errBlob))) {
            if (vsBlob) vsBlob->Release();
            if (errBlob) errBlob->Release();
            open_ = false;
            return;
        }
        device_->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader_);
        device_->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShader_);
        vsBlob->Release();
        psBlob->Release();

        D3D11_SAMPLER_DESC samplerDesc{};
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        device_->CreateSamplerState(&samplerDesc, &sampler_);
    }

    void releaseD3D() {
        if (vertexShader_) vertexShader_->Release();
        if (pixelShader_) pixelShader_->Release();
        if (sampler_) sampler_->Release();
        if (textureSRV_) textureSRV_->Release();
        if (texture_) texture_->Release();
        if (backBufferRTV_) backBufferRTV_->Release();
        if (context_) context_->Release();
        if (device_) device_->Release();
        if (swapChain_) swapChain_->Release();
    }

    HWND hwnd_{};
    ID3D11Device* device_{};
    ID3D11DeviceContext* context_{};
    IDXGISwapChain* swapChain_{};
    ID3D11RenderTargetView* backBufferRTV_{};
    ID3D11Texture2D* texture_{};
    ID3D11ShaderResourceView* textureSRV_{};
    ID3D11SamplerState* sampler_{};
    ID3D11VertexShader* vertexShader_{};
    ID3D11PixelShader* pixelShader_{};
#endif

    std::size_t width_{};
    std::size_t height_{};
    std::string title_;
    Keyboard keyboard_{};
    bool open_{true};
};

} // namespace gfx
