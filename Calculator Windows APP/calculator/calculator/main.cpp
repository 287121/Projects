#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>
#include "Calculator.h"

using namespace std;
// Open as windows application (no cmd)
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:WinMainCRTStartup")

// DirectX globals
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

// Forward declarations
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0, 0,
        GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr,
        L"Calculator App", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = CreateWindowEx(WS_EX_LAYERED | WS_EX_TOPMOST,
        _T("Calculator App"), NULL,
        WS_POPUP, 0, 0, screenWidth, screenHeight,
        NULL, NULL, wc.hInstance, NULL);
    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, ULW_COLORKEY);

    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Application state
    static Calculator calc;
    static char convInputBuf[128] = "";
    static int convFromIndex = 0;
    static int convToIndex = 0;
    static std::string convResult;
    const char* bases[] = { "Binary", "Octal", "Decimal", "Hexadecimal" };
    const int baseVals[] = { 2, 8, 10, 16 };

    ImVec2 buttonSize(80, 80);
    bool showCalculator = true;
    bool done = false;
    while (!done) {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done) break;

        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        if (g_ResizeWidth != 0 && g_ResizeHeight != 0) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowContentSize(ImVec2(4 * buttonSize.x + 40, 0));
        ImGui::Begin("Calculator", &showCalculator, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Text("Expr: %s", calc.getExpression().c_str());
        ImGui::Text("%s", calc.getResult().c_str());
        ImGui::Spacing();

        ImGui::Columns(4, "extraColumns", false);
        if (ImGui::Button("CE", buttonSize)) calc.clear();
        ImGui::NextColumn();
        if (ImGui::Button("<", buttonSize)) calc.backspace();
        ImGui::NextColumn();
        if (ImGui::Button("MS", buttonSize)) calc.memoryStore();
        ImGui::NextColumn();
        if (ImGui::Button("MR", buttonSize)) calc.memoryRecall();
        ImGui::Columns(1);
        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

        // Number and operator buttons
        ImGui::Columns(4, "calcColumns", false);
        if (ImGui::Button("7", buttonSize)) calc.append("7"); ImGui::NextColumn();
        if (ImGui::Button("8", buttonSize)) calc.append("8"); ImGui::NextColumn();
        if (ImGui::Button("9", buttonSize)) calc.append("9"); ImGui::NextColumn();
        if (ImGui::Button("*", buttonSize)) calc.selectOperator('*'); ImGui::NextColumn();

        if (ImGui::Button("4", buttonSize)) calc.append("4"); ImGui::NextColumn();
        if (ImGui::Button("5", buttonSize)) calc.append("5"); ImGui::NextColumn();
        if (ImGui::Button("6", buttonSize)) calc.append("6"); ImGui::NextColumn();
        if (ImGui::Button("/", buttonSize)) calc.selectOperator('/'); ImGui::NextColumn();

        if (ImGui::Button("1", buttonSize)) calc.append("1"); ImGui::NextColumn();
        if (ImGui::Button("2", buttonSize)) calc.append("2"); ImGui::NextColumn();
        if (ImGui::Button("3", buttonSize)) calc.append("3"); ImGui::NextColumn();
        if (ImGui::Button("+", buttonSize)) calc.selectOperator('+'); ImGui::NextColumn();

        if (ImGui::Button("0", buttonSize)) calc.append("0"); ImGui::NextColumn();
        if (ImGui::Button(".", buttonSize)) calc.append("."); ImGui::NextColumn();
        if (ImGui::Button("%", buttonSize)) calc.selectOperator('%'); ImGui::NextColumn();
        if (ImGui::Button("-", buttonSize)) calc.selectOperator('-'); ImGui::NextColumn();
        ImGui::Columns(1);

        // Function buttons
        ImGui::Columns(4, "funcColumns", false);
        if (ImGui::Button("NEG", buttonSize)) calc.toggleSign();
        ImGui::NextColumn();
        static bool showConvWindow = false;
        if (ImGui::Button("CONV", buttonSize)) showConvWindow = true;
        if (showConvWindow) {
            ImGui::Begin("Converter", &showConvWindow, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::Text("Number System Conversion"); ImGui::Spacing();
            if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
            ImGui::InputText("Input", convInputBuf, IM_ARRAYSIZE(convInputBuf));
            ImGui::Combo("From", &convFromIndex, bases, IM_ARRAYSIZE(bases));
            ImGui::Combo("To", &convToIndex, bases, IM_ARRAYSIZE(bases));
            if (ImGui::Button("Convert"))
                convResult = Calculator::convertNumber(convInputBuf, baseVals[convFromIndex], baseVals[convToIndex]);
            ImGui::Spacing();
            ImGui::Text("Result: %s", convResult.c_str());
            ImGui::Spacing();
            if (ImGui::Button("Close")) showConvWindow = false;
            ImGui::End();
        }
        ImGui::NextColumn();

        static bool showAboutWindow = false;
        if (ImGui::Button("ABOUT", buttonSize)) showAboutWindow = true;
        if (showAboutWindow) {
            ImGui::Begin("About Me", &showAboutWindow, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
            ImGui::Text("Calculator Application");
            ImGui::Text("Version: 1.0"); ImGui::Spacing();
            ImGui::Text("Created by: Milosz Pacek - 287121");
            ImGui::Text("This is a simple calculator project built using ImGui."); ImGui::Spacing();
            if (ImGui::Button("Close")) showAboutWindow = false;
            ImGui::End();
        }
        ImGui::NextColumn();

        static bool showErrorWindow = false;
        static std::string errorMessage;
        if (ImGui::Button("ENTER", buttonSize)) {
            bool ok = calc.calculate();
            if (!ok) {
                showErrorWindow = true;
                errorMessage = calc.getResult();
            }
        }
        if (showErrorWindow) {
            ImGui::Begin("Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
            ImGui::Text("%s", errorMessage.c_str());
            ImGui::Spacing();
            if (ImGui::Button("Close")) showErrorWindow = false;
            ImGui::End();
        }
        ImGui::NextColumn();
        ImGui::Columns(1);

        ImGui::End();
        if (!showCalculator) { PostQuitMessage(0); break; }

        ImGui::Render();
        const float clear_color_with_alpha[4] = { 0,0,0,0 };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        HRESULT hr = g_pSwapChain->Present(0, 0);
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// DirectX helper functions

bool CreateDeviceD3D(HWND hWnd) {
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
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
        featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain,
        &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags,
            featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain,
            &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;
    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED) return 0;
        g_ResizeWidth = LOWORD(lParam);
        g_ResizeHeight = HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
