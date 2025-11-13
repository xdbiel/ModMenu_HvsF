// dllmain.cpp
#include <windows.h>
#include <thread>

// --- Bibliotecas de Sistema ---
#include <d3d11.h>
#include <dxgi.h>

// --- Nossas Bibliotecas ---
#include "MinHook.h" 
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

LRESULT __stdcall hkWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
// Declaração 'extern' que você encontrou
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// --- Variáveis Globais ---
typedef HRESULT(__stdcall* Present)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
static Present oPresent = NULL;

typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);
static WNDPROC oWndProc = NULL;

// Globais para o Dispositivo e o Alvo de Renderização
static ID3D11Device* g_pDevice = NULL;
static ID3D11DeviceContext* g_pContext = NULL;
static ID3D11RenderTargetView* g_pRenderTargetView = NULL; // <-- A CHAVE

static bool g_bInit = false;
static bool g_bShowMenu = true;
static HWND g_hWindow = NULL;


// --- Função para criar o Render Target (Sua solução!) ---
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

// --- Nossa Função Hook do ImGui (Onde o Menu é Desenhado) ---
HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
    if (!g_bInit)
    {
        // Pega o Dispositivo e o Contexto (agora salvando globalmente)
        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_pDevice)))
        {
            g_pDevice->GetImmediateContext(&g_pContext);
            DXGI_SWAP_CHAIN_DESC desc;
            pSwapChain->GetDesc(&desc);
            g_hWindow = desc.OutputWindow;

            // --- CRIA O RENDER TARGET ---
            CreateRenderTarget(pSwapChain); // <-- Chamando sua solução

            // Inicializa o ImGui
            ImGui::CreateContext();
            ImGui_ImplWin32_Init(g_hWindow);
            ImGui_ImplDX11_Init(g_pDevice, g_pContext);

            // Hooka o WndProc
            oWndProc = (WNDPROC)SetWindowLongPtr(g_hWindow, GWLP_WNDPROC, (LONG_PTR)hkWndProc);

            g_bInit = true;
        }
        else
        {
            return oPresent(pSwapChain, SyncInterval, Flags);
        }
    }

    // --- Loop do Menu ---
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGuiIO& io = ImGui::GetIO();
    io.MouseDrawCursor = g_bShowMenu;

    if (g_bShowMenu)
    {
        // Força a posição (como você sugeriu)
        const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + 100, main_viewport->WorkPos.y + 100), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);

        // Mostra a janela de demonstração
        ImGui::ShowDemoWindow();
    }

    ImGui::Render();

    // --- O PASSO MAIS IMPORTANTE QUE FALTAVA ---
    // Diz ao DirectX para "mirar" no nosso alvo de renderização ANTES de desenhar
    g_pContext->OMSetRenderTargets(1, &g_pRenderTargetView, NULL);
    // ---------------------------------------------

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    return oPresent(pSwapChain, SyncInterval, Flags);
}

// --- NOSSA FUNÇÃO DE HOOK DA JANELA (INPUT) ---
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


// --- Thread Principal do Cheat (Exatamente como antes) ---
void MainThread()
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

    if (FAILED(hr))
    {
        MH_Uninitialize();
        return;
    }

    DWORD_PTR* pSwapChainVTable = (DWORD_PTR*)pSwapChain;
    pSwapChainVTable = (DWORD_PTR*)pSwapChainVTable[0];
    DWORD_PTR pPresentAddress = pSwapChainVTable[8];

    if (MH_CreateHook(reinterpret_cast<LPVOID>(pPresentAddress), &hkPresent, reinterpret_cast<void**>(&oPresent)) != MH_OK)
    {
        MH_Uninitialize();
        return;
    }

    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
    {
        MH_Uninitialize();
        return;
    }

    pDevice->Release();
    pContext->Release();
    pSwapChain->Release();
}

// --- Ponto de Entrada da DLL (Com limpeza do RenderTarget) ---
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        std::thread(MainThread).detach();
        break;

    case DLL_PROCESS_DETACH:
        if (oWndProc)
        {
            SetWindowLongPtr(g_hWindow, GWLP_WNDPROC, (LONG_PTR)oWndProc);
        }

        if (g_bInit)
        {
            ImGui_ImplDX11_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();

            // --- Limpa nosso RenderTarget ---
            if (g_pRenderTargetView)
            {
                g_pRenderTargetView->Release();
                g_pRenderTargetView = NULL;
            }
            if (g_pContext) g_pContext->Release();
            if (g_pDevice) g_pDevice->Release();
            // --------------------------------
        }
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
        break;
    }
    return TRUE;
}