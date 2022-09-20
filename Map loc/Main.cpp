#include "d3d11.h"
#pragma comment(lib, "d3d11.lib")
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_internal.h"

#define STB_IMAGE_IMPLEMENTATION
#include "ImGui/stb_image.h"
#include <iostream>
#include <iomanip>
#include <cinttypes>
#include <cstddef>
#include <cstdint>
#include <string>

#include <tchar.h>
#define DIRECTINPUT_VERSION 0x0800
#define VK_NOTHING	0x00				/*NULL*/
#define VK_KEY_0	0x30                //('0')	0
#define VK_KEY_1	0x31                //('1')	1
#define VK_KEY_2	0x32                //('2')	2
#define VK_KEY_3	0x33                //('3')	3
#define VK_KEY_4	0x34                //('4')	4
#define VK_KEY_5	0x35                //('5')	5
#define VK_KEY_6	0x36                //('6')	6
#define VK_KEY_7	0x37                //('7')	7
#define VK_KEY_8	0x38                //('8')	8
#define VK_KEY_9	0x39                //('9')	9
#define VK_KEY_A	0x41                //('A')	A
#define VK_KEY_B	0x42                //('B')	B
#define VK_KEY_C	0x43                //('C')	C
#define VK_KEY_D	0x44                //('D')	D
#define VK_KEY_E	0x45                //('E')	E
#define VK_KEY_F	0x46                //('F')	F
#define VK_KEY_G	0x47                //('G')	G
#define VK_KEY_H	0x48                //('H')	H
#define VK_KEY_I	0x49                //('I')	I
#define VK_KEY_J	0x4A                //('J')	J
#define VK_KEY_K	0x4B                //('K')	K
#define VK_KEY_L	0x4C                //('L')	L
#define VK_KEY_M	0x4D                //('M')	M
#define VK_KEY_N	0x4E                //('N')	N
#define VK_KEY_O	0x4F                //('O')	O
#define VK_KEY_P	0x50                //('P')	P
#define VK_KEY_Q	0x51                //('Q')	Q
#define VK_KEY_R	0x52                //('R')	R
#define VK_KEY_S	0x53                //('S')	S
#define VK_KEY_T	0x54                //('T')	T
#define VK_KEY_U	0x55                //('U')	U
#define VK_KEY_V	0x56                //('V')	V
#define VK_KEY_W	0x57                //('W')	W
#define VK_KEY_X	0x58                //('X')	X
#define VK_KEY_Y	0x59                //('Y')	Y
#define VK_KEY_Z	0x5A                //('Z')	Z
#define VK_OEM_3	0xC0                //('~  `')	` ~
#define VK_OEM_4	0xDB                //('~  `')	` ~
#pragma warning(disable : 4996).
// Data
static ID3D11Device* g_pd3dDevice = NULL;
static ID3D11DeviceContext* g_pd3dDeviceContext = NULL;
static IDXGISwapChain* g_pSwapChain = NULL;
static ID3D11RenderTargetView* g_mainRenderTargetView = NULL;
ImFont* m_font, * m_font_big, * m_font_title;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
extern void horizonTick();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool LoadTextureFromFile(const char* filename, ID3D11ShaderResourceView** out_srv)
{
    // Load from disk into a raw RGBA buffer
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
    if (image_data == NULL)
        return false;

    // Create texture
    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Width = image_width;
    desc.Height = image_height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    ID3D11Texture2D* pTexture = NULL;
    D3D11_SUBRESOURCE_DATA subResource;
    subResource.pSysMem = image_data;
    subResource.SysMemPitch = desc.Width * 4;
    subResource.SysMemSlicePitch = 0;
    g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);

    // Create texture view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
    pTexture->Release();

    stbi_image_free(image_data);

    return true;
}
bool LoadTexturesOnce = true;

ImVec2 add(ImVec2* vectorA, ImVec2* vectorB) {
    ImVec2 result;
    result.x = vectorA->x;
    result.y = vectorA->y;
    result.x += vectorB->x;
    result.y += vectorB->y;

    return result;
}

void LoadTexture(const char* FilePath, ID3D11ShaderResourceView** out_srv)
{
    LoadTextureFromFile(FilePath, out_srv);
}
void RectFilled(ImVec4 Colour, ImVec2 Pos, ImVec2 Size, bool Filled, float thik, float round)
{
    ImVec2 size = ImGui::CalcItemSize(ImVec2(Size.x, Size.y), 0.0f, 0.0f);
    const ImRect bb(ImVec2(Pos.x, Pos.y), add(&Pos, &size));


    if (!Filled)
        ImGui::GetCurrentWindow()->DrawList->AddRect(bb.Max, bb.Min, ImGui::GetColorU32(Colour), round, 15, thik);
    else
        ImGui::GetCurrentWindow()->DrawList->AddRectFilledMultiColor(bb.Max, bb.Min, ImGui::GetColorU32(Colour),
            ImGui::GetColorU32(Colour), ImGui::GetColorU32(Colour), ImGui::GetColorU32(Colour));
}



ID3D11ShaderResourceView
* Map = NULL,
* Pin = NULL;

void LoadTextureFiles()
{
    if (LoadTexturesOnce)
    {
        LoadTexture("C:\\Users\\gilla\\Desktop\\GTA5Map.jpg", &Map);

    }LoadTexturesOnce = false;

}

ImVec2 PinCoordsStart = { -1070.906250f, -2972.122803f };//Game Coords
ImVec2 gameMap = { 6000, 8000 };
ImVec2 mapSize = { 400 , 600 };
ImVec2 mapCenter = { 195, 430};
ImVec2 pinSize = { 5 , 5 };
ImVec2 pinLoc = { (mapCenter.x) + (PinCoordsStart.x * (mapCenter.x) / gameMap.x), (mapCenter.y) - (PinCoordsStart.y * (mapCenter.y) / gameMap.y) };
ImVec2 pinLocOld;
/*
* Find the center
* after resizing
* X should be the 2 / PX IMAGE
* Y might a bit hard
*/

//LSA = -1070.906250f, -2972.122803f
//LSBP =  -1374.881f, -1398.835f
//MAZEBANK = -75.015f, -818.215f
// BC = -109.299f, 6464.035f
//Sandy = 1938.357910f, 3717.808350f
//EXTOWER = -786.8663, 315.7642

/*
 pixel_x = (mapSize.x / 2) + (-1070.906250f * (mapSize.x / 2) / 4000.0)
 pixel_y = (mapSize.y / 2) - ( -2972.122803f * ( mapSize.y / 2) / 4000.0)
*/
bool oldXy = true;
void renderMenu()
{
    if (oldXy)
    {
        pinLocOld.x = pinLoc.x;
        pinLocOld.y = pinLoc.y;
    }oldXy = false;
    ImGui::SetNextWindowSize({ mapSize.x + 50, mapSize.y + 50 });
    if (ImGui::Begin("Window", 0, ImGuiWindowFlags_NoBringToFrontOnFocus))
    {
        ImGui::Image((void*)Map, mapSize);
        RectFilled(ImColor(245, 2, 2, 255), { pinLoc.x + ImGui::GetWindowPos().x, pinLoc.y + ImGui::GetWindowPos().y }, pinSize, true, NULL, NULL);
    }
    ImGui::End();
    if (ImGui::Begin("Debug", 0))
    {
        ImGui::BulletText(("Pin X: " + std::to_string(pinLoc.x)).c_str());
        ImGui::BulletText(("Pin Y: " + std::to_string(pinLoc.y)).c_str());
        ImGui::BulletText(("Old Pin X: " + std::to_string(pinLocOld.x)).c_str());
        ImGui::BulletText(("Old Pin Y: " + std::to_string(pinLocOld.y)).c_str());
        ImGui::BulletText(("From X Edge: " + std::to_string((pinLoc.x * (mapCenter.x) / gameMap.x))).c_str());
        ImGui::BulletText(("From Y Edge: " + std::to_string((pinLoc.y * (mapCenter.y) / gameMap.y))).c_str());
        ImGui::BulletText(("From X Center: " + std::to_string(pinLoc.x - mapCenter.x)).c_str());
        ImGui::BulletText(("From Y Center: " + std::to_string(pinLoc.y - mapCenter.y)).c_str());
        ImGui::Separator();
        ImGui::BulletText(("Map center X: " + std::to_string(mapCenter.x)).c_str());
        ImGui::BulletText(("Map center Y: " + std::to_string(mapCenter.y)).c_str());
        ImGui::BulletText(("Game Map X: " + std::to_string(gameMap.x)).c_str());
        ImGui::BulletText(("Game Map Y: " + std::to_string(gameMap.y)).c_str());
        ImGui::BulletText(("Game Coords Start X: " + std::to_string(PinCoordsStart.x)).c_str());
        ImGui::BulletText(("Game Coords Start Y: " + std::to_string(PinCoordsStart.y)).c_str());
        ImGui::Separator();
        ImGui::BulletText(("Map center to game map X: " + std::to_string(mapCenter.x / gameMap.x)).c_str());
        ImGui::BulletText(("Map center to game map Y: " + std::to_string(mapCenter.y / gameMap.y)).c_str());


    }
    ImGui::End();

    if (GetAsyncKeyState(VK_KEY_W))
        pinLoc.y -= .1f;
    if (GetAsyncKeyState(VK_KEY_S))
        pinLoc.y += .1f;
    if (GetAsyncKeyState(VK_KEY_A))
        pinLoc.x -= .1f;
    if (GetAsyncKeyState(VK_KEY_D))
        pinLoc.x += .1f;

}

int main()
{
    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("ImGui Example"), NULL };
    ::RegisterClassEx(&wc);
    HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("DX11 Testing"), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);


    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        if (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        LoadTextureFiles();
        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);
        LoadTextureFiles();
        renderMenu();

        // Rendering
        ImGui::Render();
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, (float*)&clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0); // Present with vsync
        //g_pSwapChain->Present(0, 0); // Present without vsync

    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}
// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}