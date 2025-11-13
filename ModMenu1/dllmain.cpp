

#include <windows.h>
#include <thread>
#include <vector>
#include <chrono> 


#include <d3d11.h>
#include <dxgi.h>


#include "MinHook.h" 
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

LRESULT __stdcall hkWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


typedef HRESULT(__stdcall* Present)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
static Present oPresent = NULL;
typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);
static WNDPROC oWndProc = NULL;

static ID3D11Device* g_pDevice = NULL;
static ID3D11DeviceContext* g_pContext = NULL;
static ID3D11RenderTargetView* g_pRenderTargetView = NULL;

static bool g_bInit = false;
static bool g_bShowMenu = true;
static HWND g_hWindow = NULL;

static bool bInfiniteAmmo = false; 
static uintptr_t g_baseModule = 0; 


uintptr_t ResolverCaminhoPonteiro(uintptr_t baseModule, uintptr_t staticOffset, std::vector<unsigned int> offsets)
{
    if (baseModule == 0) return 0;
    uintptr_t addr = *(uintptr_t*)(baseModule + staticOffset);
    if (addr == 0) return 0;
    for (size_t i = 0; i < offsets.size() - 1; ++i)
    {
        addr = *(uintptr_t*)(addr + offsets[i]);
        if (addr == 0) return 0;
    }
    return addr + offsets.back();
}


void CheatLoop()
{
 
    g_baseModule = (uintptr_t)GetModuleHandleA(NULL);

    while (true) 
    {
        
        if (bInfiniteAmmo)
        {
   
            uintptr_t staticOffset = 0x02FEE750;
            std::vector<unsigned int> ammoOffsets = { 0x4E8, 0x8, 0x360, 0x98, 0x2B8 };

       
            uintptr_t ammoAddress = ResolverCaminhoPonteiro(g_baseModule, staticOffset, ammoOffsets);

       
            if (ammoAddress != 0)
            {
                *(int*)ammoAddress = 999;
            }
        }


        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}



void CreateRenderTarget(IDXGISwapChain* pSwapChain)
{
    ID3D11Texture2D* pBackBuffer;
    pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    if (pBackBuffer != NULL)
    {
        g_pDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pRenderTargetView);
        pBackBuffer->Release();
    }
}


HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
    if (!g_bInit)
    {
        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_pDevice)))
        {
            g_pDevice->GetImmediateContext(&g_pContext);
            DXGI_SWAP_CHAIN_DESC desc;
            pSwapChain->GetDesc(&desc);
            g_hWindow = desc.OutputWindow;

            CreateRenderTarget(pSwapChain);

            ImGui::CreateContext();
            ImGuiStyle& style = ImGui::GetStyle();
            style.Colors[ImGuiCol_Text] = ImVec4(0.00f, 1.00f, 0.09f, 1.00f);
            style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
            style.Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
            style.Colors[ImGuiCol_ChildBg] = ImVec4(0.34f, 0.22f, 0.22f, 0.00f);
            style.Colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
            style.Colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
            style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
            style.Colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.29f, 0.48f, 0.54f);
            style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
            style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
            style.Colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
            style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
            style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
            style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
            style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
            style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
            style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
            style.Colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 0.00f, 0.86f, 1.00f);
            style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.88f, 0.24f, 0.64f, 1.00f);
            style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.98f, 0.26f, 0.26f, 1.00f);
            style.Colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
            style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.98f, 0.26f, 0.26f, 1.00f);
            style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.98f, 0.06f, 0.76f, 1.00f);
            style.Colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
            style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
            style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.96f, 0.26f, 0.98f, 1.00f);
            style.Colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
            style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
            style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
            style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
            style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
            style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
            style.Colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
            style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
            style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
            style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
            style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
            style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
            style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
            style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
            style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
            style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
            style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
            style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

            ImGui_ImplWin32_Init(g_hWindow);
            ImGui_ImplDX11_Init(g_pDevice, g_pContext);

            oWndProc = (WNDPROC)SetWindowLongPtr(g_hWindow, GWLP_WNDPROC, (LONG_PTR)hkWndProc);

            g_bInit = true;
        }
        else
        {
            return oPresent(pSwapChain, SyncInterval, Flags);
        }
    }



    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGuiIO& io = ImGui::GetIO();
    io.MouseDrawCursor = g_bShowMenu;

    if (g_bShowMenu)
    {

     
        const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + 100, main_viewport->WorkPos.y + 100), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_FirstUseEver);

        ImGui::Begin("ModMenu HvsF (by angxxj33)");
        {
            ImGui::Text("Pressione 'Insert' para esconder.");
            ImGui::Separator();

          
            ImGui::Checkbox("Municao Infinita", &bInfiniteAmmo);
        }
        ImGui::End();
    }

    ImGui::Render();
    g_pContext->OMSetRenderTargets(1, &g_pRenderTargetView, NULL);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    return oPresent(pSwapChain, SyncInterval, Flags);
}


LRESULT __stdcall hkWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_KEYDOWN && wParam == VK_INSERT)
    {
        g_bShowMenu = !g_bShowMenu;
        return true;
    }
    if (g_bInit && g_bShowMenu)
    {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
        {
            return true;
        }
    }
    return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}



void HookThread()
{
    if (MH_Initialize() != MH_OK) return;


    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = 1;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = GetForegroundWindow();
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    ID3D11Device* pDevice = NULL;
    ID3D11DeviceContext* pContext = NULL;
    IDXGISwapChain* pSwapChain = NULL;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, &featureLevel, 1, D3D11_SDK_VERSION, &swapChainDesc, &pSwapChain, &pDevice, NULL, &pContext);
    if (FAILED(hr)) { MH_Uninitialize(); return; }


    DWORD_PTR* pSwapChainVTable = (DWORD_PTR*)pSwapChain;
    pSwapChainVTable = (DWORD_PTR*)pSwapChainVTable[0];
    DWORD_PTR pPresentAddress = pSwapChainVTable[8];


    if (MH_CreateHook(reinterpret_cast<LPVOID>(pPresentAddress), &hkPresent, reinterpret_cast<void**>(&oPresent)) != MH_OK) { MH_Uninitialize(); return; }
    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) { MH_Uninitialize(); return; }


    pDevice->Release();
    pContext->Release();
    pSwapChain->Release();
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
      
        std::thread(HookThread).detach();
       
        std::thread(CheatLoop).detach();
        break;

    case DLL_PROCESS_DETACH:
        if (oWndProc) { SetWindowLongPtr(g_hWindow, GWLP_WNDPROC, (LONG_PTR)oWndProc); }
        if (g_bInit)
        {
            ImGui_ImplDX11_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
            if (g_pRenderTargetView) g_pRenderTargetView->Release();
            if (g_pContext) g_pContext->Release();
            if (g_pDevice) g_pDevice->Release();
        }
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
        break;
    }
    return TRUE;
}