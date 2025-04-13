// Dear ImGui: standalone example application for DirectX 11

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>

using namespace std;

// Data
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


//==================================================================================
// Helper functions - Calculator
//==================================================================================
void AppendToOperand(string& operand, const char* str)
{
    if (str[0] == '.')
    {
        if (operand.find('.') != string::npos)
            return;
        if (operand.empty())
            operand = "0"; // Prepend 0 if starting with a dot
    }
    operand.append(str);
}

void BackspaceOperand(string& operand)
{
    if (!operand.empty())
        operand.pop_back();
}

float StringToFloat(const string& s)
{
    try { return stof(s); }
    catch (...) { return 0.0f; }
}



int main(int, char**)

    //==================================================================================
    //Gui setup
    //==================================================================================
{
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    // Render window in frameless GUI
    HWND hwnd = CreateWindowEx(WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_NOACTIVATE, _T("ImGui Example"), NULL, WS_POPUP, 0, 0, screenWidth, screenHeight, NULL, NULL, wc.hInstance, NULL);
    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, ULW_COLORKEY);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Application state variables:
    static string leftOperand;
    static string rightOperand;
    static char currentOperator = '\0'; // '+', '-', '*', or '/'
    static float memoryValue = 0.0f;      // Memory storage
    static string resultStr = "Result: N/A";

    // Calculator button size
    ImVec2 buttonSize(80, 80);

    // Main loop
    bool showCalculator = true;
    bool done = false;
    while (!done)
    {

        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Handle window being minimized or screen locked
        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        // Handle window resizing if requested
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        //================================================================================== \*

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Create the Calculator window
        {
            ImGui::Begin("Calculator", &showCalculator, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

            // Display the current expression
            stringstream expr;
            expr << (leftOperand.empty() ? "0" : leftOperand);
            if (currentOperator != '\0')
                expr << " " << currentOperator << " ";
            expr << rightOperand;
            ImGui::Text("Expr: %s", expr.str().c_str());
            ImGui::Text("%s", resultStr.c_str());
            ImGui::Spacing();

            // New row: CE, Backspace ("<"), MS, MR
            ImGui::Columns(4, "extraColumns", false);
            if (ImGui::Button("CE", buttonSize))
            {
                leftOperand.clear();
                rightOperand.clear();
                currentOperator = '\0';
                resultStr = "Result: N/A";
            }
            ImGui::NextColumn();
            if (ImGui::Button("<", buttonSize))
            {
                // If right operand is active, backspace there.
                if (currentOperator != '\0' && !rightOperand.empty())
                {
                    BackspaceOperand(rightOperand);
                }
                // Otherwise, if an operator has been chosen but right is empty, remove the operator.
                else if (currentOperator != '\0')
                {
                    currentOperator = '\0';
                }
                // Otherwise backspace left operand.
                else if (!leftOperand.empty())
                {
                    BackspaceOperand(leftOperand);
                }
            }
            ImGui::NextColumn();
            if (ImGui::Button("MS", buttonSize))
            {
                // Store the result from leftOperand as memory if available.
                if (!leftOperand.empty())
                    memoryValue = StringToFloat(leftOperand);
            }
            ImGui::NextColumn();
            if (ImGui::Button("MR", buttonSize))
            {
                // Load the memory value as current operand.
                string memStr = to_string(memoryValue);
                // Remove trailing zeros for aesthetics
                if (memStr.find('.') != string::npos)
                {
                    while (memStr.back() == '0')
                        memStr.pop_back();
                    if (memStr.back() == '.')
                        memStr.pop_back();
                }
                if (currentOperator == '\0')
                    leftOperand = memStr;
                else
                    rightOperand = memStr;
            }
            ImGui::Columns(1);
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Buttons grid in a 4-column layout (calculator keys)
            ImGui::Columns(4, "calcColumns", false);

            // Row 1: "7", "8", "9", "*"
            if (ImGui::Button("7", buttonSize))
            {
                if (currentOperator == '\0')
                    AppendToOperand(leftOperand, "7");
                else
                    AppendToOperand(rightOperand, "7");
            }
            ImGui::NextColumn();
            if (ImGui::Button("8", buttonSize))
            {
                if (currentOperator == '\0')
                    AppendToOperand(leftOperand, "8");
                else
                    AppendToOperand(rightOperand, "8");
            }
            ImGui::NextColumn();
            if (ImGui::Button("9", buttonSize))
            {
                if (currentOperator == '\0')
                    AppendToOperand(leftOperand, "9");
                else
                    AppendToOperand(rightOperand, "9");
            }
            ImGui::NextColumn();
            if (ImGui::Button("*", buttonSize))
            {
                if (!leftOperand.empty())
                    currentOperator = '*';
            }
            ImGui::NextColumn();

            // Row 2: "4", "5", "6", "/"
            if (ImGui::Button("4", buttonSize))
            {
                if (currentOperator == '\0')
                    AppendToOperand(leftOperand, "4");
                else
                    AppendToOperand(rightOperand, "4");
            }
            ImGui::NextColumn();
            if (ImGui::Button("5", buttonSize))
            {
                if (currentOperator == '\0')
                    AppendToOperand(leftOperand, "5");
                else
                    AppendToOperand(rightOperand, "5");
            }
            ImGui::NextColumn();
            if (ImGui::Button("6", buttonSize))
            {
                if (currentOperator == '\0')
                    AppendToOperand(leftOperand, "6");
                else
                    AppendToOperand(rightOperand, "6");
            }
            ImGui::NextColumn();
            if (ImGui::Button("/", buttonSize))
            {
                if (!leftOperand.empty())
                    currentOperator = '/';
            }
            ImGui::NextColumn();

            // Row 3: "1", "2", "3", "+"
            if (ImGui::Button("1", buttonSize))
            {
                if (currentOperator == '\0')
                    AppendToOperand(leftOperand, "1");
                else
                    AppendToOperand(rightOperand, "1");
            }
            ImGui::NextColumn();
            if (ImGui::Button("2", buttonSize))
            {
                if (currentOperator == '\0')
                    AppendToOperand(leftOperand, "2");
                else
                    AppendToOperand(rightOperand, "2");
            }
            ImGui::NextColumn();
            if (ImGui::Button("3", buttonSize))
            {
                if (currentOperator == '\0')
                    AppendToOperand(leftOperand, "3");
                else
                    AppendToOperand(rightOperand, "3");
            }
            ImGui::NextColumn();
            if (ImGui::Button("+", buttonSize))
            {
                if (!leftOperand.empty())
                    currentOperator = '+';
            }
            ImGui::NextColumn();

            // Row 4: "0", "ENTER", ".", "-"
            if (ImGui::Button("0", buttonSize))
            {
                if (currentOperator == '\0')
                    AppendToOperand(leftOperand, "0");
                else
                    AppendToOperand(rightOperand, "0");
            }
            ImGui::NextColumn();
            if (ImGui::Button("ENTER", buttonSize))
            {
                if (!leftOperand.empty() && currentOperator != '\0' && !rightOperand.empty())
                {
                    float a = StringToFloat(leftOperand);
                    float b = StringToFloat(rightOperand);
                    float calc = 0.0f;
                    switch (currentOperator)
                    {
                    case '+': calc = a + b; break;
                    case '-': calc = a - b; break;
                    case '*': calc = a * b; break;
                    case '/': calc = (b != 0.0f) ? a / b : 0.0f; break;
                    }
                    char buf[64];
                    snprintf(buf, sizeof(buf), "Result: %.4f", calc);
                    resultStr = buf;
                    // Chain the result: new left operand, clear operator and right operand
                    leftOperand = to_string(calc);
                    if (leftOperand.find('.') != string::npos)
                    {
                        while (leftOperand.back() == '0')
                            leftOperand.pop_back();
                        if (leftOperand.back() == '.')
                            leftOperand.pop_back();
                    }
                    currentOperator = '\0';
                    rightOperand.clear();
                }
            }
            ImGui::NextColumn();
            if (ImGui::Button(".", buttonSize))
            {
                if (currentOperator == '\0')
                    AppendToOperand(leftOperand, ".");
                else
                    AppendToOperand(rightOperand, ".");
            }
            ImGui::NextColumn();
            if (ImGui::Button("-", buttonSize))
            {
                if (!leftOperand.empty())
                    currentOperator = '-';
            }
            ImGui::NextColumn();
            ImGui::Columns(1);
            ImGui::End();

            if (!showCalculator)
            {
                ::PostQuitMessage(0);
                break;
            }
        }

        //================================================================================== /*

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { 0.00f, 0.00f, 0.00f, 0.00f }; // INVISIBLE BACKGROUND
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        HRESULT hr = g_pSwapChain->Present(0, 0); // 0,0 -> uncapped | 1,0 -> vsync
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

//----------------------------------------------------------------------------
// DirectX helper functions
//----------------------------------------------------------------------------

bool CreateDeviceD3D(HWND hWnd)
{
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

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// Forward declaration for ImGui's Win32 message handler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = LOWORD(lParam);
        g_ResizeHeight = HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
