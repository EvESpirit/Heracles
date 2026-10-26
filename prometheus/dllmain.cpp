// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <stdio.h>
#include <MinHook.h>
#include <string>
#define LAZY_IMPORTER_CASE_INSENSITIVE
#include "lazy_importer.h"
#include <mutex>
#include <TlHelp32.h>
#include <thread>
#include <string>
#include <locale>
#include <codecvt>
#include "ExceptionFormatter.h"
#include <winternl.h>
#include <algorithm>
#include <vector>
#include <set>
#include <string>
#include <format>
#include <thread>
#include <nlohmann/json.hpp>
#include <d3d11.h>
#include "pe.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_freetype.h"
#include "idadefs.h"
#include "window_manager/window_manager.h"
#include "window_manager/management_window.h"
#include "globals.h"
#include "JAM.h"
#include "stringhash_library.h"
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "ntdll.lib")
#pragma comment(lib, "Bcrypt.lib")
#pragma comment(lib, "Crypt32.lib")
#include "Statescript.h"
#include "StatescriptVar.h"
#include "game.h"
#include "ResourceManager.h"
#include "STU.h"
#include "statescript_logger.h" 
#include <imnodes/imnodes.h>
#include "stu_resources.h"
#include "entity_admin.h"
#include "MovementState.h"
#include <array>

//Who needs a build system?
#include <imnodes/imnodes.cpp>

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ImVec2, x, y);

struct memmgr;
struct memmgr_vt {
    union {
        STRUCT_PLACE_CUSTOM(a, 0x30, __int64(__fastcall* allocate)(memmgr*, __int64, int));
        STRUCT_PLACE_CUSTOM(b, 0x40, void(__fastcall* deallocate)(memmgr*, __int64));
    };
};

struct memmgr {
    memmgr_vt* vfptr;
};

memmgr* get_memmgr() {
    return *(memmgr**)(globals::gameBase + 0x181ee50);
}

__int64 ow_memalloc(int size) {
    auto memmgr = get_memmgr();
    return memmgr->vfptr->allocate(memmgr, size, 0x10);
}

void ow_dealloc(__int64 address) {
    auto memmgr = get_memmgr();
    __try {
        return memmgr->vfptr->deallocate(memmgr, address);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        printf("failed ow_dealloc for %p\n", address);
    }
}

//_insn decodeInsn(DWORD_PTR where);
//void patchJump(DWORD_PTR location, DWORD_PTR toAddr);
//void patchToJump(DWORD_PTR location);

const DWORD_PTR TlsCallback_Addr = 0x137f000;
const DWORD_PTR Start_Addr = 0x11fe3ac;
const DWORD_PTR TlsCallback2_Addr = 0x113cd10;
//const DWORD_PTR TlsCallback2_Addr = 0x113cd10;
//const DWORD_PTR owVEH_Addr = 0x13810d0;

DWORD mainThreadId;
__int64(__stdcall* AddVeh_orig)(__int64 old, __int64 func);
void(__fastcall* ExitProcess_orig)(int);
__int64(__fastcall* createwindow_orig)(__int64);

std::wstring s2ws(const std::string& str)
{
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.from_bytes(str);
}

std::string ws2s(const std::wstring& wstr)
{
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.to_bytes(wstr);
}

unsigned char TlsCallback_0[] =
{
  0x4C, 0x8B, 0xDC, 0x49, 0x89, 0x4B, 0x08, 0x55, 0x49, 0x8D,
  0xAB, 0xB8, 0xF9, 0xFF, 0xFF, 0x48, 0x81, 0xEC, 0x40, 0x07,
  0x00, 0x00, 0x53, 0x81, 0xCB, 0xF3, 0xA4, 0x27, 0x5A, 0x5B,
  0x71, 0x02, 0x49, 0xBE, 0x83, 0xFA, 0x01, 0x0F, 0x85, 0x92,
  0x20, 0x00, 0x00, 0x80, 0x3D, 0x0E, 0x5D, 0x49, 0x00, 0x00,
  0x0F, 0x85, 0x85, 0x20, 0x00, 0x00, 0x88, 0x15, 0x02, 0x5D,
  0x49, 0x00, 0x52, 0x81, 0xCA, 0x27, 0xB7, 0x61, 0x62, 0x5A,
  0x71, 0x02, 0x48, 0xBD, 0xC7, 0x85, 0x58, 0x06, 0x00, 0x00,
  0x60, 0x00, 0x00, 0x00, 0x8B, 0x85, 0x58, 0x06, 0x00, 0x00,
  0x65, 0x48, 0x8B, 0x00, 0x48, 0x85, 0xC0, 0x0F, 0x84, 0x56,
  0x20, 0x00, 0x00, 0x48, 0x8B, 0x40, 0x18, 0x48, 0x8B, 0x48,
  0x20, 0x48, 0x83, 0xC0, 0x20, 0x48, 0x3B, 0xC8, 0x0F, 0x84,
  0x41, 0x20, 0x00, 0x00, 0x4D, 0x89, 0x7B, 0xC8, 0x4C, 0x8D,
  0x79, 0xF0, 0x4D, 0x85, 0xFF, 0x0F, 0x84, 0x28, 0x20, 0x00,
  0x00, 0x52, 0x81, 0xE2, 0x29, 0x44, 0xB9, 0xB6, 0x5A, 0x73,
  0x02, 0x48, 0xBB, 0xC7, 0x45, 0x38, 0x6B, 0x65, 0x72, 0x6E,
  0xC7, 0x45, 0x3C, 0x65, 0x6C, 0x33, 0x32, 0xC7, 0x45, 0x40,
  0x2E, 0x64, 0x6C, 0x6C, 0x50, 0x81, 0xC8, 0xD7, 0x3A, 0xA9,
  0x6A, 0x58, 0x73, 0x02, 0x48, 0xBA, 0xC7, 0x85, 0x58, 0x06,
  0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x8B, 0x85, 0x58, 0x06,
  0x00, 0x00, 0x65, 0x48, 0x8B, 0x08, 0x48, 0x85, 0xC9, 0x0F,
  0x84, 0xDE, 0x1F, 0x00, 0x00, 0x49, 0x89, 0x5B, 0x18, 0x48,
  0x8B, 0x59, 0x18, 0x4C, 0x8B, 0x53, 0x20, 0x48, 0x83, 0xC3,
  0x20, 0x4C, 0x3B, 0xD3, 0x0F, 0x84, 0xBD, 0x1F, 0x00, 0x00,
  0x49, 0x89, 0x73, 0xF0, 0x49, 0x89, 0x7B, 0xE8, 0x4D, 0x89,
  0x6B, 0xD8, 0x45, 0x33, 0xED, 0x90, 0x45, 0x0F, 0xB7, 0x4A,
  0x38, 0x4D, 0x8B, 0x42, 0x40, 0x4D, 0x8D, 0x5A, 0xF0, 0x49,
  0x8B, 0xFD, 0x41, 0x8B, 0xF5, 0x45, 0x85, 0xC9, 0x0F, 0x8E,
  0xCB, 0x01, 0x00, 0x00, 0x0F, 0x1F, 0x40, 0x00, 0x41, 0x0F,
  0xB6, 0x08, 0x8D, 0x41, 0xBF, 0x3C, 0x19, 0x77, 0x03, 0x80,
  0xC1, 0x20, 0x0F, 0xB6, 0x54, 0x3D, 0x38, 0x8D, 0x42, 0xBF,
  0x3C, 0x19, 0x77, 0x03, 0x80, 0xC2, 0x20, 0x3A, 0xCA, 0x0F,
  0x85, 0x8F, 0x01, 0x00, 0x00, 0x48, 0xFF, 0xC7, 0x48, 0x83,
  0xFF, 0x0C, 0x0F, 0x85, 0x85, 0x01, 0x00, 0x00, 0x4D, 0x85,
  0xDB, 0x0F, 0x84, 0x3A, 0x1F, 0x00, 0x00, 0x4C, 0x89, 0xA4,
  0x24, 0x28, 0x07
};

DWORD GetMainThreadId() {
    auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        printf("Could not open handle to get main thread\n");
        return 0;
    }
    THREADENTRY32 tEntry;
    tEntry.dwSize = sizeof(THREADENTRY32);
    DWORD result = 0;
    DWORD currentPID = GetCurrentProcessId();
    for (BOOL success = Thread32First(snapshot, &tEntry);
        !result && success && GetLastError() != ERROR_NO_MORE_FILES;
        success = Thread32Next(snapshot, &tEntry))
    {
        if (tEntry.th32OwnerProcessID == currentPID) {
            result = tEntry.th32ThreadID;
        }
    }
    CloseHandle(snapshot);
    return result;
}

void exit_handler() {
    printf("=============== shutdown ===============\n");
    if (!globals::exit_normal)
        system("pause");
}

void __fastcall ExitProcessHook(int exitCode) {
    printf("Process exit initiazed: %d\n", exitCode);
    ExitProcess_orig(exitCode);
}

void decryptPage(__int64 page) {
    auto testHandler = (void(__fastcall*)(_EXCEPTION_POINTERS*))(globals::gameBase + 0x13810d0);
    //PVECTORED_EXCEPTION_HANDLER vehHandler = (PVECTORED_EXCEPTION_HANDLER)&;
    _EXCEPTION_POINTERS exc{};
    _EXCEPTION_RECORD record{};
    exc.ExceptionRecord = &record;
    exc.ExceptionRecord->ExceptionCode = 0xC0000005;
    exc.ExceptionRecord->ExceptionAddress = (PVOID)(globals::gameBase + page);
    exc.ExceptionRecord->ExceptionInformation[1] = globals::gameBase + page;
    testHandler(&exc);
}

int DebuggerTrapHook() {
    //printf("Debugger Trap from %p\n", _ReturnAddress());
    return false;
}

ID3D11Device* g_pd3dDevice = 0;
ID3D11DeviceContext* g_pd3dContext = 0;
typedef HRESULT(__stdcall* D3D11PresentHook) (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
D3D11PresentHook phookD3D11Present = nullptr;
LONG_PTR m_ulOldWndProc;
ID3D11RenderTargetView* g_mainRenderTargetView = NULL;
std::once_flag imgui_init{};

void CreateRenderTarget(IDXGISwapChain* swap, bool firstTime)
{
    if (firstTime)
    {
        DXGI_SWAP_CHAIN_DESC sd;
        swap->GetDesc(&sd);
        // Create the render target
        ID3D11Texture2D* pBackBuffer;
        D3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc;
        ZeroMemory(&render_target_view_desc, sizeof(render_target_view_desc));
        render_target_view_desc.Format = sd.BufferDesc.Format;
        render_target_view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        swap->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, &render_target_view_desc, &g_mainRenderTargetView);
        g_pd3dContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        pBackBuffer->Release();
    }
    else
        g_pd3dContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT __stdcall WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui::GetCurrentContext() != NULL) {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
            return true;
        else if (ImGui::GetIO().WantCaptureKeyboard) {
            switch (msg) {
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            case WM_CHAR:
                return false;
            }
        }
    }
    return CallWindowProcW((WNDPROC)m_ulOldWndProc, hWnd, msg, wParam, lParam);
}

typedef bool(*clipboard_fn)(__int64, string_rep* str_out);

const char* getClipboard(void*) {
    string_rep out{};
    clipboard_fn fn = (clipboard_fn)(globals::gameBase + 0x8ab990);
    if (fn(0, &out)) {
        printf("GetClipboard succeessful\n");
        char* str = new char[strlen(out.get())];
        strcpy(str, out.get());
        return str;
    }
    else
        printf("GetClipboard failed\n");
    return new char[0];
}

IDXGISwapChain* main_swapchain = nullptr;
bool ui_invisible = false;
HRESULT __stdcall PresentHook(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
    if (!main_swapchain)
        main_swapchain = pSwapChain;
    if (main_swapchain == pSwapChain) {
        //fuck fullscreen, all my homies hate fullscreen in 0.8
        *(int*)(globals::gameBase + 0x17a1123) = 0;
        std::call_once(imgui_init, [&]() {
            globals::gameWindow = (DWORD_PTR)FindWindow(L"tankWindowClass", NULL);
            pSwapChain->GetDevice(__uuidof(g_pd3dDevice), (void**)&g_pd3dDevice);
            g_pd3dDevice->GetImmediateContext(&g_pd3dContext);

            ImGui::CreateContext();
            ImNodes::CreateContext();

            ImGui_ImplWin32_Init((void*)globals::gameWindow);
            ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dContext);

            static ImWchar ranges[] = { 0x1, 0x1FFFF, 0 };
            static ImFontConfig cfg;
            cfg.OversampleH = cfg.OversampleV = 1;
            cfg.MergeMode = true;
            cfg.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_LoadColor;
            auto& io = ImGui::GetIO();

            imgui_helpers::BoldFont = io.Fonts->AddFontFromFileTTF("MonaspaceXenon-ExtraBold.otf", 13);
            io.Fonts->AddFontFromFileTTF("MonaspaceXenon-Regular.otf", 13/*, &cfg, ImGui::GetIO().Fonts->GetGlyphRangesDefault()*/);
            io.FontDefault = ImGui::GetIO().Fonts->AddFontFromFileTTF("Font Awesome 6 Free-Solid-900.otf", 13, &cfg, ranges);

            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable /*| ImGuiConfigFlags_ViewportsEnable*/; //TODO: Viewports
            io.ConfigDockingWithShift = true;

            //io.ConfigViewportsNoTaskBarIcon = true;
            //io.ConfigViewportsNoDefaultParent = true;
            ///*io.ConfigDockingAlwaysTabBar = true;*/
            //io.ConfigDockingTransparentPayload = true;

            CreateRenderTarget(pSwapChain, true);
            m_ulOldWndProc = SetWindowLongPtr((HWND)globals::gameWindow, GWLP_WNDPROC, (LONG_PTR)WndProc);
            /*io.KeysData[ImGuiKey_Backspace]. = VK_BACK;
            io.KeysData[ImGuiKey_LeftArrow].Down = VK_LEFT;
            io.KeysData[ImGuiKey_RightArrow].Down = VK_RIGHT;
            io.KeysData[ImGuiKey_UpArrow].Down = VK_UP;
            io.KeysData[ImGuiKey_DownArrow].Down = VK_DOWN;
            io.KeysData[ImGuiKey_Escape].Down = VK_ESCAPE;*/
            //http://kbdedit.com/manual/low_level_vk_list.html
            io.GetClipboardTextFn = getClipboard;

            printf("Filling STU vfptr table\n");
            STURegistryData::initialize();
            statescript_logger::Initialize();
            window_manager::add_window(new management_window);
        });
        globals::pauseLogHook = true;
        CreateRenderTarget(pSwapChain, false);

        if (ImGui::IsKeyPressed(ImGuiKey_Insert, false)) {
            publish_game_msg(0x0240000000000164);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_GraveAccent, false)) {
            ui_invisible = !ui_invisible;
        }

        auto& io = ImGui::GetIO();
        bool menu_down = GetAsyncKeyState(VK_MENU) < 0;
        io.AddKeyEvent(ImGuiKey_LeftAlt, menu_down);
        io.AddKeyEvent(ImGuiKey_Menu, menu_down);
        io.AddKeyEvent(ImGuiKey_RightAlt, menu_down);

        /*ImGui::GetIO().DisplaySize.x = 500;
        ImGui::GetIO().DisplaySize.y = 500;*/
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (!ui_invisible)
            window_manager::render();

        ImGui::Render();

        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        globals::pauseLogHook = false;

        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }
    return phookD3D11Present(pSwapChain, SyncInterval, Flags);
}

__int64 __fastcall createwindow_hook(__int64 gameManager) {
    auto handle = createwindow_orig(gameManager);
    printf("window handle: %x\n", handle);
    globals::gameWindow = handle;
    std::thread([]() {
        while (true) {
            Sleep(100);
            //teEngine Instance
            DWORD_PTR render = *(DWORD_PTR*)(globals::gameBase + 0x181e3e0);
            if (!render)
                continue;
            render = *(DWORD_PTR*)(render + 0x900);
            if (!render)
                continue;
            render = *(DWORD_PTR*)(render + 0x2e0);
            if (!render)
                continue;
            render = *(DWORD_PTR*)(render);
            if (!render)
                continue;
            render = *(DWORD_PTR*)(render + 8);
            if (!render)
                continue;
            render = *(DWORD_PTR*)(render);
            if (!render)
                continue;
            render = *(DWORD_PTR*)(render + 0x40);
            if (!render)
                continue;
            MH_VERIFY(MH_CreateHook((DWORD_PTR*)render, PresentHook, reinterpret_cast<void**>(&phookD3D11Present)));
            MH_VERIFY(MH_EnableHook((DWORD_PTR*)render));
            break;
        }
        }).detach();
    return handle;
}

typedef __int64(__cdecl* regedit_fn)(JamType13String*, JamType13String*, __int64, __int64);
regedit_fn regedit_initialize_orig;
__int64 regedit_initialize(JamType13String* a1, JamType13String* a2, __int64 a3, __int64 a4) {
    auto str = std::string(readJamType13String(a2));
    //printf("regedit: %s\n", str.c_str());
    if (str == "REGION" || str == "LOCALE" || str == "LOCALE_AUDIO") {
        printf("patch regedit get-region.\n");
        return 1;
    }
    return regedit_initialize_orig(a1, a2, a3, a4);
}

struct CascLogEntry {
    const char* msg;
    char* buffer_begin;
    __int64 buffer_max_size;
    __int64 buffer_size;
    int four;
    int unk;
    const char* culprit;
};

typedef __int64(__cdecl* cascLogHook_fn)(int, const char*, const char*);
cascLogHook_fn cascLogHook_orig;
__int64 cascLogHook(int level, const char* culprit, const char* msg) {
    printf("CASC %d (%s): %s\n", level, culprit, msg);
    auto result = cascLogHook_orig(level, culprit, msg);
    return result;
}

int (*statescript_load_orig)(StatescriptInstance*);
int statescript_load_hook(StatescriptInstance* ss) {
    return statescript_load_orig(ss);
}

typedef __int64 (*stuux_string_hash_fn)(char* input);

stuux_string_hash_fn stuux_string_hash_orig;
__int64 last_stuux_hash;
__int64 stuux_string_hash_hook(char* input) {
    __int64 hash = stuux_string_hash_orig(input);
    if (last_stuux_hash != hash) {
        last_stuux_hash = hash;
        if (hash != 0)
            allmighty_hash_lib::add_comment(hash, input, true);
    }
    return hash;
}

//*chefs kiss* to casclib
#define CASC_KEY_LENGTH 0x10
struct CASC_ENCRYPTION_KEY
{
    ULONGLONG KeyName;                          // "Name" of the key
    BYTE Key[CASC_KEY_LENGTH];                  // The key itself
};

CASC_ENCRYPTION_KEY casc_keys[] = {
    { 0xFB680CB6A8BF81F3ULL, { 0x62, 0xD9, 0x0E, 0xFA, 0x7F, 0x36, 0xD7, 0x1C, 0x39, 0x8A, 0xE2, 0xF1, 0xFE, 0x37, 0xBD, 0xB9 } },   // 0.8.0.24919_retailx64 (hardcoded)
    { 0x402CD9D8D6BFED98ULL, { 0xAE, 0xB0, 0xEA, 0xDE, 0xA4, 0x76, 0x12, 0xFE, 0x6C, 0x04, 0x1A, 0x03, 0x95, 0x8D, 0xF2, 0x41 } },   // 0.8.0.24919_retailx64 (hardcoded)
    { 0xDBD3371554F60306ULL, { 0x34, 0xE3, 0x97, 0xAC, 0xE6, 0xDD, 0x30, 0xEE, 0xFD, 0xC9, 0x8A, 0x2A, 0xB0, 0x93, 0xCD, 0x3C } },   // 0.8.0.24919_retailx64 (streamed from server)
    { 0x11A9203C9881710AULL, { 0x2E, 0x2C, 0xB8, 0xC3, 0x97, 0xC2, 0xF2, 0x4E, 0xD0, 0xB5, 0xE4, 0x52, 0xF1, 0x8D, 0xC2, 0x67 } },   // 0.8.0.24919_retailx64 (streamed from server)
    { 0xA19C4F859F6EFA54ULL, { 0x01, 0x96, 0xCB, 0x6F, 0x5E, 0xCB, 0xAD, 0x7C, 0xB5, 0x28, 0x38, 0x91, 0xB9, 0x71, 0x2B, 0x4B } },   // 0.8.0.24919_retailx64 (streamed from server)
    { 0x87AEBBC9C4E6B601ULL, { 0x68, 0x5E, 0x86, 0xC6, 0x06, 0x3D, 0xFD, 0xA6, 0xC9, 0xE8, 0x52, 0x98, 0x07, 0x6B, 0x3D, 0x42 } },   // 0.8.0.24919_retailx64 (streamed from server)
    { 0xDEE3A0521EFF6F03ULL, { 0xAD, 0x74, 0x0C, 0xE3, 0xFF, 0xFF, 0x92, 0x31, 0x46, 0x81, 0x26, 0x98, 0x57, 0x08, 0xE1, 0xB9 } },   // 0.8.0.24919_retailx64 (streamed from server)
    { 0x8C9106108AA84F07ULL, { 0x53, 0xD8, 0x59, 0xDD, 0xA2, 0x63, 0x5A, 0x38, 0xDC, 0x32, 0xE7, 0x2B, 0x11, 0xB3, 0x2F, 0x29 } },   // 0.8.0.24919_retailx64 (streamed from server)
    { 0x49166D358A34D815ULL, { 0x66, 0x78, 0x68, 0xCD, 0x94, 0xEA, 0x01, 0x35, 0xB9, 0xB1, 0x6C, 0x93, 0xB1, 0x12, 0x4A, 0xBA } },   // 0.8.0.24919_retailx64 (streamed from server)
};
struct tact_key_struct {
    CASC_ENCRYPTION_KEY key;
    char what_is_this;
};

typedef __int64(__fastcall* manager_14_fn)(__int64);
manager_14_fn manager_14_orig;
__int64 __fastcall manager_14_init_hook(__int64 manager) {
    __int64 storageHandler = *(__int64*)(globals::gameBase + 0x182ca50);
    if (storageHandler && *(__int64*)(storageHandler + 0x50)) {
        __int64 magic_fairy_dust = *(__int64*)(storageHandler + 0x58);
        if (magic_fairy_dust) {
            auto emplace_keystruct_fn = (tact_key_struct * (__fastcall*)(__int64, ULONGLONG*))(globals::gameBase + 0x9d0470);
            for (auto& key : casc_keys) {
                tact_key_struct* key_struct = emplace_keystruct_fn(magic_fairy_dust + 8, &key.KeyName);
                printf("Emplacing TACT key %llx\n", key.KeyName);
                memcpy(&key_struct->key, &key, sizeof(CASC_ENCRYPTION_KEY));
                key_struct->what_is_this = 1;
            }
        }
    }
    return manager_14_orig(manager);
}

typedef Entity* (__fastcall* load_entity_fn)(EntityAdminBase*, int* entid, EntityLoader*);
load_entity_fn emplace_entity_orig;
Entity* emplace_entity_hook(EntityAdminBase* ea, int* entid, EntityLoader* loader) {
    if (loader->stu_id == 0x400000000000433 || //bigass boat on Gibraltar
        loader->stu_id == 0x04000000000001BE || //Spawn no entry grill
        loader->stu_id == 0x0400000000000254 || //Spawn no entry grill
        loader->stu_id == 0x0400000000000255) { //Spawn no entry grill
        for (auto& item : loader->loader_entries)
            item.component_id = 0;
    }
    uintptr_t caller = (uintptr_t)_ReturnAddress();
    uintptr_t caller_rva = (caller >= globals::gameBase && caller < globals::gameBase + globals::gameSize)
        ? (caller - globals::gameBase) : caller;
    bool had_init = (loader && loader->loader_entries[1].init_data != 0);
    Vector4 init_pos{}, init_scale{}, init_rot{};
    if (had_init) {
        auto* id = (Component_1_SceneRendering::InitData*)loader->loader_entries[1].init_data;
        init_pos = id->position;
        init_scale = id->scale;
        init_rot = id->rotation;
    }

    Entity* ent = emplace_entity_orig(ea, entid, loader);

    EntityAdminBase* game_ea = GameEntityAdmin();
    if (ent && game_ea && ea == game_ea) {
        auto* slot = OriginClutterTracker::GetSlot(ent->entity_id, ent);
        if (slot) {
            slot->spawn_caller_rva = caller_rva;
            if (loader) {
                slot->stu_id = loader->stu_id;
                slot->additional_stu_id = loader->additional_stu_id;
            }
            slot->had_loader_init_data = had_init;
            if (had_init) {
                slot->loader_init_pos = init_pos;
                slot->loader_init_scale = init_scale;
                slot->loader_init_rot = init_rot;
                float init_abs = fabsf(init_pos.X) + fabsf(init_pos.Y) + fabsf(init_pos.Z);
                if (init_abs > 0.5f && !slot->has_nonzero_pos) {
                    slot->has_nonzero_pos = true;
                    slot->first_nonzero_pos = init_pos;
                    slot->first_nonzero_rot = init_rot;
                    slot->first_nonzero_caller_rva = caller_rva;
                    slot->latest_nonzero_pos = init_pos;
                    slot->latest_nonzero_rot = init_rot;
                    slot->latest_nonzero_caller_rva = caller_rva;
                }
            }
            auto* c1 = ent->getById<Component_1_SceneRendering>(1);
            if (c1 && slot->has_nonzero_pos) {
                float cur_abs = fabsf(c1->position.X) + fabsf(c1->position.Y) + fabsf(c1->position.Z);
                uintptr_t parent_c1 = *(uintptr_t*)((uintptr_t)c1 + 0x1D0);
                if (cur_abs < 0.05f && parent_c1 == 0 && OriginClutterTracker::s_block_zero_overwrites) {
                    c1->SetPosRotation(slot->latest_nonzero_pos, slot->latest_nonzero_rot);
                }
            }
            if (game_ea->local_entid != 0) {
                extern void WeaponDiagLogFmt(const char* fmt, ...);
                __int64 res_id = (ent->resload_entry && ent->resload_entry->valid()) ? ent->resload_entry->align()->resource_id : 0;
                WeaponDiagLogFmt("[EMPLACE_ENT (+0x80F7C0)] caller=+0x%llX ent=0x%X stu=0x%llX res=0x%llX has_0D=%d has_1D=%d init_pos=(%.2f,%.2f,%.2f)",
                    (unsigned long long)caller_rva, ent->entity_id,
                    (unsigned long long)(loader ? loader->stu_id : 0),
                    (unsigned long long)res_id,
                    ent->hasComponent(0x0D) ? 1 : 0,
                    ent->hasComponent(0x1D) ? 1 : 0,
                    init_pos.X, init_pos.Y, init_pos.Z);
            }
        }
    }
    return ent;
}


typedef __int64(__fastcall* afterdatapackload_)(__int64 a1, __int64 a2, __int64 a3, int a4, int a5);
afterdatapackload_ afterdatapackload_fn;//0xf3eb10
__int64 afterdatapackload_hook(__int64 a1, __int64 a2, __int64 a3, int a4, int a5) {
    __int64 result = afterdatapackload_fn(a1, a2, a3, a4, a5);
    if (a2 == 0x06E0000000000079) {
        printf("Hook AfterDataPack\n");
        __int64 packs[] = {
            //Data Packs
            0x06e0000000000079,
            0x06e00000000000b5,
            0x06e0000000000008,
            0x06e000000000008d,
            0x06e0000000000017,
            0x06e0000000000029,
            0x06e0000000000065,
            0x06e0000000000066,

            //Heroes
            0x02e0000000000003,
            0x02e0000000000002,
            0x02e00000000000dd,
            0x02e000000000000a,
            0x02e0000000000079,
            0x02e0000000000068,
            0x02e0000000000065,
            0x02e0000000000042,
            0x02e0000000000040,
            0x02e0000000000029,
            0x02e0000000000020,
            0x02e0000000000016,
            0x02e0000000000015,
            0x02e0000000000009,
            0x02e0000000000008,
            0x02e000000000007a,
            0x02e0000000000007,
            0x02e000000000006e,
            0x02e0000000000006,
            0x02e0000000000005,
            0x02e0000000000004,

            //STUSkin
            0x0ae00000000003d4,
            0x0ae00000000003d6,
            0x0ae00000000003d7,
            0x0ae00000000003d8,
            0x0ae00000000003d9,
            0x0ae00000000003da,
            0x0ae00000000003dc,
            0x0ae00000000003dd,
            0x0ae00000000003de,
            0x0ae00000000003e0,
            0x0ae00000000003e1,
            0x0ae00000000003e2,
            0x0ae00000000003e3,
            0x0ae00000000003e4,
            0x0ae00000000003e5,
            0x0ae00000000003e6,
            0x0ae00000000003e7,
            0x0ae00000000003e8,
            0x0ae00000000003e9,
            0x0ae00000000003ea,
            0x0ae00000000003eb,

            //STUMapHeader
            0x0790000000000679,
            0x07900000000000d4,
            0x07900000000001c2,
            0x07900000000001d4,
            0x07900000000001db,
            0x07900000000002af,
            0x07900000000002c3,
            0x079000000000005b,
            0x079000000000067a,
            0x079000000000067b,
            0x0790000000000165,
            0x0790000000000184,
            0x0790000000000223,
            0x0790000000000661,
            0x0790000000000664,
            0x0790000000000675,
            0x0790000000000676,
            0x0790000000000677,
            0x0790000000000678,

            //STUCatalog
            0x046000000000002B,
            0x0460000000000068,
            0x046000000000007A,
            0x04600000000000E0,
            0x046000000000003E,
            0x046000000000002A,

            //Test
            0x0B90000000000001,
            0x0B90000000000002,
            0x0B90000000000003,
            0x0B90000000000004,
        };
        for (auto pack : packs) {
            afterdatapackload_fn(a1, pack, a3, 0, 3);
            afterdatapackload_fn(a1, pack, a3, 0, 5);
        }

        char ret_1[] = { 0xB0, 0x01, 0xC3 };
        memcpy((void*)(globals::gameBase + 0xc7c960), ret_1, sizeof(ret_1));
        printf("Force chat enabled\n"); //Must be done after data packs loaded
    }
    return result;
}
__int64(__fastcall* vmstu_stringhash_orig)(__int64);
__int64 vmstu_stringhash_hook(__int64 viewmodel) {
    const char* str = *(const char**)(viewmodel + 0x18);
    //printf("ViewModel: %s\n", str);
    return vmstu_stringhash_orig(viewmodel);
}

__int64(__fastcall* dataflow_bugfix_orig)(__int64 a1, __int64 a2, int a3, float* a4);
__int64 __fastcall dataflow_bugfix_fn(__int64 a1, __int64 a2, int a3, float* a4) {
    if (a3 == 0xF5) {
        auto ptr = *(_QWORD*)(a2 + 0x28);
        if (ptr) {
            ptr = *(_QWORD*)(ptr + 0x208);
            if (!ptr) {
                *a4 = 0;
                return 1;
            }
        }
    }
    return dataflow_bugfix_orig(a1, a2, a3, a4);
}

bool ea_firstCall = true;
EntityAdminBase* (*Construct_GameEntityAdmin_orig)(EntityAdminBase* ea, EntityAdminCreationInfo* ci);
EntityAdminBase* Construct_GameEntityAdmin_fn(EntityAdminBase* ea, EntityAdminCreationInfo* ci) {
    ea = Construct_GameEntityAdmin_orig(ea, ci);

    //auto emplace_fn = (void(*)(EntityAdminBase*, mapfunc_outer_vt**, __int64 inheritance))(globals::gameBase + 0x80f450);
    //emplace_fn(ea, (mapfunc_outer_vt**)PrometheusSystem::create(ea), globals::gameBase + 0x80f450);

    if (ea_firstCall) {
        ea_firstCall = false;
        return ea;
    }

    auto sys = PrometheusSystem::create(ea);
    if (sys) //The second call is for GameEntityAdmin
        ea->systems_array.emplace_item((system_vt**)sys);
    return ea;
}

char (*selectiveResLoad_orig)(__int64, __int64);
char __fastcall selectiveResLoad_hook(__int64 a1, __int64 a2)
{
    char v2; // al
    int v3; // eax

    // Allow server-replicated map props (((a2 + 0x10) & 2) == 0) to load offline,
    // but do NOT return 1 early so they still go through the LoadEntIndex (+0x182b718) filter below.
    if ((*(_BYTE*)(a2 + 0x10) & 1) == 0)
    {
        v2 = *(_BYTE*)(a2 + 0x13);
        if (v2 == 8)
        {
            v3 = *(_DWORD*)(a2 + 0x5C);
        }
        else
        {
            if (v2 != 0xB)
                return 1;
            v3 = *(_DWORD*)(a2 + 0x78);
        }
        if (!v3)
            v3 = 2;
        if (v3 > *(int*)(globals::gameBase + 0x182b718)) {
            //printf("> LoadEntIndex\n");
            return 0;
        }
    }
    return 1;
}

char (*component_1_oncreate_orig)(Component_1_SceneRendering*, EntityLoader*);
char componen1_oncreate_hook(Component_1_SceneRendering* comp, EntityLoader* loader) {
    char res = component_1_oncreate_orig(comp, loader);
    if (comp && comp->base.entity_backref) {
        Entity* ent = comp->base.entity_backref;
        EntityAdminBase* game_ea = GameEntityAdmin();
        if (game_ea && ent->entity_admin_backref == game_ea) {
            auto* slot = OriginClutterTracker::GetSlot(ent->entity_id, ent);
            if (slot) {
                if (loader) {
                    slot->stu_id = loader->stu_id;
                    slot->additional_stu_id = loader->additional_stu_id;
                }
                float abs_pos = fabsf(comp->position.X) + fabsf(comp->position.Y) + fabsf(comp->position.Z);
                if (abs_pos > 0.5f && comp->position.Y > -4000.0f) {
                    if (!slot->has_nonzero_pos) {
                        slot->has_nonzero_pos = true;
                        slot->first_nonzero_pos = comp->position;
                        slot->first_nonzero_rot = comp->rotation;
                        slot->first_nonzero_caller_rva = 0xA00450;
                    }
                    slot->latest_nonzero_pos = comp->position;
                    slot->latest_nonzero_rot = comp->rotation;
                    slot->latest_nonzero_caller_rva = 0xA00450;
                }
            }
        }
    }
    return res;
}

typedef void(__fastcall* set_pos_rot_fn)(Component_1_SceneRendering* comp1, Vector4* rot, Vector4* pos, uint8_t flag);
set_pos_rot_fn SetPosRotation_orig = nullptr;
set_pos_rot_fn SetPosRotationInternal_orig = nullptr;

static bool IsLocalPlayerOrWeaponEntity(EntityAdminBase* game_ea, Entity* ent) {
    if (!ent) return false;
    if (ent->hasComponent(0x2F) || ent->hasComponent(0x3F) || ent->hasComponent(0x1D))
        return true;
    if (ent->hasComponent(0x15)) {
        if (!game_ea || game_ea->local_entid == 0)
            return false;
        if (ent->entity_id == game_ea->local_entid)
            return true;
        if (auto* c1f = ent->getById<Component_1F_Backref>(0x1F)) {
            if (c1f->backref_entid != 0 && c1f->backref_entid == game_ea->local_entid)
                return true;
        }
        if (Entity* local_ctrl = game_ea->getLocalEnt()) {
            if (auto* pet = local_ctrl->getById<Component_20_ModelReference>(0x20)) {
                if (pet->aim_entid == ent->entity_id ||
                    pet->cam_attach_entid == ent->entity_id ||
                    pet->movement_attach_entid == ent->entity_id) {
                    return true;
                }
            }
        }
    }
    return false;
}

static bool HandleComponent1TransformHook(Component_1_SceneRendering* comp1, Vector4* rot, Vector4* pos, uintptr_t caller_rva, Vector4& out_rot, Vector4& out_pos) {
    if (!comp1 || !pos || !comp1->base.entity_backref)
        return false;
    Entity* ent = comp1->base.entity_backref;
    EntityAdminBase* game_ea = GameEntityAdmin();
    if (!game_ea || ent->entity_admin_backref != game_ea)
        return false;
    auto* slot = OriginClutterTracker::GetSlot(ent->entity_id, ent);
    if (!slot)
        return false;

    // 1. If culled by Heracles, ALWAYS lock both physics (+0xA04000) and visual render transform (+0xA045C0) to (0, -5000, 0)
    if (slot->culled_by_heracles) {
        out_pos = Vector4(0.0f, -5000.0f, 0.0f, 0.0f);
        out_rot = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
        return true;
    }

    float abs_sum = fabsf(pos->X) + fabsf(pos->Y) + fabsf(pos->Z);
    bool is_cull_pos = (pos->Y < -4000.0f);
    uintptr_t parent_c1 = *(uintptr_t*)((uintptr_t)comp1 + 0x1D0);

    // Never treat attached child entities (whose pos at +0x60 is a parent/bone-relative offset) as world-space clutter
    if (parent_c1 != 0)
        return false;

    // 2. Record valid non-zero world transforms
    if (!is_cull_pos && abs_sum > 0.5f) {
        if (!slot->has_nonzero_pos) {
            slot->has_nonzero_pos = true;
            slot->first_nonzero_pos = *pos;
            slot->first_nonzero_rot = rot ? *rot : comp1->rotation;
            slot->first_nonzero_caller_rva = caller_rva;
        }
        slot->latest_nonzero_pos = *pos;
        if (rot) slot->latest_nonzero_rot = *rot;
        slot->latest_nonzero_caller_rva = caller_rva;
        return false;
    }

    // 3. Intercept (0,0,0) overwrites (including +0xD06600 -> +0xA045C0 pre-render Component_12 ticks on non-local Enemy Bots!)
    if (!is_cull_pos && abs_sum < 0.05f) {
        slot->zero_set_count++;
        slot->last_zero_caller_rva = caller_rva;
        bool is_attach_call = (caller_rva >= 0xA01000 && caller_rva < 0xA01600);
        bool is_player_or_weapon_ent = IsLocalPlayerOrWeaponEntity(game_ea, ent);
        if (slot->has_nonzero_pos && parent_c1 == 0 && !is_attach_call && !is_player_or_weapon_ent) {
            slot->zero_overwrite_count++;
            slot->last_zero_overwrite_caller_rva = caller_rva;
            if (OriginClutterTracker::s_block_zero_overwrites) {
                slot->zero_overwrite_blocked_count++;
                OriginClutterTracker::s_total_zero_overwrites_blocked++;
                out_pos = slot->latest_nonzero_pos;
                bool has_valid_saved_rot = (slot->latest_nonzero_rot.W != 0.0f || slot->latest_nonzero_rot.X != 0.0f || slot->latest_nonzero_rot.Y != 0.0f || slot->latest_nonzero_rot.Z != 0.0f);
                out_rot = has_valid_saved_rot ? slot->latest_nonzero_rot : (rot ? *rot : Vector4(0.0f, 0.0f, 0.0f, 1.0f));
                return true;
            }
        }
    }
    return false;
}

static constexpr int WEAPON_DIAG_RING_SIZE = 256;
struct WeaponDiagEntry {
    uint32_t seq = 0;
    char text[380]{};
};
static WeaponDiagEntry s_weapon_diag_ring[WEAPON_DIAG_RING_SIZE]{};
static volatile LONG s_weapon_diag_seq = 0;

template<typename T>
static T SafeReadVal(uintptr_t addr, T fallback = T{}) {
    if (addr < 0x10000ULL) return fallback;
    __try {
        return *(const T*)addr;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return fallback;
    }
}

void WeaponDiagLogFmt(const char* fmt, ...) {
    LONG seq = InterlockedIncrement(&s_weapon_diag_seq);
    WeaponHardpointDiag::s_total_events_logged = (int)seq;
    WeaponDiagEntry& e = s_weapon_diag_ring[(seq - 1) & (WEAPON_DIAG_RING_SIZE - 1)];
    e.seq = (uint32_t)seq;
    va_list args;
    va_start(args, fmt);
    vsnprintf(e.text, sizeof(e.text) - 1, fmt, args);
    e.text[sizeof(e.text) - 1] = '\0';
    va_end(args);
}

// Dedicated ring for shot/raycast events so high-frequency ATTACH_INT spam (+0xA010D9) cannot evict them.
static constexpr int SHOT_DIAG_RING_SIZE = 256;
static WeaponDiagEntry s_shot_diag_ring[SHOT_DIAG_RING_SIZE]{};
static volatile LONG s_shot_diag_seq = 0;
static void ShotDiagLogFmt(const char* fmt, ...) {
    LONG seq = InterlockedIncrement(&s_shot_diag_seq);
    WeaponDiagEntry& e = s_shot_diag_ring[(seq - 1) & (SHOT_DIAG_RING_SIZE - 1)];
    e.seq = (uint32_t)seq;
    va_list args;
    va_start(args, fmt);
    vsnprintf(e.text, sizeof(e.text) - 1, fmt, args);
    e.text[sizeof(e.text) - 1] = '\0';
    va_end(args);
}

// ---- Projectile lifetime tracker (projectiles spawned by +0xD354B0, STU 0x400000000000018, Component_27) ----
// Snapshots Component_27 at spawn and every tick while alive; when the projectile disappears, logs its last
// position and every qword of Component_27 that changed between spawn and the last alive tick.
static constexpr int PROJ_TRACK_MAX = 4;
static constexpr size_t PROJ_C27_SNAP = 0xF00;
struct ProjTrack {
    bool active = false;
    uint32_t id = 0;
    Entity* ent = nullptr;
    uintptr_t c27 = 0;
    int ticks = 0;
    Vector4 spawn_pos{}, last_pos{};
    uint8_t snap0[PROJ_C27_SNAP]{};
    uint8_t snapL[PROJ_C27_SNAP]{};
};
static ProjTrack s_proj_track[PROJ_TRACK_MAX]{};
static int s_proj_track_next = 0;

static bool SafeMemcpy(void* dst, uintptr_t src, size_t n) {
    if (src < 0x10000ULL) return false;
    __try { memcpy(dst, (const void*)src, n); return true; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}

static Vector4 ReadEntPos(Entity* ent) {
    if (!ent) return {};
    auto* c1 = ent->getById<Component_1_SceneRendering>(1);
    return c1 ? SafeReadVal<Vector4>((uintptr_t)&c1->position) : Vector4{};
}

void WeaponHardpointDiag::TrackProjectile(Entity* proj) {
    if (!proj) return;
    ProjTrack& t = s_proj_track[s_proj_track_next];
    s_proj_track_next = (s_proj_track_next + 1) % PROJ_TRACK_MAX;
    if (t.active) ShotDiagLogFmt("[PROJ_EVICT] id=0x%X still alive after %d ticks (tracker full)", t.id, t.ticks);
    t = ProjTrack{};
    t.active = true;
    t.ent = proj;
    t.id = SafeReadVal<uint32_t>((uintptr_t)&proj->entity_id);
    ComponentBase* c27 = proj->getById(0x27);
    t.c27 = (uintptr_t)c27;
    if (c27) { SafeMemcpy(t.snap0, t.c27, PROJ_C27_SNAP); memcpy(t.snapL, t.snap0, PROJ_C27_SNAP); }
    t.spawn_pos = t.last_pos = ReadEntPos(proj);
    std::string comps;
    for (int i = 0; i < proj->component_list.num - 1 && i < 40; i++) {
        ComponentBase* c = proj->component_list.ptr[i];
        if (c) comps += std::format("{:02X},", (int)c->component_id);
    }
    uintptr_t vt = c27 ? SafeReadVal<uintptr_t>(t.c27) : 0;
    uintptr_t vt_rva = (vt >= globals::gameBase && vt < globals::gameBase + globals::gameSize) ? vt - globals::gameBase : vt;
    ShotDiagLogFmt("[PROJ_SPAWN] id=0x%X comps=[%s] c27=0x%llX c27_vt=+0x%llX E2A=%u pos=(%.2f,%.2f,%.2f)",
        t.id, comps.c_str(), (unsigned long long)t.c27, (unsigned long long)vt_rva,
        c27 ? SafeReadVal<uint8_t>(t.c27 + 0xE2A) : 0, t.spawn_pos.X, t.spawn_pos.Y, t.spawn_pos.Z);
}

static int s_pending_dummy_hit_dump_ticks = 0;
static const char* s_pending_dummy_dump_reason = "AUTO_POST_DUMMY_HIT_3TICKS";

static void PatchHeroAbilityGraphsAndVars(EntityAdminBase* game_ea);

void WeaponHardpointDiag::TickProjectileTracker(EntityAdminBase* game_ea) {
    if (!game_ea) return;
    PatchHeroAbilityGraphsAndVars(game_ea);
    if ((GetAsyncKeyState('Q') & 1) != 0) {
        statescript_logger::StartTrace(3000, "KEY_Q_PULSE_BOMB");
        ShotDiagLogFmt("[ABILITY_TRACE] Key 'Q' pressed -> tracing Statescript for 3000ms, auto-dump in 110 ticks");
        s_pending_dummy_dump_reason = "AUTO_POST_KEY_Q_110TICKS";
        s_pending_dummy_hit_dump_ticks = 110;
    }
    if ((GetAsyncKeyState('E') & 1) != 0) {
        statescript_logger::StartTrace(3000, "KEY_E_RECALL");
        ShotDiagLogFmt("[ABILITY_TRACE] Key 'E' pressed -> tracing Statescript for 3000ms, auto-dump in 45 ticks");
        s_pending_dummy_dump_reason = "AUTO_POST_KEY_E_45TICKS";
        s_pending_dummy_hit_dump_ticks = 45;
    }
    if ((GetAsyncKeyState(VK_F8) & 1) != 0) {
        WeaponHardpointDiag::DumpDummyCombatForensics(game_ea, "MANUAL_F8_KEY");
    }
    if (s_pending_dummy_hit_dump_ticks > 0) {
        if (--s_pending_dummy_hit_dump_ticks == 0) {
            WeaponHardpointDiag::DumpDummyCombatForensics(game_ea, s_pending_dummy_dump_reason);
            s_pending_dummy_dump_reason = "AUTO_POST_DUMMY_HIT_3TICKS";
        }
    }
    for (ProjTrack& t : s_proj_track) {
        if (!t.active) continue;
        Entity* ent = game_ea->getEntById(t.id);
        if (ent && ent == t.ent && t.c27 && ent->getById(0x27) == (ComponentBase*)t.c27) {
            t.ticks++;
            t.last_pos = ReadEntPos(ent);
            SafeMemcpy(t.snapL, t.c27, PROJ_C27_SNAP);
            if (t.ticks <= 3)
                ShotDiagLogFmt("[PROJ_TICK] id=0x%X tick=%d pos=(%.2f,%.2f,%.2f)", t.id, t.ticks, t.last_pos.X, t.last_pos.Y, t.last_pos.Z);
            continue;
        }
        // Gone: report last-alive state and Component_27 diff (spawn -> last alive tick).
        ShotDiagLogFmt("[PROJ_GONE] id=0x%X alive_ticks=%d spawn=(%.2f,%.2f,%.2f) last=(%.2f,%.2f,%.2f)",
            t.id, t.ticks, t.spawn_pos.X, t.spawn_pos.Y, t.spawn_pos.Z, t.last_pos.X, t.last_pos.Y, t.last_pos.Z);
        std::string line;
        int changed = 0, lines = 0;
        for (size_t off = 0x18; off + 8 <= PROJ_C27_SNAP && lines < 6; off += 8) {
            uint64_t a = *(uint64_t*)(t.snap0 + off), b = *(uint64_t*)(t.snapL + off);
            if (a == b) continue;
            changed++;
            line += std::format("+{:X}:{:X}->{:X} ", off, a, b);
            if (line.size() > 300) { ShotDiagLogFmt("[PROJ_DIFF] id=0x%X %s", t.id, line.c_str()); line.clear(); lines++; }
        }
        if (!line.empty() && lines < 6) ShotDiagLogFmt("[PROJ_DIFF] id=0x%X %s", t.id, line.c_str());
        if (changed == 0) ShotDiagLogFmt("[PROJ_DIFF] id=0x%X no Component_27 qwords changed", t.id);
        t.active = false;
    }
}

static bool Has1PViewModelEntity(uintptr_t weapon_mgr) {
    if (!weapon_mgr) return false;
    uintptr_t comp23 = SafeReadVal<uintptr_t>(weapon_mgr);
    if (!comp23) return false;
    Entity* owner = SafeReadVal<Entity*>(comp23 + 0x08);
    if (!owner || !owner->component_list.ptr) return false;
    for (int i = 0; i < owner->component_list.num - 1; i++) {
        ComponentBase* comp = owner->component_list.ptr[i];
        if (comp && comp->component_id == 0x22) {
            return SafeReadVal<uintptr_t>((uintptr_t)comp + 0x20) != 0;
        }
    }
    return false;
}

typedef void(__fastcall* update_weapon_attachments_fn)(uintptr_t weapon_mgr, uint8_t is_1p);
update_weapon_attachments_fn UpdateWeaponAttachments_orig = nullptr;
void __fastcall UpdateWeaponAttachments_hook(uintptr_t weapon_mgr, uint8_t is_1p) {
    uintptr_t caller = (uintptr_t)_ReturnAddress();
    uintptr_t caller_rva = (caller >= globals::gameBase && caller < globals::gameBase + globals::gameSize)
        ? (caller - globals::gameBase) : caller;
    bool has_1p = Has1PViewModelEntity(weapon_mgr);
    uint32 owner_entid = 0;
    int32_t entries_cnt = 0;
    if (weapon_mgr) {
        uintptr_t comp23 = SafeReadVal<uintptr_t>(weapon_mgr);
        if (comp23) {
            Entity* owner = SafeReadVal<Entity*>(comp23 + 0x08);
            if (owner) owner_entid = SafeReadVal<uint32>((uintptr_t)&owner->entity_id);
        }
        entries_cnt = SafeReadVal<int32_t>(weapon_mgr + 0x10);
    }
    WeaponHardpointDiag::s_update_weapon_calls++;
    UpdateWeaponAttachments_orig(weapon_mgr, is_1p);

    uint32 slot0_0 = 0, slot1_0 = 0;
    uint64 handle_0 = 0;
    if (weapon_mgr && entries_cnt > 0 && entries_cnt < 64) {
        uintptr_t entries_ptr = SafeReadVal<uintptr_t>(weapon_mgr + 0x08);
        if (entries_ptr) {
            slot0_0 = SafeReadVal<uint32>(entries_ptr + 0x00);
            slot1_0 = SafeReadVal<uint32>(entries_ptr + 0x04);
            handle_0 = SafeReadVal<uint64>(entries_ptr + 0x168);
        }
    }
    WeaponDiagLogFmt("[UPDATE_WEAPONS (+0xD563D0)] caller=+0x%llX owner=0x%X is_1p=%u has_1p_ent=%d cnt=%d e0:(h=0x%llX s0=0x%X s1=0x%X)",
        (unsigned long long)caller_rva, owner_entid, is_1p, has_1p ? 1 : 0, entries_cnt,
        (unsigned long long)handle_0, slot0_0, slot1_0);
}

typedef uint32*(__fastcall* resolve_weapon_or_parent_fn)(uintptr_t weapon_mgr, uint32* out_entid, uint64 weapon_handle, int slot_index, int fallback_to_3p, int fallback_to_owner, void* out_ea);
resolve_weapon_or_parent_fn ResolveWeaponOrParentEntity_orig = nullptr;
uint32* __fastcall ResolveWeaponOrParentEntity_hook(uintptr_t weapon_mgr, uint32* out_entid, uint64 weapon_handle, int slot_index, int fallback_to_3p, int fallback_to_owner, void* out_ea) {
    uintptr_t caller = (uintptr_t)_ReturnAddress();
    uintptr_t caller_rva = (caller >= globals::gameBase && caller < globals::gameBase + globals::gameSize)
        ? (caller - globals::gameBase) : caller;
    WeaponHardpointDiag::s_resolve_weapon_calls++;
    uint32* res = ResolveWeaponOrParentEntity_orig(weapon_mgr, out_entid, weapon_handle, slot_index, fallback_to_3p, fallback_to_owner, out_ea);
    uint32 resolved_id = out_entid ? SafeReadVal<uint32>((uintptr_t)out_entid) : 0;
    uint32 owner_entid = 0;
    int32_t entries_cnt = 0;
    if (weapon_mgr) {
        uintptr_t comp23 = SafeReadVal<uintptr_t>(weapon_mgr);
        if (comp23) {
            Entity* owner = SafeReadVal<Entity*>(comp23 + 0x08);
            if (owner) owner_entid = SafeReadVal<uint32>((uintptr_t)&owner->entity_id);
        }
        entries_cnt = SafeReadVal<int32_t>(weapon_mgr + 0x10);
    }
    WeaponDiagLogFmt("[RESOLVE_WEAPON (+0xD560B0)] caller=+0x%llX owner=0x%X handle=0x%llX slot=%d fb3p=%d fbOwn=%d cnt=%d -> out_ent=0x%X",
        (unsigned long long)caller_rva, owner_entid, (unsigned long long)weapon_handle,
        slot_index, fallback_to_3p, fallback_to_owner, entries_cnt, resolved_id);
    return res;
}

typedef void(__fastcall* spawn_weapon_fire_effect_fn)(uintptr_t action_node, uint32_t shot_idx);
spawn_weapon_fire_effect_fn SpawnWeaponFireEffect_orig = nullptr;
void __fastcall SpawnWeaponFireEffect_hook(uintptr_t action_node, uint32_t shot_idx) {
    uintptr_t caller = (uintptr_t)_ReturnAddress();
    uintptr_t caller_rva = (caller >= globals::gameBase && caller < globals::gameBase + globals::gameSize)
        ? (caller - globals::gameBase) : caller;
    ++WeaponHardpointDiag::s_fire_shot_calls;
    if (action_node) {
        int32_t barrel_cnt = SafeReadVal<int32_t>(action_node + 0x1C8);
        uintptr_t barrels = SafeReadVal<uintptr_t>(action_node + 0x1C0);
        if (barrel_cnt > 0 && barrels != 0) {
            uint32_t muzzle_ent = SafeReadVal<uint32_t>(barrels + 0x40, 0xFFFFFFFF);
            if (muzzle_ent == 0xFFFFFFFF) {
                ((void(__fastcall*)(uintptr_t, uint8_t))(globals::gameBase + 0xd47070))(action_node, 1);
            }
        }
    }
    int32_t cnt_1C8 = SafeReadVal<int32_t>(action_node + 0x1C8, -1);
    int32_t div_14C = SafeReadVal<int32_t>(action_node + 0x14C, -1);
    WeaponDiagLogFmt("[FIRE_SHOT_BEGIN (+0xD46060)] caller=+0x%llX node=0x%llX shot_idx=%u cnt_1C8=%d div_14C=%d",
        (unsigned long long)caller_rva, (unsigned long long)action_node, shot_idx, cnt_1C8, div_14C);
    SpawnWeaponFireEffect_orig(action_node, shot_idx);
}

typedef uint8_t(__fastcall* spawn_ss_effect_fn)(uintptr_t effect_node, uintptr_t ss_inst, uintptr_t a3);
spawn_ss_effect_fn SpawnStatescriptEffect_orig = nullptr;
uint8_t __fastcall SpawnStatescriptEffect_hook(uintptr_t effect_node, uintptr_t ss_inst, uintptr_t a3) {
    uintptr_t caller = (uintptr_t)_ReturnAddress();
    uintptr_t caller_rva = (caller >= globals::gameBase && caller < globals::gameBase + globals::gameSize)
        ? (caller - globals::gameBase) : caller;
    ++WeaponHardpointDiag::s_ss_effect_calls;
    uint8_t ret = SpawnStatescriptEffect_orig(effect_node, ss_inst, a3);
    WeaponDiagLogFmt("[SS_EFFECT (+0xD58110)] caller=+0x%llX node=0x%llX ss_inst=0x%llX a3=0x%llX -> ret=%u",
        (unsigned long long)caller_rva, (unsigned long long)effect_node, (unsigned long long)ss_inst, (unsigned long long)a3, ret);
    return ret;
}

typedef uint8_t(__fastcall* attach_c1_hp_fn)(Component_1_SceneRendering* child_c1, Component_1_SceneRendering* parent_c1, uint64_t hp_id, Vector4* rot, Vector4* pos);
attach_c1_hp_fn AttachComponent1ToHardpoint_orig = nullptr;
uint8_t __fastcall AttachComponent1ToHardpoint_hook(Component_1_SceneRendering* child_c1, Component_1_SceneRendering* parent_c1, uint64_t hp_id, Vector4* rot, Vector4* pos) {
    uintptr_t caller = (uintptr_t)_ReturnAddress();
    uintptr_t caller_rva = (caller >= globals::gameBase && caller < globals::gameBase + globals::gameSize)
        ? (caller - globals::gameBase) : caller;
    Entity* c_ent = child_c1 ? SafeReadVal<Entity*>((uintptr_t)&child_c1->base.entity_backref) : nullptr;
    Entity* p_ent = parent_c1 ? SafeReadVal<Entity*>((uintptr_t)&parent_c1->base.entity_backref) : nullptr;
    uint32 child_id = c_ent ? SafeReadVal<uint32>((uintptr_t)&c_ent->entity_id) : 0;
    uint32 parent_id = p_ent ? SafeReadVal<uint32>((uintptr_t)&p_ent->entity_id) : 0;
    uint8_t ret = AttachComponent1ToHardpoint_orig(child_c1, parent_c1, hp_id, rot, pos);
    uint16_t bone_idx = SafeReadVal<uint16_t>((uintptr_t)child_c1 + 0x1F4, 0xFFFF);
    Vector4 c_world = SafeReadVal<Vector4>((uintptr_t)child_c1 + 0xC0);
    WeaponDiagLogFmt("[ATTACH_HP (+0xA01160)] caller=+0x%llX child=0x%X parent=0x%X hp=0x%llX -> bone=%u world=(%.2f,%.2f,%.2f)",
        (unsigned long long)caller_rva, child_id, parent_id, (unsigned long long)hp_id, bone_idx, c_world.X, c_world.Y, c_world.Z);
    return ret;
}

typedef uint8_t(__fastcall* attach_c1_internal_fn)(Component_1_SceneRendering* child_c1, Component_1_SceneRendering* parent_c1, uintptr_t hp_info, Vector4* rot, Vector4* pos);
attach_c1_internal_fn AttachComponent1Internal_orig = nullptr;
uint8_t __fastcall AttachComponent1Internal_hook(Component_1_SceneRendering* child_c1, Component_1_SceneRendering* parent_c1, uintptr_t hp_info, Vector4* rot, Vector4* pos) {
    uintptr_t caller = (uintptr_t)_ReturnAddress();
    uintptr_t caller_rva = (caller >= globals::gameBase && caller < globals::gameBase + globals::gameSize)
        ? (caller - globals::gameBase) : caller;
    WeaponHardpointDiag::s_attach_calls++;
    uint8_t ret = AttachComponent1Internal_orig(child_c1, parent_c1, hp_info, rot, pos);

    Entity* c_ent = child_c1 ? SafeReadVal<Entity*>((uintptr_t)&child_c1->base.entity_backref) : nullptr;
    Entity* p_ent = parent_c1 ? SafeReadVal<Entity*>((uintptr_t)&parent_c1->base.entity_backref) : nullptr;
    uint32 child_id = c_ent ? SafeReadVal<uint32>((uintptr_t)&c_ent->entity_id) : 0;
    uint32 parent_id = p_ent ? SafeReadVal<uint32>((uintptr_t)&p_ent->entity_id) : 0;
    if (parent_id != 0 && GameEntityAdmin() && GameEntityAdmin()->local_entid != 0) {
        uint64_t hp_id = SafeReadVal<uint64_t>((uintptr_t)child_c1 + 0x1E8);
        uint16_t bone_idx = SafeReadVal<uint16_t>((uintptr_t)child_c1 + 0x1F4, 0xFFFF);
        uint8_t inh_rot = SafeReadVal<uint8_t>((uintptr_t)child_c1 + 0x1F6);
        uint8_t maint_w = SafeReadVal<uint8_t>((uintptr_t)child_c1 + 0x1F7);
        Vector4 c_world = SafeReadVal<Vector4>((uintptr_t)child_c1 + 0x60);
        Vector4 p_world = SafeReadVal<Vector4>((uintptr_t)parent_c1 + 0x60);
        uintptr_t p_1a8 = SafeReadVal<uintptr_t>((uintptr_t)parent_c1 + 0x1A8);
        uint64_t p_1b8 = SafeReadVal<uint64_t>((uintptr_t)parent_c1 + 0x1B8);
        WeaponDiagLogFmt("[ATTACH_INT (+0xA01430)] caller=+0x%llX child=0x%X parent=0x%X hp=0x%llX bone=%u inh=%u mw=%u p1A8=0x%llX p1B8=0x%llX cW=(%.2f,%.2f,%.2f) pW=(%.2f,%.2f,%.2f)",
            (unsigned long long)caller_rva, child_id, parent_id, (unsigned long long)hp_id, bone_idx, inh_rot, maint_w,
            (unsigned long long)p_1a8, (unsigned long long)p_1b8,
            c_world.X, c_world.Y, c_world.Z, p_world.X, p_world.Y, p_world.Z);
    }
    return ret;
}

typedef uint8_t(__fastcall* try_play_barrel_muzzle_fn)(uintptr_t entry, uintptr_t game_ea);
try_play_barrel_muzzle_fn TryPlayBarrelMuzzleFlash_orig = nullptr;

// ---- Hitscan shot-queue processing system (+0xC34BE0) ----
// Queued shots (0x180-byte records at sys+0x20, count sys+0x28) pack the firing StatescriptInstance id into 13 bits
// ((handle >> 19) & 0x1FFF) and compare it at +0xC34E45 against `word ptr [g3_arrWithIds + 8]` of the shooter's
// Component_23 (comp23+0x1E0/+0x1E8). Client-loaded weapon scripts have negative ids (0xFFE8) -> 0x1FE8 != 0xFFE8,
// so the instance lookup fails and the hit is discarded (bullets pass through targets).
// Fix: transiently mask the local player's negative g3 ids to 13 bits across orig(), restore immediately after.
// Master switch for the downstream "teleport pellet onto dummy" experiments (2026-10-22). These fake the
// impact pipeline instead of fixing native hit detection, never produced a visible result, and the +0x168
// clearing crashed the game. Kept compiled-out for reference only. See CURRENT_TASK_STATE.md.
static constexpr bool kEnablePelletRedirectHacks = false;

typedef uint64_t(__fastcall* process_shot_queue_fn)(uintptr_t sys, uintptr_t a2, uintptr_t a3, uintptr_t a4);
process_shot_queue_fn ProcessShotQueue_orig = nullptr;
uint64_t __fastcall ProcessShotQueue_hook(uintptr_t sys, uintptr_t a2, uintptr_t a3, uintptr_t a4) {
    struct PatchedG3Slot {
        uintptr_t id_addr;
        uint16_t orig_id;
        uintptr_t ss_inst;
    };
    PatchedG3Slot patched[32]{};
    int patched_cnt = 0;

    EntityAdminBase* ea = GameEntityAdmin();
    if (ea && ea->local_entid) {
        Entity* ctrl = ea->getEntById(ea->local_entid);
        auto* c20 = ctrl ? ctrl->getById<Component_20_ModelReference>(0x20) : nullptr;
        uint32_t cand_ids[2] = {
            c20 ? c20->cam_attach_entid : 0,
            c20 ? c20->aim_entid : 0
        };
        for (uint32_t cid : cand_ids) {
            if (!cid || cid == 0xFFFFFFFF) continue;
            Entity* shooter = ea->getEntById(cid);
            ComponentBase* c23 = shooter ? shooter->getById(0x23) : nullptr;
            if (!c23) continue;
            uintptr_t g3_ptr = SafeReadVal<uintptr_t>((uintptr_t)c23 + 0x1E0);
            int32_t g3_cnt = SafeReadVal<int32_t>((uintptr_t)c23 + 0x1E8);
            if (g3_ptr && g3_cnt > 0 && g3_cnt < 256) {
                for (int32_t i = 0; i < g3_cnt && patched_cnt < 32; i++) {
                    uintptr_t slot_addr = g3_ptr + (uintptr_t)i * 0x10;
                    uint16_t raw_id = SafeReadVal<uint16_t>(slot_addr + 8);
                    if ((raw_id & 0x8000) != 0) {
                        patched[patched_cnt].id_addr = slot_addr + 8;
                        patched[patched_cnt].orig_id = raw_id;
                        patched[patched_cnt].ss_inst = SafeReadVal<uintptr_t>(slot_addr);
                        *(uint16_t*)(slot_addr + 8) = (uint16_t)(raw_id & 0x1FFF);
                        patched_cnt++;
                    }
                }
            }
        }
    }

    uint64_t ret = ProcessShotQueue_orig(sys, a2, a3, a4);

    for (int i = 0; i < patched_cnt; i++) {
        *(uint16_t*)patched[i].id_addr = patched[i].orig_id;
        // DISABLED (2026-10-22 18:50 crash, VEH loop from +0xC34AA7): blindly zeroing state_impl + 0x168 on every
        // heuristically-matching state node corrupts unrelated Statescript state machines. Do not re-enable.
        // Clear any accumulated client-predicted pellet IDs in state_impl + 0x168 so subsequent shots
        // always spawn fresh pellets instead of reusing Shot #1's already-impacted pellet entities.
        uintptr_t ss_inst = kEnablePelletRedirectHacks ? patched[i].ss_inst : 0;
        if (ss_inst) {
            uint32_t impl_cnt = SafeReadVal<uint32_t>(ss_inst + 0xA8);
            uintptr_t impl_arr = SafeReadVal<uintptr_t>(ss_inst + 0xA0);
            if (impl_arr && impl_cnt > 0 && impl_cnt < 512) {
                for (uint32_t s = 0; s < impl_cnt; s++) {
                    uintptr_t node = SafeReadVal<uintptr_t>(impl_arr + (uintptr_t)s * 8);
                    if (!node) continue;
                    uintptr_t arr_160 = SafeReadVal<uintptr_t>(node + 0x160);
                    int32_t cnt_168 = SafeReadVal<int32_t>(node + 0x168);
                    int32_t cap_16c = SafeReadVal<int32_t>(node + 0x16C);
                    if (arr_160 && cnt_168 > 0 && cnt_168 <= 16 && cap_16c >= cnt_168 && cap_16c <= 64) {
                        uint32_t first_ent = SafeReadVal<uint32_t>(arr_160);
                        if ((first_ent & 0xFF000000U) == 0x80000000U) {
                            *(int32_t*)(node + 0x168) = 0;
                        }
                    }
                }
            }
        }
    }
    return ret;
}
uint8_t __fastcall TryPlayBarrelMuzzleFlash_hook(uintptr_t entry, uintptr_t game_ea) {
    uint64_t handle = entry ? SafeReadVal<uint64_t>(entry + 0xD8) : 0;
    uint32_t e4 = entry ? SafeReadVal<uint32_t>(entry + 0xE4) : 0;
    uint32_t f4 = entry ? SafeReadVal<uint32_t>(entry + 0xF4) : 0;
    uint32_t f8 = entry ? SafeReadVal<uint32_t>(entry + 0xF8) : 0;
    uint32_t state_idx = (uint32_t)((handle >> 8) & 0x7FF);
    uint16_t inst_id_13 = (uint16_t)((handle >> 19) & 0x1FFF);

    uintptr_t patched_id_addr = 0;
    uint16_t orig_raw_id = 0;
    uintptr_t matched_node = 0;
    uint32_t b0_ent = 0, b1_ent = 0;

    if (entry && game_ea && handle != 0 && e4 != 0 && e4 != 0xFFFFFFFF) {
        Entity* shooter = ((EntityAdminBase*)game_ea)->getEntById(e4);
        ComponentBase* c23 = shooter ? shooter->getById(0x23) : nullptr;
        if (c23) {
            uintptr_t g3_ptr = SafeReadVal<uintptr_t>((uintptr_t)c23 + 0x1E0);
            int32_t g3_cnt = SafeReadVal<int32_t>((uintptr_t)c23 + 0x1E8);
            if (g3_ptr && g3_cnt > 0 && g3_cnt < 256) {
                for (int32_t i = 0; i < g3_cnt; i++) {
                    uintptr_t slot_addr = g3_ptr + (uintptr_t)i * 0x10;
                    uint16_t raw_id = SafeReadVal<uint16_t>(slot_addr + 8);
                    if ((raw_id & 0x1FFF) == inst_id_13) {
                        orig_raw_id = raw_id;
                        if (raw_id != inst_id_13) {
                            patched_id_addr = slot_addr + 8;
                            *(uint16_t*)patched_id_addr = inst_id_13;
                        }
                        uintptr_t ss_inst = SafeReadVal<uintptr_t>(slot_addr);
                        if (ss_inst) {
                            uint32_t impl_cnt = SafeReadVal<uint32_t>(ss_inst + 0xA8);
                            uintptr_t impl_arr = SafeReadVal<uintptr_t>(ss_inst + 0xA0);
                            if (impl_arr && state_idx < impl_cnt) {
                                matched_node = SafeReadVal<uintptr_t>(impl_arr + (uintptr_t)state_idx * 8);
                                if (matched_node) {
                                    int32_t b_cnt = SafeReadVal<int32_t>(matched_node + 0x1C8);
                                    uintptr_t b_arr = SafeReadVal<uintptr_t>(matched_node + 0x1C0);
                                    if (b_arr && b_cnt > 0) {
                                        b0_ent = SafeReadVal<uint32_t>(b_arr + 0x40);
                                        if (b_cnt > 1) b1_ent = SafeReadVal<uint32_t>(b_arr + 0x50 + 0x40);
                                    }
                                }
                            }
                        }
                        break;
                    }
                }
            }
        }
    }

    uint8_t ret = TryPlayBarrelMuzzleFlash_orig(entry, game_ea);

    if (patched_id_addr) {
        *(uint16_t*)patched_id_addr = orig_raw_id;
    }

    WeaponDiagLogFmt("[TRY_BARREL_MUZZLE (+0xCBA3F0)] handle=0x%llX e4=0x%X f4=%u f8=%u st=%u id13=0x%X raw_id=0x%X node=0x%llX b0=0x%X b1=0x%X -> ret=%u",
        (unsigned long long)handle, e4, f4, f8, state_idx, inst_id_13, orig_raw_id,
        (unsigned long long)matched_node, b0_ent, b1_ent, ret);
    return ret;
}

typedef uintptr_t(__fastcall* process_muzzle_flash_entry_fn)(uintptr_t entry, uintptr_t game_ea);
process_muzzle_flash_entry_fn ProcessMuzzleFlashEntry_orig = nullptr;
uintptr_t __fastcall ProcessMuzzleFlashEntry_hook(uintptr_t entry, uintptr_t game_ea) {
    uint64_t shot_node_handle = 0;
    uint32_t shooter_entid = 0;
    uint32_t orig_e4 = 0;
    uint8_t orig_106 = 0;
    if (entry) {
        shot_node_handle = SafeReadVal<uint64_t>(entry + 0xD8);
        shooter_entid = SafeReadVal<uint32_t>(entry + 0xE0);
        orig_e4 = SafeReadVal<uint32_t>(entry + 0xE4);
        orig_106 = SafeReadVal<uint8_t>(entry + 0x106);
        if (shot_node_handle != 0) {
            if (shooter_entid != 0 && shooter_entid != 0xFFFFFFFF) {
                *(uint32_t*)(entry + 0xE4) = shooter_entid;
            }
            *(uint8_t*)(entry + 0x106) &= ~0x40;
        }
    }
    uintptr_t ret = ProcessMuzzleFlashEntry_orig(entry, game_ea);
    if (entry) {
        WeaponDiagLogFmt("[MUZZLE_ENTRY (+0xCBA6D0)] handle=0x%llX e0=0x%X e4_orig=0x%X e4_new=0x%X f4=%u f8=%u flg_orig=0x%02X -> ret_ent=0x%llX",
            (unsigned long long)shot_node_handle, shooter_entid, orig_e4,
            SafeReadVal<uint32_t>(entry + 0xE4),
            SafeReadVal<uint32_t>(entry + 0xF4), SafeReadVal<uint32_t>(entry + 0xF8),
            orig_106, (unsigned long long)ret);
    }
    return ret;
}

// ---- Offline hitscan damage authority for enemy dummies ----
// In 0.8.0 the client only queues shot claims (+0xD45240 -> +0xD455E0 -> +0xC34BE0); damage to Component_28 is applied
// by the (non-existent offline) server. For each pellet the local player fires, ray-test every non-local, other-team
// hero model (Component_15 + Component_28) against a vertical capsule and apply damage to its health pools.
static float Dot3(const Vector4& a, const Vector4& b) { return a.X * b.X + a.Y * b.Y + a.Z * b.Z; }
static Vector4 Sub3(const Vector4& a, const Vector4& b) { Vector4 r{}; r.X = a.X - b.X; r.Y = a.Y - b.Y; r.Z = a.Z - b.Z; return r; }
static Vector4 MulAdd3(const Vector4& a, const Vector4& d, float t) { Vector4 r{}; r.X = a.X + d.X * t; r.Y = a.Y + d.Y * t; r.Z = a.Z + d.Z * t; return r; }

// Closest points between segments P1Q1 / P2Q2 (Ericson, RTCD 5.1.9). Returns squared distance; s/t are params.
static float ClosestSegSeg(const Vector4& p1, const Vector4& q1, const Vector4& p2, const Vector4& q2, float& s, float& t) {
    Vector4 d1 = Sub3(q1, p1), d2 = Sub3(q2, p2), r = Sub3(p1, p2);
    float a = Dot3(d1, d1), e = Dot3(d2, d2), f = Dot3(d2, r);
    auto clamp01 = [](float v) { return v < 0.f ? 0.f : (v > 1.f ? 1.f : v); };
    if (a <= 1e-6f && e <= 1e-6f) { s = t = 0.f; return Dot3(r, r); }
    if (a <= 1e-6f) { s = 0.f; t = clamp01(f / e); }
    else {
        float c = Dot3(d1, r);
        if (e <= 1e-6f) { t = 0.f; s = clamp01(-c / a); }
        else {
            float b = Dot3(d1, d2), denom = a * e - b * b;
            s = (denom != 0.f) ? clamp01((b * f - c * e) / denom) : 0.f;
            t = (b * s + f) / e;
            if (t < 0.f) { t = 0.f; s = clamp01(-c / a); }
            else if (t > 1.f) { t = 1.f; s = clamp01((b - c) / a); }
        }
    }
    Vector4 c1 = MulAdd3(p1, d1, s), c2 = MulAdd3(p2, d2, t);
    Vector4 diff = Sub3(c1, c2);
    return Dot3(diff, diff);
}

static float ResolveShotBaseDamage(Entity* shooter, Entity* proj_ent);
static float ApplyHealingToHealthComponent(Component_28_STUHealthComponent* c28, float heal_amt);

static float ApplyDamageToHealthComponent(Component_28_STUHealthComponent* c28, float dmg) {
    // Shields -> armour -> health, part by part.
    HealthPart* pools[3] = { c28->shield_health, c28->armour_health, c28->normal_health };
    for (int p_idx = 0; p_idx < 3; p_idx++) {
        HealthPart* pool = pools[p_idx];
        bool is_armour = (p_idx == 1);
        for (int i = 0; i < 0x10 && dmg > 0.f; i++) {
            if (!pool[i].part_enabled || pool[i].curr_health <= 0.f) continue;
            // Overwatch 1 armor mitigation: reduces damage by half, up to a maximum reduction of 5.0f per hit
            float eff_dmg = is_armour ? (dmg - ((dmg * 0.5f < 5.0f) ? (dmg * 0.5f) : 5.0f)) : dmg;
            if (eff_dmg <= 0.f) break;
            if (pool[i].curr_health >= eff_dmg) {
                pool[i].curr_health -= eff_dmg;
                dmg = 0.f;
            } else {
                float absorbed_ratio = pool[i].curr_health / eff_dmg;
                pool[i].curr_health = 0.f;
                dmg *= (1.0f - absorbed_ratio);
            }
        }
    }
    return c28->CalcTotalCurrentHealth();
}

// Synchronizes Bit 16 (0x10000) on Component_01 + 0x1B8 for all enemy hero models:
// - Alive enemies (curr_hp > 0.0f) get Bit 16 set so +0x10830C0 Loop 1 (+0x1083B68: bt rax, 10h) includes them in
//   skeletal hitbox candidates (even on Shot #1 when +0x1032CB0 pre-computes [pellet_c27 + 0x228] before +0x1038860).
// - Dead ragdoll corpses (curr_hp <= 0.0f) get Bit 16 cleared so bullets pass right through corpses without triggering
//   live hitmarkers or hurt audio (Phase 7 Stage 2A).
static void SyncEnemyHitboxFlags(EntityAdminBase* ea) {
    if (!ea || !ea->local_entid) return;
    uint32_t local_ctrl_id = ea->local_entid;
    uint32_t local_mdl_id = 0;
    if (Entity* local_ctrl = ea->getLocalEnt()) {
        if (auto* pet = local_ctrl->getById<Component_20_ModelReference>(0x20)) {
            local_mdl_id = pet->cam_attach_entid ? pet->cam_attach_entid : (pet->aim_entid ? pet->aim_entid : pet->movement_attach_entid);
        }
    }
    for (Entity* ent : *ea) {
        if (!ent || ent->entity_id == local_ctrl_id || ent->entity_id == local_mdl_id) continue;
        if (!ent->hasComponent(0x15) || !ent->hasComponent(0x28)) continue;
        auto* c1 = ent->getById<Component_1_SceneRendering>(1);
        auto* c28 = ent->getById<Component_28_STUHealthComponent>(0x28);
        if (!c1 || !c28) continue;
        float curr_hp = c28->CalcTotalCurrentHealth();
        if (curr_hp > 0.0f) {
            *(uint32_t*)((uintptr_t)c1 + 0x1B8) |= 0x10000u;
        } else {
            *(uint32_t*)((uintptr_t)c1 + 0x1B8) &= ~0x10000u;
        }
    }
}

static void AppendHeroEntityForensics(std::string& out, const char* label, Entity* ctrl_ent, Entity* mdl_ent);
static std::string s_alive_hit_snapshot;
static Vector4 s_last_shot_orig{};
static uint32_t s_last_shot_shooter_id = 0;
static uint64_t s_last_shot_team = 0;
static uint64_t s_last_shot_tick_ms = 0;
static uint32_t s_last_hit_dummy_entid = 0;
static uint64_t s_last_hit_dummy_ms = 0;
static Component_1_SceneRendering* s_redirect_proj_c1 = nullptr;
static Component_1_SceneRendering* s_redirect_dummy_c1 = nullptr;
static Vector4 s_redirect_hit_pos{};

static bool NotifyVarInBag(StatescriptSyncMgr* ss_inner, StatescriptInstance* script, StatescriptVar_VarBag* vb, __int64 var_id, StatescriptPrimitive new_val) {
    if (!vb || IsBadReadPtr(vb, sizeof(StatescriptVar_VarBag))) return false;
    for (int i = 0; i < 16; i++) {
        auto* item = &vb->sub_items[i];
        if (!item || !item->arr_ptr || IsBadReadPtr(item->arr_ptr, sizeof(void*))) continue;
        for (int j = 0; j < item->item_count; j++) {
            StatescriptVar_Base* rid = item->arr_ptr[j].ptr;
            if (!rid || IsBadReadPtr(rid, sizeof(StatescriptVar_Base))) continue;
            if (rid->var_id == var_id && rid->is_cvd()) {
                auto* prim = (StatescriptVar_Primitive*)rid;
                StatescriptSyncMgr* mgr = (prim->base.parent_rid && prim->base.parent_rid->ss_compo_inner)
                    ? prim->base.parent_rid->ss_compo_inner : ss_inner;
                StatescriptInstance* inst = (prim->base.parent_rid && prim->base.parent_rid->ss_instance)
                    ? prim->base.parent_rid->ss_instance : script;
                prim->base.stu_base.vfptr->Primitive_SetValueFromRemote(
                    &prim->base, mgr, inst, &new_val, &prim->base.user, 1);
                return true;
            }
        }
    }
    return false;
}

static void SyncEntityHealthVarBag(Entity* ent) {
    auto* ss = ent->getById<Component_23_Statescript>(0x23);
    auto* c28 = ent->getById<Component_28_STUHealthComponent>(0x28);
    if (!ss || !c28 || !ss->ss_inner.rid_entity_varbag) return;
    StatescriptPrimitive cv{};
    cv.type = StatescriptPrimitive_FLT;
    *(float*)&cv.value = c28->CalcTotalCurrentHealth();
    ss->ss_inner.rid_entity_varbag->SetVar({ 0x0D800000000006C4 }, cv);
    ss->ss_inner.rid_entity_varbag->SetVar({ StatescriptVar_ENTITY_VARBAG, 0x0D800000000006C4 }, cv);
    NotifyVarInBag(&ss->ss_inner, nullptr, ss->ss_inner.rid_entity_varbag, 0x0D800000000006C4LL, cv);

    StatescriptPrimitive max_cv{};
    max_cv.type = StatescriptPrimitive_FLT;
    *(float*)&max_cv.value = c28->CalcTotalMaxHealth();
    ss->ss_inner.rid_entity_varbag->SetVar({ 0x0D800000000006C9 }, max_cv);
    ss->ss_inner.rid_entity_varbag->SetVar({ StatescriptVar_ENTITY_VARBAG, 0x0D800000000006C9 }, max_cv);
    NotifyVarInBag(&ss->ss_inner, nullptr, ss->ss_inner.rid_entity_varbag, 0x0D800000000006C9LL, max_cv);
    ss->ss_inner.needs_process_e = 1;
}

// ============================================================================
// Phase 7 Stage 3B: Native Component_23 VarBag Getters, Setters & Queue Drain
// Verified from +0x103B800..+0x103BE20, +0xF45D90, +0xF45E40, +0xF64BB0 (No 'MZ' traps)
// ============================================================================
#pragma pack(push, 1)
struct VarBagKeyPath {
    uint16_t scope_inst_id; // +0x00: 0x0000 for entity VarBag (ss_core + 0x2E0)
    uint16_t bag_id_u16;    // +0x02: 0x0004 (0x0D80000000000004 = rid_entity_varbag)
    uint16_t var_id_u16;    // +0x04: low 16 bits of 0x0D80... variable ID
    uint16_t pad_06;        // +0x06: 0x0000
};
static_assert(sizeof(VarBagKeyPath) == 0x08, "VarBagKeyPath must be 8 bytes");

struct StatescriptVariant {
    union {
        uint8_t       b_val;
        int32_t       i32_val;
        float         f32_val;
        uint64_t      u64_val;
        void*         ptr_val;
        VarBagKeyPath redirect_path;
    };
    uint32_t aux_08;
    uint32_t type; // 1 = bool, 2 = int32, 3 = float, 4 = res_handle, 5 = heap, 7 = redirect
};
static_assert(sizeof(StatescriptVariant) == 0x10, "StatescriptVariant must be 16 bytes");

struct StatescriptSetVarCmd {
    uintptr_t             vtable_or_hdr_00;
    StatescriptSetVarCmd* next_08;
    StatescriptSetVarCmd* prev_10;
    uint64_t              timestamp_18;
    uint16_t              scope_inst_id_20;
    uint8_t               cmd_type_22;
    uint8_t               pad_23[0x05];
    uint8_t               flags_28;
    uint8_t               pad_29[0x07];
    VarBagKeyPath         path_30;
    uint64_t              field_38;
    StatescriptVariant    value_40;
};
static_assert(sizeof(StatescriptSetVarCmd) == 0x50, "StatescriptSetVarCmd must be 0x50 bytes");
#pragma pack(pop)

static inline int32_t GetEntityVarBagInt32(Component_23_Statescript* c23, uint64_t var_id_0D80) {
    if (!c23) return 0;
    __try {
        VarBagKeyPath path{ 0x0000, 0x0004, static_cast<uint16_t>(var_id_0D80 & 0xFFFFu), 0x0000 };
        auto fn = reinterpret_cast<int32_t(__fastcall*)(void*, const VarBagKeyPath*)>(
            globals::gameBase + 0x103BB20);
        return fn(c23, &path);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
}

static inline float GetEntityVarBagFloat(Component_23_Statescript* c23, uint64_t var_id_0D80) {
    if (!c23) return 0.0f;
    __try {
        VarBagKeyPath path{ 0x0000, 0x0004, static_cast<uint16_t>(var_id_0D80 & 0xFFFFu), 0x0000 };
        auto fn = reinterpret_cast<float(__fastcall*)(void*, const VarBagKeyPath*)>(
            globals::gameBase + 0x103BD50);
        return fn(c23, &path);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0.0f;
    }
}

static inline bool GetEntityVarBagBool(Component_23_Statescript* c23, uint64_t var_id_0D80) {
    if (!c23) return false;
    auto fn = reinterpret_cast<bool(__fastcall*)(void*, __int64, uint64_t)>(
        globals::gameBase + 0x103B930);
    return fn(c23, 0, var_id_0D80);
}

static bool ApplyNativeSetVarInt32Seh(Component_23_Statescript* c23, uint64_t var_id_0D80, int32_t val) {
    __try {
        uintptr_t ss_core = reinterpret_cast<uintptr_t>(c23) + 0xC0;
        uint8_t saved_gate = *reinterpret_cast<uint8_t*>(ss_core + 0x374);
        *reinterpret_cast<uint8_t*>(ss_core + 0x374) = 0; // Ensure +0xF45D96 does not drop cmd

        VarBagKeyPath path{ 0x0000, 0x0004, static_cast<uint16_t>(var_id_0D80 & 0xFFFFu), 0x0000 };
        auto set_fn = reinterpret_cast<bool(__fastcall*)(void*, const VarBagKeyPath*, int32_t)>(
            globals::gameBase + 0x103B9F0);
        bool ok = set_fn(c23, &path, val);

        // Immediately drain ss_core + 0x308 via +0xF45E40 so vt[+0x218] (+0xF647E0) executes now
        auto drain_fn = reinterpret_cast<void(__fastcall*)(uintptr_t)>(
            globals::gameBase + 0xF45E40);
        int safety = 0;
        while (*reinterpret_cast<int32_t*>(ss_core + 0x318) > 0 && safety++ < 16) {
            drain_fn(ss_core);
        }

        *reinterpret_cast<uint8_t*>(ss_core + 0x374) = saved_gate;
        return ok;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

static bool ApplyNativeSetVarFloatSeh(Component_23_Statescript* c23, uint64_t var_id_0D80, float val) {
    __try {
        uintptr_t ss_core = reinterpret_cast<uintptr_t>(c23) + 0xC0;
        uint8_t saved_gate = *reinterpret_cast<uint8_t*>(ss_core + 0x374);
        *reinterpret_cast<uint8_t*>(ss_core + 0x374) = 0;

        uintptr_t ss_vt = *reinterpret_cast<uintptr_t*>(ss_core);
        auto alloc_cmd = *reinterpret_cast<StatescriptSetVarCmd*(__fastcall**)(uintptr_t, uint8_t)>(ss_vt + 0xA0);
        StatescriptSetVarCmd* cmd = alloc_cmd(ss_core, 4);
        if (!cmd) {
            *reinterpret_cast<uint8_t*>(ss_core + 0x374) = saved_gate;
            return false;
        }

        VarBagKeyPath path{ 0x0000, 0x0004, static_cast<uint16_t>(var_id_0D80 & 0xFFFFu), 0x0000 };
        cmd->timestamp_18     = *reinterpret_cast<uint64_t*>(ss_core + 0x2D0);
        cmd->scope_inst_id_20 = 0;
        cmd->flags_28        |= 0x08;
        cmd->path_30          = path;
        cmd->field_38         = 0;
        cmd->value_40.u64_val = 0;
        cmd->value_40.f32_val = val;
        cmd->value_40.aux_08  = 0;
        cmd->value_40.type    = 3; // SSVAR_FLOAT

        auto apply_cmd = reinterpret_cast<void(__fastcall*)(uintptr_t, StatescriptSetVarCmd*)>(
            globals::gameBase + 0xF45D90);
        apply_cmd(ss_core, cmd);
        *reinterpret_cast<uint8_t*>(ss_core + 0x359) = 1;

        auto drain_fn = reinterpret_cast<void(__fastcall*)(uintptr_t)>(
            globals::gameBase + 0xF45E40);
        int safety = 0;
        while (*reinterpret_cast<int32_t*>(ss_core + 0x318) > 0 && safety++ < 16) {
            drain_fn(ss_core);
        }

        *reinterpret_cast<uint8_t*>(ss_core + 0x374) = saved_gate;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// Updates an int32_t variable in rid_entity_varbag via BOTH:
// 1. Direct SetVar + NotifyVarInBag (like SyncEntityHealthVarBag)
// 2. Native +0x103B9F0 (Component23_SetVarBagInt32) -> +0xF45D90 -> +0xF45E40 queue drain
static inline bool SetEntityVarBagInt32Native(Component_23_Statescript* c23, uint64_t var_id_0D80, int32_t val) {
    if (!c23) return false;

    if (c23->ss_inner.rid_entity_varbag && !IsBadReadPtr(c23->ss_inner.rid_entity_varbag, sizeof(StatescriptVar_VarBag))) {
        StatescriptPrimitive cv{};
        cv.type = (StatescriptPrimitive_Type)2; // SSVAR_INT32
        *(int32_t*)&cv.value = val;
        c23->ss_inner.rid_entity_varbag->SetVar({ (int64_t)var_id_0D80 }, cv);
        c23->ss_inner.rid_entity_varbag->SetVar({ StatescriptVar_ENTITY_VARBAG, (int64_t)var_id_0D80 }, cv);
        NotifyVarInBag(&c23->ss_inner, nullptr, c23->ss_inner.rid_entity_varbag, (int64_t)var_id_0D80, cv);
        c23->ss_inner.needs_process_e = 1;
    }

    return ApplyNativeSetVarInt32Seh(c23, var_id_0D80, val);
}

// Reconstructs +0x103BC10 for arbitrary float values (since +0x103BC10 hardcodes 1.0f)
static inline bool SetEntityVarBagFloatNative(Component_23_Statescript* c23, uint64_t var_id_0D80, float val) {
    if (!c23) return false;

    if (c23->ss_inner.rid_entity_varbag && !IsBadReadPtr(c23->ss_inner.rid_entity_varbag, sizeof(StatescriptVar_VarBag))) {
        StatescriptPrimitive cv{};
        cv.type = StatescriptPrimitive_FLT;
        *(float*)&cv.value = val;
        c23->ss_inner.rid_entity_varbag->SetVar({ (int64_t)var_id_0D80 }, cv);
        c23->ss_inner.rid_entity_varbag->SetVar({ StatescriptVar_ENTITY_VARBAG, (int64_t)var_id_0D80 }, cv);
        NotifyVarInBag(&c23->ss_inner, nullptr, c23->ss_inner.rid_entity_varbag, (int64_t)var_id_0D80, cv);
        c23->ss_inner.needs_process_e = 1;
    }

    return ApplyNativeSetVarFloatSeh(c23, var_id_0D80, val);
}

static void AccumulateAttackerUltimateCharge(EntityAdminBase* ea, uint32_t local_mdl_id, int32_t damage_int) {
    if (!ea || !local_mdl_id || damage_int <= 0) return;

    Entity* local_hero = ea->getEntById(local_mdl_id);
    if (!local_hero) return;

    auto* c23 = local_hero->getById<Component_23_Statescript>(0x23);
    if (!c23) return;

    constexpr uint64_t kVarUltCost   = 0x0D80000000000152ULL; // e.g., 900 on Tracer
    constexpr uint64_t kVarUltCharge = 0x0D80000000000156ULL; // Current Ult charge points

    int32_t max_cost = GetEntityVarBagInt32(c23, kVarUltCost);
    if (max_cost <= 0) {
        max_cost = 900;
        SetEntityVarBagInt32Native(c23, kVarUltCost, max_cost);
    }

    int32_t old_charge = GetEntityVarBagInt32(c23, kVarUltCharge);
    int32_t add_points = (damage_int > 0) ? damage_int : 1;

    int32_t new_charge = old_charge + add_points;
    if (new_charge > max_cost) new_charge = max_cost;

    if (new_charge != old_charge) {
        SetEntityVarBagInt32Native(c23, kVarUltCharge, new_charge);

        int32_t pct = (new_charge * 100) / max_cost;
        if (pct > 100) pct = 100;

        // Also sync LocalPlayer companion t2 variables in rid_entity_varbag
        SetEntityVarBagInt32Native(c23, 0x0D800000000000F3ULL, pct);
        SetEntityVarBagInt32Native(c23, 0x0D80000000000F0CULL, pct);

        ShotDiagLogFmt("[ULT_CHARGE] dmg=%d charge=%d/%d (%d%%)", damage_int, new_charge, max_cost, pct);
    }
}

static void EmulateDummyHitscanDamage(uintptr_t shot_ctx) {
    if (!WeaponHardpointDiag::s_dummy_damage_enabled || !shot_ctx) return;
    Entity* shooter = SafeReadVal<Entity*>(shot_ctx + 0x08);
    if (!shooter) return;
    EntityAdminBase* ea = SafeReadVal<EntityAdminBase*>((uintptr_t)shooter + 0x28);
    if (!ea || !ea->local_entid) return;
    // Only the local player's own shots.
    Entity* ctrl = ea->getEntById(ea->local_entid);
    auto* c20 = ctrl ? ctrl->getById<Component_20_ModelReference>(0x20) : nullptr;
    uint32_t shooter_id = shooter->entity_id;
    if (!c20 || (shooter_id != c20->cam_attach_entid && shooter_id != c20->aim_entid)) return;
    auto* shooter_c10 = shooter->getById<Component_10_FilterBits>(0x10);
    uint64_t shooter_team = shooter_c10 ? shooter_c10->filter_bits : 0;

    Vector4 orig = SafeReadVal<Vector4>(shot_ctx + 0x60);
    s_last_shot_orig = orig;
    s_last_shot_shooter_id = shooter_id;
    s_last_shot_team = shooter_team;
    s_last_shot_tick_ms = GetTickCount64();

    uintptr_t rec = SafeReadVal<uintptr_t>(shot_ctx + 0xE0);
    int32_t n = SafeReadVal<int32_t>(shot_ctx + 0xE8);
    if (!rec || n <= 0 || n > 64) return;
    constexpr float kRange = 100.f, kRadius = 0.40f, kFootOff = 0.25f, kHeadOff = 1.45f, kCritY = 1.30f;

    for (int32_t p = 0; p < n; p++) {
        Vector4 dir = SafeReadVal<Vector4>(rec + (uintptr_t)p * 0x20);
        uintptr_t pellet_c27 = SafeReadVal<uintptr_t>(rec + (uintptr_t)p * 0x20 + 0x10);
        float len = sqrtf(Dot3(dir, dir));
        if (len < 1e-4f) continue;
        dir.X /= len; dir.Y /= len; dir.Z /= len;
        Vector4 ray_end = MulAdd3(orig, dir, kRange);

        Entity* best = nullptr;
        float best_t = 2.f, best_hit_y = 0.f, best_base_y = 0.f;
        for (Entity* ent : *ea) {
            if (!ent || ent == shooter || ent->entity_id == c20->aim_entid || ent->entity_id == c20->cam_attach_entid) continue;
            if (!ent->hasComponent(0x15) || !ent->hasComponent(0x28)) continue;
            auto* c10 = ent->getById<Component_10_FilterBits>(0x10);
            if (c10 && shooter_team && (c10->filter_bits & 0x3C0000ULL) == (shooter_team & 0x3C0000ULL)) continue; // same team
            auto* c1 = ent->getById<Component_1_SceneRendering>(1);
            auto* c28 = ent->getById<Component_28_STUHealthComponent>(0x28);
            if (!c1 || !c28 || c28->CalcTotalCurrentHealth() <= 0.f) continue;
            Vector4 base = c1->position;
            Vector4 a = base, b = base;
            a.Y += kFootOff; b.Y += kHeadOff;
            float s = 0.f, t = 0.f;
            float d2 = ClosestSegSeg(orig, ray_end, a, b, s, t);
            if (d2 <= kRadius * kRadius && s < best_t) {
                best = ent; best_t = s; best_base_y = base.Y;
                best_hit_y = orig.Y + dir.Y * kRange * s;
            }
        }
        if (!best) continue;
        auto* c28 = best->getById<Component_28_STUHealthComponent>(0x28);
        bool crit = (best_hit_y - best_base_y) >= kCritY;
        float dmg = WeaponHardpointDiag::s_dummy_damage_per_pellet * (crit ? WeaponHardpointDiag::s_dummy_crit_mult : 1.f);
        float hp = ApplyDamageToHealthComponent(c28, dmg);
        SyncEntityHealthVarBag(best);
        s_last_hit_dummy_entid = best->entity_id;
        s_last_hit_dummy_ms = GetTickCount64();
        s_redirect_dummy_c1 = best->getById<Component_1_SceneRendering>(1);
        s_redirect_hit_pos = MulAdd3(orig, dir, best_t * kRange);
        s_redirect_hit_pos.W = 1.0f;
        WeaponHardpointDiag::s_dummy_hits++;
        WeaponHardpointDiag::s_last_dummy_hit = std::format("Hit #{} on 0x{:X}: -{:.1f}{} -> HP {:.1f} (dist {:.1f}m, c27=0x{:X})",
            WeaponHardpointDiag::s_dummy_hits, best->entity_id, dmg, crit ? " CRIT" : "", hp, best_t * kRange, pellet_c27);
        ShotDiagLogFmt("[DUMMY_HIT] %s", WeaponHardpointDiag::s_last_dummy_hit.c_str());

        if (s_alive_hit_snapshot.empty() && hp > 0.f) {
            uint32 ctrl_id = 0;
            if (auto* c1f = best->getById<Component_1F_Backref>(0x1F)) ctrl_id = c1f->backref_entid;
            Entity* d_ctrl = ctrl_id ? ea->getEntById(ctrl_id) : nullptr;
            s_alive_hit_snapshot.reserve(8192);
            AppendHeroEntityForensics(s_alive_hit_snapshot, "ENEMY DUMMY SNAPSHOT AT HIT #1 (WHILE ALIVE & DAMAGED)", d_ctrl, best);
        }
    }
}

using ApplyProjectileFilterBitsFn = void(__fastcall*)(Component_10_FilterBits* comp10, uint64_t new_bits);

static std::atomic<uint32_t> s_active_pulse_bomb_entid{ 0xFFFFFFFFu };
static std::atomic<uint32_t> s_pulse_bomb_stuck_entid{ 0xFFFFFFFFu };
static std::atomic<bool>     s_pulse_bomb_armed{ false };
static std::atomic<uint64_t> s_pulse_bomb_detonate_ms{ 0 };
static std::atomic<uintptr_t> s_pulse_bomb_c27{ 0 };
static Vector4               s_pulse_bomb_land_pos{};

typedef Entity*(__fastcall* perform_bullet_raycast_fn)(uintptr_t cb_vt, uintptr_t shot_ctx, uint32_t* out_hit_type, uint8_t flag);
perform_bullet_raycast_fn PerformBulletRaycast_orig = nullptr;
Entity* __fastcall PerformBulletRaycast_hook(uintptr_t cb_vt, uintptr_t shot_ctx, uint32_t* out_hit_type, uint8_t is_client_call) {
    WeaponHardpointDiag::s_shot_raycast_calls++;

    uintptr_t cfg = shot_ctx ? SafeReadVal<uintptr_t>(shot_ctx + 0x18) : 0;
    uint64_t proj_resid = shot_ctx ? SafeReadVal<uint64_t>(shot_ctx + 0x28) : 0;
    uint32_t orig_mode104 = cfg ? SafeReadVal<uint32_t>(cfg + 0x104) : 0;
    uint8_t  orig_b108    = cfg ? SafeReadVal<uint8_t>(cfg + 0x108) : 0;

    // When [cfg + 0x104] == 1 (server-spawned projectile like Pulse Bomb 0x400000000000045),
    // Branch A (+0xD3574D) always fails offline with type = 8 because [GameEntityAdmin + 0x210] is NULL.
    // Temporarily switch [cfg + 0x104] = 3, [cfg + 0x108] = 0, and is_client_call = 0 so +0xD354B0:
    //  1) Takes Branch B (+0xD35904) and spawns the projectile via EmplaceEntity (+0x80F7C0 at +0xD35A6F),
    //  2) Calls cb_vt->vtable[+0x08] (+0xD35C20 -> +0x1032420 Component27_InitSpawnedProjectile) at +0xD35B86,
    //  3) Calls +0xD38C00 (+0xD35E68, gated on [cfg + 0x108] == 0) and +0xD38CF0 (+0xD35E7D).
    bool reroute_to_branch_b = (cfg != 0 && orig_mode104 == 1 && proj_resid != 0);
    if (reroute_to_branch_b) {
        *(uint32_t*)(cfg + 0x104) = 3;
        *(uint8_t*)(cfg + 0x108)  = 0;
        is_client_call            = 0;
    }

    Entity* hit_ent = PerformBulletRaycast_orig(cb_vt, shot_ctx, out_hit_type, is_client_call);

    if (reroute_to_branch_b) {
        *(uint32_t*)(cfg + 0x104) = orig_mode104;
        *(uint8_t*)(cfg + 0x108)  = orig_b108;

        if (hit_ent) {
            if (proj_resid == 0x0400000000000045ULL) {
                s_active_pulse_bomb_entid.store(hit_ent->entity_id);
                s_pulse_bomb_stuck_entid.store(0xFFFFFFFFu);
                s_pulse_bomb_armed.store(false);
            }
            // 1. Apply Branch A's Component_10_FilterBits Bit 23 (0x800000) (+0xD35807..+0xD3581B)
            if (auto* comp10 = hit_ent->getById<Component_10_FilterBits>(0x10)) {
                using GetBitsFn = uint64_t(__fastcall*)(Component_10_FilterBits*);
                uintptr_t c10_vt = *(uintptr_t*)comp10;
                uint64_t bits = ((GetBitsFn)(*(uintptr_t*)(c10_vt + 0x30)))(comp10);
                bits |= (1ULL << 23); // bts rax, 17h
                auto apply_bits = (ApplyProjectileFilterBitsFn)(globals::gameBase + 0xCD0B00);
                apply_bits(comp10, bits);
            }

            // 2. If the spawned projectile has Component_23_Statescript (0x23), enable local execution
            if (auto* proj_c23 = hit_ent->getById<Component_23_Statescript>(0x23)) {
                proj_c23->ss_inner.flag_374 = 0;
                for (int i = 0; i < proj_c23->ss_inner.g1_instanceArr.num; ++i) {
                    StatescriptInstance* inst = proj_c23->ss_inner.g1_instanceArr.ptr[i];
                    if (!inst) continue;
                    inst->execution_flags = (inst->execution_flags | 0x400009u | 0x200000u) & ~0x1004u;
                }
            }
        }
    }
    if (shot_ctx) {
        Entity* shooter = SafeReadVal<Entity*>(shot_ctx + 0x08);
        uint32 shooter_id = shooter ? SafeReadVal<uint32>((uintptr_t)&shooter->entity_id) : 0;
        uintptr_t phys_world = SafeReadVal<uintptr_t>(shot_ctx + 0x28);
        uint64_t f30 = SafeReadVal<uint64_t>(shot_ctx + 0x30);
        uint64_t f38 = SafeReadVal<uint64_t>(shot_ctx + 0x38);
        Vector4 r_orig = SafeReadVal<Vector4>(shot_ctx + 0x60);
        Vector4 r_dir = SafeReadVal<Vector4>(shot_ctx + 0x70);
        uint32_t hit_type = out_hit_type ? SafeReadVal<uint32_t>((uintptr_t)out_hit_type) : 0;
        float hit_dist = SafeReadVal<float>(shot_ctx + 0xD4);
        uint32 hit_id = hit_ent ? SafeReadVal<uint32>((uintptr_t)&hit_ent->entity_id) : 0;
        bool has_c27 = hit_ent ? hit_ent->hasComponent(0x27) : false;
        bool has_c28 = hit_ent ? hit_ent->hasComponent(0x28) : false;
        uintptr_t cfg = SafeReadVal<uintptr_t>(shot_ctx + 0x18);
        uint32_t cfg104 = cfg ? SafeReadVal<uint32_t>(cfg + 0x104) : 0xFFFFFFFF;
        uint64_t cfgA8 = cfg ? SafeReadVal<uint64_t>(cfg + 0xA8) : 0;
        float cfg4C = cfg ? SafeReadVal<float>(cfg + 0x4C) : 0.0f;
        uintptr_t ra = (uintptr_t)_ReturnAddress();
        uintptr_t ra_rva = (ra >= globals::gameBase && ra < globals::gameBase + globals::gameSize) ? (ra - globals::gameBase) : ra;
        uintptr_t vt = cb_vt ? SafeReadVal<uintptr_t>(cb_vt) : 0;
        uintptr_t vt_rva = (vt >= globals::gameBase && vt < globals::gameBase + globals::gameSize) ? (vt - globals::gameBase) : vt;
        uintptr_t fn08 = vt ? SafeReadVal<uintptr_t>(vt + 0x08) : 0;
        uintptr_t fn08_rva = (fn08 >= globals::gameBase && fn08 < globals::gameBase + globals::gameSize) ? (fn08 - globals::gameBase) : fn08;
        ShotDiagLogFmt("[BULLET_CAST] caller=+0x%llX vt=+0x%llX(fn08=+0x%llX) shooter=0x%X world=0x%llX f30=0x%llX f38=0x%llX orig=(%.2f,%.2f,%.2f) dir=(%.2f,%.2f,%.2f) flg=%u cfg104=%u cfgA8=0x%llX cfg4C=%.2f -> hit_ent=0x%X(c27=%d,c28=%d) type=0x%X dist=%.2f",
            (unsigned long long)ra_rva, (unsigned long long)vt_rva, (unsigned long long)fn08_rva,
            shooter_id, (unsigned long long)phys_world, (unsigned long long)f30, (unsigned long long)f38,
            r_orig.X, r_orig.Y, r_orig.Z, r_dir.X, r_dir.Y, r_dir.Z, is_client_call, cfg104, (unsigned long long)cfgA8, cfg4C,
            hit_id, has_c27 ? 1 : 0, has_c28 ? 1 : 0, hit_type, hit_dist);
        if (hit_ent && has_c27) {
            uintptr_t c27 = (uintptr_t)hit_ent->getById(0x27);
            if (c27) {
                uint8_t a4 = SafeReadVal<uint8_t>(c27 + 0xA4);
                uint8_t b1e8 = SafeReadVal<uint8_t>(c27 + 0x1E8);
                uint32_t e1f8 = SafeReadVal<uint32_t>(c27 + 0x1F8);
                uint32_t e1fc = SafeReadVal<uint32_t>(c27 + 0x1FC);
                uint32_t ee00 = SafeReadVal<uint32_t>(c27 + 0xE00);
                Vector4 p40 = SafeReadVal<Vector4>(c27 + 0x40);
                Vector4 p50 = SafeReadVal<Vector4>(c27 + 0x50);
                Vector4 pB0 = SafeReadVal<Vector4>(c27 + 0xB0);
                ShotDiagLogFmt("[PELLET_INIT_C27] pellet=0x%X c27=0x%llX a4=%u 1e8=%u 1f8=0x%X(bone=0x%X) e00=0x%X p40=(%.2f,%.2f,%.2f) p50=(%.2f,%.2f,%.2f) pB0=(%.2f,%.2f,%.2f)",
                    hit_id, (unsigned long long)c27, a4, b1e8, e1f8, e1fc, ee00,
                    p40.X, p40.Y, p40.Z, p50.X, p50.Y, p50.Z, pB0.X, pB0.Y, pB0.Z);
            }
            WeaponHardpointDiag::TrackProjectile(hit_ent);
        }
    }
    return hit_ent;
}

typedef void(__fastcall* execute_shot_raycast_fn)(uintptr_t shot_ctx, uint8_t flag);
execute_shot_raycast_fn ExecuteShotRaycast_orig = nullptr;
void __fastcall ExecuteShotRaycast_hook(uintptr_t shot_ctx, uint8_t flag) {
    if (shot_ctx) {
        Entity* shooter = SafeReadVal<Entity*>(shot_ctx + 0x08);
        EntityAdminBase* ea = shooter ? SafeReadVal<EntityAdminBase*>((uintptr_t)shooter + 0x28) : nullptr;
        if (ea && ea->local_entid) {
            // Synchronize Bit 16 (0x10000) on Component_01 + 0x1B8 BEFORE ExecuteShotRaycast_orig spawns pellets
            // and calls +0x1032CB0 (which pre-computes [pellet_c27 + 0x228] via +0x10830C0 on Shot #1).
            SyncEnemyHitboxFlags(ea);
            Entity* ctrl = ea->getEntById(ea->local_entid);
            auto* c20 = ctrl ? ctrl->getById<Component_20_ModelReference>(0x20) : nullptr;
            uint32_t shooter_id = shooter->entity_id;
            if (c20 && (shooter_id == c20->cam_attach_entid || shooter_id == c20->aim_entid)) {
                auto* shooter_c10 = shooter->getById<Component_10_FilterBits>(0x10);
                s_last_shot_orig = SafeReadVal<Vector4>(shot_ctx + 0x60);
                s_last_shot_shooter_id = shooter_id;
                s_last_shot_team = shooter_c10 ? shooter_c10->filter_bits : 0;
                s_last_shot_tick_ms = GetTickCount64();
                if (kEnablePelletRedirectHacks && flag == 1) {
                    // Always force b8_cnt ([shot_ctx + 0xB8]) to 0 on local fire so +0xD45333 spawns fresh pellets on every shot.
                    *(int32_t*)(shot_ctx + 0xB8) = 0;
                }
            }
        }
    }
    ExecuteShotRaycast_orig(shot_ctx, flag);
    // Legacy ClosestSegSeg cylinder damage (EmulateDummyHitscanDamage) replaced in Phase 7 Stage 1 by native
    // skeletal hitbox damage inside OnPelletHitboxHit_hook (+0xD381F0).
    if (shot_ctx) {
        Entity* shooter = SafeReadVal<Entity*>(shot_ctx + 0x08);
        uint32 shooter_id = shooter ? SafeReadVal<uint32>((uintptr_t)&shooter->entity_id) : 0;
        int32_t hits_cnt = SafeReadVal<int32_t>(shot_ctx + 0xE8);
        int32_t b8_cnt = SafeReadVal<int32_t>(shot_ctx + 0xB8);
        ShotDiagLogFmt("[SHOT_RAY (+0xD45240)] shooter=0x%X flg=%u b8_cnt=%d hits_recorded_E8=%d",
            shooter_id, flag, b8_cnt, hits_cnt);
    }
}

static bool s_dumped_pre_redirect = false;
static uintptr_t s_active_pellet_c27 = 0;
static Vector4 s_active_pellet_ray_start{};

static bool RedirectPelletC27ToDummy(uintptr_t pellet_c27, const char* tag, Entity** out_dummy = nullptr) {
    if (out_dummy) *out_dummy = nullptr;
    if (!WeaponHardpointDiag::s_dummy_damage_enabled || !pellet_c27) return false;
    bool is_active_pellet = (pellet_c27 == s_active_pellet_c27);
    if (!is_active_pellet && (s_last_shot_tick_ms == 0 || (GetTickCount64() - s_last_shot_tick_ms >= 500))) return false;

    uint8_t hit_a4 = SafeReadVal<uint8_t>(pellet_c27 + 0xA4);
    uint8_t hit_1e8 = SafeReadVal<uint8_t>(pellet_c27 + 0x1E8);
    uint32_t orig_1f8 = SafeReadVal<uint32_t>(pellet_c27 + 0x1F8);
    if (hit_a4 == 0 && hit_1e8 == 0) return false;

    Entity* pellet_ent = SafeReadVal<Entity*>(pellet_c27 + 0x08);
    EntityAdminBase* ea = pellet_ent ? SafeReadVal<EntityAdminBase*>((uintptr_t)pellet_ent + 0x28) : GameEntityAdmin();
    if (!ea || !ea->local_entid) return false;

    // Prefer the exact ray_start passed into +0x1033510 (s_active_pellet_ray_start), falling back to s_last_shot_orig.
    Vector4 ray_start = is_active_pellet ? s_active_pellet_ray_start : s_last_shot_orig;
    if (fabsf(ray_start.X) + fabsf(ray_start.Y) + fabsf(ray_start.Z) < 0.1f) {
        ray_start = s_last_shot_orig;
    }
    if (fabsf(ray_start.X) + fabsf(ray_start.Y) + fabsf(ray_start.Z) < 0.1f) {
        ray_start = SafeReadVal<Vector4>(pellet_c27 + 0x40);
    }
    Vector4 seg_end = SafeReadVal<Vector4>(pellet_c27 + 0xB0);
    if (fabsf(seg_end.X) + fabsf(seg_end.Y) + fabsf(seg_end.Z) < 0.1f) return false;

    constexpr float kRadius = 0.65f;
    float best_s = 1.0f;
    Entity* best_dummy = nullptr;
    for (Entity* ent : *ea) {
        if (!ent || ent->entity_id == s_last_shot_shooter_id) continue;
        if (!ent->hasComponent(0x15) || !ent->hasComponent(0x28)) continue;
        auto* c10 = ent->getById<Component_10_FilterBits>(0x10);
        if (s_last_shot_team && c10 && (c10->filter_bits & 0x3C0000ULL) == (s_last_shot_team & 0x3C0000ULL)) continue;
        auto* c28 = ent->getById<Component_28_STUHealthComponent>(0x28);
        if (!c28 || c28->CalcTotalCurrentHealth() <= 0.f) continue;

        Vector4 pos{};
        if (auto* c15 = ent->getById<Component_15_STUCharacterMoverComponent>(0x15))
            pos = c15->movement_state_1.absolute_position;
        else if (auto* c1 = ent->getById<Component_1_SceneRendering>(1))
            pos = c1->position;
        Vector4 foot{ pos.X, pos.Y + 0.25f, pos.Z, 0.f };
        Vector4 head{ pos.X, pos.Y + 1.85f, pos.Z, 0.f };
        float s = 0.f, t = 0.f;
        float d2 = ClosestSegSeg(ray_start, seg_end, foot, head, s, t);
        if (d2 <= kRadius * kRadius && s > 0.0f && s <= 1.0f && s < best_s) {
            best_s = s;
            best_dummy = ent;
        }
    }
    if (!best_dummy) return false;

    Vector4 new_pos{
        ray_start.X + (seg_end.X - ray_start.X) * best_s,
        ray_start.Y + (seg_end.Y - ray_start.Y) * best_s,
        ray_start.Z + (seg_end.Z - ray_start.Z) * best_s,
        seg_end.W
    };

    uint32_t pellet_id = pellet_ent ? SafeReadVal<uint32_t>((uintptr_t)&pellet_ent->entity_id) : 0;
    uint32_t orig_18 = SafeReadVal<uint32_t>(pellet_c27 + 0x18);
    uint32_t orig_1fc = SafeReadVal<uint32_t>(pellet_c27 + 0x1FC);
    Vector4 orig_200 = SafeReadVal<Vector4>(pellet_c27 + 0x200);

    ShotDiagLogFmt("[PELLET_C27_REDIRECT (%s)] pellet=0x%X c27=0x%llX -> dummy=0x%X a4=%u 1e8=%u orig18=0x%X orig1F8=0x%X(bone=0x%X) orig200=(%.2f,%.2f,%.2f) segEnd=(%.2f,%.2f,%.2f)->newB0=(%.2f,%.2f,%.2f)",
        tag, pellet_id, (unsigned long long)pellet_c27, best_dummy->entity_id,
        hit_a4, hit_1e8, orig_18, orig_1f8, orig_1fc, orig_200.X, orig_200.Y, orig_200.Z,
        seg_end.X, seg_end.Y, seg_end.Z, new_pos.X, new_pos.Y, new_pos.Z);

    if (!s_dumped_pre_redirect) {
        s_dumped_pre_redirect = true;
        WeaponHardpointDiag::DumpDummyCombatForensics(ea, "PRE_REDIRECT_BEFORE_HIT");
    }

    __try {
        *(Vector4*)(pellet_c27 + 0xB0) = new_pos;
        *(Vector4*)(pellet_c27 + 0x200) = new_pos;
        // Keep +0x1F8 as 0xFFFFFFFF so +0x1036C30 detaches pellet_c1 and applies +0x200 (new_pos) in world space
        // on the surface of the dummy rather than treating new_pos as a 100m local offset relative to best_dummy.
        *(uint32_t*)(pellet_c27 + 0x1F8) = 0xFFFFFFFFU;
        *(uint32_t*)(pellet_c27 + 0x1FC) = 0xFFFFFFFFU;
        *(uint32_t*)(pellet_c27 + 0xE00) = best_dummy->entity_id;

        if (Component_1_SceneRendering* proj_c1 = pellet_ent ? pellet_ent->getById<Component_1_SceneRendering>(1) : nullptr) {
            proj_c1->position = new_pos;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        ShotDiagLogFmt("[PELLET_C27_REDIRECT_EXCEPTION (%s)] caught SEH exception writing pellet_c27=0x%llX", tag, (unsigned long long)pellet_c27);
        WeaponHardpointDiag::DumpDummyCombatForensics(ea, "SEH_IN_REDIRECT");
        return false;
    }

    if (out_dummy) *out_dummy = best_dummy;
    return true;
}

typedef void(__fastcall* process_pellet_result_fn)(uintptr_t cb_vt, uintptr_t pellet_c27, Vector4* ray_start, float a4, uint8_t a5);
process_pellet_result_fn ProcessPelletRaycastResult_orig = nullptr;
void __fastcall ProcessPelletRaycastResult_hook(uintptr_t cb_vt, uintptr_t pellet_c27, Vector4* ray_start, float a4, uint8_t a5) {
    s_active_pellet_c27 = pellet_c27;
    Vector4 rs{};
    if (ray_start) {
        rs = SafeReadVal<Vector4>((uintptr_t)ray_start);
        s_active_pellet_ray_start = rs;
    }
    uintptr_t vt = cb_vt ? SafeReadVal<uintptr_t>(cb_vt) : 0;
    uintptr_t vt_rva = (vt >= globals::gameBase && vt < globals::gameBase + globals::gameSize) ? (vt - globals::gameBase) : vt;
    uintptr_t fnB8 = vt ? SafeReadVal<uintptr_t>(vt + 0xB8) : 0;
    uintptr_t fnB8_rva = (fnB8 >= globals::gameBase && fnB8 < globals::gameBase + globals::gameSize) ? (fnB8 - globals::gameBase) : fnB8;
    Entity* p_ent = pellet_c27 ? SafeReadVal<Entity*>(pellet_c27 + 0x08) : nullptr;
    uint32_t pid = p_ent ? SafeReadVal<uint32_t>((uintptr_t)&p_ent->entity_id) : 0;

    uint8_t pre_a4 = pellet_c27 ? SafeReadVal<uint8_t>(pellet_c27 + 0xA4) : 0;
    uint8_t pre_1e8 = pellet_c27 ? SafeReadVal<uint8_t>(pellet_c27 + 0x1E8) : 0;
    uint32_t pre_1f8 = pellet_c27 ? SafeReadVal<uint32_t>(pellet_c27 + 0x1F8) : 0;
    uint32_t pre_e00 = pellet_c27 ? SafeReadVal<uint32_t>(pellet_c27 + 0xE00) : 0;
    Vector4 pre_b0 = pellet_c27 ? SafeReadVal<Vector4>(pellet_c27 + 0xB0) : Vector4{};

    ProcessPelletRaycastResult_orig(cb_vt, pellet_c27, ray_start, a4, a5);

    uint8_t post_a4 = pellet_c27 ? SafeReadVal<uint8_t>(pellet_c27 + 0xA4) : 0;
    uint8_t post_1e8 = pellet_c27 ? SafeReadVal<uint8_t>(pellet_c27 + 0x1E8) : 0;
    uint32_t post_1f8 = pellet_c27 ? SafeReadVal<uint32_t>(pellet_c27 + 0x1F8) : 0;
    uint32_t post_1fc = pellet_c27 ? SafeReadVal<uint32_t>(pellet_c27 + 0x1FC) : 0;
    uint32_t post_e00 = pellet_c27 ? SafeReadVal<uint32_t>(pellet_c27 + 0xE00) : 0;
    Vector4 post_b0 = pellet_c27 ? SafeReadVal<Vector4>(pellet_c27 + 0xB0) : Vector4{};
    Vector4 post_200 = pellet_c27 ? SafeReadVal<Vector4>(pellet_c27 + 0x200) : Vector4{};

    if (pre_a4 == 0 && pre_1e8 == 0) {
        ShotDiagLogFmt("[PELLET_STEP (+0xD37230)] pellet=0x%X vt=+0x%llX fnB8=+0x%llX rs=(%.2f,%.2f,%.2f) a4=%.3f a5=%u | PRE(a4=%u,1e8=%u,1f8=0x%X,e00=0x%X,B0=(%.2f,%.2f,%.2f)) -> POST(a4=%u,1e8=%u,1f8=0x%X,bone=0x%X,e00=0x%X,B0=(%.2f,%.2f,%.2f),200=(%.2f,%.2f,%.2f))",
            pid, (unsigned long long)vt_rva, (unsigned long long)fnB8_rva, rs.X, rs.Y, rs.Z, a4, a5,
            pre_a4, pre_1e8, pre_1f8, pre_e00, pre_b0.X, pre_b0.Y, pre_b0.Z,
            post_a4, post_1e8, post_1f8, post_1fc, post_e00, post_b0.X, post_b0.Y, post_b0.Z, post_200.X, post_200.Y, post_200.Z);
    }

    s_active_pellet_c27 = 0;
}

typedef int32_t(__fastcall* test_pellet_candidates_fn)(
    uintptr_t cb_vt,
    uintptr_t ray_world_ctx,
    uintptr_t pellet_c27,
    const Vector4* ray_start,
    const Vector4* ray_end,
    uint32_t tick,
    uintptr_t out_hit);

test_pellet_candidates_fn TestRayAgainstCandidates_orig = nullptr;
int32_t __fastcall TestRayAgainstCandidates_hook(
    uintptr_t cb_vt,
    uintptr_t ray_world_ctx,
    uintptr_t pellet_c27,
    const Vector4* ray_start,
    const Vector4* ray_end,
    uint32_t tick,
    uintptr_t out_hit)
{
    int32_t cand_count = ray_world_ctx ? SafeReadVal<int32_t>(ray_world_ctx + 0x28) : -1;
    uintptr_t cand_arr = ray_world_ctx ? SafeReadVal<uintptr_t>(ray_world_ctx + 0x20) : 0;
    uint8_t c27_1e8 = pellet_c27 ? SafeReadVal<uint8_t>(pellet_c27 + 0x1E8) : 0xFF;

    // Sign-extend 13-bit negative client Statescript instance_id in candidate->field_04 if needed
    if (cand_arr && cand_count > 0 && cand_count < 64) {
        for (int32_t i = 0; i < cand_count; ++i) {
            uintptr_t entry = cand_arr + (uintptr_t)i * 0x160;
            uint16_t inst_id = SafeReadVal<uint16_t>(entry + 0x04);
            if ((inst_id & 0x1000) != 0 && (inst_id & 0xE000) == 0) {
                *(uint16_t*)(entry + 0x04) = (uint16_t)(inst_id | 0xE000);
            }
        }
    }

    int32_t ret = TestRayAgainstCandidates_orig(cb_vt, ray_world_ctx, pellet_c27, ray_start, ray_end, tick, out_hit);

    Vector4 rs = ray_start ? SafeReadVal<Vector4>((uintptr_t)ray_start) : Vector4{};
    Vector4 re = ray_end ? SafeReadVal<Vector4>((uintptr_t)ray_end) : Vector4{};
    Entity* hit_ent = (ret >= 0 && out_hit) ? SafeReadVal<Entity*>(out_hit + 0x20) : nullptr;
    uint32_t hit_entid = hit_ent ? SafeReadVal<uint32_t>((uintptr_t)&hit_ent->entity_id) : 0;
    float hit_t = (ret >= 0 && out_hit) ? SafeReadVal<float>(out_hit + 0x30) : -1.0f;

    char cand_summary[256] = { 0 };
    int pos = 0;
    if (cand_arr && cand_count > 0) {
        int max_c = cand_count < 4 ? cand_count : 4;
        for (int i = 0; i < max_c; ++i) {
            uintptr_t entry = cand_arr + (uintptr_t)i * 0x160;
            uint32_t eid = SafeReadVal<uint32_t>(entry + 0x00);
            uint32_t f04 = SafeReadVal<uint32_t>(entry + 0x04);
            uint32_t mid = SafeReadVal<uint32_t>(entry + 0x08);
            int32_t nhb = SafeReadVal<int32_t>(entry + 0x18);
            uint8_t f154 = SafeReadVal<uint8_t>(entry + 0x154);
            int n = snprintf(cand_summary + pos, sizeof(cand_summary) - pos, "[%d:ent=0x%X,f04=0x%X,mdl=0x%X,hb=%d,f154=%u] ",
                i, eid, f04, mid, nhb, f154);
            if (n > 0 && pos + n < (int)sizeof(cand_summary)) pos += n;
        }
    }

    ShotDiagLogFmt("[RAY_CAND (+0x1034300)] ret=%d cands=%d c27_1e8=%u tick=0x%X rs=(%.2f,%.2f,%.2f)->re=(%.2f,%.2f,%.2f) hit_ent=0x%X t=%.3f | %s",
        ret, cand_count, c27_1e8, tick, rs.X, rs.Y, rs.Z, re.X, re.Y, re.Z, hit_entid, hit_t, cand_summary);

    return ret;
}

test_pellet_candidates_fn TestPelletAgainstCandidates_orig = nullptr;
int32_t __fastcall TestPelletAgainstCandidates_hook(
    uintptr_t cb_vt,
    uintptr_t ray_world_ctx,
    uintptr_t pellet_c27,
    const Vector4* ray_start,
    const Vector4* ray_end,
    uint32_t tick,
    uintptr_t out_hit)
{
    // In +0xD37730:
    // Server (bit 31 of +0x828640(pellet_c27) == 1) directly calls +0x1034300(..., out_hit) at +0xD37789
    // and returns the hit candidate index so +0x1038860 -> +0x1032CB0 -> +0x10396E0 -> +0xD3B720 runs!
    // Client (bit 31 == 0) jumps to +0xD3779C, passes a dummy stack out_hit, and returns -1 at +0xD37AA2.
    // Call +0x1034300 directly with the caller's out_hit (the exact authoritative server branch at +0xD37789):
    if (TestRayAgainstCandidates_orig && out_hit) {
        int32_t auth_ret = TestRayAgainstCandidates_hook(cb_vt, ray_world_ctx, pellet_c27, ray_start, ray_end, tick, out_hit);
        ShotDiagLogFmt("[PELLET_CAND (+0xD37730)] auth_ret=%d pellet_c27=%p out_hit=%p", auth_ret, (void*)pellet_c27, (void*)out_hit);
        return auth_ret;
    }
    return TestPelletAgainstCandidates_orig(cb_vt, ray_world_ctx, pellet_c27, ray_start, ray_end, tick, out_hit);
}

typedef uintptr_t(__fastcall* lookup_ss_node_fn)(uintptr_t ss_mgr, uint32_t packed_id);
lookup_ss_node_fn LookupStatescriptNode_orig = nullptr;
uintptr_t __fastcall LookupStatescriptNode_hook(uintptr_t ss_mgr, uint32_t packed_id) {
    uintptr_t res = LookupStatescriptNode_orig(ss_mgr, packed_id);
    if (!res && ((packed_id & 0x1000u) != 0) && ((packed_id & 0xE000u) == 0)) {
        res = LookupStatescriptNode_orig(ss_mgr, packed_id | 0xE000u);
    }
    return res;
}

typedef void(__fastcall* process_pellet_entity_hits_fn)(uintptr_t a1, uintptr_t pellet_c27, uintptr_t hit_rec, uintptr_t a4);
process_pellet_entity_hits_fn ProcessPelletEntityHits_orig = nullptr;
void __fastcall ProcessPelletEntityHits_hook(uintptr_t a1, uintptr_t pellet_c27, uintptr_t hit_rec, uintptr_t a4) {
    int32_t hit_count = pellet_c27 ? SafeReadVal<int32_t>(pellet_c27 + 0x178) : -1;
    uint32_t hit_entid = pellet_c27 ? SafeReadVal<uint32_t>(pellet_c27 + 0x1E0) : 0;
    ShotDiagLogFmt("[PELLET_ENT_HITS (+0xD3B720)] pellet_c27=%p hit_count=%d hit_entid=0x%X",
        (void*)pellet_c27, hit_count, hit_entid);
    ProcessPelletEntityHits_orig(a1, pellet_c27, hit_rec, a4);
}

static volatile bool s_in_pellet_raycast_1038860 = false;
static uintptr_t s_dummy_c12_aabb_addr = 0;
static int s_a45aa0_total_calls = 0;
static int s_a45aa0_dummy_calls = 0;
static int s_a45aa0_dummy_last_ret = -1;
static Vector4 s_a45aa0_dummy_rs{};
static Vector4 s_a45aa0_dummy_rd{};
static float s_a45aa0_dummy_maxt = 0.0f;

typedef uint8_t(__fastcall* ray_intersects_aabb_fn)(const Vector4* aabb_min_max, const Vector4* ray_start, const Vector4* ray_delta, float max_t);
ray_intersects_aabb_fn RayIntersectsAABB_orig = nullptr;
uint8_t __fastcall RayIntersectsAABB_hook(const Vector4* aabb_min_max, const Vector4* ray_start, const Vector4* ray_delta, float max_t) {
    uint8_t ret = RayIntersectsAABB_orig(aabb_min_max, ray_start, ray_delta, max_t);
    if (s_in_pellet_raycast_1038860) {
        s_a45aa0_total_calls++;
        if (s_dummy_c12_aabb_addr != 0 && (uintptr_t)aabb_min_max == s_dummy_c12_aabb_addr) {
            s_a45aa0_dummy_calls++;
            s_a45aa0_dummy_last_ret = (int)ret;
            if (ray_start) s_a45aa0_dummy_rs = SafeReadVal<Vector4>((uintptr_t)ray_start);
            if (ray_delta) s_a45aa0_dummy_rd = SafeReadVal<Vector4>((uintptr_t)ray_delta);
            s_a45aa0_dummy_maxt = max_t;
        }
    }
    return ret;
}

typedef void(__fastcall* on_pellet_hitbox_hit_fn)(uintptr_t cb_vt, uintptr_t ctx_ptr, uintptr_t a3);
on_pellet_hitbox_hit_fn OnPelletHitboxHit_orig = nullptr;
void __fastcall OnPelletHitboxHit_hook(uintptr_t cb_vt, uintptr_t ctx_ptr, uintptr_t a3) {
    uintptr_t pellet_c27 = ctx_ptr ? SafeReadVal<uintptr_t>(ctx_ptr + 0x00) : 0;
    uintptr_t out_hit = ctx_ptr ? SafeReadVal<uintptr_t>(ctx_ptr + 0x08) : 0;
    Entity* hit_ent = out_hit ? SafeReadVal<Entity*>(out_hit + 0x20) : nullptr;
    uint32_t hit_entid = hit_ent ? SafeReadVal<uint32_t>((uintptr_t)&hit_ent->entity_id) : 0;
    Vector4 hit_pos = out_hit ? SafeReadVal<Vector4>(out_hit + 0x00) : Vector4{};
    float hit_frac = out_hit ? SafeReadVal<float>(out_hit + 0x30) : -1.0f;
    uint16_t w34 = out_hit ? SafeReadVal<uint16_t>(out_hit + 0x34) : 0xFFFF;
    uint16_t bone36 = out_hit ? SafeReadVal<uint16_t>(out_hit + 0x36) : 0xFFFF;
    uint32_t f50 = out_hit ? SafeReadVal<uint32_t>(out_hit + 0x50) : 0;
    uint32_t f54 = out_hit ? SafeReadVal<uint32_t>(out_hit + 0x54) : 0;
    uint16_t w60 = out_hit ? SafeReadVal<uint16_t>(out_hit + 0x60) : 0;
    uint8_t c27_230 = pellet_c27 ? SafeReadVal<uint8_t>(pellet_c27 + 0x230) : 0xFF;
    int32_t pre_a38 = pellet_c27 ? SafeReadVal<int32_t>(pellet_c27 + 0xA38) : 0;

    OnPelletHitboxHit_orig(cb_vt, ctx_ptr, a3);

    int32_t post_a38 = pellet_c27 ? SafeReadVal<int32_t>(pellet_c27 + 0xA38) : 0;
    float r12_4c = a3 ? SafeReadVal<float>(a3 + 0x4C) : 0.0f;
    uintptr_t r12_88 = a3 ? SafeReadVal<uintptr_t>(a3 + 0x88) : 0;
    uintptr_t r12_b0 = a3 ? SafeReadVal<uintptr_t>(a3 + 0xB0) : 0;

    ShotDiagLogFmt("[HITBOX_CB (+0xD381F0 vt+0x68)] pellet_c27=%p out_hit=%p hit_ent=0x%X w34=0x%X bone36=0x%X f50=0x%X f54=0x%X w60=0x%X a38=%d->%d c27_230=%u a3=%p(4C=%.2f,88=%p,B0=%p) pos=(%.2f,%.2f,%.2f)",
        (void*)pellet_c27, (void*)out_hit, hit_entid, w34, bone36, f50, f54, w60, pre_a38, post_a38, c27_230,
        (void*)a3, r12_4c, (void*)r12_88, (void*)r12_b0, hit_pos.X, hit_pos.Y, hit_pos.Z);
}

static std::mutex s_pellet_hit_mutex;
static void NotifyTrackedHealthbarViewModels(uint32_t target_entid, Component_28_STUHealthComponent* target_c28, float cur_hp);

struct PendingDamagePacket {
    uint32_t target_entid;
    int32_t damage_int;
    uint8_t flags; // Bit 0 (0x01) = Critical Hit, Bit 1 (0x02) = Kill
    bool allow_ult_charge;
};
static PendingDamagePacket s_pending_dmg_pkts[32] = {};
static int s_pending_dmg_pkt_count = 0;

static bool IsPulseBombEntity(Entity* ent) {
    if (!ent) return false;
    uint32_t eid = SafeReadVal<uint32_t>((uintptr_t)&ent->entity_id);
    if (eid != 0 && eid != 0xFFFFFFFFu && eid == s_active_pulse_bomb_entid.load()) {
        return true;
    }
    if (ent->resload_entry && ent->resload_entry->valid()) {
        uint64_t res_id = (uint64_t)ent->resload_entry->align()->resource_id;
        if (res_id == 0x0400000000000045ULL) {
            return true;
        }
    }
    return false;
}
static Component_28_STUHealthComponent* s_last_crit_c28 = nullptr;
static uint64_t s_last_crit_capsule_tag = 0;
static uint64_t s_last_crit_ms = 0;
static uint32_t s_pending_snapshot_entid = 0;
static uintptr_t s_last_sys46 = 0;

// +0x1023E80: IsCriticalHitCapsule(Component_28_STUHealthComponent* c28, uint64_t capsule_tag)
// Called by the hit audio/impact worker +0xCB9F60 at +0xCBA214 to decide whether to play the critical hit "dink" sound
// and spawn crit impact effects ([rsp+40h] = al).
// Note: +0x1023E80 first calls +0xCB05A0(c28) which reads dword ptr [c28 + 0x14] (c28->base.is_mirrored) and returns 0
// immediately if != 1. Then it calls +0xF193A0(c28) and checks if capsule_tag is in STUHealthComponent + 0x38.
typedef uint8_t(__fastcall* is_crit_capsule_fn)(Component_28_STUHealthComponent* c28, uint64_t capsule_tag);
static is_crit_capsule_fn IsCriticalHitCapsule_orig = nullptr;
uint8_t __fastcall IsCriticalHitCapsule_hook(Component_28_STUHealthComponent* c28, uint64_t capsule_tag) {
    uint32_t pre_mir14 = c28 ? SafeReadVal<uint32_t>((uintptr_t)c28 + 0x14) : 0;
    uint32_t hash18 = c28 ? SafeReadVal<uint32_t>((uintptr_t)c28 + 0x18) : 0;
    if (c28 && pre_mir14 != 1) {
        *(uint32_t*)((uintptr_t)c28 + 0x14) = 1;
    }
    uint8_t orig_ret = IsCriticalHitCapsule_orig ? IsCriticalHitCapsule_orig(c28, capsule_tag) : 0;
    uint8_t final_ret = orig_ret;
    if (final_ret == 0 && c28 && c28 == s_last_crit_c28 && (GetTickCount64() - s_last_crit_ms) < 250) {
        if (capsule_tag == s_last_crit_capsule_tag || s_last_crit_capsule_tag == 0 || capsule_tag == 0) {
            final_ret = 1;
        }
    }
    ShotDiagLogFmt("[CRIT_CHECK (+0x1023E80)] c28=%p mir14=%u->1 hash18=0x%X tag=0x%llX orig_ret=%u final_ret=%u",
        (void*)c28, pre_mir14, hash18, (unsigned long long)capsule_tag, orig_ret, final_ret);
    return final_ret;
}

typedef uint8_t(__fastcall* get_c1_world_aabb_fn)(Component_1_SceneRendering* c1, Vector4* out_aabb_min_max);

typedef uint8_t(__fastcall* perform_pellet_raycast_fn)(
    uintptr_t cb_vt,
    uintptr_t pellet_c27,
    uintptr_t a3,
    const Vector4* ray_start,
    uintptr_t out_hit,
    uintptr_t hit_ctx,
    uintptr_t a7);
perform_pellet_raycast_fn PerformPelletWorldAndHitboxRaycast_orig = nullptr;
uint8_t __fastcall PerformPelletWorldAndHitboxRaycast_hook(
    uintptr_t cb_vt,
    uintptr_t pellet_c27,
    uintptr_t a3,
    const Vector4* ray_start,
    uintptr_t out_hit,
    uintptr_t hit_ctx,
    uintptr_t a7)
{
    uint64_t dmy_1b8_pre = 0;
    uint64_t dmy_1b8_post = 0;
    uintptr_t dmy_198 = 0;
    Vector4 dmy_c12_min_pre{};
    Vector4 dmy_c12_max_pre{};
    Vector4 dmy_c12_min_post{};
    Vector4 dmy_c12_max_post{};
    s_dummy_c12_aabb_addr = 0;

    EntityAdminBase* ea = GameEntityAdmin();
    if (ea) {
        uint32_t local_ctrl_id = ea->local_entid;
        uint32_t local_mdl_id = 0;
        if (Entity* local_ctrl = ea->getLocalEnt()) {
            if (auto* pet = local_ctrl->getById<Component_20_ModelReference>(0x20)) {
                local_mdl_id = pet->cam_attach_entid ? pet->cam_attach_entid : (pet->aim_entid ? pet->aim_entid : pet->movement_attach_entid);
            }
        }
        for (Entity* ent : *ea) {
            if (!ent || ent->entity_id == local_ctrl_id || ent->entity_id == local_mdl_id) continue;
            if (!ent->hasComponent(0x15) || !ent->hasComponent(0x28)) continue;
            auto* c1 = ent->getById<Component_1_SceneRendering>(1);
            auto* c12 = ent->getById<Component_12_STUMovementStateComponent>(0x12);
            if (c1) {
                dmy_1b8_pre = SafeReadVal<uint64_t>((uintptr_t)c1 + 0x1B8);
                dmy_198 = SafeReadVal<uintptr_t>((uintptr_t)c1 + 0x198);
                dmy_1b8_post = dmy_1b8_pre;
            }
            if (c12) {
                uintptr_t aabb_addr = (uintptr_t)c12 + 0xA2D0;
                s_dummy_c12_aabb_addr = aabb_addr;
                dmy_c12_min_pre = SafeReadVal<Vector4>(aabb_addr + 0x00);
                dmy_c12_max_pre = SafeReadVal<Vector4>(aabb_addr + 0x10);
                dmy_c12_min_post = dmy_c12_min_pre;
                dmy_c12_max_post = dmy_c12_max_pre;
            }
            break;
        }
    }

    s_a45aa0_total_calls = 0;
    s_a45aa0_dummy_calls = 0;
    s_a45aa0_dummy_last_ret = -1;
    s_in_pellet_raycast_1038860 = true;

    uint8_t ret = PerformPelletWorldAndHitboxRaycast_orig(cb_vt, pellet_c27, a3, ray_start, out_hit, hit_ctx, a7);

    s_in_pellet_raycast_1038860 = false;

    uint64_t c27_228 = pellet_c27 ? SafeReadVal<uint64_t>(pellet_c27 + 0x228) : 0;
    uint8_t c27_230 = pellet_c27 ? SafeReadVal<uint8_t>(pellet_c27 + 0x230) : 0xFF;
    uintptr_t ctx_90 = hit_ctx ? SafeReadVal<uintptr_t>(hit_ctx + 0x90) : 0;
    uint8_t hb_aa49 = ctx_90 ? SafeReadVal<uint8_t>(ctx_90 + 0xAA49) : 0xFF;
    uint8_t hb_aa48 = ctx_90 ? SafeReadVal<uint8_t>(ctx_90 + 0xAA48) : 0xFF;

    Vector4 hit_pos = out_hit ? SafeReadVal<Vector4>(out_hit + 0x00) : Vector4{};
    Entity* hit_ent = out_hit ? SafeReadVal<Entity*>(out_hit + 0x20) : nullptr;
    uint32_t hit_entid = hit_ent ? SafeReadVal<uint32_t>((uintptr_t)&hit_ent->entity_id) : 0;
    float hit_frac = out_hit ? SafeReadVal<float>(out_hit + 0x30) : -1.0f;
    uint16_t hit_w34 = out_hit ? SafeReadVal<uint16_t>(out_hit + 0x34) : 0;
    uint16_t hit_bone36 = out_hit ? SafeReadVal<uint16_t>(out_hit + 0x36) : 0xFFFF;
    uint64_t hit_tag40 = out_hit ? SafeReadVal<uint64_t>(out_hit + 0x40) : 0;
    uint32_t hit_f50 = out_hit ? SafeReadVal<uint32_t>(out_hit + 0x50) : 0;
    uint32_t hit_f54 = out_hit ? SafeReadVal<uint32_t>(out_hit + 0x54) : 0;
    uint16_t hit_w60 = out_hit ? SafeReadVal<uint16_t>(out_hit + 0x60) : 0;

    ShotDiagLogFmt("[RAY_1038860] ret=%u c27(228=0x%llX,230=%u) a45aa0(tot=%d,dmy=%d,dret=%d) hb(aa49=%u,aa48=%u) c1(1b8=0x%llX->0x%llX,198=%p) c12_aabb((%.1f,%.1f,%.1f)..(%.1f,%.1f,%.1f)) -> HIT(ent=0x%X,t=%.3f,w34=0x%X,bone36=0x%X,tag40=0x%llX,f50=0x%X,f54=0x%X,w60=0x%X,pos=(%.2f,%.2f,%.2f))",
        ret, (unsigned long long)c27_228, c27_230,
        s_a45aa0_total_calls, s_a45aa0_dummy_calls, s_a45aa0_dummy_last_ret,
        hb_aa49, hb_aa48,
        (unsigned long long)dmy_1b8_pre, (unsigned long long)dmy_1b8_post, (void*)dmy_198,
        dmy_c12_min_pre.X, dmy_c12_min_pre.Y, dmy_c12_min_pre.Z, dmy_c12_max_pre.X, dmy_c12_max_pre.Y, dmy_c12_max_pre.Z,
        hit_entid, hit_frac, hit_w34, hit_bone36, (unsigned long long)hit_tag40, hit_f50, hit_f54, hit_w60, hit_pos.X, hit_pos.Y, hit_pos.Z);

    // Apply native skeletal hitbox damage when +0x1038860 intersects a skeletal hitbox capsule (hit_w34 != 0xFFFF, hit_f54 != 0xFFFFFFFF),
    // AND apply distance-falloff AoE splash damage when an explosive projectile (e.g. Soldier: 76 Helix Rockets) hits level geometry or an enemy.
    // NOTE: +0x1032CB0 -> +0x1038860 runs in parallel on worker threads for multi-pellet weapons (e.g. Tracer's 2 pellets per shot),
    // so guard all shared state with s_pellet_hit_mutex and leave VarBag / UI ViewModel / System #46 notifications to the main thread (+0xCA7200).
    if (WeaponHardpointDiag::s_dummy_damage_enabled && ret != 0 && c27_230 == 0) {
        std::lock_guard<std::mutex> hit_lock(s_pellet_hit_mutex);
        uint32_t local_ctrl_id = ea ? ea->local_entid : 0;
        uint32_t local_mdl_id = 0;
        uint64_t shooter_heroid = 0;
        if (ea && local_ctrl_id) {
            if (Entity* ctrl = ea->getEntById(local_ctrl_id)) {
                if (auto* c20 = ctrl->getById<Component_20_ModelReference>(0x20)) {
                    local_mdl_id = c20->cam_attach_entid ? c20->cam_attach_entid : c20->aim_entid;
                }
                if (auto* c3f = ctrl->getById<Component_3F_PlayerInfo>(0x3F)) {
                    shooter_heroid = (uint64_t)c3f->selected_heroid;
                }
            }
        }
        Entity* shooter_ent = (ea && s_last_shot_shooter_id) ? ea->getEntById(s_last_shot_shooter_id) : nullptr;
        if (!shooter_ent && ea && local_mdl_id) shooter_ent = ea->getEntById(local_mdl_id);
        Entity* proj_ent = pellet_c27 ? SafeReadVal<Entity*>(pellet_c27 + 0x08) : nullptr;
        uintptr_t cb_vt_ptr = cb_vt ? SafeReadVal<uintptr_t>(cb_vt) : 0;
        bool is_proj_vt = (cb_vt_ptr == globals::gameBase + 0x15C3048);
        bool is_standalone_proj = is_proj_vt || (proj_ent != nullptr && proj_ent != shooter_ent);
        bool is_pulse_bomb = IsPulseBombEntity(proj_ent);
        // Explosive projectile types (Soldier: 76 Helix Rockets 0x6E, Pharah Rocket 0x08, Junkrat Frag 0x65, Zarya RMB 0x68)
        bool is_explosive_proj = !is_pulse_bomb && is_standalone_proj && (
            shooter_heroid == 0x02E000000000006EULL ||
            shooter_heroid == 0x02E0000000000008ULL ||
            shooter_heroid == 0x02E0000000000065ULL ||
            shooter_heroid == 0x02E0000000000068ULL);

        uint32_t direct_damaged_entid = 0;
        if (hit_ent && hit_w34 != 0xFFFF && hit_f54 != 0xFFFFFFFF &&
            hit_entid != local_ctrl_id && hit_entid != local_mdl_id && hit_entid != s_last_shot_shooter_id) {
            auto* c10 = hit_ent->getById<Component_10_FilterBits>(0x10);
            bool same_team = (s_last_shot_team && c10 && (c10->filter_bits & 0x3C0000ULL) == (s_last_shot_team & 0x3C0000ULL));
            auto* c28 = hit_ent->getById<Component_28_STUHealthComponent>(0x28);
            auto* c1 = hit_ent->getById<Component_1_SceneRendering>(1);
            if (!same_team && c28 && c28->CalcTotalCurrentHealth() > 0.0f) {
                if (is_pulse_bomb) {
                    uint32_t prev_stuck = s_pulse_bomb_stuck_entid.exchange(hit_entid);
                    if (prev_stuck == hit_entid) {
                        // Already applied direct stick damage on initial impact
                        return ret;
                    }
                }

                direct_damaged_entid = hit_entid;
                // Ensure c28->base.is_mirrored (+0x14) is 1 so +0x1023E80 -> +0xCB05A0(c28) allows critical hit checks
                *(uint32_t*)((uintptr_t)c28 + 0x14) = 1;
                float base_y = c1 ? c1->position.Y : 0.0f;
                float rel_y = hit_pos.Y - base_y;
                // Native critical hit region tag 0x0DA000000000000D, skeletal head bone 0x11, or head capsule 0x2A (explosive projectiles do not crit)
                bool crit = !is_pulse_bomb && !is_explosive_proj && (hit_tag40 == 0x0DA000000000000DULL || hit_f54 == 0x11 || hit_w34 == 0x2A);
                if (crit) {
                    if (out_hit) {
                        *(uint64_t*)(out_hit + 0x40) = 0x0DA000000000000DULL;
                    }
                    hit_tag40 = 0x0DA000000000000DULL;
                    s_last_crit_c28 = c28;
                    s_last_crit_capsule_tag = hit_tag40;
                    s_last_crit_ms = GetTickCount64();
                } else if (s_last_crit_c28 == c28 && (GetTickCount64() - s_last_crit_ms) >= 50) {
                    s_last_crit_c28 = nullptr;
                }
                // Pulse Bomb direct stick deals 5.0 damage (0x86000000000006C) and does NOT grant Ult charge
                float base_pellet_dmg = ResolveShotBaseDamage(shooter_ent, proj_ent);
                float dmg = is_pulse_bomb ? 5.0f : (base_pellet_dmg * (crit ? WeaponHardpointDiag::s_dummy_crit_mult : 1.0f));
                float hp = ApplyDamageToHealthComponent(c28, dmg);
                // Immediately clear Bit 16 (0x10000) when HP reaches 0 so remaining pellets / shots pass through the ragdoll corpse
                if (hp <= 0.0f && c1) {
                    *(uint32_t*)((uintptr_t)c1 + 0x1B8) &= ~0x10000u;
                }
                if (s_pending_dmg_pkt_count < 32) {
                    int32_t dmg_int = (int32_t)(dmg + 0.5f);
                    if (dmg_int < 1) dmg_int = 1;
                    uint8_t pkt_flags = (uint8_t)((crit ? 1 : 0) | (hp <= 0.0f ? 2 : 0));
                    s_pending_dmg_pkts[s_pending_dmg_pkt_count++] = PendingDamagePacket{ hit_entid, dmg_int, pkt_flags, !is_pulse_bomb };
                }
                s_last_hit_dummy_entid = hit_entid;
                s_last_hit_dummy_ms = GetTickCount64();
                s_redirect_dummy_c1 = c1;
                s_redirect_hit_pos = hit_pos;
                s_redirect_hit_pos.W = 1.0f;
                WeaponHardpointDiag::s_dummy_hits++;
                if (WeaponHardpointDiag::s_dummy_hits == 1 && hp > 0.0f) {
                    s_pending_snapshot_entid = hit_entid;
                    s_pending_dummy_hit_dump_ticks = 4;
                } else if (hp <= 0.0f) {
                    s_pending_dummy_hit_dump_ticks = 4;
                }
                std::string hit_msg = std::format(
                    "Hit #{} on 0x{:X} (bone=0x{:X},w34=0x{:X},tag40=0x{:X},relY={:.2f}m): -{:.1f}{} -> HP {:.1f}",
                    WeaponHardpointDiag::s_dummy_hits, hit_entid, hit_f54, hit_w34, (unsigned long long)hit_tag40, rel_y, dmg, crit ? " CRIT" : "", hp);
                WeaponHardpointDiag::s_last_dummy_hit = hit_msg;
                ShotDiagLogFmt("[DUMMY_HIT] %s", hit_msg.c_str());
            }
        }

        // Explosive projectile AoE splash damage on geometry/wall/floor impact (or splash to other enemies near a direct hit)
        static uintptr_t s_last_aoe_proj_c27 = 0;
        if (is_explosive_proj && ea && pellet_c27 != 0 && pellet_c27 != s_last_aoe_proj_c27) {
            s_last_aoe_proj_c27 = pellet_c27;
            const float kAoeOuterRadius = 3.5f; // 3.5m splash radius around geometry impact point
            const float kAoeInnerRadius = 0.35f; // Point-blank capsule contact reaches max damage cap
            const float kMaxHelixDmgCap = ResolveShotBaseDamage(shooter_ent, proj_ent); // 120.0f for Helix Rockets
            const float kMinSplashDmg = 15.0f; // Minimum splash damage at outer edge

            for (Entity* ent : *ea) {
                if (!ent) continue;
                uint32_t tid = ent->entity_id;
                if (tid == local_ctrl_id || tid == local_mdl_id || tid == s_last_shot_shooter_id || tid == direct_damaged_entid) continue;
                if (!ent->hasComponent(0x15) || !ent->hasComponent(0x28)) continue;
                auto* c10 = ent->getById<Component_10_FilterBits>(0x10);
                bool same_team = (s_last_shot_team && c10 && (c10->filter_bits & 0x3C0000ULL) == (s_last_shot_team & 0x3C0000ULL));
                if (same_team) continue;
                auto* c28 = ent->getById<Component_28_STUHealthComponent>(0x28);
                auto* c1 = ent->getById<Component_1_SceneRendering>(1);
                if (!c28 || !c1 || c28->CalcTotalCurrentHealth() <= 0.0f) continue;

                // Measure distance from impact point (hit_pos) to closest point on enemy vertical capsule [pos.Y .. pos.Y + 1.8m]
                Vector4 ep = c1->position;
                float closest_y = hit_pos.Y;
                if (closest_y < ep.Y) closest_y = ep.Y;
                else if (closest_y > ep.Y + 1.8f) closest_y = ep.Y + 1.8f;
                float dx = ep.X - hit_pos.X;
                float dy = closest_y - hit_pos.Y;
                float dz = ep.Z - hit_pos.Z;
                float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
                if (dist > kAoeOuterRadius) continue;

                float eff_dist = (dist > kAoeInnerRadius) ? (dist - kAoeInnerRadius) : 0.0f;
                float t_falloff = eff_dist / (kAoeOuterRadius - kAoeInnerRadius);
                if (t_falloff > 1.0f) t_falloff = 1.0f;
                float aoe_dmg = kMaxHelixDmgCap - t_falloff * (kMaxHelixDmgCap - kMinSplashDmg);
                if (aoe_dmg > kMaxHelixDmgCap) aoe_dmg = kMaxHelixDmgCap;
                if (aoe_dmg < kMinSplashDmg) aoe_dmg = kMinSplashDmg;

                *(uint32_t*)((uintptr_t)c28 + 0x14) = 1;
                float hp = ApplyDamageToHealthComponent(c28, aoe_dmg);
                if (hp <= 0.0f) {
                    *(uint32_t*)((uintptr_t)c1 + 0x1B8) &= ~0x10000u;
                }
                if (s_pending_dmg_pkt_count < 32) {
                    int32_t dmg_int = (int32_t)(aoe_dmg + 0.5f);
                    if (dmg_int < 1) dmg_int = 1;
                    uint8_t pkt_flags = (uint8_t)(hp <= 0.0f ? 2 : 0);
                    s_pending_dmg_pkts[s_pending_dmg_pkt_count++] = PendingDamagePacket{ tid, dmg_int, pkt_flags, true };
                }
                s_last_hit_dummy_entid = tid;
                s_last_hit_dummy_ms = GetTickCount64();
                WeaponHardpointDiag::s_dummy_hits++;
                std::string aoe_msg = std::format(
                    "AoE Splash #{} on 0x{:X} (dist={:.2f}m from impact ({:.1f},{:.1f},{:.1f})): -{:.1f} -> HP {:.1f}",
                    WeaponHardpointDiag::s_dummy_hits, tid, dist, hit_pos.X, hit_pos.Y, hit_pos.Z, aoe_dmg, hp);
                WeaponHardpointDiag::s_last_dummy_hit = aoe_msg;
                ShotDiagLogFmt("[HELIX_AOE_HIT] %s", aoe_msg.c_str());
            }
        }
    }

    return ret;
}

// +0xCA7200: System #46's per-frame overhead healthbar visibility updater (called at the end of System #46::Tick
// via +0xCA5C8D -> jmp +0xCA7200). In online games, network damage packets (+0xCA7860) append 16-byte entries
// { target_entid, hit_timestamp, damage_int, flags } to the LocalPlayer's Component_2F ring buffer at +0x15E0
// (capacity at +0x17E0, head at +0x17F0, count at +0x17F4), increment the LocalPlayer's reticle HitPip counters
// (HitPip_HitCount at +0xC0, HitPip_CritCount at +0xC4, HitPip_KillCount at +0xC8 via +0xCF7B10), mark the reticle
// ViewModel (0x400) dirty, and broadcast a 0x160-byte combat event to System #46 listeners.
// Here we drain s_pending_dmg_pkts through the native +0xCA7860 handler on the main thread and expire the ring-buffer
// entry when the target is at full health or dead.
typedef void(__fastcall* update_hb_vis_fn)(uintptr_t sys46);
static update_hb_vis_fn UpdateHealthbarVisibility_orig = nullptr;
static uint8_t s_last_logged_dummy_485 = 0xFF;
static float s_last_logged_dummy_hp = -1.0f;
static constexpr int kMaxTrackedHbSubobjs = 64;
static uintptr_t s_tracked_hb_subobjs[kMaxTrackedHbSubobjs] = {};
static uint32_t s_tracked_hb_entids[kMaxTrackedHbSubobjs] = {};
static float s_tracked_hb_last_hp[kMaxTrackedHbSubobjs] = {};

static void TrackHealthbarSubobj(uintptr_t subobj) {
    if (!subobj) return;
    uintptr_t expected_vt = globals::gameBase + 0x15D1B08;
    if (SafeReadVal<uintptr_t>(subobj) != expected_vt) return;
    for (int i = 0; i < kMaxTrackedHbSubobjs; i++) {
        if (s_tracked_hb_subobjs[i] == subobj) return;
    }
    for (int i = 0; i < kMaxTrackedHbSubobjs; i++) {
        if (!s_tracked_hb_subobjs[i] || SafeReadVal<uintptr_t>(s_tracked_hb_subobjs[i]) != expected_vt) {
            s_tracked_hb_subobjs[i] = subobj;
            s_tracked_hb_entids[i] = 0;
            s_tracked_hb_last_hp[i] = -1.0f;
            return;
        }
    }
    for (int i = 0; i < kMaxTrackedHbSubobjs; i++) {
        if (SafeReadVal<uint32_t>(s_tracked_hb_subobjs[i] + 0x10) == 0) {
            s_tracked_hb_subobjs[i] = subobj;
            s_tracked_hb_entids[i] = 0;
            s_tracked_hb_last_hp[i] = -1.0f;
            return;
        }
    }
}

static void NotifyTrackedHealthbarViewModels(uint32_t target_entid, Component_28_STUHealthComponent* target_c28, float cur_hp) {
    if (!target_entid || !target_c28) return;
    uintptr_t expected_sub_vt = globals::gameBase + 0x15D1B08;
    uintptr_t expected_vm_vt = globals::gameBase + 0x15D1B20;
    auto on_c28_changed = (void(__fastcall*)(Component_28_STUHealthComponent*, uintptr_t, uint8_t, uintptr_t))(globals::gameBase + 0xD1D6F0);
    for (int i = 0; i < kMaxTrackedHbSubobjs; i++) {
        uintptr_t subobj = s_tracked_hb_subobjs[i];
        if (!subobj) continue;
        if (SafeReadVal<uintptr_t>(subobj) != expected_sub_vt) {
            s_tracked_hb_subobjs[i] = 0;
            s_tracked_hb_entids[i] = 0;
            s_tracked_hb_last_hp[i] = -1.0f;
            continue;
        }
        uintptr_t vm = subobj - 0x3E0;
        if (SafeReadVal<uintptr_t>(vm) != expected_vm_vt) continue;
        uint32_t bound_entid = SafeReadVal<uint32_t>(subobj + 0x10);
        if (bound_entid != target_entid) continue;
        if (s_tracked_hb_entids[i] != target_entid) {
            s_tracked_hb_entids[i] = target_entid;
            s_tracked_hb_last_hp[i] = -1.0f;
        }
        float vm_cached_hp = SafeReadVal<float>(subobj + 0x14);
        if (fabsf(s_tracked_hb_last_hp[i] - cur_hp) > 0.01f || fabsf(vm_cached_hp - cur_hp) > 0.01f) {
            s_tracked_hb_last_hp[i] = cur_hp;
            on_c28_changed(target_c28, 0, 0x28, vm);
        }
    }
}

static void TriggerHeroDeathScream(EntityAdminBase* ea, Entity* victim_mdl, uint32_t attacker_entid) {
    if (!ea || !victim_mdl) return;
    STUBase<>* vs_stu = nullptr;
    // +0xC4070F..+0xC40798: STUVoiceSetComponent is Component_46 (0x46, bit 6 of bitmask_A0, key 0x528F2F94), NOT 0x40!
    if (ComponentBase* c46 = victim_mdl->getById(0x46)) {
        auto get_comp_stu = (STUBase<>*(__fastcall*)(ComponentBase*))(globals::gameBase + 0xF193A0);
        STUBase<>* stu_vc = get_comp_stu(c46);
        uint8_t b30 = SafeReadVal<uint8_t>((uintptr_t)c46 + 0x30);
        if (b30 != 0 || (stu_vc && !IsBadReadPtr(stu_vc, 0x18))) {
            // +0x1019430(Component_46*): native non-obfuscated getter returning STUVoiceSet* directly
            // (Only call when b30 != 0 or stu_vc != nullptr, because +0x101944A dereferences [rax+0x10] without a null check!)
            auto get_voiceset_stu = (STUBase<>*(__fastcall*)(ComponentBase*))(globals::gameBase + 0x1019430);
            vs_stu = get_voiceset_stu(c46);
        }
        if ((!vs_stu || IsBadReadPtr(vs_stu, 0x58)) && stu_vc && !IsBadReadPtr(stu_vc, 0x18)) {
            auto* vd_ref = (STUResourceReference*)((uintptr_t)stu_vc + 0x08);
            vs_stu = vd_ref->get_STU();
            if (!vs_stu && vd_ref->resource_id) vs_stu = stu_resources::GetByID(vd_ref->resource_id);
        }
    }
    if (!vs_stu || IsBadReadPtr(vs_stu, 0x58)) {
        ShotDiagLogFmt("[DEATH_SCREAM] victim=0x%X ABORT: no STUVoiceSet on Component_46", victim_mdl->entity_id);
        return;
    }

    struct DeadStimGroup {
        uint64_t stim_id = 0;
        STUBase<>* stim_stu = nullptr;
        uint32_t u7c = 0;
        std::vector<uint64_t> uncond_vls;
        std::vector<uint64_t> cond_vls;
    };
    std::vector<DeadStimGroup> groups;

    // +0x104D191..+0x104D1C2: STUBullshitListFull<STUVoiceLineInstance*> at vs_stu + 0x48 (flag at +0x50)
    uintptr_t bs_ptr = SafeReadVal<uintptr_t>((uintptr_t)vs_stu + 0x48);
    uint64_t bs_flag = SafeReadVal<uint64_t>((uintptr_t)vs_stu + 0x50);
    int32_t vl_count = 0;
    uintptr_t vl_arr = 0;
    if (bs_ptr && !IsBadReadPtr((const void*)bs_ptr, 0x10)) {
        if (bs_flag == 1) {
            vl_arr = SafeReadVal<uintptr_t>(bs_ptr + 0x00);
            vl_count = SafeReadVal<int32_t>(bs_ptr + 0x08);
        } else {
            vl_count = SafeReadVal<int32_t>(bs_ptr + 0x00);
            vl_arr = SafeReadVal<uintptr_t>(bs_ptr + 0x08);
        }
    }
    if (vl_arr && vl_count > 0 && vl_count < 4096 && !IsBadReadPtr((const void*)vl_arr, (size_t)vl_count * 8)) {
        for (int32_t i = 0; i < vl_count; i++) {
            uintptr_t inst = SafeReadVal<uintptr_t>(vl_arr + (uintptr_t)i * 8);
            if (!inst || IsBadReadPtr((const void*)inst, 0x30)) continue;
            // Verify STUVoiceLineInstance has audio data at +0x28 (checked by +0x104CE30)
            if (SafeReadVal<uintptr_t>(inst + 0x28) == 0) continue;
            // STUResourceReference for m_voiceLine at inst + 0x08 (resource_id at inst + 0x10)
            auto* vl_ref = (STUResourceReference*)(inst + 0x08);
            uint64_t vl_id = SafeReadVal<uint64_t>(inst + 0x10);
            if (!vl_id) continue;
            STUBase<>* vl_stu = vl_ref->get_STU();
            if (!vl_stu) vl_stu = stu_resources::GetByID(vl_id);
            if (!vl_stu || IsBadReadPtr(vl_stu, 0x40)) continue;

            // STUResourceReference for STUVoiceStimulus at vl_stu + 0x18 (resource_id at vl_stu + 0x20)
            auto* stim_ref = (STUResourceReference*)((uintptr_t)vl_stu + 0x18);
            uint64_t stim_id = SafeReadVal<uint64_t>((uintptr_t)vl_stu + 0x20);
            uintptr_t crit_ptr = SafeReadVal<uintptr_t>((uintptr_t)vl_stu + 0x38);
            if (!stim_id) continue;
            STUBase<>* stim_stu = stim_ref->get_STU();
            if (!stim_stu) stim_stu = stu_resources::GetByID(stim_id);
            if (!stim_stu || IsBadReadPtr(stim_stu, 0x94)) continue;

            // +0xC40835 in PlayVoice (+0xC40350): byte ptr [stim_stu + 0x90] != 0 allows playing while dead!
            uint8_t allow_dead = SafeReadVal<uint8_t>((uintptr_t)stim_stu + 0x90);
            if (!allow_dead) continue;
            // +0xC40858..+0xC408B2 in PlayVoice (+0xC40350): dword ptr [stim_stu + 0x7C] is audience team filter:
            //   0 = ally-only, 1 = enemy-only, >=2 = audible to both teams
            uint32_t u7c = SafeReadVal<uint32_t>((uintptr_t)stim_stu + 0x7C);

            DeadStimGroup* grp = nullptr;
            for (auto& g : groups) {
                if (g.stim_id == stim_id) { grp = &g; break; }
            }
            if (!grp) {
                groups.push_back(DeadStimGroup{ stim_id, stim_stu, u7c, {}, {} });
                grp = &groups.back();
            }
            if (crit_ptr == 0) grp->uncond_vls.push_back(vl_id);
            else grp->cond_vls.push_back(vl_id);
        }
    }

    if (groups.empty()) {
        ShotDiagLogFmt("[DEATH_SCREAM] victim=0x%X ABORT: no STUVoiceStimulus with byte_90 != 0 (vl_count=%d)", victim_mdl->entity_id, vl_count);
        return;
    }

    // Sort candidate death stimuli:
    //   1. Prefer u7c >= 2 (audible to both teams) over u7c == 1 (enemy-only) over u7c == 0 (ally-only)
    //   2. Prefer more unconditional voice lines (normal combat death scream)
    auto tier_of = [](uint32_t u7c) -> int {
        if (u7c >= 2) return 2;
        if (u7c == 1) return 1;
        return 0;
    };
    std::sort(groups.begin(), groups.end(), [&](const DeadStimGroup& a, const DeadStimGroup& b) {
        int ta = tier_of(a.u7c), tb = tier_of(b.u7c);
        if (ta != tb) return ta > tb;
        if (a.uncond_vls.size() != b.uncond_vls.size()) return a.uncond_vls.size() > b.uncond_vls.size();
        return a.cond_vls.size() > b.cond_vls.size();
    });

    static uint32_t s_scream_rng = 0x13579BDFu;
    s_scream_rng = s_scream_rng * 1664525u + 1013904223u;

    VoiceSystem* voicesys = GetVoiceSystem(ea);
    uint8_t play_ret = 0;
    uint64_t used_stim = 0;
    uint64_t used_vl = 0;
    uint32_t used_u7c = 0;
    size_t used_uncond = 0, used_cond = 0;

    if (voicesys && voicesys->vfptr && voicesys->vfptr->PlayVoice) {
        // +0xC40935 calls +0xC40950(voicesys, speaking_ent_id, new_handle) to stop prior active voice lines on speaking_ent.
        // Calling it with 0xFFFFFFFFu BEFORE PlayVoice clears any active hurt grunt on victim_mdl so +0xC408DD concurrency checks pass!
        auto stop_ent_voice = (void(__fastcall*)(VoiceSystem*, uint32_t, uint32_t))(globals::gameBase + 0xC40950);
        stop_ent_voice(voicesys, victim_mdl->entity_id, 0xFFFFFFFFu);

        auto play_fn = (uint8_t(__fastcall*)(VoiceSystem*, VoiceSystem_PlayVoice*))voicesys->vfptr->PlayVoice;
        for (const auto& g : groups) {
            const auto& pool = !g.uncond_vls.empty() ? g.uncond_vls : g.cond_vls;
            if (pool.empty()) continue;
            uint64_t chosen_vl = pool[s_scream_rng % pool.size()];
            used_stim = g.stim_id;
            used_vl = chosen_vl;
            used_u7c = g.u7c;
            used_uncond = g.uncond_vls.size();
            used_cond = g.cond_vls.size();

            VoiceSystem_PlayVoice vp{};
            vp.entity_1 = victim_mdl->entity_id;
            vp.entity_2 = victim_mdl->entity_id;
            vp.instigator_ent = victim_mdl->entity_id;
            vp.voice_stimulus = g.stim_id;
            vp.voice_line = chosen_vl;
            vp.field_20 = 0;
            vp.field_21 = 0;
            play_ret = play_fn(voicesys, &vp);
            if (!play_ret && attacker_entid != 0) {
                // Retry with LocalPlayer attacker_entid as instigator (+0xC40697 sets sil=1 to bypass team filter at +0xC40858)
                vp.instigator_ent = attacker_entid;
                play_ret = play_fn(voicesys, &vp);
            }
            if (play_ret) break;
        }
    }
    ShotDiagLogFmt("[DEATH_SCREAM] victim=0x%X groups=%zu stim=0x%llX u7c=%u vl=0x%llX (uncond=%zu,cond=%zu) -> play_ret=%u",
        victim_mdl->entity_id, groups.size(),
        (unsigned long long)used_stim, used_u7c, (unsigned long long)used_vl,
        used_uncond, used_cond, play_ret);
}

// +0x1027940: GetEntityDisplayName(Entity* ent, uint8_t sanitize_flag, char* out_buf)
// Offline, *(uintptr_t*)(gameBase + 0x1854DE0) (BNet player-name callback) is null or fails for locally spawned dummies,
// causing +0x1027940 to write "" into out_buf (0x40 bytes). Hooking +0x1027940 populates out_buf from the entity's
// STUHero display name (0xcd16baa, e.g. "Tracer", "Reinhardt") when out_buf[0] == '\0'.
typedef void(__fastcall* get_entity_display_name_fn)(Entity* ent, uint8_t sanitize_flag, char* out_buf);
static get_entity_display_name_fn GetEntityDisplayName_orig = nullptr;

static void __fastcall GetEntityDisplayName_hook(Entity* ent, uint8_t sanitize_flag, char* out_buf) {
    GetEntityDisplayName_orig(ent, sanitize_flag, out_buf);
    if (!ent || !out_buf || out_buf[0] != '\0') return;

    EntityAdminBase* ea = GameEntityAdmin();
    uint64_t heroid = 0;
    uint64_t model_res_id = ent->resload_entry ? (uint64_t)ent->resload_entry->resource_id : 0;

    if (auto* c3f = ent->getById<Component_3F_PlayerInfo>(0x3F)) {
        heroid = (uint64_t)c3f->selected_heroid;
        if (heroid == 0 && ea) {
            if (auto* c20 = ent->getById<Component_20_ModelReference>(0x20)) {
                uint32_t mdl_id = c20->cam_attach_entid ? c20->cam_attach_entid : c20->aim_entid;
                if (Entity* mdl_ent = mdl_id ? ea->getEntById(mdl_id) : nullptr) {
                    if (mdl_ent->resload_entry) model_res_id = (uint64_t)mdl_ent->resload_entry->resource_id;
                }
            }
        }
    } else if (auto* c1f = ent->getById<Component_1F_Backref>(0x1F)) {
        if (c1f->backref_entid && ea) {
            if (Entity* ctrl_ent = ea->getEntById(c1f->backref_entid)) {
                if (auto* c3f = ctrl_ent->getById<Component_3F_PlayerInfo>(0x3F)) {
                    heroid = (uint64_t)c3f->selected_heroid;
                }
            }
        }
    }

    if (heroid == 0 && model_res_id != 0) {
        for (auto& hero : stu_resources::GetIfAssignableTo(stringHash("STUHero"))) {
            auto stu = hero.second->to_editable();
            auto* ent_ref = stu.get_argument_resource("m_gameplayEntity");
            if (ent_ref && (uint64_t)ent_ref->resource_id == model_res_id) {
                heroid = (uint64_t)hero.first;
                break;
            }
        }
    }

    const char* hero_name = nullptr;
    if (heroid != 0) {
        if (auto* hero_stu = stu_resources::GetByID((__int64)heroid)) {
            auto stu = hero_stu->to_editable();
            if (auto* name_ref = stu.get_argument_resource(0xcd16baa)) {
                hero_name = name_ref->get_UXDisplayString();
            }
        }
    }

    if (hero_name && hero_name[0] != '\0') {
        strncpy_s(out_buf, 0x40, hero_name, _TRUNCATE);
    } else {
        strncpy_s(out_buf, 0x40, "BOT", _TRUNCATE);
    }
}

void __fastcall UpdateHealthbarVisibility_hook(uintptr_t sys46) {
    s_last_sys46 = sys46;
    EntityAdminBase* ea = sys46 ? SafeReadVal<EntityAdminBase*>(sys46 + 0x08) : nullptr;
    Entity* target_dummy = nullptr;
    Component_28_STUHealthComponent* target_c28 = nullptr;
    Component_2F_LocalPlayer* local_c2f = nullptr;
    float cur_hp = 0.0f, max_hp = 0.0f;
    bool should_show_hb = false;

    if (ea && ea->local_entid) {
        SyncEnemyHitboxFlags(ea);
        Entity* local_ctrl = ea->getEntById(ea->local_entid);
        local_c2f = local_ctrl ? local_ctrl->getById<Component_2F_LocalPlayer>(0x2F) : nullptr;
        uint32_t local_mdl_id = 0;
        if (local_ctrl) {
            if (auto* c20 = local_ctrl->getById<Component_20_ModelReference>(0x20)) {
                local_mdl_id = c20->cam_attach_entid ? c20->cam_attach_entid : c20->aim_entid;
            }
        }
        if (local_c2f) {
            if (local_c2f->field_17E0 <= 0) local_c2f->field_17E0 = 0x20;

            // Check if an armed Pulse Bomb's 1.0s fuse has expired and detonate AoE damage on the main thread
            if (s_pulse_bomb_armed.load() && GetTickCount64() >= s_pulse_bomb_detonate_ms.load()) {
                s_pulse_bomb_armed.store(false);
                uint32_t stuck_entid = s_pulse_bomb_stuck_entid.exchange(0xFFFFFFFFu);
                uint32_t pb_entid = s_active_pulse_bomb_entid.load();
                Vector4 bomb_pos = s_pulse_bomb_land_pos;
                if (pb_entid != 0) {
                    if (Entity* pb_ent = ea->getEntById(pb_entid)) {
                        if (ComponentBase* pb_c27 = pb_ent->getById(0x27)) {
                            Vector4 c27_b0 = SafeReadVal<Vector4>((uintptr_t)pb_c27 + 0xB0);
                            if (c27_b0.X != 0.0f || c27_b0.Y != 0.0f || c27_b0.Z != 0.0f) {
                                bomb_pos = c27_b0;
                            }
                        }
                    }
                }
                if (stuck_entid != 0 && stuck_entid != 0xFFFFFFFFu) {
                    if (Entity* stuck_ent = ea->getEntById(stuck_entid)) {
                        if (auto* stuck_c1 = stuck_ent->getById<Component_1_SceneRendering>(1)) {
                            bomb_pos = stuck_c1->position;
                            bomb_pos.Y += 1.0f;
                        }
                    }
                }

                uint32_t local_ctrl_id = ea->local_entid;
                for (Entity* ent : *ea) {
                    if (!ent || ent->entity_id == local_ctrl_id || ent->entity_id == local_mdl_id) continue;
                    if (!ent->hasComponent(0x15) || !ent->hasComponent(0x28)) continue;

                    auto* c28 = ent->getById<Component_28_STUHealthComponent>(0x28);
                    auto* c1  = ent->getById<Component_1_SceneRendering>(1);
                    if (!c28 || !c1 || c28->CalcTotalCurrentHealth() <= 0.0f) continue;

                    Vector4 target_pos = c1->position;
                    float dx = bomb_pos.X - target_pos.X;
                    float dz = bomb_pos.Z - target_pos.Z;
                    float dy_clamped = 0.0f;
                    if (bomb_pos.Y < target_pos.Y) dy_clamped = target_pos.Y - bomb_pos.Y;
                    else if (bomb_pos.Y > target_pos.Y + 1.8f) dy_clamped = bomb_pos.Y - (target_pos.Y + 1.8f);
                    float dist = std::sqrt(dx * dx + dy_clamped * dy_clamped + dz * dz);

                    bool is_stuck_target = (ent->entity_id == stuck_entid);
                    if (is_stuck_target || dist <= 5.5f) {
                        float dmg = 400.0f;
                        if (!is_stuck_target && dist > 3.5f) {
                            float t = (dist - 3.5f) / (5.5f - 3.5f); // 0.0 at 3.5m -> 1.0 at 5.5m
                            dmg = 400.0f * (1.0f - 0.8f * t);        // 400.0 -> 80.0
                        }

                        std::lock_guard<std::mutex> hit_lock(s_pellet_hit_mutex);
                        *(uint32_t*)((uintptr_t)c28 + 0x14) = 1;
                        float hp = ApplyDamageToHealthComponent(c28, dmg);
                        if (hp <= 0.0f) {
                            *(uint32_t*)((uintptr_t)c1 + 0x1B8) &= ~0x10000u;
                        }
                        if (s_pending_dmg_pkt_count < 32) {
                            int32_t dmg_int = (int32_t)(dmg + 0.5f);
                            uint8_t pkt_flags = (uint8_t)(hp <= 0.0f ? 2 : 0);
                            s_pending_dmg_pkts[s_pending_dmg_pkt_count++] = PendingDamagePacket{ ent->entity_id, dmg_int, pkt_flags, false };
                        }
                        s_pending_dummy_hit_dump_ticks = 4;
                        ShotDiagLogFmt("[PULSE_BOMB_DETONATE] proj=0x%X target=0x%X stuck=%d dist=%.2fm dmg=%.1f -> HP=%.1f bomb=(%.2f,%.2f,%.2f)",
                            s_active_pulse_bomb_entid.load(), ent->entity_id, is_stuck_target ? 1 : 0, dist, dmg, hp,
                            bomb_pos.X, bomb_pos.Y, bomb_pos.Z);
                    }
                }
            }

            // Drain any pending damage confirmation packets queued by PerformPelletWorldAndHitboxRaycast_hook (+0x1038860)
            PendingDamagePacket local_pkts[32];
            int pkt_cnt = 0;
            uint32_t snap_entid = 0;
            {
                std::lock_guard<std::mutex> hit_lock(s_pellet_hit_mutex);
                pkt_cnt = s_pending_dmg_pkt_count;
                for (int i = 0; i < pkt_cnt; i++) local_pkts[i] = s_pending_dmg_pkts[i];
                s_pending_dmg_pkt_count = 0;
                snap_entid = s_pending_snapshot_entid;
                s_pending_snapshot_entid = 0;
            }
            if (pkt_cnt > 0) {
                auto on_net_dmg = (void(__fastcall*)(uintptr_t, uintptr_t, const uint8_t*))(globals::gameBase + 0xCA7860);
                Entity* local_mdl_ent = local_mdl_id ? ea->getEntById(local_mdl_id) : nullptr;
                uintptr_t local_c1f = local_mdl_ent ? (uintptr_t)local_mdl_ent->getById(0x1F) : 0;
                for (int i = 0; i < pkt_cnt; i++) {
                    uint8_t pkt[0x80] = {};
                    *(uint32_t*)(pkt + 0x68) = local_pkts[i].target_entid;
                    *(int32_t*)(pkt + 0x6C) = local_pkts[i].damage_int;
                    *(int32_t*)(pkt + 0x70) = 62500; // ~1000s duration_ticks so cur_tick + 62500 >= prev_ts at +0xCA79AF
                    *(uint8_t*)(pkt + 0x74) = local_pkts[i].flags;
                    on_net_dmg(sys46, 0, pkt);
                    // Phase 7 Stage 3B: Accumulate LocalPlayer Ultimate Charge via native Component_23 VarBag pipeline
                    if (local_pkts[i].allow_ult_charge) {
                        AccumulateAttackerUltimateCharge(ea, local_mdl_id, local_pkts[i].damage_int);
                    }
                    // Read HitPip counters directly from local hero model's Component_1F (+0x170/+0x174/+0x178);
                    // NEVER call +0xCF7B10 directly as it has an 'MZ' (0x5A4D) return-address check at +0xCF8291!
                    uint32_t pip_hit = local_c1f ? SafeReadVal<uint32_t>(local_c1f + 0x170) : 0;
                    uint32_t pip_crit = local_c1f ? SafeReadVal<uint32_t>(local_c1f + 0x174) : 0;
                    uint32_t pip_kill = local_c1f ? SafeReadVal<uint32_t>(local_c1f + 0x178) : 0;
                    ShotDiagLogFmt("[DMG_PKT (+0xCA7860)] target=0x%X dmg=%d flags=0x%X -> HitPip(hit=%u,crit=%u,kill=%u)",
                        local_pkts[i].target_entid, local_pkts[i].damage_int, local_pkts[i].flags,
                        pip_hit, pip_crit, pip_kill);
                    if ((local_pkts[i].flags & 2) != 0) {
                        if (Entity* victim_ent = ea->getEntById(local_pkts[i].target_entid)) {
                            TriggerHeroDeathScream(ea, victim_ent, local_mdl_id);
                        }
                        // Dispatch native Elimination Banner ("ELIMINATED [HERO] 100") & 2D Elimination Audio Sting via +0xCA7EE0
                        auto on_elim_banner = (int(__fastcall*)(uintptr_t, __int64, const uint8_t*))(globals::gameBase + 0xCA7EE0);
                        uint8_t elim_pkt[0x80] = {};
                        *(uint64_t*)(elim_pkt + 0x68) = 0ULL; // label_res_id (unused in Branch B)
                        *(uint32_t*)(elim_pkt + 0x70) = *(const uint32_t*)(globals::gameBase + 0x185A0C8); // INVALID_ENTITY_ID -> Branch B (Solo Elimination)
                        *(uint32_t*)(elim_pkt + 0x74) = local_pkts[i].target_entid; // victim_entid
                        *(uint16_t*)(elim_pkt + 0x78) = 100; // fire_points ("100")
                        int elim_rc = on_elim_banner(sys46, 0, elim_pkt);
                        ShotDiagLogFmt("[ELIM_BANNER (+0xCA7EE0)] victim=0x%X points=100 rc=%d", local_pkts[i].target_entid, elim_rc);
                    }
                }
            }
            if (snap_entid != 0 && s_alive_hit_snapshot.empty()) {
                if (Entity* snap_ent = ea->getEntById(snap_entid)) {
                    uint32 ctrl_id = 0;
                    if (auto* c1f = snap_ent->getById<Component_1F_Backref>(0x1F)) ctrl_id = c1f->backref_entid;
                    Entity* d_ctrl = ctrl_id ? ea->getEntById(ctrl_id) : nullptr;
                    s_alive_hit_snapshot.reserve(8192);
                    AppendHeroEntityForensics(s_alive_hit_snapshot, "ENEMY DUMMY SNAPSHOT AT HIT #1 (WHILE ALIVE & DAMAGED)", d_ctrl, snap_ent);
                }
            }

            // Clean up deleted entities from local_c2f + 0x15E0 ring buffer
            // IMPORTANT (+0xCA7414..+0xCA741D & +0xCA748C..+0xCA748F):
            // Never zero entry->timestamp (+0x04) on an active ring buffer, because +0xCA741D ('jbe +0xCA7555')
            // aborts the ENTIRE ring-buffer scan on the first expired timestamp!
            // Instead, zero entry->target_entid (+0x00) while preserving entry->timestamp (+0x04), which causes
            // +0xCA748F ('je +0xCA753B') to skip that slot and continue scanning the rest of the ring buffer!
            uintptr_t ring = (uintptr_t)local_c2f + 0x15E0;
            int32_t cap = local_c2f->field_17E0 > 0 ? local_c2f->field_17E0 : 0x20;
            if (cap > 0x20) cap = 0x20;
            for (int32_t idx = 0; idx < cap; idx++) {
                uintptr_t entry = ring + (uintptr_t)idx * 0x10;
                uint32_t eid = *(uint32_t*)(entry + 0x00);
                if (eid != 0 && !ea->getEntById(eid)) {
                    *(uint32_t*)(entry + 0x00) = 0;
                    *(uint8_t*)(sys46 + 0x99) = 1;
                }
            }

            bool any_show_hb = false;
            for (Entity* ent : *ea) {
                if (!ent || ent->entity_id == ea->local_entid || ent->entity_id == local_mdl_id) continue;
                if (!ent->hasComponent(0x15) || !ent->hasComponent(0x28)) continue;
                auto* c28 = ent->getById<Component_28_STUHealthComponent>(0x28);
                if (!c28) continue;
                float ent_cur_hp = c28->CalcTotalCurrentHealth();
                float ent_max_hp = c28->CalcTotalMaxHealth();
                bool ent_show_hb = (ent_cur_hp > 0.0f && ent_max_hp > 0.0f && ent_cur_hp < ent_max_hp);
                if (ent_show_hb) {
                    any_show_hb = true;
                    bool in_ring = false;
                    for (int32_t idx = 0; idx < cap; idx++) {
                        uintptr_t entry = ring + (uintptr_t)idx * 0x10;
                        if (*(uint32_t*)(entry + 0x00) == ent->entity_id) {
                            in_ring = true;
                            break;
                        }
                    }
                    if (!in_ring) {
                        auto on_net_dmg = (void(__fastcall*)(uintptr_t, uintptr_t, const uint8_t*))(globals::gameBase + 0xCA7860);
                        uint8_t pkt[0x80] = {};
                        *(uint32_t*)(pkt + 0x68) = ent->entity_id;
                        *(int32_t*)(pkt + 0x6C) = 0;
                        *(int32_t*)(pkt + 0x70) = 62500;
                        *(uint8_t*)(pkt + 0x74) = 0;
                        on_net_dmg(sys46, 0, pkt);
                        *(uint8_t*)(sys46 + 0x99) = 1;
                    }
                } else {
                    for (int32_t idx = 0; idx < cap; idx++) {
                        uintptr_t entry = ring + (uintptr_t)idx * 0x10;
                        if (*(uint32_t*)(entry + 0x00) == ent->entity_id) {
                            *(uint32_t*)(entry + 0x00) = 0;
                            *(uint8_t*)(sys46 + 0x99) = 1;
                        }
                    }
                }
                if (!target_dummy || ent->entity_id == s_last_hit_dummy_entid || ent_show_hb) {
                    target_dummy = ent;
                    target_c28 = c28;
                    cur_hp = ent_cur_hp;
                    max_hp = ent_max_hp;
                    should_show_hb = ent_show_hb;
                }
            }
            if (!any_show_hb && local_c2f->field_17F4 > 0) {
                for (int32_t idx = 0; idx < cap; idx++) {
                    uintptr_t entry = ring + (uintptr_t)idx * 0x10;
                    *(uint32_t*)(entry + 0x00) = 0;
                    *(uint32_t*)(entry + 0x04) = 0;
                }
                local_c2f->field_17F0 = 0;
                local_c2f->field_17F4 = 0;
                *(uint8_t*)(sys46 + 0x99) = 1;
            }
        }
    }

    UpdateHealthbarVisibility_orig(sys46);

    if (ea && ea->local_entid && local_c2f) {
        uint32_t local_mdl_id = 0;
        if (Entity* local_ctrl = ea->getEntById(ea->local_entid)) {
            if (auto* c20 = local_ctrl->getById<Component_20_ModelReference>(0x20)) {
                local_mdl_id = c20->cam_attach_entid ? c20->cam_attach_entid : c20->aim_entid;
            }
        }
        for (Entity* ent : *ea) {
            if (!ent || ent->entity_id == ea->local_entid || ent->entity_id == local_mdl_id) continue;
            if (!ent->hasComponent(0x15) || !ent->hasComponent(0x28)) continue;
            auto* c28 = ent->getById<Component_28_STUHealthComponent>(0x28);
            if (!c28) continue;
            float ent_cur_hp = c28->CalcTotalCurrentHealth();
            float ent_max_hp = c28->CalcTotalMaxHealth();
            if (ent_cur_hp > 0.0f && ent_max_hp > 0.0f && ent_cur_hp < ent_max_hp) {
                *(uint8_t*)((uintptr_t)c28 + 0x485) = 1;
            }
            NotifyTrackedHealthbarViewModels(ent->entity_id, c28, ent_cur_hp);
        }
    }

    if (target_dummy && target_c28 && local_c2f) {
        if (cur_hp != s_last_logged_dummy_hp) {
            SyncEntityHealthVarBag(target_dummy);
        }
        uint8_t post_484 = SafeReadVal<uint8_t>((uintptr_t)target_c28 + 0x484);
        uint8_t post_485 = SafeReadVal<uint8_t>((uintptr_t)target_c28 + 0x485);
        uint8_t sys46_99 = SafeReadVal<uint8_t>(sys46 + 0x99);
        if (post_485 != s_last_logged_dummy_485 || cur_hp != s_last_logged_dummy_hp) {
            s_last_logged_dummy_485 = post_485;
            s_last_logged_dummy_hp = cur_hp;
            uintptr_t ring = (uintptr_t)local_c2f + 0x15E0;
            ShotDiagLogFmt("[HB_VIS (+0xCA7200)] dummy=0x%X hp=%.1f/%.1f c28(484=%u,485=%u) sys46_99=%u c2f(cap=%d,head=%d,cnt=%d,e0_ent=0x%X,e0_ts=0x%X)",
                target_dummy->entity_id, cur_hp, max_hp, post_484, post_485, sys46_99,
                local_c2f->field_17E0, local_c2f->field_17F0, local_c2f->field_17F4,
                SafeReadVal<uint32_t>(ring + 0x00), SafeReadVal<uint32_t>(ring + 0x04));
        }
    }
}

// +0xD8BC90: UI Data-Binding Evaluator that reads Component_28 + 0x485 (+0xD8BEA4)
typedef void(__fastcall* eval_c28_485_fn)(uintptr_t a1, uintptr_t a2, uintptr_t expr, uintptr_t out_val);
static eval_c28_485_fn EvalComp28Byte485_orig = nullptr;
static int s_eval_c28_485_logs = 0;
void __fastcall EvalComp28Byte485_hook(uintptr_t a1, uintptr_t a2, uintptr_t expr, uintptr_t out_val) {
    EvalComp28Byte485_orig(a1, a2, expr, out_val);
    if (s_eval_c28_485_logs < 24 && out_val) {
        s_eval_c28_485_logs++;
        uint8_t val_b = SafeReadVal<uint8_t>(out_val + 0x08);
        ShotDiagLogFmt("[UI_EVAL_485 (+0xD8BC90)] #%d a2=0x%llX expr=0x%llX out_b=%u",
            s_eval_c28_485_logs, (unsigned long long)a2, (unsigned long long)expr, val_b);
    }
}

// +0xD1D7F0: Healthbar ViewModel UpdateHealthbarState (subobj at vm + 0x3E0, tracked entity_id at subobj + 0x10)
typedef void(__fastcall* update_hb_state_fn)(uintptr_t subobj);
static update_hb_state_fn UpdateHealthbarState_orig = nullptr;
static int s_update_hb_state_logs = 0;
void __fastcall UpdateHealthbarState_hook(uintptr_t subobj) {
    TrackHealthbarSubobj(subobj);
    uint32_t pre_entid = subobj ? SafeReadVal<uint32_t>(subobj + 0x10) : 0;
    UpdateHealthbarState_orig(subobj);
    uint32_t post_entid = subobj ? SafeReadVal<uint32_t>(subobj + 0x10) : 0;
    uint8_t dirty_56 = subobj ? SafeReadVal<uint8_t>(subobj + 0x56) : 0;
    float cur_total = subobj ? SafeReadVal<float>(subobj + 0x14) : 0.0f;
    float max_total = subobj ? SafeReadVal<float>(subobj + 0x18) : 0.0f;
    if (s_update_hb_state_logs < 24 || (post_entid != 0 && dirty_56 != 0 && s_update_hb_state_logs < 48)) {
        s_update_hb_state_logs++;
        ShotDiagLogFmt("[HB_VM_UPDATE (+0xD1D7F0)] #%d subobj=0x%llX ent=0x%X->0x%X hp=%.1f/%.1f dirty56=%u",
            s_update_hb_state_logs, (unsigned long long)subobj, pre_entid, post_entid, cur_total, max_total, dirty_56);
    }
}

// ============================================================================
// Hero Abilities: Tracer Recall (E, inst_id=-1, 0x58000000000001A) &
// Pulse Bomb (Q, inst_id=-3, 0x580000000000034) Offline Execution Fixes
// Verified RVAs (No 'MZ' traps):
//   +0xD67BF0: RecallPreRewindState_OnTimerTick (node[4], stu=0xCD396790)
//   +0x108A160: RecallHistoryRecorder_Update1Tick (node[5], stu=0x029BF346)
//   +0xD667A0: RecallRewindState_Update1Tick (node[1], stu=0x7211CF70)
//   +0xD44D60: WeaponFiringState_Update1Tick (stu=0xC2A01675)
//   +0xF4A9C0: ScheduleFinishState(state, lifetimeType, delayMs)
// ============================================================================

typedef void(__fastcall* schedule_finish_state_fn)(uintptr_t state, int lifetimeType, int delayMs);

static Entity* GetLocalHeroModelEnt() {
    auto* game_ea = GameEntityAdmin();
    if (!game_ea || !game_ea->local_entid) return nullptr;
    Entity* local_ctrl = game_ea->getEntById(game_ea->local_entid);
    if (!local_ctrl) return nullptr;
    if (auto* c20 = local_ctrl->getById<Component_20_ModelReference>(0x20)) {
        uint32_t mid = c20->cam_attach_entid ? c20->cam_attach_entid : c20->aim_entid;
        if (mid) return game_ea->getEntById(mid);
    }
    return nullptr;
}

static void PatchHeroAbilityGraphsAndVars(EntityAdminBase* game_ea) {
    (void)game_ea;
}

// 1. +0xD67BF0: Universal Server-Timed Wait State OnTimerTick (stu = 0xCD396790, vt = +0x15D4840, 28 nodes in 15 hero graphs)
// Offline, [stu + 0x108] (0xB4FE98D8 condition CV) is NULL in almost all ability graphs (Recall 0x1A, Grapple 0x20,
// Shadow Step 0x48, Charge 0x7D, Earthshatter 0xE2, Chain Hook 0xFE, Take a Breather 0x101, Combat Roll 0x17B, Deadeye 0x17E,
// RIP-Tire 0x232, Cryo-Freeze 0x623, etc.), so EvalConfigVarBool(+0xD16BF0) returns 0 and +0xD67C42 skips
// ScheduleFinishState(state, 1, 0) (+0xF4A9C0), wedging the state and its StateStack weapon/ability locks forever.
typedef void(__fastcall* recall_pre_timer_fn)(uintptr_t state, int timerUserData);
static recall_pre_timer_fn RecallPreRewindState_OnTimerTick_orig = nullptr;
void __fastcall RecallPreRewindState_OnTimerTick_hook(uintptr_t state, int timerUserData) {
    RecallPreRewindState_OnTimerTick_orig(state, timerUserData);
    if (timerUserData == 1 && state) {
        uintptr_t ss_inst = SafeReadVal<uintptr_t>(state + 0x78);
        uintptr_t ss_inner = ss_inst ? SafeReadVal<uintptr_t>(ss_inst + 0x10) : 0;
        auto* comp23 = ss_inner ? (Component_23_Statescript*)SafeReadVal<uintptr_t>(ss_inner + 0x450) : nullptr;
        if (!comp23 || IsBadReadPtr(comp23, sizeof(Component_23_Statescript))) return;

        uint64_t script_id = ss_inst ? SafeReadVal<uint64_t>(ss_inst + 0x150) : 0;
        uintptr_t graph = ss_inst ? SafeReadVal<uintptr_t>(ss_inst + 0x160) : 0;
        uintptr_t stu_state = 0;
        if (graph && !IsBadReadPtr((const void*)graph, sizeof(STUStatescriptGraph))) {
            stu_state = (uintptr_t)((STUStatescriptGraph*)graph)->STUFromState((StatescriptState*)state);
        }
        uintptr_t cv_108 = (stu_state && !IsBadReadPtr((const void*)stu_state, 0x110))
            ? SafeReadVal<uintptr_t>(stu_state + 0x108) : 0;
        if (cv_108 == 0) {
            auto sched_finish = (schedule_finish_state_fn)(globals::gameBase + 0xF4A9C0);
            // Recall (0x058000000000001A) waits the remaining 280ms of its 380ms node[14] fade timer; all other 27 nodes finish immediately (0ms)
            int delay_ms = (script_id == 0x058000000000001AULL) ? 280 : 0;
            sched_finish(state, 1, delay_ms);
            ShotDiagLogFmt("[WAIT_STATE_0xCD396790 (+0xD67BF0)] script=0x%llX state=0x%llX stu+0x108=NULL -> FinishState(delay=%dms)",
                (unsigned long long)script_id, (unsigned long long)state, delay_ms);
        }
    }
}

// 2. +0x108A160: RecallHistoryRecorder_Update1Tick (node[5], stu = 0x029BF346)
// Offline, comp12->history_1.count (+0x434) and movstate_arr_used (+0xA054) are 0 because no network snapshots
// arrive, causing +0x108A217 to exit without recording any 0x350-byte Recall history entries at [state + 0xA8].
// IMPORTANT: Must filter strictly to LocalPlayer's Hero Model AND restore c12+0x434 / c12+0xA054 to 0 immediately
// after orig(state) returns so +0x1023045 does not corrupt comp12's AABB (+0xA2D0) or 3D HUD (Component_1C).
typedef void(__fastcall* recall_hist_tick_fn)(uintptr_t state);
static recall_hist_tick_fn RecallHistoryRecorder_Update1Tick_orig = nullptr;
void __fastcall RecallHistoryRecorder_Update1Tick_hook(uintptr_t state) {
    uintptr_t c12 = 0;
    uint32_t old_420 = 0;
    double old_20 = 0.0;
    int32_t old_430 = 0;
    uint32_t old_434 = 0;
    int32_t old_A050 = 0;
    uint32_t old_A054 = 0;

    if (state) {
        uintptr_t ss_inst = SafeReadVal<uintptr_t>(state + 0x78);
        uintptr_t ss_inner = ss_inst ? SafeReadVal<uintptr_t>(ss_inst + 0x10) : 0;
        auto* comp23 = ss_inner ? (Component_23_Statescript*)SafeReadVal<uintptr_t>(ss_inner + 0x450) : nullptr;
        Entity* hero_ent = (comp23 && !IsBadReadPtr(comp23, sizeof(Component_23_Statescript))) ? comp23->base.entity_backref : nullptr;
        if (hero_ent && hero_ent == GetLocalHeroModelEnt()) {
            auto* comp12 = hero_ent->getById<Component_12_STUMovementStateComponent>(0x12);
            auto* comp15 = hero_ent->getById<Component_15_STUCharacterMoverComponent>(0x15);
            if (comp12 && !IsBadReadPtr(comp12, sizeof(Component_12_STUMovementStateComponent)) &&
                comp15 && !IsBadReadPtr(comp15, sizeof(Component_15_STUCharacterMoverComponent))) {
                c12 = (uintptr_t)comp12;
                old_420 = *(uint32_t*)(c12 + 0x420);
                old_20 = *(double*)(c12 + 0x20);
                old_430 = *(int32_t*)(c12 + 0x430);
                old_434 = *(uint32_t*)(c12 + 0x434);
                old_A050 = *(int32_t*)(c12 + 0xA050);
                old_A054 = *(uint32_t*)(c12 + 0xA054);

                uint64_t cf_ms = SafeReadVal<uint64_t>(ss_inner + 0x2D0);
                double cur_time = (cf_ms != 0) ? ((double)cf_ms * 0.001) : ((double)GetTickCount64() * 0.001);

                if (old_420 == 0) {
                    *(uint32_t*)(c12 + 0x420) = 60;
                }
                *(double*)(c12 + 0x20) = cur_time;
                *(int32_t*)(c12 + 0x430) = 0;
                *(uint32_t*)(c12 + 0x434) = 1;
                *(int32_t*)(c12 + 0xA050) = 0;
                *(uint32_t*)(c12 + 0xA054) = 1;

                MovementState* dst_ms = (MovementState*)(c12 + 0x440);
                memcpy(dst_ms, &comp15->movement_state_2, sizeof(MovementState));
                // Ensure bit 0x2000 on movement_flags (+0x198) is clear so +0x108A28F records snapshot
                dst_ms->movement_flags &= ~0x2000u;
            }
        }
    }

    RecallHistoryRecorder_Update1Tick_orig(state);

    if (c12) {
        *(uint32_t*)(c12 + 0x420) = old_420;
        *(double*)(c12 + 0x20) = old_20;
        *(int32_t*)(c12 + 0x430) = old_430;
        *(uint32_t*)(c12 + 0x434) = old_434;
        *(int32_t*)(c12 + 0xA050) = old_A050;
        *(uint32_t*)(c12 + 0xA054) = old_A054;
    }
}

// 3. +0xD667A0: RecallRewindState_Update1Tick (node[1], stu = 0x7211CF70)
// Propagates the interpolated rewound MovementState from comp12->mov_state (+0xA060) back to
// Component_15 (CharacterMover) and Component_01 (SceneRendering), restores rewound max health,
// and finishes node[1] once elapsed_sec (+0xA8) reaches the 0.90s rewind duration.
typedef void(__fastcall* recall_rewind_tick_fn)(uintptr_t state, int64_t delta_ms);
static recall_rewind_tick_fn RecallRewindState_Update1Tick_orig = nullptr;
void __fastcall RecallRewindState_Update1Tick_hook(uintptr_t state, int64_t delta_ms) {
    RecallRewindState_Update1Tick_orig(state, delta_ms);
    if (!state) return;

    uintptr_t ss_inst = SafeReadVal<uintptr_t>(state + 0x78);
    uintptr_t ss_inner = ss_inst ? SafeReadVal<uintptr_t>(ss_inst + 0x10) : 0;
    auto* comp23 = ss_inner ? (Component_23_Statescript*)SafeReadVal<uintptr_t>(ss_inner + 0x450) : nullptr;
    Entity* hero_ent = (comp23 && !IsBadReadPtr(comp23, sizeof(Component_23_Statescript))) ? comp23->base.entity_backref : nullptr;
    if (!hero_ent || hero_ent != GetLocalHeroModelEnt()) return;

    float elapsed_sec = SafeReadVal<float>(state + 0xA8);
    int32_t oldest_idx = SafeReadVal<int32_t>(state + 0xC8);

    auto* comp12 = hero_ent->getById<Component_12_STUMovementStateComponent>(0x12);
    auto* comp15 = hero_ent->getById<Component_15_STUCharacterMoverComponent>(0x15);
    auto* comp01 = hero_ent->getById<Component_1_SceneRendering>(1);
    if (comp12 && !IsBadReadPtr(comp12, sizeof(Component_12_STUMovementStateComponent))) {
        const Vector4& rew_pos = comp12->mov_state.absolute_position;
        const Vector4& rew_rot = comp12->mov_state.rotation_quaternion;
        float abs_p = fabsf(rew_pos.X) + fabsf(rew_pos.Y) + fabsf(rew_pos.Z);
        if (abs_p > 0.05f) {
            if (comp15 && !IsBadReadPtr(comp15, sizeof(Component_15_STUCharacterMoverComponent))) {
                comp15->movement_state_1.absolute_position = rew_pos;
                comp15->movement_state_1.rotation_quaternion = rew_rot;
                comp15->movement_state_2.absolute_position = rew_pos;
                comp15->movement_state_2.rotation_quaternion = rew_rot;
            }
            if (comp01 && !IsBadReadPtr(comp01, sizeof(Component_1_SceneRendering))) {
                comp01->SetPosRotation(rew_pos, rew_rot);
            }
        }
    }

    // Finish node[1] after 0.90s rewind duration (or immediately if no history snapshot was available)
    if (elapsed_sec >= 0.90f || oldest_idx < 0) {
        if (auto* c28 = hero_ent->getById<Component_28_STUHealthComponent>(0x28)) {
            float cur_hp = c28->CalcTotalCurrentHealth();
            float max_hp = c28->CalcTotalMaxHealth();
            float snap_hp = SafeReadVal<float>(state + 0x198);
            float target_hp = (snap_hp > cur_hp && snap_hp <= max_hp) ? snap_hp : max_hp;
            if (target_hp > cur_hp && c28->normal_health[0].max_health > 0.0f) {
                c28->normal_health[0].curr_health = min(c28->normal_health[0].max_health, target_hp);
                SyncEntityHealthVarBag(hero_ent);
            }
        }
        auto sched_finish = (schedule_finish_state_fn)(globals::gameBase + 0xF4A9C0);
        sched_finish(state, 1, 0);
        ShotDiagLogFmt("[RECALL_REWIND (+0xD667A0)] state=0x%llX elapsed=%.3fs oldest_idx=%d -> scheduled FinishState(0)",
            (unsigned long long)state, elapsed_sec, oldest_idx);
    }
}

// 4. +0xD44D60: WeaponFiringState_Update1Tick (stu = 0xC2A01675 / 0x860EE5FC, vt = +0x15D2450)
// Ensures client firing authority gate at +0xD44E23..+0xD44E40 ([cfg+0x104]==3 || ([cfg+0x108]!=0 && (exec_flags & 0x400000)!=0))
// passes for all local hero weapon and ability firing states across all 21 heroes without permanently mutating execution_flags.
typedef void(__fastcall* weapon_firing_tick_fn)(uintptr_t state);
static weapon_firing_tick_fn WeaponFiringState_Update1Tick_orig = nullptr;
void __fastcall WeaponFiringState_Update1Tick_hook(uintptr_t state) {
    uintptr_t ss_inst = state ? SafeReadVal<uintptr_t>(state + 0x78) : 0;
    int16_t scope_id = ss_inst ? SafeReadVal<int16_t>(ss_inst + 0x18) : 0;
    uintptr_t ss_inner = ss_inst ? SafeReadVal<uintptr_t>(ss_inst + 0x10) : 0;
    auto* comp23 = ss_inner ? (Component_23_Statescript*)SafeReadVal<uintptr_t>(ss_inner + 0x450) : nullptr;
    Entity* hero_ent = (comp23 && !IsBadReadPtr(comp23, sizeof(Component_23_Statescript))) ? comp23->base.entity_backref : nullptr;

    bool is_local_hero = (hero_ent && hero_ent == GetLocalHeroModelEnt());
    uint32_t old_exec = 0;

    if (is_local_hero && !IsBadReadPtr((const void*)ss_inst, 0x260)) {
        old_exec = *(uint32_t*)(ss_inst + 0x258);
        *(uint32_t*)(ss_inst + 0x258) = (old_exec | 0x400008u) & ~0x1000u;

        auto resolve_cfg = (void(__fastcall*)(uintptr_t))(globals::gameBase + 0x10798F0);
        resolve_cfg(state);
        uintptr_t cfg = SafeReadVal<uintptr_t>(state + 0xD0);
        if (cfg && !IsBadReadPtr((const void*)cfg, 0x190)) {
            uint8_t b108 = SafeReadVal<uint8_t>(cfg + 0x108);
            uint32_t mode104 = SafeReadVal<uint32_t>(cfg + 0x104);
            uintptr_t sub_c0 = SafeReadVal<uintptr_t>(cfg + 0xC0);
            uint64_t state_a8 = SafeReadVal<uint64_t>(state + 0xA8);
            uint64_t proj_resid = (sub_c0 && !IsBadReadPtr((const void*)sub_c0, 0x38)) ? SafeReadVal<uint64_t>(sub_c0 + 0x28) : 0;
            if (b108 == 0) {
                *(uint8_t*)(cfg + 0x108) = 1;
                ShotDiagLogFmt("[WPN_FIRE_TICK (+0xD44D60)] inst=%d state=0x%llX A8=0x%llX cfg=0x%llX sub_c0=0x%llX proj_resid=0x%llX mode104=%u b108=%u->1",
                    (int)scope_id, (unsigned long long)state,
                    (unsigned long long)state_a8,
                    (unsigned long long)cfg, (unsigned long long)sub_c0, (unsigned long long)proj_resid,
                    mode104, b108);
            }
            if (mode104 == 0) {
                *(uint32_t*)(cfg + 0x104) = 1;
            }
        }
    }

    WeaponFiringState_Update1Tick_orig(state);

    if (is_local_hero && !IsBadReadPtr((const void*)ss_inst, 0x260)) {
        *(uint32_t*)(ss_inst + 0x258) = old_exec;
    }
}

// ============================================================================
// Horizon 1 Stage H1.2: Generalizing All 21 Launch Heroes & Stripped Server Nodes
// - ResolveShotBaseDamage (per-weapon STU 0x04CC5359 & 21-hero 0.8.0 damage table)
// - ApplyHealingToHealthComponent (Normal -> Armor -> Shield pool restoration)
// - +0xF43450 FindStatescriptStateImpl_hook (4 Group-1 client stub vtable overrides)
// - +0xF47740 StatescriptAction_ExecuteAndContinue_hook (0x4E83BC01 & 0x04CC5359)
// - +0xF495C0 StatescriptState_StartState_hook (0x619D9E95 Melee/Sweep & 0xD78490D1 Impulse)
// ============================================================================
static uint32_t s_last_melee_hit_entid = 0;
static uint64_t s_last_melee_hit_ms    = 0;
static uint64_t s_last_melee_dmg_ms    = 0;

static Vector4  s_active_impulse_vel{};
static uint64_t s_active_impulse_end_ms    = 0;
static float    s_active_impulse_min_y     = -10000.0f;

static uint64_t s_active_dash_sweep_end_ms    = 0;
static float    s_active_dash_sweep_dmg       = 0.0f;
static float    s_active_dash_sweep_radius    = 3.5f;
static uint64_t s_active_dash_sweep_script_id = 0;
static uint32_t s_active_dash_sweep_hit_ents[16] = {};
static int      s_active_dash_sweep_hit_count = 0;

static void ArmDashSweepDamage(uint64_t script_id, float dmg, float radius, uint64_t dur_ms) {
    uint64_t now_ms = GetTickCount64();
    if (s_active_dash_sweep_end_ms <= now_ms || s_active_dash_sweep_script_id != script_id) {
        s_active_dash_sweep_hit_count = 0;
    }
    s_active_dash_sweep_script_id = script_id;
    s_active_dash_sweep_dmg = dmg;
    s_active_dash_sweep_radius = radius;
    s_active_dash_sweep_end_ms = now_ms + dur_ms;
}

// Stage H1.2 / Suite E: Enemy Knockback Impulse Tracker (D.Va Boosters, Reinhardt Charge, Winston Jump Pack / Primal Rage, Lucio Soundwave)
struct EnemyKnockbackEntry {
    uint32_t ent_id;
    Vector4  vel;
    float    ground_y;
    uint64_t end_ms;
};
static EnemyKnockbackEntry s_enemy_knockbacks[16] = {};
static int s_enemy_knockback_count = 0;

static void ArmEnemyKnockback(Entity* ent, const Vector4& push_dir, float horiz_speed, float up_speed, uint64_t dur_ms) {
    if (!ent || IsBadReadPtr(ent, sizeof(Entity)) || dur_ms == 0) return;
    auto* c1 = ent->getById<Component_1_SceneRendering>(1);
    if (!c1 || IsBadReadPtr(c1, sizeof(Component_1_SceneRendering))) return;

    float dx = push_dir.X;
    float dz = push_dir.Z;
    float len = sqrtf(dx * dx + dz * dz);
    if (len > 1e-4f) { dx /= len; dz /= len; }
    else { dx = 0.0f; dz = 1.0f; }

    uint64_t now_ms = GetTickCount64();
    Vector4 vel{ dx * horiz_speed, up_speed, dz * horiz_speed, 0.0f };
    float ground_y = c1->position.Y;
    if (auto* slot = OriginClutterTracker::GetSlot(ent->entity_id, ent)) {
        if (slot->has_nonzero_pos && slot->latest_nonzero_pos.Y < ground_y) {
            ground_y = slot->latest_nonzero_pos.Y;
        }
    }

    for (int i = 0; i < s_enemy_knockback_count; ++i) {
        if (s_enemy_knockbacks[i].ent_id == ent->entity_id) {
            s_enemy_knockbacks[i].vel = vel;
            s_enemy_knockbacks[i].end_ms = now_ms + dur_ms;
            return;
        }
    }
    if (s_enemy_knockback_count < 16) {
        s_enemy_knockbacks[s_enemy_knockback_count++] = EnemyKnockbackEntry{ ent->entity_id, vel, ground_y, now_ms + dur_ms };
    }
}

// Stage H1.2 / Suite D: Smooth 100ms Heal-Over-Time (HoT) & Biotic Field / Bastion Self-Repair Trackers
static uint64_t s_active_hot_end_ms       = 0;
static uint64_t s_active_hot_last_tick_ms = 0;
static float    s_active_hot_hp_per_100ms = 0.0f;
static uint64_t s_active_hot_script_id    = 0;
static bool     s_bastion_repair_active   = false;
static uint64_t s_bastion_last_tick_ms    = 0;
static uint64_t s_biotic_field_end_ms     = 0;
static uint64_t s_biotic_field_last_ms    = 0;
static uint32_t s_biotic_field_ent_id     = 0;
static Vector4  s_biotic_field_pos{};

static void ArmHeroHealOverTime(uint64_t script_id, float total_hp, uint64_t dur_ms, const char* tag = nullptr) {
    if (dur_ms == 0 || total_hp <= 0.0f) return;
    uint64_t now_ms = GetTickCount64();
    s_active_hot_script_id = script_id;
    s_active_hot_hp_per_100ms = total_hp / ((float)dur_ms / 100.0f);
    s_active_hot_end_ms = now_ms + dur_ms;
    s_active_hot_last_tick_ms = (now_ms >= 100) ? (now_ms - 100) : 0;
    if (tag) {
        ShotDiagLogFmt("[%s] script=0x%llX total_hp=+%.1f dur=%llums (+%.2f HP / 100ms)",
            tag, (unsigned long long)script_id, total_hp, (unsigned long long)dur_ms, s_active_hot_hp_per_100ms);
    }
}

// Stage H1.3: Created Sub-Entity Tracking (Turrets, Sentries, Teleporter, Ice Wall, Blizzard, Biotic Field, Barriers, Dragonstrike, RIP-Tire)
struct CreatedSubEntityEntry {
    uint32_t ent_id;
    uint32_t owner_ent_id;
    uint64_t res_id;
    uint64_t script_id;
    uint64_t spawn_ms;
    uint64_t expire_ms;
    uint64_t last_effect_ms;
    Vector4  vel;
    Vector4  spawn_pos;
    float    ground_y;
    bool     has_gravity;
    bool     lock_stationary;
};
static CreatedSubEntityEntry s_created_sub_entities[64] = {};
static int s_created_sub_entity_count = 0;

// Stage H1.3: Junkrat RIP-Tire (0x0580000000000232) Possession State
static uint32_t s_riptire_ent_id       = 0;
static uint32_t s_riptire_saved_mdl_id = 0;
static uint64_t s_riptire_end_ms       = 0;

template <typename T>
static T GetSTUFieldByHash(STUStatescriptBase* node, uint32_t field_hash, T default_val = T{}) {
    if (!node || IsBadReadPtr(node, sizeof(STUStatescriptBase))) return default_val;
    STUInfo* info = (node->graph_node.base.vfptr && !IsBadReadPtr(node->graph_node.base.vfptr, sizeof(STUBase_vt)))
        ? node->graph_node.base.vfptr->GetSTUInfo() : nullptr;
    if (!info || IsBadReadPtr(info, sizeof(STUInfo))) return default_val;
    STUArgumentInfo* arg = info->argumentByHash(field_hash, true);
    if (!arg || IsBadReadPtr(arg, sizeof(STUArgumentInfo))) return default_val;
    return SafeReadVal<T>((uintptr_t)node + arg->offset, default_val);
}

static void SafeTraverseOutputPlug(
    StatescriptSyncMgr* mgr,
    StatescriptInstance* inst,
    STUStatescriptBase* source_node,
    STUStatescriptOutputPlug* out_plug,
    uint64_t plug_offset)
{
    if (!mgr || !inst || !source_node || !out_plug) return;
    if (IsBadReadPtr(mgr, sizeof(StatescriptSyncMgr)) || IsBadReadPtr(out_plug, sizeof(STUStatescriptOutputPlug))) return;
    using TraversePlugFn = uint8_t(__fastcall*)(StatescriptSyncMgr*, StatescriptInstance*, STUStatescriptBase*, STUStatescriptOutputPlug*, uint64_t);
    auto traverse_plug = (TraversePlugFn)(globals::gameBase + 0xF46F80);
    __try {
        traverse_plug(mgr, inst, source_node, out_plug, plug_offset);
    } __except (EXCEPTION_EXECUTE_HANDLER) {}
}

static void WriteDynamicVarPrim(StatescriptInstance* inst, STUConfigVarDynamic* out_var, const StatescriptPrimitive& val_prim) {
    if (!inst || !out_var || IsBadReadPtr(out_var, sizeof(STUConfigVarDynamic))) return;
    int loc_cnt = out_var->item_location.count();
    STUResourceReference* loc_arr = out_var->item_location.list();
    if (loc_cnt <= 0 || !loc_arr || IsBadReadPtr(loc_arr, sizeof(STUResourceReference) * loc_cnt)) return;
    if (loc_cnt == 1) {
        __int64 var_id = loc_arr[0].resource_id;
        StatescriptVar_VarBag* inst_bags[3] = { inst->rid_instance_varbag, inst->second_inst_varbag, inst->third_inst_varbag };
        for (StatescriptVar_VarBag* vb : inst_bags) {
            if (vb && !IsBadReadPtr(vb, sizeof(StatescriptVar_VarBag))) {
                vb->SetVar({ var_id }, val_prim);
                NotifyVarInBag(inst->ss_inner, inst, vb, var_id, val_prim);
            }
        }
        if (inst->ss_inner && inst->ss_inner->rid_entity_varbag && !IsBadReadPtr(inst->ss_inner->rid_entity_varbag, sizeof(StatescriptVar_VarBag))) {
            inst->ss_inner->rid_entity_varbag->SetVar({ var_id }, val_prim);
            inst->ss_inner->rid_entity_varbag->SetVar({ StatescriptVar_ENTITY_VARBAG, var_id }, val_prim);
            NotifyVarInBag(inst->ss_inner, inst, inst->ss_inner->rid_entity_varbag, var_id, val_prim);
            inst->ss_inner->needs_process_e = 1;
        }
    } else if (loc_cnt >= 2 && inst->ss_inner && inst->ss_inner->rid_entity_varbag &&
               !IsBadReadPtr(inst->ss_inner->rid_entity_varbag, sizeof(StatescriptVar_VarBag))) {
        __int64 bag_id = loc_arr[0].resource_id;
        __int64 var_id = loc_arr[1].resource_id;
        if ((uint64_t)bag_id == StatescriptVar_ENTITY_VARBAG) {
            inst->ss_inner->rid_entity_varbag->SetVar({ var_id }, val_prim);
            inst->ss_inner->rid_entity_varbag->SetVar({ StatescriptVar_ENTITY_VARBAG, var_id }, val_prim);
            NotifyVarInBag(inst->ss_inner, inst, inst->ss_inner->rid_entity_varbag, var_id, val_prim);
            inst->ss_inner->needs_process_e = 1;
        }
    }
}

static int32_t ReadVarBagInt32(StatescriptVar_VarBag* vb, __int64 target_rid) {
    if (!vb || IsBadReadPtr(vb, sizeof(StatescriptVar_VarBag))) return 0;
    for (int b = 0; b < 16; ++b) {
        const auto& bucket = vb->sub_items[b];
        if (!bucket.arr_ptr || bucket.item_count <= 0 || bucket.item_count > 256) continue;
        if (IsBadReadPtr(bucket.arr_ptr, sizeof(StatescriptVar_VarBag::VarBag_Item) * bucket.item_count)) continue;
        for (int i = 0; i < bucket.item_count; ++i) {
            if (bucket.arr_ptr[i].rid_id == target_rid && bucket.arr_ptr[i].ptr &&
                !IsBadReadPtr(bucket.arr_ptr[i].ptr, sizeof(StatescriptVar_Primitive))) {
                auto* pvar = (StatescriptVar_Primitive*)bucket.arr_ptr[i].ptr;
                return (int32_t)pvar->global_var.value;
            }
        }
    }
    return 0;
}

static void SafeSetConfigVarFromSTU(StatescriptVar_VarBag* vb, STUConfigVarDynamic* out_var, STUConfigVar* m_value) {
    if (!vb || !out_var || !m_value || IsBadReadPtr(vb, sizeof(StatescriptVar_VarBag))) return;
    if (!vb->base.stu_base.vfptr) return;
    __try {
        vb->base.stu_base.vfptr->SetConfigVarValueFromSTUConfigVar(vb, out_var, m_value, nullptr);
    } __except (EXCEPTION_EXECUTE_HANDLER) {}
}

static void RefillInstanceAmmoVarBags(StatescriptSyncMgr* mgr, StatescriptInstance* inst, int32_t fallback_max_ammo = 6) {
    if (!inst || IsBadReadPtr(inst, sizeof(StatescriptInstance))) return;
    // Verified weapon ammo VarBag variables from live dump (e.g. McCree Peacekeeper 0x05800000000001D2 inst=-23):
    //   0x0D80000000000035ULL (INT): Current Ammo (e.g. 3/6 after firing 3 shots)
    //   0x0D800000000000C6ULL (INT): Max Ammo (6 for McCree, 200 for Bastion/Mei)
    //   0x0D800000000000E6ULL (INT): UI Max Ammo (6 for McCree)
    //   0x0D80000000000033ULL (INT): Empty/Reload flag (0 when loaded)
    //   0x0D80000000000105ULL (INT): Secondary ammo var
    constexpr __int64 kVarCurAmmo35  = (__int64)0x0D80000000000035ULL;
    constexpr __int64 kVarMaxAmmoC6  = (__int64)0x0D800000000000C6ULL;
    constexpr __int64 kVarMaxAmmoE6  = (__int64)0x0D800000000000E6ULL;
    constexpr __int64 kVarReload33   = (__int64)0x0D80000000000033ULL;
    constexpr __int64 kVarCurAmmo105 = (__int64)0x0D80000000000105ULL;

    StatescriptVar_VarBag* bags[3] = { inst->rid_instance_varbag, inst->second_inst_varbag, inst->third_inst_varbag };
    int32_t max_ammo = 0;
    for (StatescriptVar_VarBag* vb : bags) {
        if (!vb || IsBadReadPtr(vb, sizeof(StatescriptVar_VarBag))) continue;
        int32_t val = ReadVarBagInt32(vb, kVarMaxAmmoC6);
        if (val > 0 && val <= 400) { max_ammo = val; break; }
        val = ReadVarBagInt32(vb, kVarMaxAmmoE6);
        if (val > 0 && val <= 400) { max_ammo = val; break; }
    }
    if (max_ammo <= 0) max_ammo = fallback_max_ammo;

    StatescriptPrimitive prim_max(StatescriptPrimitive_INT, (__int64)max_ammo);
    StatescriptPrimitive prim_zero(StatescriptPrimitive_INT, 0);
    for (StatescriptVar_VarBag* vb : bags) {
        if (vb && !IsBadReadPtr(vb, sizeof(StatescriptVar_VarBag))) {
            vb->SetVar({ kVarCurAmmo35 }, prim_max);
            vb->SetVar({ StatescriptVar_ENTITY_VARBAG, kVarCurAmmo35 }, prim_max);
            NotifyVarInBag(mgr, inst, vb, kVarCurAmmo35, prim_max);

            vb->SetVar({ kVarReload33 }, prim_zero);
            NotifyVarInBag(mgr, inst, vb, kVarReload33, prim_zero);

            vb->SetVar({ kVarCurAmmo105 }, prim_max);
            NotifyVarInBag(mgr, inst, vb, kVarCurAmmo105, prim_max);
            if (mgr) mgr->needs_process_e = 1;
        }
    }
    // Also invoke McCree Peacekeeper's native reload assignment node (0x05800000000001D2 node[22] 0x4698E4B6: 0x35 = 0xC6) if present
    if (inst->script_id == 0x05800000000001D2ULL && inst->graph && !IsBadReadPtr(inst->graph, sizeof(STUStatescriptGraph)) &&
        inst->graph->m_nodes.count() > 22 && inst->rid_instance_varbag) {
        STUStatescriptBase* node22 = inst->graph->m_nodes.list()[22];
        if (node22 && !IsBadReadPtr(node22, 0xA8)) {
            auto* cvd35 = (STUConfigVarDynamic*)SafeReadVal<uintptr_t>((uintptr_t)node22 + 0x90);
            auto* cvC6  = (STUConfigVar*)SafeReadVal<uintptr_t>((uintptr_t)node22 + 0xA0);
            if (cvd35 && cvC6) {
                SafeSetConfigVarFromSTU(inst->rid_instance_varbag, cvd35, cvC6);
            }
        }
    }
}

static void RefillEntityAmmoVarBags(Entity* ent, int32_t default_max_ammo = 6) {
    if (!ent || IsBadReadPtr(ent, sizeof(Entity))) return;
    auto* c23 = ent->getById<Component_23_Statescript>(0x23);
    if (!c23 || IsBadReadPtr(c23, sizeof(Component_23_Statescript))) return;

    constexpr __int64 kVarCurAmmo35  = (__int64)0x0D80000000000035ULL;
    constexpr __int64 kVarMaxAmmoC6  = (__int64)0x0D800000000000C6ULL;
    constexpr __int64 kVarCurAmmo105 = (__int64)0x0D80000000000105ULL;
    constexpr __int64 kVarMaxAmmo106 = (__int64)0x0D80000000000106ULL;

    int32_t max_ammo = GetEntityVarBagInt32(c23, (uint64_t)kVarMaxAmmoC6);
    if (max_ammo <= 0 || max_ammo > 400) max_ammo = GetEntityVarBagInt32(c23, (uint64_t)kVarMaxAmmo106);
    if (max_ammo <= 0 || max_ammo > 400) max_ammo = default_max_ammo;

    StatescriptPrimitive prim(StatescriptPrimitive_INT, (__int64)max_ammo);
    if (c23->ss_inner.rid_entity_varbag && !IsBadReadPtr(c23->ss_inner.rid_entity_varbag, sizeof(StatescriptVar_VarBag))) {
        c23->ss_inner.rid_entity_varbag->SetVar({ kVarCurAmmo35 }, prim);
        c23->ss_inner.rid_entity_varbag->SetVar({ StatescriptVar_ENTITY_VARBAG, kVarCurAmmo35 }, prim);
        NotifyVarInBag(&c23->ss_inner, nullptr, c23->ss_inner.rid_entity_varbag, kVarCurAmmo35, prim);

        c23->ss_inner.rid_entity_varbag->SetVar({ kVarCurAmmo105 }, prim);
        c23->ss_inner.rid_entity_varbag->SetVar({ StatescriptVar_ENTITY_VARBAG, kVarCurAmmo105 }, prim);
        NotifyVarInBag(&c23->ss_inner, nullptr, c23->ss_inner.rid_entity_varbag, kVarCurAmmo105, prim);
        c23->ss_inner.needs_process_e = 1;
    }
    if (c23->ss_inner.g3_arrWithIds.ptr && c23->ss_inner.g3_arrWithIds.num > 0 && c23->ss_inner.g3_arrWithIds.num <= 64) {
        for (int i = 0; i < c23->ss_inner.g3_arrWithIds.num; ++i) {
            RefillInstanceAmmoVarBags(&c23->ss_inner, c23->ss_inner.g3_arrWithIds.ptr[i].instance, max_ammo);
        }
    }
    if (c23->ss_inner.g1_instanceArr.ptr && c23->ss_inner.g1_instanceArr.num > 0 && c23->ss_inner.g1_instanceArr.num <= 64) {
        for (int i = 0; i < c23->ss_inner.g1_instanceArr.num; ++i) {
            RefillInstanceAmmoVarBags(&c23->ss_inner, c23->ss_inner.g1_instanceArr.ptr[i], max_ammo);
        }
    }
}

static void SafeTickSyncMgr(StatescriptSyncMgr* mgr) {
    if (!mgr || IsBadReadPtr(mgr, sizeof(StatescriptSyncMgr))) return;
    using TickSyncMgrFn = void(__fastcall*)(StatescriptSyncMgr* mgr, int frame, int a3);
    auto tick_sync_mgr = (TickSyncMgrFn)(globals::gameBase + 0xD13C20);
    __try {
        tick_sync_mgr(mgr, ++mgr->increasing_counter1, 1);
    } __except (EXCEPTION_EXECUTE_HANDLER) {}
}

static void RefillHeroWeaponAmmo(Entity* hero_ent) {
    if (!hero_ent || IsBadReadPtr(hero_ent, sizeof(Entity))) return;
    RefillEntityAmmoVarBags(hero_ent, 6);
    EntityAdminBase* ea = GameEntityAdmin();
    auto* c23 = hero_ent->getById<Component_23_Statescript>(0x23);
    if (ea && c23 && !IsBadReadPtr(c23, sizeof(Component_23_Statescript))) {
        uintptr_t slot_arr = SafeReadVal<uintptr_t>((uintptr_t)c23 + 0x1458);
        int32_t slot_cnt   = SafeReadVal<int32_t>((uintptr_t)c23 + 0x1460);
        if (slot_arr && slot_cnt > 0 && slot_cnt <= 16) {
            for (int i = 0; i < slot_cnt; ++i) {
                uint32_t wep0 = SafeReadVal<uint32_t>(slot_arr + (uintptr_t)i * 0x18 + 0x00);
                uint32_t wep1 = SafeReadVal<uint32_t>(slot_arr + (uintptr_t)i * 0x18 + 0x04);
                if (wep0 && wep0 != 0xFFFFFFFFu) RefillEntityAmmoVarBags(ea->getEntById(wep0), 6);
                if (wep1 && wep1 != 0xFFFFFFFFu) RefillEntityAmmoVarBags(ea->getEntById(wep1), 6);
            }
        }
    }
    ShotDiagLogFmt("[AMMO_REFILL] hero=0x%X refilled weapon (0x35=0xC6) & entity ammo VarBags", hero_ent->entity_id);
}

static float ApplyHealingToHealthComponent(Component_28_STUHealthComponent* c28, float heal_amt) {
    if (!c28 || heal_amt <= 0.0f) return c28 ? c28->CalcTotalCurrentHealth() : 0.0f;
    // Restore Normal -> Armor -> Shield up to each part's max_health
    HealthPart* pools[3] = { c28->normal_health, c28->armour_health, c28->shield_health };
    for (int p_idx = 0; p_idx < 3 && heal_amt > 0.0f; p_idx++) {
        HealthPart* pool = pools[p_idx];
        for (int i = 0; i < 0x10 && heal_amt > 0.0f; i++) {
            if (!pool[i].part_enabled || pool[i].max_health <= 0.0f) continue;
            float missing = pool[i].max_health - pool[i].curr_health;
            if (missing <= 0.0f) continue;
            if (heal_amt >= missing) {
                pool[i].curr_health = pool[i].max_health;
                heal_amt -= missing;
            } else {
                pool[i].curr_health += heal_amt;
                heal_amt = 0.0f;
            }
        }
    }
    return c28->CalcTotalCurrentHealth();
}

static float ApplyHeroSelfHeal(Entity* target_ent, float heal_amt, uint64_t script_id, const char* tag) {
    if (!target_ent || IsBadReadPtr(target_ent, sizeof(Entity)) || !target_ent->hasComponent(0x28)) return 0.0f;
    auto* c28 = target_ent->getById<Component_28_STUHealthComponent>(0x28);
    if (!c28 || IsBadReadPtr(c28, sizeof(Component_28_STUHealthComponent))) return 0.0f;
    float new_hp = ApplyHealingToHealthComponent(c28, heal_amt);
    SyncEntityHealthVarBag(target_ent);
    ShotDiagLogFmt("[%s] script=0x%llX target=0x%X +%.1f -> HP %.1f",
        tag ? tag : "SELF_HEAL", (unsigned long long)script_id, target_ent->entity_id, heal_amt, new_hp);
    return new_hp;
}

static float ResolveShotBaseDamage(Entity* shooter, Entity* proj_ent) {
    if (!shooter) shooter = GetLocalHeroModelEnt();
    if (!shooter) return WeaponHardpointDiag::s_dummy_damage_per_pellet;

    // 1. Check projectile entity resource ID first for known projectile types
    uint64_t proj_res = (proj_ent && proj_ent->resload_entry) ? (uint64_t)proj_ent->resload_entry->resource_id : 0;
    if (proj_res == 0x0400000000000045ULL) return 5.0f; // Tracer Pulse Bomb stick

    // 2. Determine shooter's STUHero ID (via Component_1F controller -> Component_3F or model_res_id)
    EntityAdminBase* ea = GameEntityAdmin();
    uint64_t heroid = 0;
    uint64_t model_res_id = shooter->resload_entry ? (uint64_t)shooter->resload_entry->resource_id : 0;
    if (auto* c1f = shooter->getById<Component_1F_Backref>(0x1F)) {
        if (c1f->backref_entid && ea) {
            if (Entity* ctrl_ent = ea->getEntById(c1f->backref_entid)) {
                if (auto* c3f = ctrl_ent->getById<Component_3F_PlayerInfo>(0x3F)) {
                    heroid = (uint64_t)c3f->selected_heroid;
                }
            }
        }
    }
    if (heroid == 0 && ea && ea->local_entid) {
        if (Entity* local_ctrl = ea->getEntById(ea->local_entid)) {
            if (auto* c3f = local_ctrl->getById<Component_3F_PlayerInfo>(0x3F)) {
                heroid = (uint64_t)c3f->selected_heroid;
            }
        }
    }

    // Overwatch 0.8.0 (Build 24919) Per-Pellet / Per-Projectile Base Damage Table for All 21 Launch Heroes
    switch (heroid) {
    case 0x02E0000000000002ULL: return 7.0f;   // Reaper: Hellfire Shotguns (7.0 x 20 pellets = 140)
    case 0x02E0000000000003ULL: return 6.0f;   // Tracer: Pulse Pistols (6.0 x 2 pellets = 12)
    case 0x02E0000000000004ULL: return 20.0f;  // Mercy: Caduceus Blaster
    case 0x02E0000000000005ULL: return 125.0f; // Hanzo: Storm Bow arrow
    case 0x02E0000000000006ULL: return 70.0f;  // Torbjorn: Rivet Gun
    case 0x02E0000000000007ULL: return 100.0f; // Reinhardt: Fire Strike projectile
    case 0x02E0000000000008ULL: return 120.0f; // Pharah: Rocket Launcher
    case 0x02E0000000000009ULL: return 3.0f;   // Winston: Tesla Cannon tick
    case 0x02E000000000000AULL: return 13.0f;  // Widowmaker: Widow's Kiss (SMG / unscoped)
    case 0x02E0000000000015ULL: return 20.0f;  // Bastion: Configuration Recon / Sentry
    case 0x02E0000000000016ULL: return 25.0f;  // Symmetra: Photon Projector
    case 0x02E0000000000020ULL: return 45.0f;  // Zenyatta: Orb of Destruction
    case 0x02E0000000000029ULL: return 28.0f;  // Genji: Shuriken
    case 0x02E0000000000040ULL: return 9.0f;   // Roadhog: Scrap Gun (9.0 x 25 pellets = 225)
    case 0x02E0000000000042ULL: return 70.0f;  // McCree: Peacekeeper
    case 0x02E0000000000065ULL: return 120.0f; // Junkrat: Frag Launcher
    case 0x02E0000000000068ULL: return (proj_ent && proj_ent != shooter) ? 95.0f : 45.0f;  // Zarya: Particle Cannon (RMB grenade 95 / LMB beam 45)
    case 0x02E000000000006EULL: return (proj_ent && proj_ent != shooter) ? 120.0f : 17.0f; // Soldier: 76: Helix Rockets (120) / Heavy Pulse Rifle (17)
    case 0x02E0000000000079ULL: return 16.0f;  // Lucio: Sonic Amplifier
    case 0x02E000000000007AULL: return (model_res_id == 0x040000000000040AULL) ? 14.0f : 3.0f; // D.Va: Light Gun (14) / Fusion Cannons (3 x 8)
    case 0x02E00000000000DDULL: return 75.0f;  // Mei: Endothermic Blaster Icicle
    default: break;
    }
    return WeaponHardpointDiag::s_dummy_damage_per_pellet;
}

// 5. +0xD423C0: TickProjectileManager (EntityAdmin + 0xA80)
// Fixes Gate 3: At +0xD42595..+0xD425AD, +0xD423C0 unpacks the 13-bit unsigned inst_id ((handle >> 19) & 0x1FFF = 0x1FFD
// for inst_id = -3 = 0xFFFD) into [entry + 0x14] without sign-extending bit 12 (0x1000).
// Also integrates active 0xD78490D1 Impulse/Knockback velocity, 100ms Heal-Over-Time (HoT), and dash sweeps smoothly on the local Hero Model.
static void HideSingleEntity(Entity* ent) {
    if (!ent) return;
    const Vector4 hidden_pos(0.0f, -5000.0f, 0.0f, 0.0f);
    const Vector4 hidden_rot(0.0f, 0.0f, 0.0f, 1.0f);
    if (auto* slot = OriginClutterTracker::GetSlot(ent->entity_id, ent)) {
        slot->has_nonzero_pos = true;
        slot->latest_nonzero_pos = hidden_pos;
        slot->latest_nonzero_rot = hidden_rot;
    }
    if (ComponentBase* c12 = ent->getById(0x12)) {
        if (!IsBadReadPtr(c12, 0x40)) {
            *(Vector4*)((uintptr_t)c12 + 0x20) = hidden_pos;
        }
    }
    if (auto* c15 = ent->getById<Component_15_STUCharacterMoverComponent>(0x15)) {
        if (!IsBadReadPtr(c15, sizeof(Component_15_STUCharacterMoverComponent))) {
            c15->movement_state_1.absolute_position = hidden_pos;
            c15->movement_state_2.absolute_position = hidden_pos;
        }
    }
    if (auto* c1 = ent->getById<Component_1_SceneRendering>(1)) {
        if (!IsBadReadPtr(c1, sizeof(Component_1_SceneRendering))) {
            *(uint32_t*)((uintptr_t)c1 + 0x1B8) &= ~0x10000u;
            // Only queue scene dirty updates if [c1 + 0x148] is valid (otherwise +0x9F4720 -> +0xA453DB crashes on null [c1 + 0x148])
            if (SafeReadVal<uintptr_t>((uintptr_t)c1 + 0x148) != 0) {
                c1->SetScale(Vector4(0.0f, 0.0f, 0.0f, 0.0f));
                c1->SetInvisible();
                c1->SetPosRotation(hidden_pos, hidden_rot);
            } else {
                c1->scaling = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
                c1->position = hidden_pos;
            }
        }
    }
}

static void HideAndExpireSubEntity(EntityAdminBase* ea, Entity* sub_ent, Entity* local_hero) {
    if (!sub_ent || sub_ent == local_hero || (ea && sub_ent->entity_id == ea->local_entid)) return;
    Vector4 sub_pos{};
    if (auto* sc1 = sub_ent->getById<Component_1_SceneRendering>(1)) {
        if (!IsBadReadPtr(sc1, sizeof(Component_1_SceneRendering))) sub_pos = sc1->position;
    }

    // Clean up any 0x6E81F6D4 (STUStatescriptStateEffect) child entities & particle rings stored in sub_ent's Component_23 + 0x1458
    // (+0xD86F30 -> +0xD55AA0: slot_mgr = comp23 + 0x1458, recs = *(slot_mgr + 0x08), cnt = *(slot_mgr + 0x10), stride = 0x250, key = *(rec + 0x168))
    // IMPORTANT: In +0xD55AA0:
    //   - +0xD55B1C calls +0xD578C0 -> +0x9E3E20 (stops the yellow healing ring particle effect on Component_0D via [rec + 0x190]).
    //   - +0xD55B21..+0xD55B34 reads e0=[rec+0x00], e1=[rec+0x04] and calls +0xD576D0 -> +0x80FF50 (delEnt), which queues ce0's Component_1
    //     into the scene deletion queue ([scene_mgr + 0x2D0]) and crashes +0x9F4720 -> +0xA453C0 at +0xA453DB ([c1 + 0x148] == 0).
    // Masking [rec+0x00] = 0xFFFFFFFF and [rec+0x04] = 0xFFFFFFFF before calling +0xD55AA0 lets +0xD578C0 stop the particle effect
    // while skipping +0xD576D0 (delEnt), and we hide ce0/ce1 via HideSingleEntity instead.
    if (auto* sub_c23 = sub_ent->getById<Component_23_Statescript>(0x23)) {
        if (!IsBadReadPtr(sub_c23, sizeof(Component_23_Statescript))) {
            uintptr_t slot_mgr = (uintptr_t)sub_c23 + 0x1458;
            auto cleanup_slot_fn = (void(__fastcall*)(uintptr_t, uint64_t))(globals::gameBase + 0xD55AA0);
            for (int iter = 0; iter < 8; ++iter) {
                int32_t rec_cnt = SafeReadVal<int32_t>(slot_mgr + 0x10);
                uintptr_t recs  = SafeReadVal<uintptr_t>(slot_mgr + 0x08);
                if (rec_cnt <= 0 || !recs || IsBadReadPtr((const void*)recs, 0x250)) break;
                uint32_t e0  = SafeReadVal<uint32_t>(recs + 0x00);
                uint32_t e1  = SafeReadVal<uint32_t>(recs + 0x04);
                uint64_t key = SafeReadVal<uint64_t>(recs + 0x168);
                *(uint32_t*)(recs + 0x00) = 0xFFFFFFFFu;
                *(uint32_t*)(recs + 0x04) = 0xFFFFFFFFu;
                if (key != 0) {
                    cleanup_slot_fn(slot_mgr, key);
                }
                if (ea && e0 && e0 != 0xFFFFFFFFu) {
                    if (Entity* ce0 = ea->getEntById(e0)) HideSingleEntity(ce0);
                }
                if (ea && e1 && e1 != 0xFFFFFFFFu) {
                    if (Entity* ce1 = ea->getEntById(e1)) HideSingleEntity(ce1);
                }
                if (key == 0) break;
            }
        }
    }

    HideSingleEntity(sub_ent);

    if (!ea) return;
    for (Entity* child : *ea) {
        if (!child || child == sub_ent || child == local_hero || child->entity_id == ea->local_entid) continue;
        if (child->hasComponent(0x15) || child->hasComponent(0x28)) continue; // never hide heroes/dummies

        bool is_attached_child = (child->parent == sub_ent);
        if (!is_attached_child) {
            if (auto* c1f = child->getById<Component_1F_Backref>(0x1F)) {
                if (!IsBadReadPtr(c1f, sizeof(Component_1F_Backref)) && c1f->backref_entid == sub_ent->entity_id) {
                    is_attached_child = true;
                }
            }
        }
        uint64_t child_res_id = (child->resload_entry && !IsBadReadPtr(child->resload_entry, 0x38))
            ? SafeReadVal<uint64_t>((uintptr_t)child->resload_entry + 0x30) : 0ULL;
        if (child_res_id == 0x040000000000068AULL || child_res_id == 0x04000000000003DCULL) {
            is_attached_child = true;
        }
        if (!is_attached_child && child->entity_id > sub_ent->entity_id && child->entity_id <= sub_ent->entity_id + 16) {
            if (auto* cc1 = child->getById<Component_1_SceneRendering>(1)) {
                if (!IsBadReadPtr(cc1, sizeof(Component_1_SceneRendering))) {
                    Vector4 d = Sub3(cc1->position, sub_pos);
                    if (Dot3(d, d) <= 4.0f) {
                        is_attached_child = true;
                    }
                }
            }
        }
        if (is_attached_child) {
            HideSingleEntity(child);
        }
    }
}

using TickProjectileManagerFn = void(__fastcall*)(void* proj_sys);
static TickProjectileManagerFn TickProjectileManager_orig = nullptr;
static void __fastcall TickProjectileManager_hook(void* proj_sys) {
    EntityAdminBase* game_ea = GameEntityAdmin();
    struct SavedInstId {
        uint16_t* slot_ptr;
        uint16_t  orig_id;
    };
    SavedInstId saved[64];
    int saved_count = 0;

    uint64_t now_ms = GetTickCount64();
    Entity* local_hero = GetLocalHeroModelEnt();
    if (local_hero) {
        static uint32_t s_last_local_hero_entid = 0;
        if (local_hero->entity_id != s_last_local_hero_entid) {
            s_last_local_hero_entid = local_hero->entity_id;
            s_active_hot_end_ms = 0;
            s_active_hot_hp_per_100ms = 0.0f;
            s_bastion_repair_active = false;
            s_biotic_field_end_ms = 0;
            s_biotic_field_ent_id = 0;
            s_active_impulse_end_ms = 0;
            s_active_dash_sweep_end_ms = 0;
            s_riptire_ent_id = 0;
            s_riptire_end_ms = 0;
        }
        auto* comp15 = local_hero->getById<Component_15_STUCharacterMoverComponent>(0x15);
        auto* comp01 = local_hero->getById<Component_1_SceneRendering>(1);
        auto* owner_c10 = local_hero->getById<Component_10_FilterBits>(0x10);
        Vector4 hero_pos = (comp15 && !IsBadReadPtr(comp15, sizeof(Component_15_STUCharacterMoverComponent)))
            ? comp15->movement_state_1.absolute_position : (comp01 ? comp01->position : Vector4{});

        // 1a. Integrate active 0xD78490D1 impulse velocity if within active window
        if (s_active_impulse_end_ms > now_ms && comp15 && !IsBadReadPtr(comp15, sizeof(Component_15_STUCharacterMoverComponent))) {
            constexpr float kDt = 0.016f;
            Vector4 pos = comp15->movement_state_1.absolute_position;
            Vector4 rot = comp15->movement_state_1.rotation_quaternion;
            pos.X += s_active_impulse_vel.X * kDt;
            pos.Y += s_active_impulse_vel.Y * kDt;
            pos.Z += s_active_impulse_vel.Z * kDt;
            if (pos.Y < s_active_impulse_min_y) pos.Y = s_active_impulse_min_y;
            comp15->movement_state_1.absolute_position = pos;
            comp15->movement_state_2.absolute_position = pos;
            if (s_active_impulse_vel.Y > 0.0f) {
                comp15->movement_state_1.gravity_delta.Y = 0.0f;
                comp15->movement_state_2.gravity_delta.Y = 0.0f;
            }
            if (comp01 && !IsBadReadPtr(comp01, sizeof(Component_1_SceneRendering))) {
                comp01->SetPosRotation(pos, rot);
            }
            hero_pos = pos;
        }

        // 1b. Smooth 100ms Heal-Over-Time (Roadhog Breather 300 HP / 1.0s, Mei Cryo-Freeze 150 HP / 4.0s)
        if (s_active_hot_end_ms > now_ms && s_active_hot_hp_per_100ms > 0.0f && (now_ms - s_active_hot_last_tick_ms) >= 100) {
            s_active_hot_last_tick_ms = now_ms;
            ApplyHeroSelfHeal(local_hero, s_active_hot_hp_per_100ms, s_active_hot_script_id, "HOT_TICK");
        }

        // 1c. Bastion Self-Repair (0x0580000000000153): continuous +7.5 HP per 100ms (75 HP/s) while 'E' is held (node=47 active)
        if (s_bastion_repair_active && (now_ms - s_bastion_last_tick_ms) >= 100) {
            s_bastion_last_tick_ms = now_ms;
            ApplyHeroSelfHeal(local_hero, 7.5f, 0x0580000000000153ULL, "BASTION_REPAIR_HOT");
        }

        // 1d. Soldier: 76 Biotic Field (0x0580000000000257 / 0x0580000000000258): +4.0 HP per 100ms (40 HP/s) for 5s within 5.5m
        if (s_biotic_field_end_ms > 0) {
            if (now_ms < s_biotic_field_end_ms) {
                if ((now_ms - s_biotic_field_last_ms) >= 100) {
                    s_biotic_field_last_ms = now_ms;
                    Vector4 d = Sub3(hero_pos, s_biotic_field_pos);
                    if (sqrtf(Dot3(d, d)) <= 5.5f) {
                        ApplyHeroSelfHeal(local_hero, 4.0f, 0x0580000000000258ULL, "BIOTIC_FIELD_HOT");
                    }
                }
            } else {
                if (game_ea && s_biotic_field_ent_id != 0) {
                    if (Entity* bf_ent = game_ea->getEntById(s_biotic_field_ent_id)) {
                        if (bf_ent != local_hero) {
                            HideAndExpireSubEntity(game_ea, bf_ent, local_hero);
                        }
                    }
                }
                s_biotic_field_ent_id = 0;
                s_biotic_field_end_ms = 0;
            }
        }

        // 1e. Moving dash/leap damage sweep (Swift Strike 0x1DB = 50, Jump Pack 0x109 = 50, D.Va Boosters 0x26A = 25, Reinhardt Charge 0x07D = 75)
        // Works whether hero movement is driven by 0xD78490D1 or native movement states (0xC17B255D / 0x2A0FB7E6).
        if (s_active_dash_sweep_end_ms > now_ms && s_active_dash_sweep_dmg > 0.0f && game_ea && (comp15 || comp01)) {
            for (Entity* ent : *game_ea) {
                if (!ent || ent == local_hero || !ent->hasComponent(0x15) || !ent->hasComponent(0x28)) continue;
                auto* c10 = ent->getById<Component_10_FilterBits>(0x10);
                if (owner_c10 && c10 && (owner_c10->filter_bits & 0x3C0000ULL) == (c10->filter_bits & 0x3C0000ULL)) continue;
                bool already_hit = false;
                for (int h = 0; h < s_active_dash_sweep_hit_count; ++h) {
                    if (s_active_dash_sweep_hit_ents[h] == ent->entity_id) { already_hit = true; break; }
                }
                if (already_hit) continue;

                auto* c1 = ent->getById<Component_1_SceneRendering>(1);
                auto* c28 = ent->getById<Component_28_STUHealthComponent>(0x28);
                if (!c1 || IsBadReadPtr(c1, sizeof(Component_1_SceneRendering)) || !c28 || c28->CalcTotalCurrentHealth() <= 0.0f) continue;
                // Only hit active skeletal hitbox candidates (bit 16 = 0x10000), never hidden pilot models or non-hitbox entities
                if ((*(uint32_t*)((uintptr_t)c1 + 0x1B8) & 0x10000u) == 0) continue;

                Vector4 d = Sub3(c1->position, hero_pos);
                float dist = sqrtf(Dot3(d, d));
                if (dist <= s_active_dash_sweep_radius) {
                    if (s_active_dash_sweep_hit_count < 16) {
                        s_active_dash_sweep_hit_ents[s_active_dash_sweep_hit_count++] = ent->entity_id;
                    }
                    float new_hp = ApplyDamageToHealthComponent(c28, s_active_dash_sweep_dmg);
                    if (new_hp <= 0.0f) {
                        *(uint32_t*)((uintptr_t)c1 + 0x1B8) &= ~0x10000u;
                    }
                    *(uint32_t*)((uintptr_t)c28 + 0x14) = 1;
                    s_last_hit_dummy_entid = ent->entity_id;
                    s_last_hit_dummy_ms = GetTickCount64();
                    SyncEntityHealthVarBag(ent);
                    {
                        std::lock_guard<std::mutex> hit_lock(s_pellet_hit_mutex);
                        if (s_pending_dmg_pkt_count < 32) {
                            int32_t dmg_int = (int32_t)(s_active_dash_sweep_dmg + 0.5f);
                            if (dmg_int < 1) dmg_int = 1;
                            uint8_t pkt_flags = (uint8_t)(new_hp <= 0.0f ? 2 : 0);
                            s_pending_dmg_pkts[s_pending_dmg_pkt_count++] = PendingDamagePacket{ ent->entity_id, dmg_int, pkt_flags, true };
                        }
                    }
                    // Apply knockback impulse to hit enemy for D.Va Boosters (0x47E / 0x26A / 0xB2F), Reinhardt Charge (0x07D / 0x0BE), and Winston Jump Pack (0x109)
                    Vector4 hero_fwd{ 0.0f, 0.0f, 1.0f, 0.0f };
                    if (comp15 && !IsBadReadPtr(comp15, sizeof(Component_15_STUCharacterMoverComponent))) {
                        const Vector4& q = comp15->movement_state_1.rotation_quaternion;
                        hero_fwd.X = 2.0f * (q.X * q.Z + q.W * q.Y);
                        hero_fwd.Z = 1.0f - 2.0f * (q.X * q.X + q.Y * q.Y);
                    }
                    Vector4 push_dir{ d.X + hero_fwd.X * 1.5f, 0.0f, d.Z + hero_fwd.Z * 1.5f, 0.0f };
                    if (s_active_dash_sweep_script_id == 0x058000000000047EULL ||
                        s_active_dash_sweep_script_id == 0x058000000000026AULL ||
                        s_active_dash_sweep_script_id == 0x0580000000000B2FULL) {
                        ArmEnemyKnockback(ent, push_dir, 12.0f, 4.5f, 450);
                    } else if (s_active_dash_sweep_script_id == 0x058000000000007DULL || s_active_dash_sweep_script_id == 0x05800000000000BEULL) {
                        ArmEnemyKnockback(ent, push_dir, 16.0f, 5.5f, 550);
                    } else if (s_active_dash_sweep_script_id == 0x0580000000000109ULL || s_active_dash_sweep_script_id == 0x058000000000010AULL ||
                               s_active_dash_sweep_script_id == 0x0580000000000038ULL || s_active_dash_sweep_script_id == 0x05800000000000FEULL) {
                        ArmEnemyKnockback(ent, d, 8.0f, 3.5f, 350);
                    }
                    ShotDiagLogFmt("[DASH_SWEEP_DMG] script=0x%llX hit=0x%X dist=%.2fm -%.1f -> HP %.1f",
                        (unsigned long long)s_active_dash_sweep_script_id, ent->entity_id, dist, s_active_dash_sweep_dmg, new_hp);
                }
            }
        }

        // 1f. Integrate active enemy knockback velocities (D.Va Boosters, Reinhardt Charge, Winston Jump Pack / Primal Rage, Lucio Soundwave)
        if (game_ea && s_enemy_knockback_count > 0) {
            constexpr float kDt = 0.016f;
            for (int i = 0; i < s_enemy_knockback_count; ) {
                EnemyKnockbackEntry& kb = s_enemy_knockbacks[i];
                Entity* kb_ent = kb.ent_id ? game_ea->getEntById(kb.ent_id) : nullptr;
                if (!kb_ent || kb_ent == local_hero || now_ms >= kb.end_ms) {
                    if (kb_ent && kb_ent != local_hero) {
                        if (auto* c1 = kb_ent->getById<Component_1_SceneRendering>(1)) {
                            if (!IsBadReadPtr(c1, sizeof(Component_1_SceneRendering)) && c1->position.Y < kb.ground_y) {
                                Vector4 p = c1->position;
                                p.Y = kb.ground_y;
                                if (auto* slot = OriginClutterTracker::GetSlot(kb_ent->entity_id, kb_ent)) {
                                    slot->has_nonzero_pos = true;
                                    slot->latest_nonzero_pos = p;
                                }
                                c1->SetPosRotation(p, c1->rotation);
                                if (auto* c15 = kb_ent->getById<Component_15_STUCharacterMoverComponent>(0x15)) {
                                    if (!IsBadReadPtr(c15, sizeof(Component_15_STUCharacterMoverComponent))) {
                                        c15->movement_state_1.absolute_position = p;
                                        c15->movement_state_2.absolute_position = p;
                                    }
                                }
                            }
                        }
                    }
                    s_enemy_knockbacks[i] = s_enemy_knockbacks[--s_enemy_knockback_count];
                    continue;
                }
                auto* c1 = kb_ent->getById<Component_1_SceneRendering>(1);
                auto* c15 = kb_ent->getById<Component_15_STUCharacterMoverComponent>(0x15);
                if (c1 && !IsBadReadPtr(c1, sizeof(Component_1_SceneRendering))) {
                    Vector4 p = c1->position;
                    Vector4 r = c1->rotation;
                    p.X += kb.vel.X * kDt;
                    p.Y += kb.vel.Y * kDt;
                    p.Z += kb.vel.Z * kDt;
                    kb.vel.Y -= 19.6f * kDt;
                    kb.vel.X *= 0.96f;
                    kb.vel.Z *= 0.96f;
                    if (p.Y <= kb.ground_y) {
                        p.Y = kb.ground_y;
                        if (kb.vel.Y < 0.0f) kb.vel.Y = 0.0f;
                    }
                    if (auto* slot = OriginClutterTracker::GetSlot(kb_ent->entity_id, kb_ent)) {
                        slot->has_nonzero_pos = true;
                        slot->latest_nonzero_pos = p;
                        slot->latest_nonzero_rot = r;
                    }
                    c1->SetPosRotation(p, r);
                    if (c15 && !IsBadReadPtr(c15, sizeof(Component_15_STUCharacterMoverComponent))) {
                        c15->movement_state_1.absolute_position = p;
                        c15->movement_state_2.absolute_position = p;
                    }
                }
                ++i;
            }
        }

        if (auto* c23 = local_hero->getById<Component_23_Statescript>(0x23)) {
            int num = c23->ss_inner.g3_arrWithIds.num;
            uintptr_t arr = (uintptr_t)c23->ss_inner.g3_arrWithIds.ptr;
            if (arr && num > 0) {
                for (int i = 0; i < num && saved_count < 64; ++i) {
                    uint16_t* id_ptr = (uint16_t*)(arr + i * 0x10 + 0x08);
                    uint16_t raw_id  = *id_ptr;
                    if ((raw_id & 0xE000u) == 0xE000u) { // negative int16
                        saved[saved_count++] = { id_ptr, raw_id };
                        *id_ptr = (uint16_t)(raw_id & 0x1FFFu);
                    }
                }
            }
        }
    }

    // 2. Stage H1.3: Tick created sub-entities (Turrets, Sentries, Ice Wall, Biotic Field, Blizzard, Dragonstrike, Barriers)
    if (game_ea && s_created_sub_entity_count > 0) {
        constexpr float kDt = 0.016f;

        for (int i = 0; i < s_created_sub_entity_count; ) {
            CreatedSubEntityEntry& sub = s_created_sub_entities[i];
            Entity* sub_ent = sub.ent_id ? game_ea->getEntById(sub.ent_id) : nullptr;
            if (!sub_ent || (sub.expire_ms > 0 && now_ms >= sub.expire_ms)) {
                if (sub_ent && sub.expire_ms > 0 && now_ms >= sub.expire_ms && sub_ent != local_hero) {
                    HideAndExpireSubEntity(game_ea, sub_ent, local_hero);
                }
                s_created_sub_entities[i] = s_created_sub_entities[--s_created_sub_entity_count];
                continue;
            }

            // Integrate velocity / ground clamp for moving or stationary sub-entities
            auto* sub_c1 = sub_ent->getById<Component_1_SceneRendering>(1);
            auto* sub_c15 = sub_ent->getById<Component_15_STUCharacterMoverComponent>(0x15);
            if (sub_c1 && !IsBadReadPtr(sub_c1, sizeof(Component_1_SceneRendering))) {
                Vector4 p = sub_c1->position;
                Vector4 r = sub_c1->rotation;
                bool moved = false;
                if (sub.vel.X != 0.0f || sub.vel.Y != 0.0f || sub.vel.Z != 0.0f) {
                    p.X += sub.vel.X * kDt;
                    p.Y += sub.vel.Y * kDt;
                    p.Z += sub.vel.Z * kDt;
                    if (sub.has_gravity) {
                        sub.vel.Y -= 19.6f * kDt;
                        if (p.Y <= sub.ground_y) {
                            p.Y = sub.ground_y;
                            sub.vel = Vector4{};
                            sub.has_gravity = false;
                            sub.lock_stationary = true;
                            sub.spawn_pos = p;
                        }
                    }
                    moved = true;
                } else if (sub.lock_stationary) {
                    if (fabsf(p.X - sub.spawn_pos.X) > 0.01f || p.Y < sub.ground_y || fabsf(p.Z - sub.spawn_pos.Z) > 0.01f) {
                        p.X = sub.spawn_pos.X;
                        if (p.Y < sub.ground_y) p.Y = sub.ground_y;
                        p.Z = sub.spawn_pos.Z;
                        moved = true;
                    }
                    if (sub_c15 && !IsBadReadPtr(sub_c15, sizeof(Component_15_STUCharacterMoverComponent))) {
                        sub_c15->movement_state_1.gravity_delta = Vector4{};
                        sub_c15->movement_state_2.gravity_delta = Vector4{};
                        sub_c15->movement_state_1.absolute_position = p;
                        sub_c15->movement_state_2.absolute_position = p;
                    }
                } else if (sub.has_gravity && p.Y < sub.ground_y) {
                    p.Y = sub.ground_y;
                    moved = true;
                }
                if (moved) {
                    if (auto* slot = OriginClutterTracker::GetSlot(sub_ent->entity_id, sub_ent)) {
                        slot->has_nonzero_pos = true;
                        slot->latest_nonzero_pos = p;
                        slot->latest_nonzero_rot = r;
                    }
                    sub_c1->SetPosRotation(p, r);
                    if (sub_c15 && !IsBadReadPtr(sub_c15, sizeof(Component_15_STUCharacterMoverComponent))) {
                        sub_c15->movement_state_1.absolute_position = p;
                        sub_c15->movement_state_2.absolute_position = p;
                    }
                }
            }

            // Tick the sub-entity's StatescriptSyncMgr (+0xD13C20 with 3 arguments) for manually spawned sub-entities
            if (sub.script_id != 0x0580000000000258ULL) {
                if (auto* sub_c23 = sub_ent->getById<Component_23_Statescript>(0x23)) {
                    if (!IsBadReadPtr(sub_c23, sizeof(Component_23_Statescript))) {
                        SafeTickSyncMgr(&sub_c23->ss_inner);
                    }
                }
            }

            // Periodic 250ms combat tick for active deployables (Turret, Sentry, Blizzard, Dragonstrike)
            if (sub_c1 && (now_ms - sub.last_effect_ms) >= 250) {
                sub.last_effect_ms = now_ms;
                Entity* owner_ent = sub.owner_ent_id ? game_ea->getEntById(sub.owner_ent_id) : local_hero;
                auto* owner_c10 = owner_ent ? owner_ent->getById<Component_10_FilterBits>(0x10) : nullptr;
                Vector4 sp = sub_c1->position;

                // Note: Soldier: 76 Biotic Field healing (+4.0 HP per 100ms = 40 HP/s) is handled in section 1d above via s_biotic_field_end_ms.
                // Hanzo Dragonstrike (0x0580000000000101 / 0x04000000000002BA / 0x04000000000007C0), Mei Blizzard (0x058000000000064A),
                // Torbjorn Turret (0x05800000000000F4 / 0x0400000000000143 / 0x040000000000005E),
                // Symmetra Sentry (0x058000000000017E / 0x0400000000000251 / 0x0400000000000849)
                bool is_dragon = (sub.script_id == 0x0580000000000101ULL || sub.res_id == 0x04000000000002BAULL || sub.res_id == 0x04000000000007C0ULL);
                bool is_blizz  = (sub.script_id == 0x058000000000064AULL);
                bool is_torb_t = (sub.script_id == 0x05800000000000F4ULL || sub.res_id == 0x0400000000000143ULL || sub.res_id == 0x040000000000005EULL);
                bool is_sym_t  = (sub.script_id == 0x058000000000017EULL || sub.res_id == 0x0400000000000251ULL || sub.res_id == 0x0400000000000849ULL);
                if (is_dragon || is_blizz || is_torb_t || is_sym_t) {
                    float radius = is_torb_t ? 40.0f : (is_sym_t ? 10.0f : (is_blizz ? 10.0f : 4.5f));
                    float tick_dmg = is_dragon ? 50.0f : (is_blizz ? 20.0f : (is_torb_t ? 14.0f : 12.5f));
                    bool single_target = (is_torb_t || is_sym_t);
                    for (Entity* ent : *game_ea) {
                        if (!ent || ent == owner_ent || ent == sub_ent || !ent->hasComponent(0x15) || !ent->hasComponent(0x28)) continue;
                        auto* c10 = ent->getById<Component_10_FilterBits>(0x10);
                        if (owner_c10 && c10 && (owner_c10->filter_bits & 0x3C0000ULL) == (c10->filter_bits & 0x3C0000ULL)) continue;
                        auto* c1 = ent->getById<Component_1_SceneRendering>(1);
                        auto* c28 = ent->getById<Component_28_STUHealthComponent>(0x28);
                        if (!c1 || !c28 || c28->CalcTotalCurrentHealth() <= 0.0f) continue;

                        Vector4 d = Sub3(c1->position, sp);
                        float dist = sqrtf(Dot3(d, d));
                        if (dist <= radius) {
                            float new_hp = ApplyDamageToHealthComponent(c28, tick_dmg);
                            if (new_hp <= 0.0f) {
                                *(uint32_t*)((uintptr_t)c1 + 0x1B8) &= ~0x10000u;
                            }
                            SyncEntityHealthVarBag(ent);
                            {
                                std::lock_guard<std::mutex> hit_lock(s_pellet_hit_mutex);
                                if (s_pending_dmg_pkt_count < 32) {
                                    int32_t dmg_int = (int32_t)(tick_dmg + 0.5f);
                                    if (dmg_int < 1) dmg_int = 1;
                                    uint8_t pkt_flags = (uint8_t)(new_hp <= 0.0f ? 2 : 0);
                                    s_pending_dmg_pkts[s_pending_dmg_pkt_count++] = PendingDamagePacket{ ent->entity_id, dmg_int, pkt_flags, true };
                                }
                            }
                            if (single_target) break;
                        }
                    }
                }
            }
            ++i;
        }
    }

    // 3. Stage H1.3: Junkrat RIP-Tire (0x0580000000000232) expiration / detonation check
    if (s_riptire_ent_id != 0 && s_riptire_end_ms > 0 && now_ms >= s_riptire_end_ms && game_ea) {
        Entity* tire_ent = game_ea->getEntById(s_riptire_ent_id);
        if (tire_ent) {
            if (auto* tc1 = tire_ent->getById<Component_1_SceneRendering>(1)) {
                Vector4 tp = tc1->position;
                Entity* junk_ent = s_riptire_saved_mdl_id ? game_ea->getEntById(s_riptire_saved_mdl_id) : nullptr;
                auto* jc10 = junk_ent ? junk_ent->getById<Component_10_FilterBits>(0x10) : nullptr;
                for (Entity* ent : *game_ea) {
                    if (!ent || ent == tire_ent || ent == junk_ent || !ent->hasComponent(0x15) || !ent->hasComponent(0x28)) continue;
                    auto* c10 = ent->getById<Component_10_FilterBits>(0x10);
                    if (jc10 && c10 && (jc10->filter_bits & 0x3C0000ULL) == (c10->filter_bits & 0x3C0000ULL)) continue;
                    auto* c1 = ent->getById<Component_1_SceneRendering>(1);
                    auto* c28 = ent->getById<Component_28_STUHealthComponent>(0x28);
                    if (!c1 || !c28 || c28->CalcTotalCurrentHealth() <= 0.0f) continue;
                    Vector4 d = Sub3(c1->position, tp);
                    if (sqrtf(Dot3(d, d)) <= 10.0f) {
                        float new_hp = ApplyDamageToHealthComponent(c28, 600.0f);
                        if (new_hp <= 0.0f) *(uint32_t*)((uintptr_t)c1 + 0x1B8) &= ~0x10000u;
                        SyncEntityHealthVarBag(ent);
                    }
                }
            }
            game_ea->delEnt(tire_ent);
        }
        if (Entity* local_ctrl = game_ea->getLocalEnt()) {
            if (auto* c20 = local_ctrl->getById<Component_20_ModelReference>(0x20)) {
                if (s_riptire_saved_mdl_id != 0) {
                    c20->cam_attach_entid = s_riptire_saved_mdl_id;
                    c20->aim_entid = s_riptire_saved_mdl_id;
                    c20->movement_attach_entid = s_riptire_saved_mdl_id;
                }
            }
        }
        s_riptire_ent_id = 0;
        s_riptire_saved_mdl_id = 0;
        s_riptire_end_ms = 0;
    }

    TickProjectileManager_orig(proj_sys);

    for (int i = 0; i < saved_count; ++i) {
        *saved[i].slot_ptr = saved[i].orig_id;
    }
}

// 6. +0xD39500: OnProjectileDetonate (called from UpdateProjectileStickAndDetonationState +0xD392C0 at +0xD393B8)
// Native detonation point for projectiles whose fuse expires (+0xD3F1E0 returns 0 -> dil = 1).
// Because Statescript damage action 0x04CC5359 (node[14] & node[26] in 0x580000000000034) is stripped from the client binary,
// we apply Pulse Bomb (0x0400000000000045) AoE explosion damage (400.0f inner radius 3.5m / stuck target, linear falloff
// to 80.0f at outer radius 5.5m) here when is_detonating != 0.
using OnProjectileDetonateFn = void(__fastcall*)(
    uintptr_t proj_sys,
    uintptr_t comp27,
    uintptr_t comp27_hit_a0,
    Component_1_SceneRendering* proj_c1,
    uintptr_t proj_def_ref,
    uint8_t is_detonating);
static OnProjectileDetonateFn OnProjectileDetonate_orig = nullptr;
static void __fastcall OnProjectileDetonate_hook(
    uintptr_t proj_sys,
    uintptr_t comp27,
    uintptr_t comp27_hit_a0,
    Component_1_SceneRendering* proj_c1,
    uintptr_t proj_def_ref,
    uint8_t is_detonating)
{
    OnProjectileDetonate_orig(proj_sys, comp27, comp27_hit_a0, proj_c1, proj_def_ref, is_detonating);

    if (!is_detonating || !comp27) return;

    Entity* proj_ent = SafeReadVal<Entity*>(comp27 + 0x08);
    uint32_t proj_id = proj_ent ? SafeReadVal<uint32_t>((uintptr_t)&proj_ent->entity_id) : 0;
    bool is_pb = IsPulseBombEntity(proj_ent);

    // Determine impact/stick position: comp27 + 0xB0, fallback to proj_c1->position
    Vector4 bomb_pos = SafeReadVal<Vector4>(comp27 + 0xB0);
    if (bomb_pos.X == 0.0f && bomb_pos.Y == 0.0f && bomb_pos.Z == 0.0f && proj_c1) {
        bomb_pos = proj_c1->position;
    }

    ShotDiagLogFmt("[PROJ_ARM_OR_DETONATE (+0xD39500)] proj=0x%X is_pb=%d c27=0x%llX det=%u stuck=0x%X pos=(%.2f,%.2f,%.2f)",
        proj_id, is_pb ? 1 : 0, (unsigned long long)comp27, is_detonating, s_pulse_bomb_stuck_entid.load(),
        bomb_pos.X, bomb_pos.Y, bomb_pos.Z);

    if (is_pb) {
        // +0xD39500 fires on stick/land when spawning the armed beeping Pulse Bomb effect (1.0s fuse).
        // Schedule the 400.0f AoE detonation for +1000ms on the main thread (+0xCA7200).
        s_pulse_bomb_c27.store(comp27);
        s_pulse_bomb_land_pos = bomb_pos;
        s_pulse_bomb_detonate_ms.store(GetTickCount64() + 1000);
        s_pulse_bomb_armed.store(true);
    }
}

// Note: +0xD3EEE0 is called from TWO places:
//   1. +0xD3C3D0 (+0xD3C5A3, called from +0xD37131 when [comp27 + 0xE27] != 0 on Pulse Bomb stick/impact!)
//   2. +0xD39500 (+0xD397EE, when [comp27 + 0x66] != 0)
// For Pulse Bomb (0x0400000000000045), [comp27 + 0x66] == 0 so +0xD392C0 exits early at +0xD392DD and never calls +0xD39500;
// instead, +0xD37131 -> +0xD3C3D0 -> +0xD3EEE0 spawns the armed beeping bomb effect entity (1.0s fuse) on impact/stick!
typedef uint64_t(__fastcall* proj_spawn_impact_fn)(uintptr_t sys, uintptr_t c27, uintptr_t hit_rec_a0, Entity* effect_ent, uintptr_t a5);
proj_spawn_impact_fn ProjectileSpawnImpactEffect_orig = nullptr;
uint64_t __fastcall ProjectileSpawnImpactEffect_hook(uintptr_t sys, uintptr_t c27, uintptr_t hit_rec_a0, Entity* effect_ent, uintptr_t a5) {
    Entity* proj_ent = c27 ? SafeReadVal<Entity*>(c27 + 0x08) : nullptr;
    uint32 proj_id = proj_ent ? SafeReadVal<uint32>((uintptr_t)&proj_ent->entity_id) : 0;
    uint32 effect_id = effect_ent ? SafeReadVal<uint32>((uintptr_t)&effect_ent->entity_id) : 0;
    Vector4 p = c27 ? SafeReadVal<Vector4>(c27 + 0xB0) : Vector4{};
    if (p.X == 0.0f && p.Y == 0.0f && p.Z == 0.0f && proj_ent) {
        if (auto* proj_c1 = proj_ent->getById<Component_1_SceneRendering>(1)) {
            p = proj_c1->position;
        }
    }
    uint32_t e00 = c27 ? SafeReadVal<uint32_t>(c27 + 0xE00) : 0;
    bool is_pb = IsPulseBombEntity(proj_ent);
    ShotDiagLogFmt("[PROJ_IMPACT (+0xD3EEE0)] proj=0x%X is_pb=%d c27=0x%llX effect_ent=0x%X e00=0x%X posB0=(%.2f,%.2f,%.2f)",
        proj_id, is_pb ? 1 : 0, (unsigned long long)c27, effect_id, e00, p.X, p.Y, p.Z);

    if (is_pb && !s_pulse_bomb_armed.load()) {
        // +0xD37131 -> +0xD3C3D0 -> +0xD3EEE0 fires on the exact tick the Pulse Bomb sticks/lands and spawns the
        // armed beeping effect entity (e.g. effect_ent=0x80000A3A, 1.0s fuse). Schedule the 400.0f AoE detonation for +1000ms.
        s_pulse_bomb_c27.store(c27);
        s_pulse_bomb_land_pos = p;
        s_pulse_bomb_detonate_ms.store(GetTickCount64() + 1000);
        s_pulse_bomb_armed.store(true);
    }

    return ProjectileSpawnImpactEffect_orig(sys, c27, hit_rec_a0, effect_ent, a5);
}

// 6c. +0xD3DC50: ShouldDeferProjectileImpactForServerSync(uintptr_t sys, uintptr_t hit_ctx, uint32_t hit_type, uintptr_t effect_stu)
// On a skeletal hitbox hit (+0xD381F0 -> +0xD3B200 with r8b=0), +0xD3B290 skips +0xD3DC50 and immediately spawns the
// explosion VFX via +0xC56380 -> +0xD35450.
// On a world geometry hit (+0xD37E70 -> +0xD3B200 with r8b=1), +0xD3B200 calls +0xD3DC50 at +0xD3B2A0 (also called from
// +0xD3AAF0 at +0xD3AB83 and +0xD3BE60 at +0xD3BEC3). When [effect_stu + 0xE9] != 0 or [effect_stu + 0xEC] != 0 (as on
// Soldier: 76's Helix Rockets), +0xD3DC50 pushes the hit into [c27 + 0xB00] to wait for server snapshot replication
// (+0xD39A80 -> +0xD3DE00) and returns 1, causing +0xD3B200 to exit early without spawning the wall explosion!
// Returning 0 in offline mode lets +0xD3B200, +0xD3AAF0, and +0xD3BE60 immediately spawn the explosion VFX & decal.
typedef uint8_t(__fastcall* should_defer_proj_impact_fn)(uintptr_t sys, uintptr_t hit_ctx, uint32_t hit_type, uintptr_t effect_stu);
should_defer_proj_impact_fn ShouldDeferProjectileImpact_orig = nullptr;
uint8_t __fastcall ShouldDeferProjectileImpact_hook(uintptr_t sys, uintptr_t hit_ctx, uint32_t hit_type, uintptr_t effect_stu) {
    uint8_t e9 = effect_stu ? SafeReadVal<uint8_t>(effect_stu + 0xE9) : 0;
    uint8_t ec = effect_stu ? SafeReadVal<uint8_t>(effect_stu + 0xEC) : 0;
    ShotDiagLogFmt("[PROJ_WALL_IMPACT (+0xD3DC50)] hit_ctx=0x%llX hit_type=%u effect_stu=0x%llX (e9=%u,ec=%u) -> forcing immediate explosion (ret=0)",
        (unsigned long long)hit_ctx, hit_type, (unsigned long long)effect_stu, e9, ec);
    return 0;
}

// 7. +0xF43450: FindStatescriptStateImpl(uintptr_t rcx, STUStatescriptBase* stu_node) -> StatescriptState*
// Replaces the 4 Group-1 client stub vtables in +0x18F8E88 with their real Group-2 factory implementations
// still compiled in GameClientApp.exe:
//   - 0x160C4F1D (Placement State): stub vt +0x15D6F40 -> real factory +0xD64380 (real vt +0x15D5E68)
//   - 0x72509D3B (FindGround):      stub vt +0x15D6598 -> real factory +0xD64110 (real vt +0x15D60D8)
//   - 0x12AA4F0F (Ice Wall Start):  stub vt +0x15D6CC8 -> real factory +0xD645D0
//   - 0x614C3A0A (Ice Wall Pillar): stub vt +0x15D6A70 -> real factory +0xD647D0
typedef StatescriptState*(__fastcall* find_ss_state_impl_fn)(uintptr_t rcx, STUStatescriptBase* stu_node);
static find_ss_state_impl_fn FindStatescriptStateImpl_orig = nullptr;
static StatescriptState* __fastcall FindStatescriptStateImpl_hook(uintptr_t rcx, STUStatescriptBase* stu_node) {
    StatescriptState* state = FindStatescriptStateImpl_orig(rcx, stu_node);
    if (state && !IsBadReadPtr(state, sizeof(uintptr_t))) {
        uintptr_t vt = SafeReadVal<uintptr_t>((uintptr_t)state);
        uintptr_t vt_rva = (vt >= globals::gameBase && vt < globals::gameBase + globals::gameSize)
            ? (vt - globals::gameBase) : 0;
        using StateFactoryFn = StatescriptState*(__fastcall*)();
        if (vt_rva == 0x15D6F40) {
            state = ((StateFactoryFn)(globals::gameBase + 0xD64380))();
            ShotDiagLogFmt("[STUB_OVERRIDE (+0xF43450)] 0x160C4F1D PlacementState stub +0x15D6F40 -> +0xD64380 (state=0x%llX)", (unsigned long long)state);
        } else if (vt_rva == 0x15D6598) {
            state = ((StateFactoryFn)(globals::gameBase + 0xD64110))();
            ShotDiagLogFmt("[STUB_OVERRIDE (+0xF43450)] 0x72509D3B FindGround stub +0x15D6598 -> +0xD64110 (state=0x%llX)", (unsigned long long)state);
        } else if (vt_rva == 0x15D6CC8) {
            state = ((StateFactoryFn)(globals::gameBase + 0xD645D0))();
            ShotDiagLogFmt("[STUB_OVERRIDE (+0xF43450)] 0x12AA4F0F MeiIceWallStart stub +0x15D6CC8 -> +0xD645D0 (state=0x%llX)", (unsigned long long)state);
        } else if (vt_rva == 0x15D6A70) {
            state = ((StateFactoryFn)(globals::gameBase + 0xD647D0))();
            ShotDiagLogFmt("[STUB_OVERRIDE (+0xF43450)] 0x614C3A0A MeiIceWallPillar stub +0x15D6A70 -> +0xD647D0 (state=0x%llX)", (unsigned long long)state);
        }
    }
    return state;
}

static void SafeCallSetConfigVarValueFromSTU(StatescriptVar_VarBag* vb, STUConfigVarDynamic* out_var, STUConfigVar* m_value) {
    if (!vb || !out_var || !m_value) return;
    __try {
        vb->base.stu_base.vfptr->SetConfigVarValueFromSTUConfigVar(vb, out_var, m_value, nullptr);
    } __except (EXCEPTION_EXECUTE_HANDLER) {}
}

static bool IsHeroOrSubEntScript(StatescriptInstance* inst) {
    if (!inst || IsBadReadPtr(inst, sizeof(StatescriptInstance))) return false;
    Entity* local_hero = GetLocalHeroModelEnt();
    if (!local_hero) return false;
    Entity* owner_ent = (inst->ss_inner && !IsBadReadPtr(inst->ss_inner, sizeof(StatescriptSyncMgr)) && inst->ss_inner->component_ref)
        ? inst->ss_inner->component_ref->base.entity_backref : nullptr;
    if (!owner_ent || IsBadReadPtr(owner_ent, sizeof(Entity))) return false;
    if (owner_ent == local_hero) return true;
    uint64_t script_id = SafeReadVal<uint64_t>((uintptr_t)inst + 0x150);
    if (script_id == 0x0580000000000258ULL || script_id == 0x0580000000000636ULL) return true;

    for (int i = 0; i < s_created_sub_entity_count; ++i) {
        if (s_created_sub_entities[i].ent_id == owner_ent->entity_id) return true;
    }
    if (auto* hc23 = local_hero->getById<Component_23_Statescript>(0x23)) {
        uintptr_t slot_arr = SafeReadVal<uintptr_t>((uintptr_t)hc23 + 0x1458);
        int32_t slot_cnt   = SafeReadVal<int32_t>((uintptr_t)hc23 + 0x1460);
        if (slot_arr && slot_cnt > 0 && slot_cnt <= 16) {
            for (int i = 0; i < slot_cnt; ++i) {
                uint32_t wep0 = SafeReadVal<uint32_t>(slot_arr + (uintptr_t)i * 0x18 + 0x00);
                uint32_t wep1 = SafeReadVal<uint32_t>(slot_arr + (uintptr_t)i * 0x18 + 0x04);
                if (owner_ent->entity_id == wep0 || owner_ent->entity_id == wep1) return true;
            }
        }
    }
    return false;
}

static Entity* SpawnStandaloneSubEntity(EntityAdminBase* ea, uint64_t res_id, const Vector4& pos, const Vector4& rot) {
    if (!ea || (res_id & 0xFFFF000000000000ULL) != 0x0400000000000000ULL) return nullptr;
    auto res_ptr = try_load_resource((__int64)res_id);
    if (!res_ptr) return nullptr;

    auto spawner = EntityLoader::Create((__int64)res_id, res_ptr, false, false);
    if (!spawner) return nullptr;

    Component_1_SceneRendering::InitData init_data{};
    Vector4 valid_rot = rot;
    if (valid_rot.X == 0.0f && valid_rot.Y == 0.0f && valid_rot.Z == 0.0f && valid_rot.W == 0.0f) {
        valid_rot.W = 1.0f;
    }
    init_data.position = pos;
    init_data.scale = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    init_data.rotation = valid_rot;
    spawner->loader_entries[1].init_data = (__int64)&init_data;

    Entity* ent = spawner->Spawn(ea);
    if (!ent) return nullptr;

    if (auto* slot = OriginClutterTracker::GetSlot(ent->entity_id, ent)) {
        slot->has_nonzero_pos = true;
        slot->latest_nonzero_pos = pos;
        slot->latest_nonzero_rot = valid_rot;
    }
    if (auto* c1 = ent->getById<Component_1_SceneRendering>(1)) {
        c1->SetPosRotation(pos, valid_rot);
        c1->SetVisible();
        *(uint32_t*)((uintptr_t)c1 + 0x1B8) |= 0x10000u;
    }
    if (auto* c15 = ent->getById<Component_15_STUCharacterMoverComponent>(0x15)) {
        c15->movement_state_1.absolute_position = pos;
        c15->movement_state_2.absolute_position = pos;
        c15->movement_state_1.rotation_quaternion = valid_rot;
        c15->movement_state_2.rotation_quaternion = valid_rot;
    }
    return ent;
}

// Stage H1.3 Helper: SpawnStatescriptSubEntity (0x1BB99C36 Action, 0x4CCED352 Barrier/Deployable State, 0x0E3689F3 Dragonstrike Action/State)
static Entity* SpawnStatescriptSubEntity(StatescriptInstance* inst, STUStatescriptBase* stu_node, uint32_t stu_hash) {
    if (!inst || !stu_node || IsBadReadPtr(stu_node, sizeof(STUStatescriptBase))) return nullptr;
    EntityAdminBase* ea = GameEntityAdmin();
    Entity* owner_ent = (inst->ss_inner && !IsBadReadPtr(inst->ss_inner, sizeof(StatescriptSyncMgr)) && inst->ss_inner->component_ref)
        ? inst->ss_inner->component_ref->base.entity_backref : nullptr;
    if (!ea || !owner_ent) return nullptr;

    uint64_t script_id = SafeReadVal<uint64_t>((uintptr_t)inst + 0x150);
    // In Mei's Ice Wall (0x0580000000000622), node=52 spawns all 5 pillars (k = -2, -1, 0, +1, +2); skip duplicate CreateEntity nodes 54..57
    if (script_id == 0x0580000000000622ULL && stu_node->idx_in_nodes != 52) return nullptr;
    bool is_state_4cc = (stu_hash == 0x4CCED352u);
    bool is_node_0e3  = (stu_hash == 0x0E3689F3u);
    bool is_create_node = (stu_hash == 0x1BB99C36u || is_state_4cc || is_node_0e3);

    // Verified STU field offsets from horizon1_stu_audit.log:
    //   0x1BB99C36 (sz=0xD8):  +0x88 def_cv, +0x98 m_position, +0xA0 m_up, +0xA8 m_facing, +0xD0 out_cvd
    //   0x4CCED352 (sz=0x150): +0x100 def_cv, +0x110 m_position, +0x118 m_up, +0x120 m_facing, +0x148 out_cvd
    //   0x0E3689F3 (sz=0xD0):  +0x88 def_cv, +0x90 m_position, +0x98 m_up, +0xA0 m_facing, +0xB0 out_cvd
    auto* def_cv    = is_create_node ? (STUConfigVar*)SafeReadVal<uintptr_t>((uintptr_t)stu_node + (is_state_4cc ? 0x100 : 0x88)) : nullptr;
    auto* pos_cv    = is_create_node ? (STUConfigVar*)SafeReadVal<uintptr_t>((uintptr_t)stu_node + (is_state_4cc ? 0x110 : (is_node_0e3 ? 0x90 : 0x98))) : nullptr;
    auto* facing_cv = is_create_node ? (STUConfigVar*)SafeReadVal<uintptr_t>((uintptr_t)stu_node + (is_state_4cc ? 0x120 : (is_node_0e3 ? 0xA0 : 0xA8))) : nullptr;
    auto* out_cvd   = is_create_node ? (STUConfigVarDynamic*)SafeReadVal<uintptr_t>((uintptr_t)stu_node + (is_state_4cc ? 0x148 : (is_node_0e3 ? 0xB0 : 0xD0))) : nullptr;

    uint64_t res_id = 0;
    if (def_cv && !IsBadReadPtr(def_cv, 0x28)) {
        // STU 0xE53C2921 (STUConfigVarEntityDef) has stripped impl (+0x08 == 0); its STUResourceReference is at +0x10 (resource_id at +0x18 or +0x10)
        auto* ref = (STUResourceReference*)((uintptr_t)def_cv + 0x10);
        if (ref->has_resource() && ((ref->resource_id & 0xFFFF000000000000ULL) == 0x0400000000000000ULL)) {
            res_id = (uint64_t)ref->resource_id;
        } else {
            uint64_t r18 = SafeReadVal<uint64_t>((uintptr_t)def_cv + 0x18);
            uint64_t r10 = SafeReadVal<uint64_t>((uintptr_t)def_cv + 0x10);
            if ((r18 & 0xFFFF000000000000ULL) == 0x0400000000000000ULL) res_id = r18;
            else if ((r10 & 0xFFFF000000000000ULL) == 0x0400000000000000ULL) res_id = r10;
        }
        if (res_id == 0) {
            StatescriptPrimitive dprim{};
            def_cv->get_value(inst, &dprim);
            res_id = (uint64_t)dprim.value;
        }
    }
    if ((res_id & 0xFFFF000000000000ULL) != 0x0400000000000000ULL) {
        if (is_node_0e3 || script_id == 0x0580000000000101ULL) res_id = 0x04000000000002BAULL; // Hanzo Dragonstrike
        else if (script_id == 0x05800000000000F4ULL) res_id = 0x0400000000000143ULL; // Torbjorn Turret
        else if (script_id == 0x058000000000017EULL) res_id = 0x0400000000000251ULL; // Symmetra Sentry Turret
        else if (script_id == 0x0580000000000169ULL) res_id = 0x0400000000000233ULL; // Symmetra Teleporter
        else if (script_id == 0x0580000000000257ULL || script_id == 0x0580000000000148ULL) res_id = 0x04000000000003DCULL; // Soldier: 76 Biotic Field
        else if (script_id == 0x0580000000000622ULL) res_id = 0x0400000000000932ULL; // Mei Ice Wall Pillar
        else if (script_id == 0x05800000000005FEULL) res_id = 0x040000000000090BULL; // Mei Cryo-Freeze Block
        else if (script_id == 0x0580000000000232ULL) res_id = 0x040000000000039CULL; // Junkrat RIP-Tire
    }
    if ((res_id & 0xFFFF000000000000ULL) != 0x0400000000000000ULL) return nullptr;

    auto* owner_c1  = owner_ent->getById<Component_1_SceneRendering>(1);
    auto* owner_c10 = owner_ent->getById<Component_10_FilterBits>(0x10);
    auto* owner_c15 = owner_ent->getById<Component_15_STUCharacterMoverComponent>(0x15);

    Vector4 hero_pos = owner_c1 ? owner_c1->position : Vector4{};
    Vector4 hero_rot = owner_c1 ? owner_c1->rotation : Vector4{ 0.0f, 0.0f, 0.0f, 1.0f };
    Vector4 fwd{ 0.0f, 0.0f, 1.0f, 0.0f };
    float pitch = 0.0f;
    if (owner_c15 && !IsBadReadPtr(owner_c15, sizeof(Component_15_STUCharacterMoverComponent))) {
        hero_pos = owner_c15->movement_state_1.absolute_position;
        hero_rot = owner_c15->movement_state_1.rotation_quaternion;
        pitch = -owner_c15->movement_state_1.y_lookdir;
        fwd.X = 2.0f * (hero_rot.X * hero_rot.Z + hero_rot.W * hero_rot.Y);
        fwd.Y = 0.0f;
        fwd.Z = 1.0f - 2.0f * (hero_rot.X * hero_rot.X + hero_rot.Y * hero_rot.Y);
        float flen = sqrtf(fwd.X * fwd.X + fwd.Z * fwd.Z);
        if (flen > 1e-4f) { fwd.X /= flen; fwd.Z /= flen; }
    }

    Vector4 spawn_pos = hero_pos;
    bool got_pos = false;
    if (pos_cv && !IsBadReadPtr(pos_cv, sizeof(STUConfigVar))) {
        StatescriptPrimitive pprim{};
        pos_cv->get_value(inst, &pprim);
        if (pprim.type == StatescriptPrimitive_VEC3 && pprim.value != 0 && !IsBadReadPtr((const void*)pprim.value, 12)) {
            const float* v = (const float*)pprim.value;
            if (v[0] != 0.0f || v[1] != 0.0f || v[2] != 0.0f) {
                spawn_pos = Vector4{ v[0], v[1], v[2], 0.0f };
                got_pos = true;
            }
        }
    }
    if (!got_pos) {
        float dist = (script_id == 0x0580000000000257ULL || script_id == 0x0580000000000148ULL || script_id == 0x05800000000005FEULL) ? 0.3f : 2.8f;
        spawn_pos = Vector4{ hero_pos.X + fwd.X * dist, hero_pos.Y, hero_pos.Z + fwd.Z * dist, 0.0f };
    }
    if (spawn_pos.Y < hero_pos.Y - 1.5f) {
        spawn_pos.Y = hero_pos.Y;
    }

    Vector4 spawn_rot = hero_rot;
    if (facing_cv && !IsBadReadPtr(facing_cv, sizeof(STUConfigVar))) {
        StatescriptPrimitive rprim{};
        facing_cv->get_value(inst, &rprim);
        if (rprim.value != 0 && !IsBadReadPtr((const void*)rprim.value, 12)) {
            const float* v = (const float*)rprim.value;
            if (v[0] != 0.0f || v[1] != 0.0f || v[2] != 0.0f) {
                float fl = sqrtf(v[0] * v[0] + v[2] * v[2]);
                if (fl > 1e-4f) {
                    float yaw = atan2f(v[0] / fl, v[2] / fl);
                    spawn_rot = Vector4{ 0.0f, sinf(yaw * 0.5f), 0.0f, cosf(yaw * 0.5f) };
                }
            }
        }
    }

    Entity* new_ent = SpawnStandaloneSubEntity(ea, res_id, spawn_pos, spawn_rot);
    if (!new_ent) return nullptr;

    if (owner_c10) {
        if (auto* nc10 = new_ent->getById<Component_10_FilterBits>(0x10)) {
            nc10->pre_filter_bits = owner_c10->pre_filter_bits;
            nc10->filter_bits = ~nc10->pre_filter_bits;
            nc10->FilterBitsMakeFinal();
        }
    }
    if (auto* nc1f = new_ent->getById<Component_1F_Backref>(0x1F)) {
        nc1f->backref_entid = owner_ent->entity_id;
    }
    if (auto* nc23 = new_ent->getById<Component_23_Statescript>(0x23)) {
        if (res_id == 0x0400000000000932ULL) {
            nc23->loadScript(0x0580000000000636ULL);
        }
        if (nc23->ss_inner.g3_arrWithIds.ptr && nc23->ss_inner.g3_arrWithIds.num > 0 && nc23->ss_inner.g3_arrWithIds.num <= 64) {
            for (int i = 0; i < nc23->ss_inner.g3_arrWithIds.num; ++i) {
                StatescriptInstance* sub_ss = nc23->ss_inner.g3_arrWithIds.ptr[i].instance;
                if (sub_ss && !IsBadReadPtr(sub_ss, sizeof(StatescriptInstance))) {
                    sub_ss->execution_flags = (sub_ss->execution_flags | 0x400000u) & ~0x1000u;
                }
            }
        }
    }

    if (out_cvd && !IsBadReadPtr(out_cvd, sizeof(STUConfigVarDynamic))) {
        StatescriptPrimitive eprim(StatescriptPrimitive_ENTITY, (__int64)new_ent->entity_id);
        WriteDynamicVarPrim(inst, out_cvd, eprim);
    }

    if (s_created_sub_entity_count < 64) {
        uint64_t now_ms = GetTickCount64();
        CreatedSubEntityEntry entry{};
        entry.ent_id = new_ent->entity_id;
        entry.owner_ent_id = owner_ent->entity_id;
        entry.res_id = res_id;
        entry.script_id = script_id;
        entry.spawn_ms = now_ms;
        entry.last_effect_ms = now_ms;
        entry.spawn_pos = spawn_pos;
        entry.ground_y = (spawn_pos.Y >= hero_pos.Y - 0.5f) ? spawn_pos.Y : hero_pos.Y;
        entry.has_gravity = false;
        entry.lock_stationary = false;

        if (is_node_0e3 || script_id == 0x0580000000000101ULL || res_id == 0x04000000000002BAULL || res_id == 0x04000000000007C0ULL) {
            // Hanzo Dragonstrike: travels along aim direction at 20 m/s for 6s
            entry.vel = Vector4{ fwd.X * 20.0f, sinf(pitch) * 20.0f, fwd.Z * 20.0f, 0.0f };
            entry.expire_ms = now_ms + 6000;
        } else if (script_id == 0x0580000000000257ULL || script_id == 0x0580000000000148ULL ||
                   res_id == 0x04000000000003DCULL || res_id == 0x040000000000072DULL || res_id == 0x0400000000000A5EULL) {
            // Soldier: 76 Biotic Field: 5s stationary on ground
            entry.lock_stationary = true;
            entry.expire_ms = now_ms + 5000;
        } else if (script_id == 0x0580000000000622ULL || res_id == 0x0400000000000932ULL || res_id == 0x040000000000138EULL) {
            // Mei Ice Wall Pillar: 4.5s stationary on ground
            entry.lock_stationary = true;
            entry.expire_ms = now_ms + 4500;
        } else if (script_id == 0x05800000000005FEULL || res_id == 0x040000000000090BULL) {
            // Mei Cryo-Freeze Block: 4s stationary on ground
            entry.lock_stationary = true;
            entry.expire_ms = now_ms + 4000;
        } else if (script_id == 0x058000000000064AULL) {
            // Mei Blizzard Drone: 5s duration, lands on ground_y
            entry.has_gravity = true;
            entry.expire_ms = now_ms + 5000;
        } else if (script_id == 0x05800000000000F4ULL || res_id == 0x0400000000000143ULL || res_id == 0x040000000000005EULL) {
            // Torbjorn Turret: lock stationary on ground_y so native gravity never pulls it through floor
            entry.lock_stationary = true;
            entry.expire_ms = 0;
        } else if (script_id == 0x058000000000017EULL || script_id == 0x0580000000000169ULL ||
                   res_id == 0x0400000000000251ULL || res_id == 0x0400000000000232ULL || res_id == 0x0400000000000233ULL) {
            // Symmetra Sentry / Teleporter: lock stationary at placement coordinates
            entry.lock_stationary = true;
            entry.expire_ms = 0;
        }
        s_created_sub_entities[s_created_sub_entity_count++] = entry;
    }

    // Mei Ice Wall (0x0580000000000622 node=52): also spawn the other 4 pillars (k = -2, -1, +1, +2) along the wall's horizontal right vector
    if (script_id == 0x0580000000000622ULL && res_id == 0x0400000000000932ULL && stu_node->idx_in_nodes == 52) {
        Vector4 right{ fwd.Z, 0.0f, -fwd.X, 0.0f };
        uint64_t now_ms = GetTickCount64();
        const int kOffsets[4] = { -2, -1, 1, 2 };
        for (int k : kOffsets) {
            Vector4 p_pos = spawn_pos;
            p_pos.X += right.X * ((float)k * 1.45f);
            p_pos.Z += right.Z * ((float)k * 1.45f);
            Entity* pillar_ent = SpawnStandaloneSubEntity(ea, res_id, p_pos, spawn_rot);
            if (!pillar_ent) continue;
            if (owner_c10) {
                if (auto* nc10 = pillar_ent->getById<Component_10_FilterBits>(0x10)) {
                    nc10->pre_filter_bits = owner_c10->pre_filter_bits;
                    nc10->filter_bits = ~nc10->pre_filter_bits;
                    nc10->FilterBitsMakeFinal();
                }
            }
            if (auto* nc1f = pillar_ent->getById<Component_1F_Backref>(0x1F)) {
                nc1f->backref_entid = owner_ent->entity_id;
            }
            if (auto* nc23 = pillar_ent->getById<Component_23_Statescript>(0x23)) {
                nc23->loadScript(0x0580000000000636ULL);
                if (nc23->ss_inner.g3_arrWithIds.ptr && nc23->ss_inner.g3_arrWithIds.num > 0 && nc23->ss_inner.g3_arrWithIds.num <= 64) {
                    for (int i = 0; i < nc23->ss_inner.g3_arrWithIds.num; ++i) {
                        StatescriptInstance* sub_ss = nc23->ss_inner.g3_arrWithIds.ptr[i].instance;
                        if (sub_ss && !IsBadReadPtr(sub_ss, sizeof(StatescriptInstance))) {
                            sub_ss->execution_flags = (sub_ss->execution_flags | 0x400000u) & ~0x1000u;
                        }
                    }
                }
            }
            if (s_created_sub_entity_count < 64) {
                CreatedSubEntityEntry p_entry{};
                p_entry.ent_id = pillar_ent->entity_id;
                p_entry.owner_ent_id = owner_ent->entity_id;
                p_entry.res_id = res_id;
                p_entry.script_id = script_id;
                p_entry.spawn_ms = now_ms;
                p_entry.last_effect_ms = now_ms;
                p_entry.spawn_pos = p_pos;
                p_entry.ground_y = (p_pos.Y >= hero_pos.Y - 0.5f) ? p_pos.Y : hero_pos.Y;
                p_entry.lock_stationary = true;
                p_entry.expire_ms = now_ms + 4500;
                s_created_sub_entities[s_created_sub_entity_count++] = p_entry;
            }
        }
    }

    ShotDiagLogFmt("[CREATE_SUB_ENT (0x%08X)] script=0x%llX res=0x%llX -> ent=0x%X pos=(%.1f,%.1f,%.1f)",
        stu_hash, (unsigned long long)script_id, (unsigned long long)res_id, new_ent->entity_id,
        spawn_pos.X, spawn_pos.Y, spawn_pos.Z);
    return new_ent;
}

// 8. +0xF47740: StatescriptAction_ExecuteAndContinue(StatescriptAction* action_impl, uintptr_t sync_mgr, StatescriptInstance* inst, STUStatescriptBase* stu_node)
// Executes stripped server actions before +0xF47740 traverses stu_node->out_plug (+0x80):
//   - 0x4E83BC01 (Server VarBag Mutation Action — 377 nodes in 119 graphs)
//   - 0x04CC5359 (Server Apply Damage / Heal / Health Modifier Action — 83 nodes in 64 graphs)
//   - 0x1BB99C36 (Server Create Entity Action — Turrets, Sentries, Teleporter, Ice Wall, Biotic Field, RIP-Tire)
//   - 0x0E3689F3 (Server Create Dragonstrike Entity Action)
//   - 0x5C46D801 (Server Destroy Entity Action)
//   - 0xF9EF59A6 (PlayScript Action on created sub-entities)
typedef uint8_t(__fastcall* ss_action_exec_fn)(StatescriptAction* action_impl, uintptr_t sync_mgr, StatescriptInstance* inst, STUStatescriptBase* stu_node);
static ss_action_exec_fn StatescriptAction_ExecuteAndContinue_orig = nullptr;
static uint8_t __fastcall StatescriptAction_ExecuteAndContinue_hook(
    StatescriptAction* action_impl,
    uintptr_t sync_mgr,
    StatescriptInstance* inst,
    STUStatescriptBase* stu_node)
{
    if (IsHeroOrSubEntScript(inst) && stu_node && !IsBadReadPtr(stu_node, sizeof(STUStatescriptAction))) {
        STUInfo* info = (stu_node->graph_node.base.vfptr && !IsBadReadPtr(stu_node->graph_node.base.vfptr, sizeof(STUBase_vt)))
            ? stu_node->graph_node.base.vfptr->GetSTUInfo() : nullptr;
        uint32_t stu_hash = (info && !IsBadReadPtr(info, sizeof(STUInfo))) ? info->name_hash : 0;
        uint64_t script_id = SafeReadVal<uint64_t>((uintptr_t)inst + 0x150);
        Entity* owner_ent = (inst->ss_inner && !IsBadReadPtr(inst->ss_inner, sizeof(StatescriptSyncMgr)) && inst->ss_inner->component_ref)
            ? inst->ss_inner->component_ref->base.entity_backref : nullptr;

        if (stu_hash == 0x4E83BC01u) {
            // 0x4E83BC01: Server VarBag Mutation Action (sz = 0xA8)
            // +0x88: STUConfigVarDynamic* m_outVar, +0x98: STUConfigVar* m_value
            auto* out_var = (STUConfigVarDynamic*)SafeReadVal<uintptr_t>((uintptr_t)stu_node + 0x88);
            auto* m_value = (STUConfigVar*)SafeReadVal<uintptr_t>((uintptr_t)stu_node + 0x98);
            if (out_var && m_value && !IsBadReadPtr(out_var, sizeof(STUConfigVarDynamic)) && !IsBadReadPtr(m_value, sizeof(STUConfigVar))) {
                if (inst->rid_instance_varbag && !IsBadReadPtr(inst->rid_instance_varbag, sizeof(StatescriptVar_VarBag))) {
                    SafeCallSetConfigVarValueFromSTU(inst->rid_instance_varbag, out_var, m_value);
                }
                StatescriptPrimitive val_prim{};
                m_value->get_value(inst, &val_prim);
                WriteDynamicVarPrim(inst, out_var, val_prim);

                int loc_cnt = out_var->item_location.count();
                STUResourceReference* loc_arr = out_var->item_location.list();
                if (loc_cnt > 0 && loc_arr && !IsBadReadPtr(loc_arr, sizeof(STUResourceReference) * loc_cnt)) {
                    uint64_t last_var = (uint64_t)loc_arr[loc_cnt - 1].resource_id;
                    if ((last_var == 0x0D80000000000035ULL || last_var == 0x0D80000000000105ULL) && owner_ent) {
                        RefillHeroWeaponAmmo(owner_ent);
                    }
                }
            }
            if ((script_id == 0x05800000000001E9ULL || script_id == 0x058000000000017BULL ||
                 script_id == 0x05800000000001F0ULL || script_id == 0x058000000000022FULL) && owner_ent) {
                RefillHeroWeaponAmmo(owner_ent);
            }
        } else if (stu_hash == 0x1BB99C36u || stu_hash == 0x0E3689F3u) {
            SpawnStatescriptSubEntity(inst, stu_node, stu_hash);
        } else if (stu_hash == 0x5C46D801u) {
            // 0x5C46D801: Server Destroy Entity Action (+0x90: STUConfigVar* entity_cv)
            if (script_id == 0x05800000000005FEULL || script_id == 0x0580000000000623ULL) {
                // Mei Cryo-Freeze ice block destroyed -> stop HoT immediately if cancelled early
                s_active_hot_end_ms = 0;
            }
            EntityAdminBase* ea = GameEntityAdmin();
            uint64_t destroy_now_ms = GetTickCount64();
            if (script_id == 0x0580000000000622ULL && ea) {
                for (int i = s_created_sub_entity_count - 1; i >= 0; --i) {
                    if (s_created_sub_entities[i].script_id == 0x0580000000000622ULL &&
                        destroy_now_ms > s_created_sub_entities[i].spawn_ms + 250) {
                        uint32_t pid = s_created_sub_entities[i].ent_id;
                        s_created_sub_entities[i] = s_created_sub_entities[--s_created_sub_entity_count];
                        Entity* p_ent = ea->getEntById(pid);
                        if (p_ent && p_ent != GetLocalHeroModelEnt() && pid != ea->local_entid) {
                            HideAndExpireSubEntity(ea, p_ent, GetLocalHeroModelEnt());
                        }
                    }
                }
            }
            auto* ent_cv = (STUConfigVar*)SafeReadVal<uintptr_t>((uintptr_t)stu_node + 0x90);
            uint32_t del_id = 0;
            if (ent_cv && !IsBadReadPtr(ent_cv, sizeof(STUConfigVar))) {
                StatescriptPrimitive eprim{};
                ent_cv->get_value(inst, &eprim);
                if (eprim.type == StatescriptPrimitive_ENTITY) del_id = (uint32_t)eprim.value;
            }
            if (del_id == 0 && owner_ent && owner_ent != GetLocalHeroModelEnt()) {
                del_id = owner_ent->entity_id;
            }
            if (del_id != 0 && ea) {
                bool allow_destroy = true;
                if (script_id == 0x0580000000000258ULL && destroy_now_ms + 500 < s_biotic_field_end_ms) {
                    allow_destroy = false;
                }
                for (int i = 0; i < s_created_sub_entity_count; ++i) {
                    if (s_created_sub_entities[i].ent_id == del_id) {
                        if (s_created_sub_entities[i].script_id == 0x0580000000000622ULL &&
                            destroy_now_ms <= s_created_sub_entities[i].spawn_ms + 250) {
                            allow_destroy = false;
                            break;
                        }
                        if (s_created_sub_entities[i].script_id == 0x0580000000000258ULL &&
                            s_created_sub_entities[i].expire_ms > 0 &&
                            destroy_now_ms + 500 < s_created_sub_entities[i].expire_ms) {
                            allow_destroy = false;
                            break;
                        }
                        if (allow_destroy) {
                            s_created_sub_entities[i] = s_created_sub_entities[--s_created_sub_entity_count];
                        }
                        break;
                    }
                }
                if (allow_destroy) {
                    Entity* del_ent = ea->getEntById(del_id);
                    if (del_ent && del_ent != GetLocalHeroModelEnt() && del_id != ea->local_entid && !del_ent->hasComponent(0x28)) {
                        // Hide Component_1 and any attached child visuals instead of calling ea->delEnt(del_ent) mid-tick so SceneRendering (+0x9F29A5) never faults
                        HideAndExpireSubEntity(ea, del_ent, GetLocalHeroModelEnt());
                    }
                }
            }
        } else if (stu_hash == 0xF9EF59A6u) {
            // 0xF9EF59A6: STUStatescriptActionPlayScript (+0xA0: target_ent_cv, +0xB0: script_cv)
            EntityAdminBase* ea = GameEntityAdmin();
            auto* ent_cv = (STUConfigVar*)SafeReadVal<uintptr_t>((uintptr_t)stu_node + 0xA0);
            auto* scr_cv = (STUConfigVar*)SafeReadVal<uintptr_t>((uintptr_t)stu_node + 0xB0);
            Entity* target_ent = owner_ent;
            if (ent_cv && !IsBadReadPtr(ent_cv, sizeof(STUConfigVar)) && ea) {
                StatescriptPrimitive eprim{};
                ent_cv->get_value(inst, &eprim);
                if (eprim.type == StatescriptPrimitive_ENTITY && (uint32_t)eprim.value != 0) {
                    if (Entity* e = ea->getEntById((uint32_t)eprim.value)) target_ent = e;
                }
            }
            uint64_t target_script = 0;
            if (scr_cv && !IsBadReadPtr(scr_cv, sizeof(STUConfigVar))) {
                StatescriptPrimitive sprim{};
                scr_cv->get_value(inst, &sprim);
                target_script = (uint64_t)sprim.value;
            }
            if (target_ent && (target_script & 0xFFFF000000000000ULL) == 0x0580000000000000ULL &&
                target_script != 0x0580000000000636ULL) {
                if (auto* tc23 = target_ent->getById<Component_23_Statescript>(0x23)) {
                    tc23->loadScript(target_script);
                }
            }
        } else if (stu_hash == 0x04CC5359u) {
            // 0x04CC5359: Server Apply Damage / Heal / Health Modifier Action (sz = 0xB8)
            uintptr_t dmg_obj = SafeReadVal<uintptr_t>((uintptr_t)stu_node + 0xA8);
            auto* target_cv = (STUConfigVar*)SafeReadVal<uintptr_t>((uintptr_t)stu_node + 0xB0);

            float amt = 0.0f;
            if (dmg_obj && !IsBadReadPtr((const void*)dmg_obj, 0x20)) {
                auto* amt_cv = (STUConfigVar*)SafeReadVal<uintptr_t>(dmg_obj + 0x18);
                if (amt_cv && !IsBadReadPtr(amt_cv, sizeof(STUConfigVar))) {
                    StatescriptPrimitive aprim{};
                    amt_cv->get_value(inst, &aprim);
                    amt = aprim.get_convert_float(0.0f);
                }
            }

            EntityAdminBase* ea = GameEntityAdmin();
            Entity* target_ent = nullptr;
            if (target_cv && !IsBadReadPtr(target_cv, sizeof(STUConfigVar)) && ea) {
                StatescriptPrimitive tprim{};
                target_cv->get_value(inst, &tprim);
                if (tprim.type == StatescriptPrimitive_ENTITY && (uint32_t)tprim.value != 0) {
                    target_ent = ea->getEntById((uint32_t)tprim.value);
                }
            }
            if (!target_ent) {
                if (s_last_melee_hit_entid != 0 && (GetTickCount64() - s_last_melee_hit_ms) < 250 && ea) {
                    target_ent = ea->getEntById(s_last_melee_hit_entid);
                }
                if (!target_ent) target_ent = owner_ent;
            }

            if (target_ent && target_ent->hasComponent(0x28)) {
                auto* c28 = target_ent->getById<Component_28_STUHealthComponent>(0x28);
                auto* owner_c10 = owner_ent ? owner_ent->getById<Component_10_FilterBits>(0x10) : nullptr;
                auto* target_c10 = target_ent->getById<Component_10_FilterBits>(0x10);
                bool same_team = (target_ent == owner_ent) ||
                    (owner_c10 && target_c10 && (owner_c10->filter_bits & 0x3C0000ULL) == (target_c10->filter_bits & 0x3C0000ULL));

                if (same_team) {
                    // Self/Ally Heal: route Roadhog (0x240), Mei (0x5FE/0x623), and Bastion (0x153/0x02B) to smooth 100ms HoT!
                    if (script_id == 0x0580000000000240ULL) {
                        ArmHeroHealOverTime(script_id, 300.0f, 1000);
                    } else if (script_id == 0x05800000000005FEULL || script_id == 0x0580000000000623ULL) {
                        ArmHeroHealOverTime(script_id, 150.0f, 4000);
                    } else if (script_id == 0x0580000000000153ULL || script_id == 0x058000000000002BULL) {
                        s_bastion_repair_active = true;
                    } else {
                        if (amt <= 0.0f) amt = 40.0f;
                        ApplyHeroSelfHeal(target_ent, amt, script_id, "ACTION_HEAL (0x04CC5359)");
                    }
                } else if ((GetTickCount64() - s_last_melee_dmg_ms) >= 50) {
                    // Enemy Damage
                    if (amt <= 0.0f) amt = 30.0f;
                    if (c28->CalcTotalCurrentHealth() > 0.0f) {
                        float new_hp = ApplyDamageToHealthComponent(c28, amt);
                        if (new_hp <= 0.0f) {
                            if (auto* c1 = target_ent->getById<Component_1_SceneRendering>(1)) {
                                *(uint32_t*)((uintptr_t)c1 + 0x1B8) &= ~0x10000u;
                            }
                        }
                        SyncEntityHealthVarBag(target_ent);
                        {
                            std::lock_guard<std::mutex> hit_lock(s_pellet_hit_mutex);
                            if (s_pending_dmg_pkt_count < 32) {
                                int32_t dmg_int = (int32_t)(amt + 0.5f);
                                if (dmg_int < 1) dmg_int = 1;
                                uint8_t pkt_flags = (uint8_t)(new_hp <= 0.0f ? 2 : 0);
                                s_pending_dmg_pkts[s_pending_dmg_pkt_count++] = PendingDamagePacket{ target_ent->entity_id, dmg_int, pkt_flags, true };
                            }
                        }
                        ShotDiagLogFmt("[ACTION_DMG (0x04CC5359)] script=0x%llX target=0x%X -%.1f -> HP %.1f",
                            (unsigned long long)script_id, target_ent->entity_id, amt, new_hp);
                    }
                }
            }
        }
    }

    return StatescriptAction_ExecuteAndContinue_orig(action_impl, sync_mgr, inst, stu_node);
}

// 9. +0xF495C0: StatescriptState_StartState(StatescriptState* state, uint64_t a2)
// - Transiently lends client authority ((exec_flags | 0x400008u) & ~0x1000u) across StartState for local hero scripts
//   (fixes 0x860EE5FC WeaponVolley at +0x1077C47 and all local ability states).
// - Implements stripped server states:
//   * 0x619D9E95 (Quick Melee / AoE Hit Sweep State — 81 nodes in 52 graphs)
//   * 0xD78490D1 (Impulse / Knockback State — 34 nodes in 25 graphs)
//   * 0x4CCED352 (STUStatescriptStateCreateEntity — Reinhardt/Winston/Zarya Barriers & Deployables)
//   * 0x0E3689F3 (Hanzo Dragonstrike Entity Spawn State)
//   * 0x3A846155 & 0x74CB0002 (Junkrat RIP-Tire 0x0580000000000232 Possession & Detonation)
typedef uint32_t(__fastcall* ss_state_start_fn)(StatescriptState* state, uint64_t a2);
static ss_state_start_fn StatescriptState_StartState_orig = nullptr;
static uint32_t __fastcall StatescriptState_StartState_hook(StatescriptState* state, uint64_t a2) {
    StatescriptInstance* inst = (state && !IsBadReadPtr(state, 0x80))
        ? (StatescriptInstance*)SafeReadVal<uintptr_t>((uintptr_t)state + 0x78) : nullptr;
    if (!IsHeroOrSubEntScript(inst)) {
        return StatescriptState_StartState_orig(state, a2);
    }
    Entity* owner_ent = (inst->ss_inner && !IsBadReadPtr(inst->ss_inner, sizeof(StatescriptSyncMgr)) && inst->ss_inner->component_ref)
        ? inst->ss_inner->component_ref->base.entity_backref : nullptr;

    uint32_t saved_exec_flags = inst->execution_flags;
    inst->execution_flags = (inst->execution_flags | 0x400008u) & ~0x1000u;

    uint32_t ret = StatescriptState_StartState_orig(state, a2);
    inst->execution_flags = saved_exec_flags;

    if (inst && inst->graph && !IsBadReadPtr(inst->graph, sizeof(STUStatescriptGraph))) {
        STUStatescriptState* stu_state = inst->graph->STUFromState(state);
        if (stu_state && !IsBadReadPtr(stu_state, sizeof(STUStatescriptBase))) {
            auto* stu_base = (STUStatescriptBase*)stu_state;
            STUInfo* info = (stu_base->graph_node.base.vfptr &&
                !IsBadReadPtr(stu_base->graph_node.base.vfptr, sizeof(STUBase_vt)))
                ? stu_base->graph_node.base.vfptr->GetSTUInfo() : nullptr;
            uint32_t stu_hash = (info && !IsBadReadPtr(info, sizeof(STUInfo))) ? info->name_hash : 0;
            uint64_t script_id = SafeReadVal<uint64_t>((uintptr_t)inst + 0x150);
            bool is_local = (owner_ent && owner_ent == GetLocalHeroModelEnt());

            // 1. Arm moving dash/leap damage sweep & McCree Roll ammo refill on ability state entry
            if (is_local && owner_ent) {
                if (script_id == 0x05800000000001DBULL || script_id == 0x05800000000001DCULL ||
                    script_id == 0x0580000000000045ULL || script_id == 0x0580000000000157ULL || script_id == 0x0580000000000317ULL) {
                    // Genji Swift Strike (0x05800000000001DB): 50 damage sweep along dash trajectory
                    ArmDashSweepDamage(script_id, 50.0f, 4.0f, 600);
                } else if (script_id == 0x0580000000000109ULL || script_id == 0x058000000000010AULL ||
                           script_id == 0x0580000000000038ULL || script_id == 0x05800000000000FEULL) {
                    // Winston Jump Pack (0x0580000000000109): 50 AoE leap/land damage within 5.5m
                    ArmDashSweepDamage(script_id, 50.0f, 5.5f, 950);
                } else if (script_id == 0x058000000000047EULL || script_id == 0x058000000000026AULL || script_id == 0x0580000000000B2FULL) {
                    // D.Va Mech Boosters (0x058000000000047E / 0x058000000000026A / 0x0580000000000B2F): 25 impact damage + knockback during flight
                    if (s_active_dash_sweep_script_id != script_id || GetTickCount64() >= s_active_dash_sweep_end_ms) {
                        ArmDashSweepDamage(script_id, 25.0f, 3.8f, 2200);
                    }
                } else if (script_id == 0x058000000000007DULL || script_id == 0x05800000000000BEULL) {
                    // Reinhardt Charge (0x058000000000007D / 0x05800000000000BE): 75 impact damage + knockback
                    ArmDashSweepDamage(script_id, 75.0f, 3.5f, 1500);
                } else if (script_id == 0x05800000000001E9ULL || script_id == 0x05800000000001F0ULL ||
                           script_id == 0x058000000000017BULL || script_id == 0x058000000000022FULL) {
                    // McCree Combat Roll (0x05800000000001E9 / 0x05800000000001F0): immediate 6/6 Peacekeeper ammo refill
                    RefillHeroWeaponAmmo(owner_ent);
                }
            }

            if (stu_hash == 0x619D9E95u && owner_ent) {
                // 0x619D9E95: Quick Melee / AoE Hit Sweep / Self-Heal Trigger State (sz = 0x150)
                // +0x118: range_cv (0x9B0385B4), +0x138: on_hit_plug (0x41BA609A), +0x140: dmg_obj (0x59EF45AA), +0x148: out_hit_ent_cvd (0x2D2057F1)
                auto* on_hit_plug = (STUStatescriptOutputPlug*)SafeReadVal<uintptr_t>((uintptr_t)stu_state + 0x138);
                auto* out_hit_cvd = (STUConfigVarDynamic*)SafeReadVal<uintptr_t>((uintptr_t)stu_state + 0x148);

                // Check if this 0x619D9E95 node is a Self-Heal ability state:
                //   - Roadhog Take a Breather: 0x0580000000000240 (node[9] -> node[10] 0x04CC5359 +300 HP over 1.0s)
                //   - Mei Cryo-Freeze:         0x05800000000005FE / 0x0580000000000623 (+150 HP over 4.0s)
                //   - Bastion Self-Repair:     0x0580000000000153 / 0x058000000000002B (75 HP/s while held)
                bool is_self_heal_script = (script_id == 0x0580000000000240ULL ||
                                            script_id == 0x05800000000005FEULL || script_id == 0x0580000000000623ULL ||
                                            script_id == 0x0580000000000153ULL || script_id == 0x058000000000002BULL);
                if (is_self_heal_script) {
                    if (script_id == 0x0580000000000240ULL) {
                        ArmHeroHealOverTime(script_id, 300.0f, 1000, "ROADHOG_BREATHER_HOT");
                    } else if (script_id == 0x05800000000005FEULL || script_id == 0x0580000000000623ULL) {
                        ArmHeroHealOverTime(0x05800000000005FEULL, 150.0f, 4000, "MEI_CRYOFREEZE_HOT");
                    } else if (script_id == 0x0580000000000153ULL || script_id == 0x058000000000002BULL) {
                        s_bastion_repair_active = true;
                    }
                    if (out_hit_cvd && !IsBadReadPtr(out_hit_cvd, sizeof(STUConfigVarDynamic))) {
                        StatescriptPrimitive eprim(StatescriptPrimitive_ENTITY, (__int64)owner_ent->entity_id);
                        WriteDynamicVarPrim(inst, out_hit_cvd, eprim);
                    }
                    SafeTraverseOutputPlug(inst->ss_inner, inst, (STUStatescriptBase*)stu_state, on_hit_plug, 0x138);
                } else {
                    float range = 0.0f;
                    auto* range_cv = (STUConfigVar*)SafeReadVal<uintptr_t>((uintptr_t)stu_state + 0x118);
                    if (range_cv && !IsBadReadPtr(range_cv, sizeof(STUConfigVar))) {
                        StatescriptPrimitive rprim{};
                        range_cv->get_value(inst, &rprim);
                        range = rprim.get_convert_float(0.0f);
                    }
                    if (range <= 0.2f || range > 25.0f) {
                        if (script_id == 0x0580000000000776ULL) range = 8.0f; // Lucio Soundwave
                        else if (script_id == 0x0580000000000052ULL || script_id == 0x0580000000000044ULL ||
                                 script_id == 0x0580000000000109ULL || script_id == 0x058000000000003BULL) range = 5.0f;
                        else range = 3.5f;
                    }

                    float dmg = 0.0f;
                    uintptr_t dmg_obj = SafeReadVal<uintptr_t>((uintptr_t)stu_state + 0x140);
                    if (dmg_obj && !IsBadReadPtr((const void*)dmg_obj, 0x20)) {
                        auto* amt_cv = (STUConfigVar*)SafeReadVal<uintptr_t>(dmg_obj + 0x18);
                        if (amt_cv && !IsBadReadPtr(amt_cv, sizeof(STUConfigVar))) {
                            StatescriptPrimitive dprim{};
                            amt_cv->get_value(inst, &dprim);
                            dmg = dprim.get_convert_float(0.0f);
                        }
                    }
                    if (dmg <= 0.0f) {
                        if (script_id == 0x0580000000000052ULL || script_id == 0x0580000000000041ULL ||
                            script_id == 0x058000000000007DULL || script_id == 0x05800000000000BEULL) dmg = 75.0f; // Reinhardt / Torb Hammer / Charge
                        else if (script_id == 0x0580000000000044ULL) dmg = 120.0f; // Genji Dragonblade
                        else if (script_id == 0x05800000000001DBULL || script_id == 0x05800000000001DCULL ||
                                 script_id == 0x0580000000000045ULL || script_id == 0x0580000000000157ULL ||
                                 script_id == 0x0580000000000317ULL || script_id == 0x0580000000000038ULL ||
                                 script_id == 0x05800000000000FEULL || script_id == 0x0580000000000109ULL ||
                                 script_id == 0x058000000000010AULL) dmg = 50.0f; // Swift Strike / Jump Pack
                        else if (script_id == 0x058000000000003BULL) dmg = 40.0f; // Winston Primal Rage
                        else if (script_id == 0x058000000000017AULL || script_id == 0x058000000000047EULL ||
                                 script_id == 0x058000000000026AULL || script_id == 0x0580000000000B2FULL ||
                                 script_id == 0x0580000000000776ULL) dmg = 25.0f; // Flashbang / D.Va Boosters / Lucio Soundwave
                        else dmg = 30.0f; // Quick Melee (0x0580000000000043)
                    }

                    auto* owner_c1 = owner_ent->getById<Component_1_SceneRendering>(1);
                    auto* owner_c10 = owner_ent->getById<Component_10_FilterBits>(0x10);
                    auto* owner_c15 = owner_ent->getById<Component_15_STUCharacterMoverComponent>(0x15);
                    Vector4 my_pos = owner_c1 ? owner_c1->position : Vector4{};
                    Vector4 fwd{ 0.0f, 0.0f, 1.0f, 0.0f };
                    if (owner_c15 && !IsBadReadPtr(owner_c15, sizeof(Component_15_STUCharacterMoverComponent))) {
                        const Vector4& q = owner_c15->movement_state_1.rotation_quaternion;
                        fwd.X = 2.0f * (q.X * q.Z + q.W * q.Y);
                        fwd.Y = 0.0f;
                        fwd.Z = 1.0f - 2.0f * (q.X * q.X + q.Y * q.Y);
                        float flen = sqrtf(fwd.X * fwd.X + fwd.Z * fwd.Z);
                        if (flen > 1e-4f) { fwd.X /= flen; fwd.Z /= flen; }
                    }

                    EntityAdminBase* ea = GameEntityAdmin();
                    if (ea && owner_c1) {
                        for (Entity* ent : *ea) {
                            if (!ent || ent == owner_ent || !ent->hasComponent(0x15) || !ent->hasComponent(0x28)) continue;
                            auto* c10 = ent->getById<Component_10_FilterBits>(0x10);
                            if (owner_c10 && c10 && (owner_c10->filter_bits & 0x3C0000ULL) == (c10->filter_bits & 0x3C0000ULL)) continue;
                            auto* c1 = ent->getById<Component_1_SceneRendering>(1);
                            auto* c28 = ent->getById<Component_28_STUHealthComponent>(0x28);
                            if (!c1 || !c28 || c28->CalcTotalCurrentHealth() <= 0.0f) continue;
                            if ((*(uint32_t*)((uintptr_t)c1 + 0x1B8) & 0x10000u) == 0) continue;

                            Vector4 d = Sub3(c1->position, my_pos);
                            float dist = sqrtf(Dot3(d, d));
                            if (dist > range) continue;
                            float horiz_len = sqrtf(d.X * d.X + d.Z * d.Z);
                            float cos_ang = (horiz_len > 0.1f) ? ((d.X * fwd.X + d.Z * fwd.Z) / horiz_len) : 1.0f;
                            if (dist > 2.2f && cos_ang < -0.15f) continue; // must be in front arc unless point-blank

                            if (s_active_dash_sweep_end_ms > GetTickCount64() && s_active_dash_sweep_script_id == script_id) {
                                bool already_swept = false;
                                for (int h = 0; h < s_active_dash_sweep_hit_count; ++h) {
                                    if (s_active_dash_sweep_hit_ents[h] == ent->entity_id) { already_swept = true; break; }
                                }
                                if (already_swept) continue;
                                if (s_active_dash_sweep_hit_count < 16) {
                                    s_active_dash_sweep_hit_ents[s_active_dash_sweep_hit_count++] = ent->entity_id;
                                }
                            }

                            s_last_melee_hit_entid = ent->entity_id;
                            s_last_melee_hit_ms = GetTickCount64();
                            s_last_melee_dmg_ms = GetTickCount64();

                            if (out_hit_cvd && !IsBadReadPtr(out_hit_cvd, sizeof(STUConfigVarDynamic))) {
                                StatescriptPrimitive eprim(StatescriptPrimitive_ENTITY, (__int64)ent->entity_id);
                                WriteDynamicVarPrim(inst, out_hit_cvd, eprim);
                            }

                            float new_hp = ApplyDamageToHealthComponent(c28, dmg);
                            if (new_hp <= 0.0f) {
                                *(uint32_t*)((uintptr_t)c1 + 0x1B8) &= ~0x10000u;
                            }
                            *(uint32_t*)((uintptr_t)c28 + 0x14) = 1;
                            s_last_hit_dummy_entid = ent->entity_id;
                            s_last_hit_dummy_ms = GetTickCount64();
                            SyncEntityHealthVarBag(ent);
                            {
                                std::lock_guard<std::mutex> hit_lock(s_pellet_hit_mutex);
                                if (s_pending_dmg_pkt_count < 32) {
                                    int32_t dmg_int = (int32_t)(dmg + 0.5f);
                                    if (dmg_int < 1) dmg_int = 1;
                                    uint8_t pkt_flags = (uint8_t)(new_hp <= 0.0f ? 2 : 0);
                                    s_pending_dmg_pkts[s_pending_dmg_pkt_count++] = PendingDamagePacket{ ent->entity_id, dmg_int, pkt_flags, true };
                                }
                            }
                            Vector4 push_dir{ d.X + fwd.X * 1.5f, 0.0f, d.Z + fwd.Z * 1.5f, 0.0f };
                            if (script_id == 0x0580000000000776ULL) {
                                ArmEnemyKnockback(ent, push_dir, 15.0f, 5.0f, 500); // Lucio Soundwave
                            } else if (script_id == 0x058000000000003BULL) {
                                ArmEnemyKnockback(ent, push_dir, 14.0f, 5.5f, 500); // Winston Primal Rage
                            } else if (script_id == 0x058000000000047EULL || script_id == 0x058000000000026AULL || script_id == 0x0580000000000B2FULL) {
                                ArmEnemyKnockback(ent, push_dir, 12.0f, 4.5f, 450); // D.Va Boosters
                            } else if (script_id == 0x0580000000000052ULL || script_id == 0x0580000000000041ULL) {
                                ArmEnemyKnockback(ent, push_dir, 5.5f, 2.0f, 250); // Reinhardt / Torb Hammer
                            }
                            ShotDiagLogFmt("[MELEE_SWEEP (0x619D9E95)] script=0x%llX hit=0x%X dist=%.2fm -%.1f -> HP %.1f",
                                (unsigned long long)script_id, ent->entity_id, dist, dmg, new_hp);

                            SafeTraverseOutputPlug(inst->ss_inner, inst, (STUStatescriptBase*)stu_state, on_hit_plug, 0x138);
                        }
                    }
                }
                auto sched_finish = (schedule_finish_state_fn)(globals::gameBase + 0xF4A9C0);
                sched_finish((uintptr_t)state, 1, 120);
            } else if (stu_hash == 0xD78490D1u && is_local && owner_ent) {
                // 0xD78490D1: Impulse / Knockback State (sz = 0x128)
                auto* comp15 = owner_ent->getById<Component_15_STUCharacterMoverComponent>(0x15);
                if (comp15 && !IsBadReadPtr(comp15, sizeof(Component_15_STUCharacterMoverComponent))) {
                    const Vector4& q = comp15->movement_state_1.rotation_quaternion;
                    float pitch = -comp15->movement_state_1.y_lookdir; // y_lookdir: -1.5 is UP, +1.5 is DOWN
                    Vector4 fwd{
                        2.0f * (q.X * q.Z + q.W * q.Y),
                        sinf(pitch),
                        1.0f - 2.0f * (q.X * q.X + q.Y * q.Y),
                        0.0f
                    };
                    float flen = sqrtf(fwd.X * fwd.X + fwd.Z * fwd.Z);
                    if (flen > 1e-4f) { fwd.X /= flen; fwd.Z /= flen; }

                    float fwd_speed = 16.0f;
                    float up_speed  = 0.0f;
                    uint64_t dur_ms = 350;
                    float dash_dmg  = 0.0f;
                    float dash_rad  = 3.5f;

                    if (script_id == 0x0580000000000024ULL || script_id == 0x0580000000000025ULL || script_id == 0x0580000000000074ULL) {
                        // Pharah Jump Jet
                        fwd_speed = 2.0f; up_speed = 22.0f; dur_ms = 450;
                    } else if (script_id == 0x0580000000000038ULL || script_id == 0x05800000000000FEULL ||
                               script_id == 0x0580000000000109ULL || script_id == 0x058000000000010AULL) {
                        // Winston Jump Pack (50 AoE leap/land damage within 5.5m)
                        fwd_speed = 20.0f; up_speed = 12.0f; dur_ms = 500; dash_dmg = 50.0f; dash_rad = 5.5f;
                    } else if (script_id == 0x05800000000001DBULL || script_id == 0x05800000000001DCULL ||
                               script_id == 0x0580000000000045ULL || script_id == 0x0580000000000157ULL || script_id == 0x0580000000000317ULL) {
                        // Genji Swift Strike (50 dash sweep damage)
                        fwd_speed = 35.0f; up_speed = 35.0f * fwd.Y; dur_ms = 400; dash_dmg = 50.0f; dash_rad = 4.0f;
                    } else if (script_id == 0x05800000000001E9ULL || script_id == 0x058000000000017BULL ||
                               script_id == 0x05800000000001F0ULL || script_id == 0x058000000000022FULL) {
                        // McCree Combat Roll (+ immediate 6/6 Peacekeeper ammo refill)
                        fwd_speed = 15.0f; up_speed = 0.0f; dur_ms = 400;
                        RefillHeroWeaponAmmo(owner_ent);
                    } else if (script_id == 0x058000000000007DULL || script_id == 0x05800000000000BEULL) {
                        // Reinhardt Charge (75 dash impact damage)
                        fwd_speed = 16.5f; up_speed = 0.0f; dur_ms = 1200; dash_dmg = 75.0f; dash_rad = 3.5f;
                    } else if (script_id == 0x058000000000026AULL || script_id == 0x0580000000000B2FULL) {
                        // D.Va Boosters (25 impact damage)
                        fwd_speed = 12.5f; up_speed = 12.5f * fwd.Y; dur_ms = 1500; dash_dmg = 25.0f; dash_rad = 3.8f;
                    } else if (script_id == 0x0580000000000020ULL || script_id == 0x0580000000000067ULL) {
                        // Widowmaker Grappling Hook
                        fwd_speed = 20.0f; up_speed = (fwd.Y > 0.1f ? 20.0f * fwd.Y : 6.0f); dur_ms = 550;
                    }

                    s_active_impulse_vel = Vector4{ fwd.X * fwd_speed, up_speed, fwd.Z * fwd_speed, 0.0f };
                    s_active_impulse_min_y = comp15->movement_state_1.absolute_position.Y;
                    s_active_impulse_end_ms = GetTickCount64() + dur_ms;
                    if (dash_dmg > 0.0f) {
                        ArmDashSweepDamage(script_id, dash_dmg, dash_rad, dur_ms + 150);
                    }

                    ShotDiagLogFmt("[IMPULSE_STATE (0xD78490D1)] script=0x%llX vel=(%.1f,%.1f,%.1f) dur=%llums dmg=%.1f",
                        (unsigned long long)script_id, s_active_impulse_vel.X, s_active_impulse_vel.Y, s_active_impulse_vel.Z,
                        (unsigned long long)dur_ms, dash_dmg);
                }
                auto sched_finish = (schedule_finish_state_fn)(globals::gameBase + 0xF4A9C0);
                sched_finish((uintptr_t)state, 1, 200);
            } else if (stu_hash == 0x4CCED352u || stu_hash == 0x0E3689F3u) {
                // 0x4CCED352 (Barrier / Deployable CreateEntity State) & 0x0E3689F3 (Hanzo Dragonstrike State)
                SpawnStatescriptSubEntity(inst, (STUStatescriptBase*)stu_state, stu_hash);
                if (script_id == 0x05800000000005FEULL && owner_ent) {
                    // Mei Cryo-Freeze (0x05800000000005FE node[0]): heal +150 HP smoothly over 4.0s
                    ArmHeroHealOverTime(0x05800000000005FEULL, 150.0f, 4000, "MEI_CRYOFREEZE_HOT");
                }
                if (stu_hash == 0x0E3689F3u) {
                    auto sched_finish = (schedule_finish_state_fn)(globals::gameBase + 0xF4A9C0);
                    sched_finish((uintptr_t)state, 1, 250);
                }
            } else if (stu_hash == 0x860EE5FCu && script_id == 0x0580000000000257ULL && owner_ent) {
                // Soldier: 76 Biotic Field (0x0580000000000257 node[6] WeaponVolley):
                // Spawn physical Biotic Field Emitter (0x04000000000003DC -> 0x0580000000000258 -> 0x040000000000068A)
                // on the ground AND arm the 5s 40 HP/s radius HoT.
                Entity* emitter_ent = SpawnStatescriptSubEntity(inst, (STUStatescriptBase*)stu_state, stu_hash);
                auto* oc1  = owner_ent->getById<Component_1_SceneRendering>(1);
                auto* oc15 = owner_ent->getById<Component_15_STUCharacterMoverComponent>(0x15);
                Vector4 pos = (oc15 && !IsBadReadPtr(oc15, sizeof(Component_15_STUCharacterMoverComponent)))
                    ? oc15->movement_state_1.absolute_position : (oc1 ? oc1->position : Vector4{});
                if (emitter_ent) {
                    if (auto* ec1 = emitter_ent->getById<Component_1_SceneRendering>(1)) {
                        pos = ec1->position;
                    }
                }
                s_biotic_field_pos = pos;
                s_biotic_field_end_ms = GetTickCount64() + 5000;
            } else if (stu_hash == 0x3A846155u && script_id == 0x0580000000000232ULL && owner_ent) {
                // Junkrat RIP-Tire (0x0580000000000232) Possession Start (0x3A846155)
                EntityAdminBase* ea = GameEntityAdmin();
                if (ea) {
                    auto* oc1 = owner_ent->getById<Component_1_SceneRendering>(1);
                    Vector4 pos = oc1 ? oc1->position : Vector4{};
                    Vector4 rot = oc1 ? oc1->rotation : Vector4{ 0.0f, 0.0f, 0.0f, 1.0f };
                    Entity* tire_ent = SpawnStandaloneSubEntity(ea, 0x040000000000039CULL, pos, rot);
                    if (!tire_ent) tire_ent = SpawnStandaloneSubEntity(ea, 0x040000000000083CULL, pos, rot);
                    if (tire_ent) {
                        s_riptire_ent_id = tire_ent->entity_id;
                        s_riptire_saved_mdl_id = owner_ent->entity_id;
                        s_riptire_end_ms = GetTickCount64() + 10000;
                        if (Entity* local_ctrl = ea->getLocalEnt()) {
                            if (auto* c20 = local_ctrl->getById<Component_20_ModelReference>(0x20)) {
                                c20->cam_attach_entid = tire_ent->entity_id;
                                c20->aim_entid = tire_ent->entity_id;
                                c20->movement_attach_entid = tire_ent->entity_id;
                            }
                        }
                        ShotDiagLogFmt("[RIPTIRE_START (0x3A846155)] junkrat=0x%X -> tire=0x%X", owner_ent->entity_id, tire_ent->entity_id);
                    }
                }
            } else if (stu_hash == 0x74CB0002u && script_id == 0x0580000000000232ULL) {
                // Junkrat RIP-Tire Detonation / End State (0x74CB0002)
                if (s_riptire_ent_id != 0) {
                    s_riptire_end_ms = GetTickCount64();
                }
            }
        }
    }

    return ret;
}

// Universal State Entry & Exit Callbacks invoked from +0xD146A0 (OnNodeStateEntry) and +0xD14850 (OnNodeStateFinished).
// This catches ALL state entries/exits regardless of whether the state uses the base vtable (+0xF495C0) or a subclass vtable.
static void OnUniversalStateEntry(StatescriptSyncMgr* ss, StatescriptInstance* inst, StatescriptState* state) {
    if (!IsHeroOrSubEntScript(inst) || !state || IsBadReadPtr(state, sizeof(StatescriptState))) return;
    if (!inst->graph || IsBadReadPtr(inst->graph, sizeof(STUStatescriptGraph))) return;

    Entity* owner_ent = (ss && !IsBadReadPtr(ss, sizeof(StatescriptSyncMgr)) && ss->component_ref)
        ? ss->component_ref->base.entity_backref : nullptr;
    Entity* hero_ent = GetLocalHeroModelEnt();
    bool is_local = (owner_ent && owner_ent == hero_ent);

    STUStatescriptState* stu_state = inst->graph->STUFromState(state);
    if (!stu_state || IsBadReadPtr(stu_state, sizeof(STUStatescriptBase))) return;
    auto* stu_base = (STUStatescriptBase*)stu_state;
    STUInfo* info = (stu_base->graph_node.base.vfptr && !IsBadReadPtr(stu_base->graph_node.base.vfptr, sizeof(STUBase_vt)))
        ? stu_base->graph_node.base.vfptr->GetSTUInfo() : nullptr;
    uint32_t stu_hash = (info && !IsBadReadPtr(info, sizeof(STUInfo))) ? info->name_hash : 0;
    uint64_t script_id = SafeReadVal<uint64_t>((uintptr_t)inst + 0x150);
    int node_idx = stu_base->idx_in_nodes;

    // 1. Sub-entity scripts: Soldier: 76 Biotic Emitter (0x0580000000000258)
    if (script_id == 0x0580000000000258ULL && owner_ent) {
        auto* oc1 = owner_ent->getById<Component_1_SceneRendering>(1);
        if (oc1 && (fabsf(oc1->position.X) > 0.01f || fabsf(oc1->position.Z) > 0.01f)) {
            s_biotic_field_pos = oc1->position;
        } else if (hero_ent) {
            if (auto* hc1 = hero_ent->getById<Component_1_SceneRendering>(1)) {
                s_biotic_field_pos = hc1->position;
            }
        }
        uint64_t now_ms = GetTickCount64();
        bool already_tracked = false;
        for (int i = 0; i < s_created_sub_entity_count; ++i) {
            if (s_created_sub_entities[i].ent_id == owner_ent->entity_id) {
                already_tracked = true;
                break;
            }
        }
        if (!already_tracked && s_created_sub_entity_count < 64 && owner_ent != hero_ent) {
            CreatedSubEntityEntry entry{};
            entry.ent_id = owner_ent->entity_id;
            entry.owner_ent_id = hero_ent ? hero_ent->entity_id : 0;
            entry.res_id = 0x04000000000003DCULL;
            entry.script_id = script_id;
            entry.spawn_ms = now_ms;
            entry.last_effect_ms = now_ms;
            entry.spawn_pos = s_biotic_field_pos;
            entry.ground_y = s_biotic_field_pos.Y;
            entry.lock_stationary = false;
            entry.expire_ms = now_ms + 5000;
            s_created_sub_entities[s_created_sub_entity_count++] = entry;
        }
        if (s_biotic_field_ent_id != owner_ent->entity_id) {
            if (s_biotic_field_ent_id != 0) {
                if (EntityAdminBase* ea = GameEntityAdmin()) {
                    if (Entity* old_bf = ea->getEntById(s_biotic_field_ent_id)) {
                        if (old_bf != hero_ent) {
                            HideAndExpireSubEntity(ea, old_bf, hero_ent);
                        }
                    }
                }
            }
            s_biotic_field_ent_id = owner_ent->entity_id;
            s_biotic_field_end_ms = now_ms + 5000;
            ShotDiagLogFmt("[BIOTIC_FIELD_ACTIVE] emitter_ent=0x%X pos=(%.1f,%.1f,%.1f) dur=5000ms (40 HP/s)",
                owner_ent->entity_id, s_biotic_field_pos.X, s_biotic_field_pos.Y, s_biotic_field_pos.Z);
        }
        return;
    }

    if (!is_local || !owner_ent) return;

    // 2. McCree Combat Roll (0x05800000000001E9 / 0x05800000000001F0): immediate 6/6 Peacekeeper ammo refill
    if (script_id == 0x05800000000001E9ULL || script_id == 0x05800000000001F0ULL) {
        if (node_idx == 22 || stu_hash == 0x2A0FB7E6u || stu_hash == 0xD78490D1u) {
            RefillHeroWeaponAmmo(owner_ent);
        }
    }

    // 3. Dash / Leap Sweep Damage (catches 0x2A0FB7E6 / 0xC17B255D / 0x970EC198 / 0x498B421A / 0x7FB4BF64 subclass movement states that bypass +0xF495C0)
    if (stu_hash == 0x2A0FB7E6u || stu_hash == 0xC17B255Du || stu_hash == 0xD78490D1u ||
        stu_hash == 0x970EC198u || stu_hash == 0x498B421Au || stu_hash == 0x7FB4BF64u) {
        if (script_id == 0x05800000000001DBULL || script_id == 0x05800000000001DCULL ||
            script_id == 0x0580000000000045ULL || script_id == 0x0580000000000157ULL || script_id == 0x0580000000000317ULL) {
            ArmDashSweepDamage(script_id, 50.0f, 4.0f, 650);
        } else if (script_id == 0x0580000000000109ULL || script_id == 0x058000000000010AULL ||
                   script_id == 0x0580000000000038ULL || script_id == 0x05800000000000FEULL) {
            ArmDashSweepDamage(script_id, 50.0f, 5.5f, 950);
        } else if (script_id == 0x058000000000047EULL || script_id == 0x058000000000026AULL || script_id == 0x0580000000000B2FULL) {
            if (node_idx == 15 || s_active_dash_sweep_script_id != script_id || GetTickCount64() >= s_active_dash_sweep_end_ms) {
                ArmDashSweepDamage(script_id, 25.0f, 3.8f, 2200);
            }
        } else if (script_id == 0x058000000000007DULL || script_id == 0x05800000000000BEULL) {
            ArmDashSweepDamage(script_id, 75.0f, 3.5f, 1500);
        }
    }

    // 4. Bastion Self-Repair (0x0580000000000153): node=47 (m_st=28 channel state) heals 75 HP/s while active
    if (script_id == 0x0580000000000153ULL && (node_idx == 47 || stu_hash == 0x619D9E95u)) {
        if (!s_bastion_repair_active) {
            s_bastion_repair_active = true;
            ShotDiagLogFmt("[BASTION_REPAIR_START] script=0x%llX node=%d (75 HP/s while held)",
                (unsigned long long)script_id, node_idx);
        }
    }

    // 5. Soldier: 76 Biotic Field (0x0580000000000257 node=6 WeaponVolley 0x860EE5FC)
    if (script_id == 0x0580000000000257ULL && (node_idx == 6 || stu_hash == 0x860EE5FCu)) {
        auto* oc1  = owner_ent->getById<Component_1_SceneRendering>(1);
        auto* oc15 = owner_ent->getById<Component_15_STUCharacterMoverComponent>(0x15);
        s_biotic_field_pos = (oc15 && !IsBadReadPtr(oc15, sizeof(Component_15_STUCharacterMoverComponent)))
            ? oc15->movement_state_1.absolute_position : (oc1 ? oc1->position : Vector4{});
        uint64_t now_ms = GetTickCount64();
        if (now_ms >= s_biotic_field_end_ms) {
            s_biotic_field_end_ms = now_ms + 5000;
            ShotDiagLogFmt("[BIOTIC_FIELD_DEPLOY] hero=0x%X pos=(%.1f,%.1f,%.1f) dur=5000ms (40 HP/s)",
                owner_ent->entity_id, s_biotic_field_pos.X, s_biotic_field_pos.Y, s_biotic_field_pos.Z);
        }
    }

    // 6. Torbjorn Armor Pack (0x05800000000000F1 / 0x05800000000000F5): grant +75 HP/Armor on throw
    if ((script_id == 0x05800000000000F1ULL || script_id == 0x05800000000000F5ULL) && stu_hash == 0x860EE5FCu) {
        ApplyHeroSelfHeal(owner_ent, 75.0f, script_id, "TORB_ARMOR_PACK_SELF");
    }

    // 7. Soldier: 76 Passive Status Subgraph (0x05800000000001C7):
    // In offline mode, node[49] / node[120] (0xF7D6498C STUStatescriptStateStack) push onto comp23's ability mutex table
    // (0xD80000000000021 / 0xD80000000000EBC) with a blocking priority float at [state + 0xB8], causing +0xF4D1D0 to reject
    // Biotic Field (0x0580000000000257 node[3] 0xF53C5DA8) and play the "cannot use ability yet" buzzer sound (+0xD65BE0).
    if (script_id == 0x05800000000001C7ULL && stu_hash == 0xF7D6498Cu && !IsBadReadPtr(state, 0xD0)) {
        float old_prio = *(float*)((uintptr_t)state + 0xB8);
        if (old_prio > -999.0f) {
            *(float*)((uintptr_t)state + 0xB8) = -1000.0f;
            ShotDiagLogFmt("[STATE_STACK_PRIO_LOWER] script=0x%llX node=%d state=0x%llX old_prio=%.2f -> -1000.0",
                (unsigned long long)script_id, node_idx, (unsigned long long)state, old_prio);
        }
    }
}

static void OnUniversalStateExit(StatescriptSyncMgr* ss, StatescriptInstance* inst, StatescriptState* state) {
    if (!IsHeroOrSubEntScript(inst) || !state || IsBadReadPtr(state, sizeof(StatescriptState))) return;
    if (!inst->graph || IsBadReadPtr(inst->graph, sizeof(STUStatescriptGraph))) return;

    STUStatescriptState* stu_state = inst->graph->STUFromState(state);
    if (!stu_state || IsBadReadPtr(stu_state, sizeof(STUStatescriptBase))) return;
    auto* stu_base = (STUStatescriptBase*)stu_state;
    uint64_t script_id = SafeReadVal<uint64_t>((uintptr_t)inst + 0x150);
    int node_idx = stu_base->idx_in_nodes;

    // Stop Bastion Self-Repair HoT when channel state node=47 exits (key released or full HP)
    if (script_id == 0x0580000000000153ULL && node_idx == 47) {
        if (s_bastion_repair_active) {
            s_bastion_repair_active = false;
            ShotDiagLogFmt("[BASTION_REPAIR_END] script=0x%llX node=%d", (unsigned long long)script_id, node_idx);
        }
    }

    // Stop D.Va Boosters sweep when flight state node=15 or node=19 exits early (cancelled by player)
    if (script_id == 0x058000000000047EULL && (node_idx == 15 || node_idx == 19)) {
        if (s_active_dash_sweep_script_id == script_id) {
            s_active_dash_sweep_end_ms = 0;
        }
    }

    // Stop Mei Cryo-Freeze HoT if ice block state exits early (cancelled by player)
    if (script_id == 0x05800000000005FEULL && (node_idx == 0 || node_idx == 15)) {
        if (s_active_hot_script_id == 0x05800000000005FEULL) {
            s_active_hot_end_ms = 0;
            s_active_hot_script_id = 0;
        }
    }
}

// 10. +0xD739A0 (IsGraphNodeClientOnly):
// Strictly gated by IsHeroOrSubEntScript(inst) so main menu, map scripts, loading screens, and doors are 100% untouched.
// For the LocalPlayer Hero Model (and its weapons/deployables), allows server-tagged ability nodes (0x4E83BC01, 0x04CC5359,
// 0x1BB99C36, 0x4CCED352, 0x0E3689F3) to be entered without mutating shared CASC STUStatescriptGraph memory.
typedef uint8_t(__fastcall* is_node_client_only_fn)(StatescriptInstance* inst, STUStatescriptBase* node);
static is_node_client_only_fn IsGraphNodeClientOnly_orig = nullptr;
static uint8_t __fastcall IsGraphNodeClientOnly_hook(StatescriptInstance* inst, STUStatescriptBase* node) {
    uint8_t res = IsGraphNodeClientOnly_orig(inst, node);
    if (res == 0 && IsHeroOrSubEntScript(inst)) {
        return 1;
    }
    return res;
}

// 10b. +0xF4D1D0 (CanAcquireAbilityMutex):
// Called by +0x1088810 (gate #5 at +0x1088BD7 and gate #6 at +0x1088C0B for 0xF53C5DA8 ability input states).
// In offline mode, passive status subgraphs (such as Soldier: 76's 0x05800000000001C7 node[49] 0xF7D6498C holding 0xD80000000000021)
// remain on comp23's 8-bucket ability mutex table ([comp23 + 0x10 + bucket*24]) with a higher priority float at [holder_state + 0xB8].
// That causes +0xF4D1D0 to return 0, making +0x1088810 set *out_play_cannot_use_sound = 1 (+0xD65BE0 buzzer sound) and block
// +0x1088460 -> +0xF55FD0 -> +0xF596E0 from placing the ability state at the tail [entry + 0x18] to fire vt+0x260 (+0xD65AC0).
typedef uint8_t(__fastcall* can_acquire_ability_mutex_fn)(
    StatescriptInstance* inst,
    STUConfigVar* mutex_var,
    STUConfigVar* priority_var,
    uintptr_t state_impl);
static can_acquire_ability_mutex_fn CanAcquireAbilityMutex_orig = nullptr;
static uint8_t __fastcall CanAcquireAbilityMutex_hook(
    StatescriptInstance* inst,
    STUConfigVar* mutex_var,
    STUConfigVar* priority_var,
    uintptr_t state_impl)
{
    uint8_t res = CanAcquireAbilityMutex_orig(inst, mutex_var, priority_var, state_impl);
    if (res == 0 && IsHeroOrSubEntScript(inst) && state_impl != 0 && !IsBadReadPtr((const void*)state_impl, 0xD0)) {
        uintptr_t comp23 = SafeReadVal<uintptr_t>((uintptr_t)inst + 0x10);
        uint64_t req_script = SafeReadVal<uint64_t>((uintptr_t)inst + 0x150);
        int32_t req_node = SafeReadVal<int32_t>(state_impl + 0x70);
        float req_prio = *(float*)(state_impl + 0xB8);

        if (comp23 && !IsBadReadPtr((const void*)comp23, 0x100)) {
            for (int b = 0; b < 8; ++b) {
                int32_t cnt = SafeReadVal<int32_t>(comp23 + 0x10 + (uintptr_t)b * 24);
                uintptr_t arr = SafeReadVal<uintptr_t>(comp23 + 0x18 + (uintptr_t)b * 24);
                if (cnt <= 0 || cnt > 64 || !arr || IsBadReadPtr((const void*)arr, (size_t)cnt * 48)) continue;
                for (int i = 0; i < cnt; ++i) {
                    uintptr_t entry = arr + (uintptr_t)i * 48;
                    uint32_t key = SafeReadVal<uint32_t>(entry + 0x28);
                    uintptr_t holders[2] = {
                        SafeReadVal<uintptr_t>(entry + 0x10),
                        SafeReadVal<uintptr_t>(entry + 0x18)
                    };
                    for (int h = 0; h < 2; ++h) {
                        uintptr_t hs = holders[h];
                        if (!hs || hs == state_impl || IsBadReadPtr((const void*)hs, 0xD0)) continue;
                        float h_prio = *(float*)(hs + 0xB8);
                        if (h_prio > -999.0f && h_prio >= req_prio) {
                            StatescriptInstance* h_inst = (StatescriptInstance*)SafeReadVal<uintptr_t>(hs + 0x78);
                            uint64_t h_script = (h_inst && !IsBadReadPtr(h_inst, 0x160))
                                ? SafeReadVal<uint64_t>((uintptr_t)h_inst + 0x150) : 0;
                            int32_t h_node = SafeReadVal<int32_t>(hs + 0x70);
                            *(float*)(hs + 0xB8) = -1000.0f;
                            ShotDiagLogFmt("[ABILITY_MUTEX_UNLOCK] key=0x%X req=(0x%llX n=%d prio=%.2f) holder=(0x%llX n=%d prio=%.2f->-1000.0)",
                                key,
                                (unsigned long long)req_script, req_node, req_prio,
                                (unsigned long long)h_script, h_node, h_prio);
                        }
                    }
                }
            }
        }
        res = CanAcquireAbilityMutex_orig(inst, mutex_var, priority_var, state_impl);
        if (res == 0) {
            res = 1;
            ShotDiagLogFmt("[ABILITY_MUTEX_FORCE_ALLOW] req=(0x%llX n=%d prio=%.2f)",
                (unsigned long long)req_script, req_node, req_prio);
        }
    }
    return res;
}

// 11. +0x1096500 (PlacementState_TickRay for 0x160C4F1D) & +0xD6D270 (FindGround_Begin for 0x72509D3B):
// Ensures Symmetra Sentry/Teleporter, Torbjorn Turret, and Mei Ice Wall previews always compute valid placement coordinates
// and mark their placement state valid (turning Symmetra's red sentry preview blue and allowing LMB confirmation).
typedef void(__fastcall* placement_tick_ray_fn)(StatescriptState* state);
static placement_tick_ray_fn PlacementState_TickRay_orig = nullptr;
static void __fastcall PlacementState_TickRay_hook(StatescriptState* state) {
    PlacementState_TickRay_orig(state);
    if (!state || IsBadReadPtr(state, 0x100)) return;
    if (*((uint8_t*)state + 0xB0) == 0) {
        StatescriptInstance* inst = (StatescriptInstance*)SafeReadVal<uintptr_t>((uintptr_t)state + 0x78);
        Entity* owner_ent = (inst && !IsBadReadPtr(inst, sizeof(StatescriptInstance)) && inst->ss_inner &&
            !IsBadReadPtr(inst->ss_inner, sizeof(StatescriptSyncMgr)) && inst->ss_inner->component_ref)
            ? inst->ss_inner->component_ref->base.entity_backref : GetLocalHeroModelEnt();
        if (owner_ent) {
            auto* c1 = owner_ent->getById<Component_1_SceneRendering>(1);
            auto* c15 = owner_ent->getById<Component_15_STUCharacterMoverComponent>(0x15);
            Vector4 pos = c1 ? c1->position : Vector4{};
            Vector4 fwd{ 0.0f, 0.0f, 1.0f, 0.0f };
            if (c15 && !IsBadReadPtr(c15, sizeof(Component_15_STUCharacterMoverComponent))) {
                pos = c15->movement_state_1.absolute_position;
                const Vector4& q = c15->movement_state_1.rotation_quaternion;
                fwd.X = 2.0f * (q.X * q.Z + q.W * q.Y);
                fwd.Z = 1.0f - 2.0f * (q.X * q.X + q.Y * q.Y);
                float fl = sqrtf(fwd.X * fwd.X + fwd.Z * fwd.Z);
                if (fl > 1e-4f) { fwd.X /= fl; fwd.Z /= fl; }
            }
            float* hit_pos = (float*)((uintptr_t)state + 0xC0);
            float* hit_nrm = (float*)((uintptr_t)state + 0xD0);
            float* hit_fwd = (float*)((uintptr_t)state + 0xE0);
            if (hit_pos[0] == 0.0f && hit_pos[1] == 0.0f && hit_pos[2] == 0.0f) {
                hit_pos[0] = pos.X + fwd.X * 2.8f;
                hit_pos[1] = pos.Y;
                hit_pos[2] = pos.Z + fwd.Z * 2.8f;
            }
            if (hit_nrm[0] == 0.0f && hit_nrm[1] == 0.0f && hit_nrm[2] == 0.0f) {
                hit_nrm[0] = 0.0f; hit_nrm[1] = 1.0f; hit_nrm[2] = 0.0f;
            }
            if (hit_fwd[0] == 0.0f && hit_fwd[1] == 0.0f && hit_fwd[2] == 0.0f) {
                hit_fwd[0] = fwd.X; hit_fwd[1] = 0.0f; hit_fwd[2] = fwd.Z;
            }
            *((uint8_t*)state + 0xB0) = 1;
            *((uint8_t*)state + 0xB1) = 1;
        }
    }
}

typedef void(__fastcall* find_ground_begin_fn)(StatescriptState* state, uint64_t a2);
static find_ground_begin_fn FindGround_Begin_orig = nullptr;
static void __fastcall FindGround_Begin_hook(StatescriptState* state, uint64_t a2) {
    FindGround_Begin_orig(state, a2);
    if (state && !IsBadReadPtr(state, 0xD8)) {
        *((uint8_t*)state + 0xC8) = 1;
    }
}

// 12. +0xF471F0: TraverseLink(StatescriptSyncMgr* mgr, StatescriptInstance* inst, STUStatescriptBase* source_node,
//                             STUStatescriptOutputPlug* output_plug, STUStatescriptInputPlug* input_plug, uint32_t a6)
// Intercepts custom "place now" input plugs on the 4 Group-2 placement / Mei Ice Wall states:
//   - 0x160C4F1D (Placement State — Symmetra Sentry/Teleporter, Torbjorn Turret, Junkrat RIP-Tire): input plug 0xC63FD388 -> output plug 0x02701BF5
//   - 0x72509D3B (FindGround — Mei Ice Wall): input plug 0xC63FD388 -> output plug 0x02701BF5
//   - 0x12AA4F0F (Mei Ice Wall Start State): input plug 0x5556C513 -> output plug 0x8CF01B33
//   - 0x614C3A0A (Mei Ice Wall Pillar Segment State): input plug 0x5556C513 -> output plug 0x8CF01B33
typedef uint8_t(__fastcall* traverse_link_fn)(
    StatescriptSyncMgr* mgr,
    StatescriptInstance* inst,
    STUStatescriptBase* source_node,
    STUStatescriptOutputPlug* output_plug,
    STUStatescriptInputPlug* input_plug,
    uint32_t a6);
static traverse_link_fn TraverseLink_orig = nullptr;
static uint8_t __fastcall TraverseLink_hook(
    StatescriptSyncMgr* mgr,
    StatescriptInstance* inst,
    STUStatescriptBase* source_node,
    STUStatescriptOutputPlug* output_plug,
    STUStatescriptInputPlug* input_plug,
    uint32_t a6)
{
    uint8_t res = TraverseLink_orig(mgr, inst, source_node, output_plug, input_plug, a6);

    if (IsHeroOrSubEntScript(inst) && input_plug && !IsBadReadPtr(input_plug, sizeof(STUStatescriptInputPlug))) {
        STUStatescriptBase* target_node = (STUStatescriptBase*)SafeReadVal<uintptr_t>((uintptr_t)input_plug + 0x18);
        if (target_node && !IsBadReadPtr(target_node, sizeof(STUStatescriptState))) {
            STUInfo* info = (target_node->graph_node.base.vfptr && !IsBadReadPtr(target_node->graph_node.base.vfptr, sizeof(STUBase_vt)))
                ? target_node->graph_node.base.vfptr->GetSTUInfo() : nullptr;
            uint32_t stu_hash = (info && !IsBadReadPtr(info, sizeof(STUInfo))) ? info->name_hash : 0;

            if (stu_hash == 0x160C4F1Du || stu_hash == 0x72509D3Bu || stu_hash == 0x12AA4F0Fu || stu_hash == 0x614C3A0Au) {
                auto* stu_state = (STUStatescriptState*)target_node;
                int32_t state_idx = SafeReadVal<int32_t>((uintptr_t)stu_state + 0xF8);
                StatescriptState* state = (state_idx >= 0 && state_idx < inst->state_impl_list.num && inst->state_impl_list.ptr)
                    ? inst->state_impl_list.ptr[state_idx] : nullptr;
                StatescriptSyncMgr* sync_mgr = mgr ? mgr : inst->ss_inner;

                if (state && !IsBadReadPtr(state, 0x100)) {
                    if (stu_hash == 0x160C4F1Du) {
                        auto* place_in = GetSTUFieldByHash<STUStatescriptInputPlug*>(target_node, 0xC63FD388u);
                        if (place_in && input_plug == place_in) {
                            PlacementState_TickRay_hook(state);
                            auto* cvd_pos = GetSTUFieldByHash<STUConfigVarDynamic*>(target_node, 0xC500D8CFu);
                            auto* cvd_nrm = GetSTUFieldByHash<STUConfigVarDynamic*>(target_node, 0x57216848u);
                            auto* cvd_fwd = GetSTUFieldByHash<STUConfigVarDynamic*>(target_node, 0x2650F0BEu);
                            if (cvd_pos) WriteDynamicVarPrim(inst, cvd_pos, StatescriptPrimitive(StatescriptPrimitive_VEC3, (__int64)((uintptr_t)state + 0xC0)));
                            if (cvd_nrm) WriteDynamicVarPrim(inst, cvd_nrm, StatescriptPrimitive(StatescriptPrimitive_VEC3, (__int64)((uintptr_t)state + 0xD0)));
                            if (cvd_fwd) WriteDynamicVarPrim(inst, cvd_fwd, StatescriptPrimitive(StatescriptPrimitive_VEC3, (__int64)((uintptr_t)state + 0xE0)));

                            auto* out_plug = GetSTUFieldByHash<STUStatescriptOutputPlug*>(target_node, 0x02701BF5u);
                            if (out_plug && !IsBadReadPtr(out_plug, sizeof(STUStatescriptOutputPlug))) {
                                ShotDiagLogFmt("[PLACEMENT_CONFIRM (0x160C4F1D)] state=0x%llX pos=(%.1f,%.1f,%.1f) -> plug 0x02701BF5",
                                    (unsigned long long)state, *(float*)((uintptr_t)state + 0xC0), *(float*)((uintptr_t)state + 0xC4), *(float*)((uintptr_t)state + 0xC8));
                                SafeTraverseOutputPlug(sync_mgr, inst, target_node, out_plug, 0x108);
                            }
                        }
                    } else if (stu_hash == 0x72509D3Bu) {
                        auto* place_in = GetSTUFieldByHash<STUStatescriptInputPlug*>(target_node, 0xC63FD388u);
                        if (place_in && input_plug == place_in) {
                            *((uint8_t*)state + 0xC8) = 1;
                            float* gpos = (float*)((uintptr_t)state + 0xB0);
                            if (gpos[0] == 0.0f && gpos[1] == 0.0f && gpos[2] == 0.0f) {
                                if (Entity* hero = GetLocalHeroModelEnt()) {
                                    auto* c1 = hero->getById<Component_1_SceneRendering>(1);
                                    auto* c15 = hero->getById<Component_15_STUCharacterMoverComponent>(0x15);
                                    Vector4 hp = c1 ? c1->position : Vector4{};
                                    Vector4 fwd{ 0.0f, 0.0f, 1.0f, 0.0f };
                                    if (c15 && !IsBadReadPtr(c15, sizeof(Component_15_STUCharacterMoverComponent))) {
                                        hp = c15->movement_state_1.absolute_position;
                                        const Vector4& q = c15->movement_state_1.rotation_quaternion;
                                        fwd.X = 2.0f * (q.X * q.Z + q.W * q.Y);
                                        fwd.Z = 1.0f - 2.0f * (q.X * q.X + q.Y * q.Y);
                                        float fl = sqrtf(fwd.X * fwd.X + fwd.Z * fwd.Z);
                                        if (fl > 1e-4f) { fwd.X /= fl; fwd.Z /= fl; }
                                    }
                                    gpos[0] = hp.X + fwd.X * 4.5f;
                                    gpos[1] = hp.Y;
                                    gpos[2] = hp.Z + fwd.Z * 4.5f;
                                }
                            }
                            auto* cvd_pos = (STUConfigVarDynamic*)SafeReadVal<uintptr_t>((uintptr_t)target_node + 0x170);
                            auto* cvd_ok  = (STUConfigVarDynamic*)SafeReadVal<uintptr_t>((uintptr_t)target_node + 0x178);
                            if (cvd_pos) WriteDynamicVarPrim(inst, cvd_pos, StatescriptPrimitive(StatescriptPrimitive_VEC3, (__int64)gpos));
                            if (cvd_ok)  WriteDynamicVarPrim(inst, cvd_ok,  StatescriptPrimitive(StatescriptPrimitive_BYTE, 1));

                            auto* out_plug = GetSTUFieldByHash<STUStatescriptOutputPlug*>(target_node, 0x02701BF5u);
                            if (out_plug && !IsBadReadPtr(out_plug, sizeof(STUStatescriptOutputPlug))) {
                                ShotDiagLogFmt("[ICEWALL_FINDGROUND (0x72509D3B)] state=0x%llX pos=(%.1f,%.1f,%.1f) -> plug 0x02701BF5",
                                    (unsigned long long)state, gpos[0], gpos[1], gpos[2]);
                                SafeTraverseOutputPlug(sync_mgr, inst, target_node, out_plug, 0x108);
                            }
                        }
                    } else if (stu_hash == 0x12AA4F0Fu) {
                        auto* place_in = GetSTUFieldByHash<STUStatescriptInputPlug*>(target_node, 0x5556C513u);
                        if (place_in && input_plug == place_in) {
                            __try {
                                ((void(__fastcall*)(StatescriptState*))(globals::gameBase + 0x1099FA0))(state);
                            } __except (EXCEPTION_EXECUTE_HANDLER) {}
                            for (int k = 0; k < 4; ++k) {
                                auto* cvd = (STUConfigVarDynamic*)SafeReadVal<uintptr_t>((uintptr_t)target_node + 0x148 + (uintptr_t)k * 8);
                                float fval = SafeReadVal<float>((uintptr_t)state + 0xA8 + (uintptr_t)k * 4);
                                if (cvd) WriteDynamicVarPrim(inst, cvd, StatescriptPrimitive(fval));
                            }
                            auto* out_plug = GetSTUFieldByHash<STUStatescriptOutputPlug*>(target_node, 0x8CF01B33u);
                            if (out_plug && !IsBadReadPtr(out_plug, sizeof(STUStatescriptOutputPlug))) {
                                SafeTraverseOutputPlug(sync_mgr, inst, target_node, out_plug, 0x108);
                            }
                        }
                    } else if (stu_hash == 0x614C3A0Au) {
                        auto* place_in = GetSTUFieldByHash<STUStatescriptInputPlug*>(target_node, 0x5556C513u);
                        if (place_in && input_plug == place_in) {
                            __try {
                                ((void(__fastcall*)(StatescriptState*))(globals::gameBase + 0x109A580))(state);
                            } __except (EXCEPTION_EXECUTE_HANDLER) {}
                            for (int k = 0; k < 2; ++k) {
                                auto* cvd = (STUConfigVarDynamic*)SafeReadVal<uintptr_t>((uintptr_t)target_node + 0x140 + (uintptr_t)k * 8);
                                float fval = SafeReadVal<float>((uintptr_t)state + 0xA8 + (uintptr_t)k * 4);
                                if (cvd) WriteDynamicVarPrim(inst, cvd, StatescriptPrimitive(fval));
                            }
                            auto* out_plug = GetSTUFieldByHash<STUStatescriptOutputPlug*>(target_node, 0x8CF01B33u);
                            if (out_plug && !IsBadReadPtr(out_plug, sizeof(STUStatescriptOutputPlug))) {
                                SafeTraverseOutputPlug(sync_mgr, inst, target_node, out_plug, 0x108);
                            }
                        }
                    }
                }
            }
        }
    }

    return res;
}

typedef uint8_t(__fastcall* attach_c1_world_fn)(Component_1_SceneRendering* child_c1, Component_1_SceneRendering* parent_c1);
attach_c1_world_fn AttachComponent1World_orig = nullptr;
uint8_t __fastcall AttachComponent1World_hook(Component_1_SceneRendering* child_c1, Component_1_SceneRendering* parent_c1) {
    return AttachComponent1World_orig(child_c1, parent_c1);
}

void __fastcall SetPosRotation_hook(Component_1_SceneRendering* comp1, Vector4* rot, Vector4* pos, uint8_t flag) {
    uintptr_t caller = (uintptr_t)_ReturnAddress();
    uintptr_t caller_rva = (caller >= globals::gameBase && caller < globals::gameBase + globals::gameSize)
        ? (caller - globals::gameBase) : caller;
    Vector4 override_rot{}, override_pos{};
    if (HandleComponent1TransformHook(comp1, rot, pos, caller_rva, override_rot, override_pos)) {
        SetPosRotation_orig(comp1, &override_rot, &override_pos, flag);
        return;
    }
    SetPosRotation_orig(comp1, rot, pos, flag);
}

set_pos_rot_fn SetWorldPosRotation_orig = nullptr;
void __fastcall SetWorldPosRotation_hook(Component_1_SceneRendering* comp1, Vector4* rot, Vector4* pos, uint8_t flag) {
    SetWorldPosRotation_orig(comp1, rot, pos, flag);
}

void __fastcall SetPosRotationInternal_hook(Component_1_SceneRendering* comp1, Vector4* rot, Vector4* pos, uint8_t flag) {
    uintptr_t caller = (uintptr_t)_ReturnAddress();
    uintptr_t caller_rva = (caller >= globals::gameBase && caller < globals::gameBase + globals::gameSize)
        ? (caller - globals::gameBase) : caller;
    // Inside +0x1037520 at +0x10378C9 (return RVA +0x10378CE), Havok raycast has just finished and calls
    // +0xA045C0(proj_c1, &[pellet_c27 + 0xB0], nullptr, 1) BEFORE +0x1036C30 or +0xD37230 run.
    if (kEnablePelletRedirectHacks && caller_rva == 0x10378CE && rot != nullptr) {
        RedirectPelletC27ToDummy((uintptr_t)rot - 0xB0, "+0x10378CE_pre_A045C0");
    }
    Vector4 override_rot{}, override_pos{};
    if (HandleComponent1TransformHook(comp1, rot, pos, caller_rva, override_rot, override_pos)) {
        SetPosRotationInternal_orig(comp1, &override_rot, &override_pos, flag);
        return;
    }
    SetPosRotationInternal_orig(comp1, rot, pos, flag);
}

typedef void(__fastcall* set_pos_fn)(Component_1_SceneRendering* comp1, Vector4* pos);
set_pos_fn SetPosition_orig = nullptr;
void __fastcall SetPosition_hook(Component_1_SceneRendering* comp1, Vector4* pos) {
    uintptr_t caller = (uintptr_t)_ReturnAddress();
    uintptr_t caller_rva = (caller >= globals::gameBase && caller < globals::gameBase + globals::gameSize)
        ? (caller - globals::gameBase) : caller;
    Vector4 override_rot{}, override_pos{};
    if (HandleComponent1TransformHook(comp1, nullptr, pos, caller_rva, override_rot, override_pos)) {
        SetPosition_orig(comp1, &override_pos);
        return;
    }
    SetPosition_orig(comp1, pos);
}

bool OriginClutterTracker::IsSpawnDoorEntity(Entity* ent, uint32 local_ctrl_id, uint32 local_mdl_id) {
    if (!ent) return false;
    if (local_ctrl_id != 0 && ent->entity_id == local_ctrl_id) return false;
    if (local_mdl_id != 0 && ent->entity_id == local_mdl_id) return false;
    // Never touch player controllers, player models, PvP game state, weapons (0x1D), or spawn room aura/trigger volumes (0x13 / 0x2B)
    if (ent->hasComponent(0x15) || ent->hasComponent(0x2F) || ent->hasComponent(0x3F) ||
        ent->hasComponent(0x24) || ent->hasComponent(0x1D) || ent->hasComponent(0x13) || ent->hasComponent(0x2B)) {
        return false;
    }
    // Map spawn door entities possess Component_1 (SceneRendering), Component_04 (Animation/Skeleton),
    // Component_12 (MovementState), Component_1C (3D Model), Component_23 (Statescript),
    // and the door mover/NavMesh components Component_29 (0x29) & Component_2A (0x2A).
    if (!ent->hasComponent(0x01) || !ent->hasComponent(0x04) || !ent->hasComponent(0x12) ||
        !ent->hasComponent(0x1C) || !ent->hasComponent(0x23) ||
        !ent->hasComponent(0x29) || !ent->hasComponent(0x2A)) {
        return false;
    }

    auto* c1 = ent->getById<Component_1_SceneRendering>(1);
    if (!c1) return false;
    uintptr_t parent_c1 = *(uintptr_t*)((uintptr_t)c1 + 0x1D0);
    if (parent_c1 != 0) return false;

    auto* slot = GetSlot(ent->entity_id, ent);
    if (slot && slot->spawn_caller_rva == 0xD56BFB) return false;
    return true;
}

static bool SetDoorVarInBag(StatescriptSyncMgr* ss_inner, StatescriptInstance* script, StatescriptVar_VarBag* vb, __int64 var_id, StatescriptPrimitive new_val) {
    if (!vb || IsBadReadPtr(vb, sizeof(StatescriptVar_VarBag))) return false;
    for (int i = 0; i < 16; i++) {
        auto* item = &vb->sub_items[i];
        if (!item || !item->arr_ptr || IsBadReadPtr(item->arr_ptr, sizeof(void*))) continue;
        for (int j = 0; j < item->item_count; j++) {
            StatescriptVar_Base* rid = item->arr_ptr[j].ptr;
            if (!rid || IsBadReadPtr(rid, sizeof(StatescriptVar_Base))) continue;
            if (rid->var_id == var_id && rid->is_cvd()) {
                auto* prim = (StatescriptVar_Primitive*)rid;
                StatescriptSyncMgr* mgr = (prim->base.parent_rid && prim->base.parent_rid->ss_compo_inner)
                    ? prim->base.parent_rid->ss_compo_inner : ss_inner;
                StatescriptInstance* inst = (prim->base.parent_rid && prim->base.parent_rid->ss_instance)
                    ? prim->base.parent_rid->ss_instance : script;
                prim->base.stu_base.vfptr->Primitive_SetValueFromRemote(
                    &prim->base, mgr, inst, &new_val, &prim->base.user, 1);
                return true;
            }
        }
    }
    return false;
}

static bool IsDoorScriptOpen(Entity* ent) {
    if (!ent) return false;
    auto* c23 = ent->getById<Component_23_Statescript>(0x23);
    if (!c23 || IsBadReadPtr(c23, sizeof(Component_23_Statescript))) return false;
    for (int s = 0; s < c23->ss_inner.g1_instanceArr.num; s++) {
        StatescriptInstance* script = c23->ss_inner.g1_instanceArr.ptr[s];
        if (!script || IsBadReadPtr(script, sizeof(StatescriptInstance))) continue;
        StatescriptVar_VarBag* vb = script->rid_instance_varbag;
        if (!vb || IsBadReadPtr(vb, sizeof(StatescriptVar_VarBag))) continue;
        for (int i = 0; i < 16; i++) {
            auto* item = &vb->sub_items[i];
            if (!item || !item->arr_ptr || IsBadReadPtr(item->arr_ptr, sizeof(void*))) continue;
            for (int j = 0; j < item->item_count; j++) {
                StatescriptVar_Base* rid = item->arr_ptr[j].ptr;
                if (!rid || IsBadReadPtr(rid, sizeof(StatescriptVar_Base))) continue;
                if (rid->var_id == 0xd80000000000192LL && rid->is_cvd()) {
                    auto* prim = (StatescriptVar_Primitive*)rid;
                    if (prim->global_var.get_convert_float(0.0f) > 0.01f) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

int OriginClutterTracker::OpenSpawnDoors(EntityAdminBase* game_ea) {
    if (!game_ea) return 0;
    int opened = 0;
    uint32 local_ctrl_id = game_ea->local_entid;
    uint32 local_mdl_id = 0;
    if (Entity* lc = game_ea->getLocalEnt()) {
        if (auto* pet = lc->getById<Component_20_ModelReference>(0x20)) {
            local_mdl_id = pet->cam_attach_entid ? pet->cam_attach_entid : pet->aim_entid;
        }
    }
    for (Entity* ent : *game_ea) {
        if (!IsSpawnDoorEntity(ent, local_ctrl_id, local_mdl_id)) continue;
        auto* c1 = ent->getById<Component_1_SceneRendering>(1);
        auto* slot = GetSlot(ent->entity_id, ent);
        if (slot && slot->culled_by_heracles) {
            slot->culled_by_heracles = false;
            if (s_total_culled_props > 0) s_total_culled_props--;
            if (c1) {
                Vector4 target_pos = slot->has_nonzero_pos ? slot->latest_nonzero_pos : slot->pre_cull_pos;
                Vector4 target_rot = slot->has_nonzero_pos ? slot->latest_nonzero_rot : slot->pre_cull_rot;
                if (target_rot.X == 0.0f && target_rot.Y == 0.0f && target_rot.Z == 0.0f && target_rot.W == 0.0f) {
                    target_rot.W = 1.0f;
                }
                c1->SetScale(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
                c1->SetPosRotation(target_pos, target_rot);
                c1->SetVisible();
            }
        } else if (c1 && !c1->IsVisible()) {
            c1->SetVisible();
        }

        auto* c23 = ent->getById<Component_23_Statescript>(0x23);
        if (!c23 || IsBadReadPtr(c23, sizeof(Component_23_Statescript))) continue;

        for (int s = 0; s < c23->ss_inner.g1_instanceArr.num; s++) {
            StatescriptInstance* script = c23->ss_inner.g1_instanceArr.ptr[s];
            if (!script || IsBadReadPtr(script, sizeof(StatescriptInstance))) continue;

            // 1. Set 0xd800000000002bb = 0 (no wait delay in Node 25 StateWait)
            StatescriptPrimitive zero_int{ 0, 0, StatescriptPrimitive_INT };
            SetDoorVarInBag(&c23->ss_inner, script, script->rid_instance_varbag, 0xd800000000002bbLL, zero_int);

            // 2. Set 0xd80000000000192 = 1.0f (triggers Node 6 BooleanSwitch -> True branch -> Node 25 -> Node 2 Open Door)
            float one_f = 1.0f;
            StatescriptPrimitive open_flt{ 0, 0, StatescriptPrimitive_FLT };
            *(float*)&open_flt.value = one_f;
            SetDoorVarInBag(&c23->ss_inner, script, script->rid_instance_varbag, 0xd80000000000192LL, open_flt);

            // 3. Ensure Node 2 (Open Door State) is started if BooleanSwitch deferred transition
            if (script->script_id == 0x0580000000000042LL && script->graph && script->graph->m_nodes.count() > 3) {
                auto st2 = statescript_logger::GetScriptNodeState(script, 2);
                if (st2.is_finish_state) {
                    script->StartStopState(3, false);
                    script->StartStopState(2, true);
                }
            }
            c23->ss_inner.needs_process_e = 1;
            c23->ss_inner.needs_e_process_0 = 0;
        }
        opened++;
    }
    return opened;
}

int OriginClutterTracker::RestoreSpawnDoors(EntityAdminBase* game_ea) {
    if (!game_ea) return 0;
    s_auto_open_spawn_doors = false;
    int restored = 0;
    uint32 local_ctrl_id = game_ea->local_entid;
    uint32 local_mdl_id = 0;
    if (Entity* lc = game_ea->getLocalEnt()) {
        if (auto* pet = lc->getById<Component_20_ModelReference>(0x20)) {
            local_mdl_id = pet->cam_attach_entid ? pet->cam_attach_entid : pet->aim_entid;
        }
    }
    for (Entity* ent : *game_ea) {
        if (!IsSpawnDoorEntity(ent, local_ctrl_id, local_mdl_id)) continue;
        auto* c1 = ent->getById<Component_1_SceneRendering>(1);
        auto* slot = GetSlot(ent->entity_id, ent);
        if (slot && slot->culled_by_heracles) {
            slot->culled_by_heracles = false;
            if (s_total_culled_props > 0) s_total_culled_props--;
            if (c1) {
                Vector4 target_pos = slot->has_nonzero_pos ? slot->latest_nonzero_pos : slot->pre_cull_pos;
                Vector4 target_rot = slot->has_nonzero_pos ? slot->latest_nonzero_rot : slot->pre_cull_rot;
                if (target_rot.X == 0.0f && target_rot.Y == 0.0f && target_rot.Z == 0.0f && target_rot.W == 0.0f) {
                    target_rot.W = 1.0f;
                }
                c1->SetScale(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
                c1->SetPosRotation(target_pos, target_rot);
                c1->SetVisible();
            }
        } else if (c1 && !c1->IsVisible()) {
            c1->SetVisible();
        }

        auto* c23 = ent->getById<Component_23_Statescript>(0x23);
        if (!c23 || IsBadReadPtr(c23, sizeof(Component_23_Statescript))) continue;

        for (int s = 0; s < c23->ss_inner.g1_instanceArr.num; s++) {
            StatescriptInstance* script = c23->ss_inner.g1_instanceArr.ptr[s];
            if (!script || IsBadReadPtr(script, sizeof(StatescriptInstance))) continue;

            // 1. Set 0xd800000000002bb = 0 (no wait delay in Node 26 StateWait)
            StatescriptPrimitive zero_int{ 0, 0, StatescriptPrimitive_INT };
            SetDoorVarInBag(&c23->ss_inner, script, script->rid_instance_varbag, 0xd800000000002bbLL, zero_int);

            // 2. Set 0xd80000000000838 = 1 (as Node 21 does before Node 22 so Node 3 plays closing anim)
            StatescriptPrimitive one_int{ 1, 0, StatescriptPrimitive_INT };
            SetDoorVarInBag(&c23->ss_inner, script, script->rid_instance_varbag, 0xd80000000000838LL, one_int);

            // 3. Set 0xd80000000000192 = 0.0f (as Node 22 does -> triggers Node 6 BooleanSwitch -> False branch -> Node 26 -> Node 3 Closed Door)
            StatescriptPrimitive close_flt{ 0, 0, StatescriptPrimitive_FLT };
            SetDoorVarInBag(&c23->ss_inner, script, script->rid_instance_varbag, 0xd80000000000192LL, close_flt);

            // 4. Ensure Node 3 (Closed Door State) is started if BooleanSwitch deferred transition
            if (script->script_id == 0x0580000000000042LL && script->graph && script->graph->m_nodes.count() > 3) {
                auto st3 = statescript_logger::GetScriptNodeState(script, 3);
                if (st3.is_finish_state) {
                    script->StartStopState(2, false);
                    script->StartStopState(3, true);
                }
            }
            c23->ss_inner.needs_process_e = 1;
            c23->ss_inner.needs_e_process_0 = 0;
        }
        restored++;
    }
    return restored;
}

void OriginClutterTracker::GetSpawnDoorCounts(EntityAdminBase* game_ea, int& out_total, int& out_open) {
    out_total = 0;
    out_open = 0;
    if (!game_ea) return;
    uint32 local_ctrl_id = game_ea->local_entid;
    uint32 local_mdl_id = 0;
    if (Entity* lc = game_ea->getLocalEnt()) {
        if (auto* pet = lc->getById<Component_20_ModelReference>(0x20)) {
            local_mdl_id = pet->cam_attach_entid ? pet->cam_attach_entid : pet->aim_entid;
        }
    }
    for (Entity* ent : *game_ea) {
        if (!IsSpawnDoorEntity(ent, local_ctrl_id, local_mdl_id)) continue;
        out_total++;
        auto* slot = GetSlot(ent->entity_id, ent);
        if ((slot && slot->culled_by_heracles) || IsDoorScriptOpen(ent)) {
            out_open++;
        }
    }
}

int OriginClutterTracker::CullUnplacedOriginProps(EntityAdminBase* game_ea) {
    if (!game_ea) return 0;
    int culled = 0;
    uint32 local_ctrl_id = game_ea->local_entid;
    uint32 local_mdl_id = 0;
    if (Entity* lc = game_ea->getLocalEnt()) {
        if (auto* pet = lc->getById<Component_20_ModelReference>(0x20)) {
            local_mdl_id = pet->cam_attach_entid ? pet->cam_attach_entid : pet->aim_entid;
        }
    }
    for (Entity* ent : *game_ea) {
        if (!ent) continue;
        if (ent->entity_id == local_ctrl_id || ent->entity_id == local_mdl_id) continue;
        // Never touch player controllers, player models, PvP game state, weapons (0x1D = Tracer's Pulse Pistols!), or spawn room auras (0x13 / 0x2B)
        if (ent->hasComponent(0x15) || ent->hasComponent(0x2F) || ent->hasComponent(0x3F) || ent->hasComponent(0x24) || ent->hasComponent(0x1D) || ent->hasComponent(0x13) || ent->hasComponent(0x2B)) continue;
        // Must have a 3D model component (0x1C) to be visual/physical map clutter
        if (!ent->hasComponent(0x1C)) continue;

        auto* c1 = ent->getById<Component_1_SceneRendering>(1);
        if (!c1) continue;
        uintptr_t parent_c1 = *(uintptr_t*)((uintptr_t)c1 + 0x1D0);
        if (parent_c1 != 0) continue;

        auto* slot = GetSlot(ent->entity_id, ent);
        if (slot && slot->culled_by_heracles) continue;
        // Never touch child/hardpoint-spawned entities (+0xD56BFB = weapon/hardpoint spawner)
        if (slot && slot->spawn_caller_rva == 0xD56BFB) continue;

        float abs_pos = fabsf(c1->position.X) + fabsf(c1->position.Y) + fabsf(c1->position.Z);
        bool had_zero_overwrite = slot && (slot->zero_overwrite_count > 0 || slot->direct_zero_overwrite_count > 0);
        if (abs_pos < 0.25f || had_zero_overwrite) {
            if (slot) {
                slot->pre_cull_pos = slot->has_nonzero_pos ? slot->latest_nonzero_pos : c1->position;
                slot->pre_cull_rot = slot->has_nonzero_pos ? slot->latest_nonzero_rot : c1->rotation;
                slot->culled_by_heracles = true;
            }
            Vector4 hidden_pos(0.0f, -5000.0f, 0.0f, 0.0f);
            Vector4 ident_rot(0.0f, 0.0f, 0.0f, 1.0f);
            c1->SetPosRotation(hidden_pos, ident_rot);
            c1->SetScale(Vector4(0.0f, 0.0f, 0.0f, 0.0f));
            c1->SetInvisible();
            culled++;
            s_total_culled_props++;
        }
    }
    return culled;
}

int OriginClutterTracker::RestoreCulledProps(EntityAdminBase* game_ea) {
    if (!game_ea) return 0;
    int restored = 0;
    for (Entity* ent : *game_ea) {
        if (!ent) continue;
        auto* slot = GetSlot(ent->entity_id, ent);
        if (slot && slot->culled_by_heracles) {
            slot->culled_by_heracles = false;
            auto* c1 = ent->getById<Component_1_SceneRendering>(1);
            if (c1) {
                Vector4 target_pos = slot->has_nonzero_pos ? slot->latest_nonzero_pos : slot->pre_cull_pos;
                Vector4 target_rot = slot->has_nonzero_pos ? slot->latest_nonzero_rot : slot->pre_cull_rot;
                if (target_rot.X == 0.0f && target_rot.Y == 0.0f && target_rot.Z == 0.0f && target_rot.W == 0.0f) {
                    target_rot.W = 1.0f;
                }
                c1->SetScale(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
                c1->SetPosRotation(target_pos, target_rot);
                c1->SetVisible();
            }
            restored++;
        }
    }
    s_total_culled_props = 0;
    return restored;
}

void OriginClutterTracker::DumpReportToFile(EntityAdminBase* game_ea, const char* trigger_reason) {
    if (!game_ea) return;
    auto* world = get_system27_WorldEngineSystem(game_ea);
    __int64 map_id = world ? world->loaded_map_id : 0;
    int wstate = world ? (int)world->world_state : -1;

    std::string out;
    out.reserve(65536);
    out += "# Origin Clutter Runtime Diagnostic Log (Step 2C)\n\n";
    out += std::format("- **Trigger Reason**: `{}`\n", trigger_reason ? trigger_reason : "Auto");
    out += std::format("- **Loaded Map ID**: `0x{:016X}` (`world_state = {}`)\n", (unsigned long long)map_id, wstate);
    out += std::format("- **Case B (`s_block_zero_overwrites`)**: `{}` (Total Blocked Calls: `{}`)\n",
        s_block_zero_overwrites ? "ENABLED" : "DISABLED", s_total_zero_overwrites_blocked);
    out += std::format("- **Case A (`s_auto_cull_unplaced_origin_props`)**: `{}` (Total Culled Props: `{}`)\n\n",
        s_auto_cull_unplaced_origin_props ? "ENABLED" : "DISABLED", s_total_culled_props);

    int total_ents = 0;
    int total_nonzero = 0;
    int total_near_origin = 0;
    int total_had_overwrites = 0;

    for (Entity* ent : *game_ea) {
        if (!ent) continue;
        total_ents++;
        auto* slot = GetSlot(ent->entity_id, ent);
        if (slot && slot->has_nonzero_pos) total_nonzero++;
        if (slot && (slot->zero_overwrite_count > 0 || slot->direct_zero_overwrite_count > 0)) total_had_overwrites++;
        auto* c1 = ent->getById<Component_1_SceneRendering>(1);
        if (c1) {
            float abs_pos = fabsf(c1->position.X) + fabsf(c1->position.Y) + fabsf(c1->position.Z);
            if (abs_pos < 10.0f || (slot && slot->culled_by_heracles)) {
                total_near_origin++;
            }
        }
    }

    out += std::format("## Summary Counts\n- **Total `GameEntityAdmin` Entities**: `{}`\n", total_ents);
    out += std::format("- **Entities With Valid Non-Zero World Position**: `{}`\n", total_nonzero);
    out += std::format("- **Entities With Attempted Non-Zero -> `(0,0,0)` Overwrites (Case B)**: `{}`\n", total_had_overwrites);
    out += std::format("- **Entities Currently Within 10m of `(0,0,0)` (or Culled)**: `{}`\n\n", total_near_origin);

    out += "## 1. Entities Where Non-Zero Position Was Overwritten Toward `(0,0,0)` (Case B)\n";
    out += "| EntID | STU ID | Resload ID | Spawn Caller | First NonZero Pos | First Caller | Zero Overwrite Count | Last Zero Caller | Current Pos | Components |\n";
    out += "|---|---|---|---|---|---|---|---|---|---|\n";

    for (Entity* ent : *game_ea) {
        if (!ent) continue;
        auto* slot = GetSlot(ent->entity_id, ent);
        if (!slot || (slot->zero_overwrite_count == 0 && slot->direct_zero_overwrite_count == 0)) continue;

        __int64 res_id = (ent->resload_entry && ent->resload_entry->valid()) ? ent->resload_entry->align()->resource_id : 0;
        auto* c1 = ent->getById<Component_1_SceneRendering>(1);
        Vector4 cur_pos = c1 ? c1->position : Vector4{};

        std::string comps_str;
        for (int i = 0; i < ent->component_list.num - 1; i++) {
            if (ent->component_list.ptr[i]) {
                if (!comps_str.empty()) comps_str += ",";
                comps_str += std::format("{:02X}", (int)ent->component_list.ptr[i]->component_id);
            }
        }

        out += std::format("| `0x{:X}` | `0x{:X}` | `0x{:X}` | `+0x{:X}` | `({:.2f}, {:.2f}, {:.2f})` | `+0x{:X}` | `{}` (direct `{}`) | `+0x{:X}` | `({:.2f}, {:.2f}, {:.2f})` | `[{}]` |\n",
            ent->entity_id,
            (unsigned long long)slot->stu_id,
            (unsigned long long)res_id,
            (unsigned int)slot->spawn_caller_rva,
            slot->first_nonzero_pos.X, slot->first_nonzero_pos.Y, slot->first_nonzero_pos.Z,
            (unsigned int)slot->first_nonzero_caller_rva,
            slot->zero_overwrite_count, slot->direct_zero_overwrite_count,
            (unsigned int)slot->last_zero_overwrite_caller_rva,
            cur_pos.X, cur_pos.Y, cur_pos.Z,
            comps_str);
    }

    out += "\n## 2. All Entities Near `(0,0,0)` (`|X|+|Y|+|Z| < 10.0`) or Culled (Case A & Origin Entities)\n";
    out += "| EntID | STU ID | Resload ID | Spawn Caller | Loader InitPos | Current Pos | Parent C1 | HasNonZero | Culled | Components |\n";
    out += "|---|---|---|---|---|---|---|---|---|---|\n";

    for (Entity* ent : *game_ea) {
        if (!ent) continue;
        auto* c1 = ent->getById<Component_1_SceneRendering>(1);
        if (!c1) continue;
        auto* slot = GetSlot(ent->entity_id, ent);
        float abs_pos = fabsf(c1->position.X) + fabsf(c1->position.Y) + fabsf(c1->position.Z);
        if (abs_pos >= 10.0f && !(slot && slot->culled_by_heracles)) continue;

        __int64 res_id = (ent->resload_entry && ent->resload_entry->valid()) ? ent->resload_entry->align()->resource_id : 0;
        uintptr_t parent_c1 = *(uintptr_t*)((uintptr_t)c1 + 0x1D0);
        uint32 parent_entid = 0;
        if (parent_c1 != 0) {
            auto* p_comp = (Component_1_SceneRendering*)parent_c1;
            if (p_comp->base.entity_backref) parent_entid = p_comp->base.entity_backref->entity_id;
        }

        std::string comps_str;
        for (int i = 0; i < ent->component_list.num - 1; i++) {
            if (ent->component_list.ptr[i]) {
                if (!comps_str.empty()) comps_str += ",";
                comps_str += std::format("{:02X}", (int)ent->component_list.ptr[i]->component_id);
            }
        }

        Vector4 init_p = slot ? slot->loader_init_pos : Vector4{};
        out += std::format("| `0x{:X}` | `0x{:X}` | `0x{:X}` | `+0x{:X}` | `({:.2f}, {:.2f}, {:.2f})` | `({:.2f}, {:.2f}, {:.2f})` | `0x{:X}` | `{}` | `{}` | `[{}]` |\n",
            ent->entity_id,
            slot ? (unsigned long long)slot->stu_id : 0ULL,
            (unsigned long long)res_id,
            slot ? (unsigned int)slot->spawn_caller_rva : 0U,
            init_p.X, init_p.Y, init_p.Z,
            c1->position.X, c1->position.Y, c1->position.Z,
            parent_entid,
            (slot && slot->has_nonzero_pos) ? "YES" : "NO",
            (slot && slot->culled_by_heracles) ? "YES" : "NO",
            comps_str);
    }

    const char* paths[] = {
        "origin_clutter_diag.log"
    };
    for (const char* p : paths) {
        FILE* f = fopen(p, "wb");
        if (f) {
            fwrite(out.data(), 1, out.size(), f);
            fclose(f);
        }
    }
    printf("[OriginClutterTracker] Wrote origin_clutter_diag.log (%zu bytes, reason=%s)\n",
        out.size(), trigger_reason ? trigger_reason : "Auto");
}

void OriginClutterTracker::OnTick(EntityAdminBase* game_ea, int world_state) {
    if (!game_ea) return;
    auto* world = get_system27_WorldEngineSystem(game_ea);
    __int64 cur_map = world ? world->loaded_map_id : 0;
    if (cur_map != 0 && cur_map != s_last_loaded_map_id) {
        s_last_loaded_map_id = cur_map;
        s_dumped_report_for_current_map = false;
        s_ticks_since_world_ready = 0;
    }

    // Scan for direct comp1->position writes or newly initialized positions
    for (Entity* ent : *game_ea) {
        if (!ent) continue;
        auto* c1 = ent->getById<Component_1_SceneRendering>(1);
        if (!c1) continue;
        auto* slot = GetSlot(ent->entity_id, ent);
        if (!slot) continue;
        if (slot->culled_by_heracles) {
            if (c1->position.Y > -4000.0f) {
                Vector4 hidden_pos(0.0f, -5000.0f, 0.0f, 0.0f);
                Vector4 ident_rot(0.0f, 0.0f, 0.0f, 1.0f);
                c1->SetPosRotation(hidden_pos, ident_rot);
                c1->SetScale(Vector4(0.0f, 0.0f, 0.0f, 0.0f));
                c1->SetInvisible();
            }
            continue;
        }

        float abs_pos = fabsf(c1->position.X) + fabsf(c1->position.Y) + fabsf(c1->position.Z);
        uintptr_t parent_c1 = *(uintptr_t*)((uintptr_t)c1 + 0x1D0);
        if (parent_c1 == 0 && abs_pos > 0.5f && c1->position.Y > -4000.0f) {
            if (!slot->has_nonzero_pos) {
                slot->has_nonzero_pos = true;
                slot->first_nonzero_pos = c1->position;
                slot->first_nonzero_rot = c1->rotation;
                slot->first_nonzero_caller_rva = 0xDEAD0001;
            }
            slot->latest_nonzero_pos = c1->position;
            slot->latest_nonzero_rot = c1->rotation;
        } else if (abs_pos < 0.05f && slot->has_nonzero_pos && parent_c1 == 0) {
            bool is_player_or_weapon_ent = IsLocalPlayerOrWeaponEntity(game_ea, ent);
            if (!is_player_or_weapon_ent) {
                slot->direct_zero_overwrite_count++;
                if (s_block_zero_overwrites) {
                    slot->zero_overwrite_blocked_count++;
                    s_total_zero_overwrites_blocked++;
                    c1->SetPosRotation(slot->latest_nonzero_pos, slot->latest_nonzero_rot);
                }
            }
        }
    }

    if (world_state == 8) {
        if (s_auto_open_spawn_doors) {
            int total_doors = 0, open_doors = 0;
            GetSpawnDoorCounts(game_ea, total_doors, open_doors);
            if (total_doors > open_doors) {
                OpenSpawnDoors(game_ea);
            }
        }
        if (s_ticks_since_world_ready >= 0) {
            s_ticks_since_world_ready++;
            if (s_ticks_since_world_ready == 90 && !s_dumped_report_for_current_map) {
                s_dumped_report_for_current_map = true;
                if (s_auto_cull_unplaced_origin_props) {
                    CullUnplacedOriginProps(game_ea);
                }
            }
        }
    } else if (world_state >= 3 && world_state <= 7) {
        s_ticks_since_world_ready = 0;
        s_dumped_report_for_current_map = false;
    }
}

void WeaponHardpointDiag::ClearTrace() {
    for (int i = 0; i < WEAPON_DIAG_RING_SIZE; i++) {
        s_weapon_diag_ring[i] = WeaponDiagEntry{};
    }
    s_weapon_diag_seq = 0;
    s_total_events_logged = 0;
    s_fire_shot_calls = 0;
    s_ss_effect_calls = 0;
    s_attach_calls = 0;
    s_resolve_weapon_calls = 0;
    s_update_weapon_calls = 0;
    s_last_dump_status = "Cleared trace ring buffer.";
}

void WeaponHardpointDiag::ForceUpdateWeapons(EntityAdminBase* game_ea, uint8_t is_1p) {
    if (!game_ea) {
        s_last_dump_status = "No GameEntityAdmin!";
        return;
    }
    Entity* lc = game_ea->getLocalEnt();
    Entity* mdl = nullptr;
    if (lc) {
        if (auto* pet = lc->getById<Component_20_ModelReference>(0x20)) {
            uint32 mid = pet->cam_attach_entid ? pet->cam_attach_entid : pet->aim_entid;
            if (mid) mdl = game_ea->getEntById(mid);
        }
    }
    if (!mdl) {
        for (Entity* ent : *game_ea) {
            if (ent && ent->hasComponent(0x23) && ent->hasComponent(0x1C)) {
                mdl = ent;
                break;
            }
        }
    }
    if (!mdl) {
        s_last_dump_status = "Could not find local hero model entity!";
        return;
    }
    ComponentBase* c23 = mdl->getById(0x23);
    if (!c23) {
        s_last_dump_status = "Model entity has no Component_23!";
        return;
    }
    uintptr_t weapon_mgr = (uintptr_t)c23 + 0x1458;
    if (UpdateWeaponAttachments_orig) {
        UpdateWeaponAttachments_hook(weapon_mgr, is_1p);
        s_last_dump_status = std::format("Called UpdateWeaponAttachments(0x{:X}, is_1p={}) on model ent 0x{:X}.",
            (unsigned long long)weapon_mgr, is_1p, mdl->entity_id);
    }
}

void WeaponHardpointDiag::DumpReportToFile(EntityAdminBase* game_ea, const char* trigger_reason) {
    std::string out;
    out.reserve(131072);
    out += "# Muzzle Flash & Weapon Hardpoint Runtime Diagnostic Log\n\n";
    out += std::format("- **Trigger Reason**: `{}`\n", trigger_reason ? trigger_reason : "Manual");
    out += std::format("- **Counters**: `TotalEvents={}`, `FireShot(+0xD46060)={}`, `SSEffect(+0xD58110)={}`, `Attach(+0xA01430)={}`, `ResolveWeapon(+0xD560B0)={}`, `UpdateWeapons(+0xD563D0)={}`\n\n",
        s_total_events_logged, s_fire_shot_calls, s_ss_effect_calls, s_attach_calls, s_resolve_weapon_calls, s_update_weapon_calls);

    if (!game_ea) {
        out += "**ERROR: `GameEntityAdmin()` is NULL!**\n";
    } else {
        Entity* lc = game_ea->getLocalEnt();
        uint32 local_ctrl_id = game_ea->local_entid;
        uint32 local_mdl_id = 0;
        Entity* mdl = nullptr;
        if (lc) {
            if (auto* pet = lc->getById<Component_20_ModelReference>(0x20)) {
                local_mdl_id = pet->cam_attach_entid ? pet->cam_attach_entid : pet->aim_entid;
                if (local_mdl_id) mdl = game_ea->getEntById(local_mdl_id);
            }
        }
        if (!mdl) {
            for (Entity* ent : *game_ea) {
                if (ent && ent->hasComponent(0x23) && ent->hasComponent(0x1C) && ent->entity_id != local_ctrl_id) {
                    mdl = ent;
                    local_mdl_id = ent->entity_id;
                    break;
                }
            }
        }

        out += "## 1. Local Player Controller & Hero Model Overview\n";
        out += std::format("- **`game_ea->local_entid`**: `0x{:X}` (`lc = 0x{:X}`)\n", local_ctrl_id, (unsigned long long)lc);
        out += std::format("- **`local_mdl_id`**: `0x{:X}` (`mdl = 0x{:X}`)\n", local_mdl_id, (unsigned long long)mdl);

        // NOTE: Do NOT call +0xC5E8E0 or +0xC5F8B0 directly from prometheus.dll!
        // +0xC5E8E0 contains an obfuscated PE 'MZ' (0x5A4D) return-address check relative to the caller's code page.

        if (lc) {
            if (auto* c1 = lc->getById<Component_1_SceneRendering>(1)) {
                Vector4 wpos = SafeReadVal<Vector4>((uintptr_t)c1 + 0xC0);
                out += std::format("- **Controller `Component_1`**: `local_pos=({:.2f}, {:.2f}, {:.2f})`, `world_pos(+0xC0)=({:.2f}, {:.2f}, {:.2f})`, `parent_c1=0x{:X}`, `first_child=0x{:X}`\n",
                    c1->position.X, c1->position.Y, c1->position.Z, wpos.X, wpos.Y, wpos.Z,
                    SafeReadVal<uintptr_t>((uintptr_t)c1 + 0x1D0), SafeReadVal<uintptr_t>((uintptr_t)c1 + 0x1E0));
            }
        }

        if (mdl) {
            if (auto* c1 = mdl->getById<Component_1_SceneRendering>(1)) {
                Vector4 wpos = SafeReadVal<Vector4>((uintptr_t)c1 + 0xC0);
                uintptr_t c1c_link = SafeReadVal<uintptr_t>((uintptr_t)c1 + 0x1A8);
                uint64_t flags_1b8 = SafeReadVal<uint64_t>((uintptr_t)c1 + 0x1B8);
                uint64_t rflags_180 = SafeReadVal<uint64_t>((uintptr_t)c1 + 0x180);
                uintptr_t anim_inst = c1c_link ? SafeReadVal<uintptr_t>((c1c_link - 0x20) + 0x1F0) : 0;
                uintptr_t bone_mats = anim_inst ? SafeReadVal<uintptr_t>(anim_inst + 0x20) : 0;
                uint16_t bone_cnt = anim_inst ? SafeReadVal<uint16_t>(anim_inst + 0x40) : 0;
                out += std::format("- **Hero Model `Component_1` (`0x{:X}`)**:\n", (unsigned long long)c1);
                out += std::format("  - `local_pos(+0x60)`: `({:.2f}, {:.2f}, {:.2f})`, `world_pos(+0xC0)`: `({:.2f}, {:.2f}, {:.2f})`\n",
                    c1->position.X, c1->position.Y, c1->position.Z, wpos.X, wpos.Y, wpos.Z);
                out += std::format("  - `render_flags(+0x180)`: `0x{:016X}`, `comp1C_link(+0x1A8)`: `0x{:X}`, `internal_flags(+0x1B8)`: `0x{:016X}`\n",
                    rflags_180, c1c_link, flags_1b8);
                out += std::format("  - `anim_inst([comp1C+0x1F0])`: `0x{:X}`, `bone_matrices([anim+0x20])`: `0x{:X}`, `bone_count([anim+0x40])`: `{}`\n",
                    anim_inst, bone_mats, bone_cnt);
                out += std::format("  - `parent_c1(+0x1D0)`: `0x{:X}`, `first_child_c1(+0x1E0)`: `0x{:X}`\n",
                    SafeReadVal<uintptr_t>((uintptr_t)c1 + 0x1D0), SafeReadVal<uintptr_t>((uintptr_t)c1 + 0x1E0));
            }

            if (ComponentBase* c22 = mdl->getById(0x22)) {
                out += std::format("- **Hero Model `Component_22` (`0x{:X}`) QWORD dump (`+0x00..+0x60`)**:\n", (unsigned long long)c22);
                for (int off = 0; off <= 0x60; off += 8) {
                    uintptr_t val = SafeReadVal<uintptr_t>((uintptr_t)c22 + off);
                    uint32 ent_id_if_ent = 0;
                    if (off == 0x18 || off == 0x20 || off == 0x28) {
                        if (val > 0x10000) {
                            ent_id_if_ent = SafeReadVal<uint32>(val + 0x94);
                        }
                    }
                    out += std::format("  - `+0x{:02X}`: `0x{:016X}` (if Entity* -> `entid=0x{:X}`)\n", off, (unsigned long long)val, ent_id_if_ent);
                }
            } else {
                out += "- **Hero Model `Component_22`**: `NULL` (not present on model entity!)\n";
            }

            if (ComponentBase* c23 = mdl->getById(0x23)) {
                uintptr_t wmgr = (uintptr_t)c23 + 0x1458;
                uintptr_t entries_ptr = SafeReadVal<uintptr_t>(wmgr + 0x08);
                int32_t entries_cnt = SafeReadVal<int32_t>(wmgr + 0x10);
                out += std::format("- **Hero Model `Component_23 + 0x1458` Weapon Attachment Table (`0x{:X}`)**:\n", (unsigned long long)wmgr);
                out += std::format("  - `entries_ptr(+0x08)`: `0x{:X}`, `entries_count(+0x10)`: `{}`\n", (unsigned long long)entries_ptr, entries_cnt);
                for (int i = 0; i < entries_cnt && i < 16 && entries_ptr != 0; i++) {
                    uintptr_t e = entries_ptr + (uintptr_t)i * 0x250;
                    uint32 s0 = SafeReadVal<uint32>(e + 0x00);
                    uint32 s1 = SafeReadVal<uint32>(e + 0x04);
                    uint64 wh = SafeReadVal<uint64>(e + 0x168);
                    uint64 ml1p = SafeReadVal<uint64>(e + 0x170);
                    uint64 ml3p = SafeReadVal<uint64>(e + 0x178);
                    uint64 ed1p = SafeReadVal<uint64>(e + 0x180);
                    uint64 ed3p = SafeReadVal<uint64>(e + 0x188);
                    uint32 tent = SafeReadVal<uint32>(e + 0x190);
                    uint64 hp1p = SafeReadVal<uint64>(e + 0x1A0);
                    uint64 hp3p = SafeReadVal<uint64>(e + 0x1A8);
                    Vector4 poff = SafeReadVal<Vector4>(e + 0x1E0);
                    float sc1p = SafeReadVal<float>(e + 0x1F0);
                    float sc3p = SafeReadVal<float>(e + 0x1F4);
                    uint8_t b240 = SafeReadVal<uint8_t>(e + 0x240);
                    out += std::format("  - **Entry [{}] (`0x{:X}`)**:\n", i, (unsigned long long)e);
                    out += std::format("    - `slot0_entid(1P)`: `0x{:X}`, `slot1_entid(3P)`: `0x{:X}`, `weapon_handle(+0x168)`: `0x{:X}`\n", s0, s1, (unsigned long long)wh);
                    out += std::format("    - `model_look_1p(+0x170)`: `0x{:X}`, `model_look_3p(+0x178)`: `0x{:X}`\n", (unsigned long long)ml1p, (unsigned long long)ml3p);
                    out += std::format("    - `ent_def_1p(+0x180)`: `0x{:X}`, `ent_def_3p(+0x188)`: `0x{:X}`\n", (unsigned long long)ed1p, (unsigned long long)ed3p);
                    out += std::format("    - `target_entid(+0x190)`: `0x{:X}`, `hardpoint_1p(+0x1A0)`: `0x{:X}`, `hardpoint_3p(+0x1A8)`: `0x{:X}`\n", tent, (unsigned long long)hp1p, (unsigned long long)hp3p);
                    out += std::format("    - `pos_offset(+0x1E0)`: `({:.2f}, {:.2f}, {:.2f})`, `scale_1p`: `{:.2f}`, `scale_3p`: `{:.2f}`, `flag_240`: `{}`\n",
                        poff.X, poff.Y, poff.Z, sc1p, sc3p, b240);
                }
            }
        }

        out += "\n## 2. All Attached / Weapon / Effect / Player Entities in `GameEntityAdmin`\n";
        out += "| EntID | STU ID | Resload ID | SpawnCaller | Components | LocalPos (+0x60) | WorldPos (+0xC0) | ParentEnt | Hardpoint (+0x1E8) | BoneIdx (+0x1F4) | Inh/MW | C1_1A8 / 1B8 | Anim / Bones | ParentBonePos |\n";
        out += "|---|---|---|---|---|---|---|---|---|---|---|---|---|---|\n";

        for (Entity* ent : *game_ea) {
            if (!ent) continue;
            auto* c1 = ent->getById<Component_1_SceneRendering>(1);
            uintptr_t parent_c1 = c1 ? SafeReadVal<uintptr_t>((uintptr_t)c1 + 0x1D0) : 0;
            uint32 p_entid = 0;
            if (parent_c1 != 0) {
                Entity* p_ent = SafeReadVal<Entity*>(parent_c1 + 0x08);
                if (p_ent) p_entid = SafeReadVal<uint32>((uintptr_t)p_ent + 0x94);
            }
            auto* slot = OriginClutterTracker::GetSlot(ent->entity_id, ent);
            uintptr_t sp_caller = slot ? slot->spawn_caller_rva : 0;

            bool interesting =
                ent->entity_id == local_ctrl_id ||
                ent->entity_id == local_mdl_id ||
                (local_ctrl_id != 0 && ent->entity_id >= local_ctrl_id && (
                    ent->hasComponent(0x1D) ||
                    ent->hasComponent(0x20) ||
                    ent->hasComponent(0x22) ||
                    sp_caller == 0xD56BFB ||
                    sp_caller == 0xD472FF ||
                    sp_caller == 0xD35A74 ||
                    sp_caller == 0xCBA83F ||
                    p_entid == local_ctrl_id ||
                    p_entid == local_mdl_id));
            if (!interesting) continue;

            __int64 stu_id = slot ? slot->stu_id : 0;
            __int64 res_id = (ent->resload_entry && ent->resload_entry->valid()) ? ent->resload_entry->align()->resource_id : 0;

            std::string comps_str;
            for (int i = 0; i < ent->component_list.num - 1; i++) {
                if (ent->component_list.ptr[i]) {
                    if (!comps_str.empty()) comps_str += ",";
                    comps_str += std::format("{:02X}", (int)ent->component_list.ptr[i]->component_id);
                }
            }

            Vector4 lpos = c1 ? c1->position : Vector4{};
            Vector4 wpos = c1 ? SafeReadVal<Vector4>((uintptr_t)c1 + 0xC0) : Vector4{};
            std::string p_bone_pos_str = "-";
            uint64_t hp_id = c1 ? SafeReadVal<uint64_t>((uintptr_t)c1 + 0x1E8) : 0;
            uint16_t bone_idx = c1 ? SafeReadVal<uint16_t>((uintptr_t)c1 + 0x1F4, 0xFFFF) : 0xFFFF;
            uint8_t inh_rot = c1 ? SafeReadVal<uint8_t>((uintptr_t)c1 + 0x1F6) : 0;
            uint8_t maint_w = c1 ? SafeReadVal<uint8_t>((uintptr_t)c1 + 0x1F7) : 0;
            uintptr_t c1_1a8 = c1 ? SafeReadVal<uintptr_t>((uintptr_t)c1 + 0x1A8) : 0;
            uint64_t c1_1b8 = c1 ? SafeReadVal<uint64_t>((uintptr_t)c1 + 0x1B8) : 0;
            uintptr_t anim_inst = c1_1a8 ? SafeReadVal<uintptr_t>((c1_1a8 - 0x20) + 0x1F0) : 0;
            uint16_t bone_cnt = anim_inst ? SafeReadVal<uint16_t>(anim_inst + 0x40) : 0;

            if (parent_c1 != 0) {
                auto* pc = (Component_1_SceneRendering*)parent_c1;
                uintptr_t p_1a8 = SafeReadVal<uintptr_t>((uintptr_t)pc + 0x1A8);
                uintptr_t p_anim = p_1a8 ? SafeReadVal<uintptr_t>((p_1a8 - 0x20) + 0x1F0) : 0;
                uintptr_t p_bmats = p_anim ? SafeReadVal<uintptr_t>(p_anim + 0x20) : 0;
                uint16_t p_bcnt = p_anim ? SafeReadVal<uint16_t>(p_anim + 0x40) : 0;
                if (p_bmats && bone_idx < p_bcnt) {
                    uintptr_t bmat = p_bmats + (uintptr_t)bone_idx * 0x30;
                    Vector4 bpos = SafeReadVal<Vector4>(bmat + 0x20);
                    p_bone_pos_str = std::format("({:.2f},{:.2f},{:.2f})", bpos.X, bpos.Y, bpos.Z);
                }
            }

            out += std::format("| `0x{:X}` | `0x{:X}` | `0x{:X}` | `+0x{:X}` | `[{}]` | `({:.2f},{:.2f},{:.2f})` | `({:.2f},{:.2f},{:.2f})` | `0x{:X}` | `0x{:X}` | `{}` | `{}/{}` | `0x{:X}` / `0x{:X}` | `0x{:X}` (`{}`) | `{}` |\n",
                ent->entity_id, (unsigned long long)stu_id, (unsigned long long)res_id, (unsigned int)sp_caller,
                comps_str,
                lpos.X, lpos.Y, lpos.Z,
                wpos.X, wpos.Y, wpos.Z,
                p_entid, (unsigned long long)hp_id, bone_idx, inh_rot, maint_w,
                (unsigned long long)c1_1a8, (unsigned long long)c1_1b8,
                (unsigned long long)anim_inst, bone_cnt,
                p_bone_pos_str);
        }
    }

    out += "\n## 3. Chronological Ring Buffer Trace (Last 60 Events)\n```text\n";
    LONG cur_seq = s_weapon_diag_seq;
    LONG start_seq = (cur_seq > 60) ? (cur_seq - 60 + 1) : 1;
    for (LONG s = start_seq; s <= cur_seq; s++) {
        const WeaponDiagEntry& e = s_weapon_diag_ring[(s - 1) & (WEAPON_DIAG_RING_SIZE - 1)];
        if (e.seq == (uint32_t)s && e.text[0] != '\0') {
            out += std::format("[#{:04d}] {}\n", (int)e.seq, (const char*)e.text);
        }
    }
    out += "```\n";

    const char* paths[] = {
        "weapon_hardpoint_diag.log"
    };
    bool wrote_ok = false;
    for (const char* p : paths) {
        FILE* f = fopen(p, "wb");
        if (f) {
            fwrite(out.data(), 1, out.size(), f);
            fclose(f);
            wrote_ok = true;
        }
    }
    s_last_dump_status = std::format("Wrote weapon_hardpoint_diag.log ({} bytes, {} trace events, ok={})",
        out.size(), (int)cur_seq, wrote_ok ? "YES" : "NO");
    printf("[WeaponHardpointDiag] %s\n", s_last_dump_status.c_str());
}

static void AppendSingleVarBagEntries(std::string& out, StatescriptVar_VarBag* vb, int depth = 0) {
    if (!vb || depth > 2 || IsBadReadPtr(vb, sizeof(StatescriptVar_VarBag)) || !vb->base.is_varbag()) return;
    int count = 0;
    for (int i = 0; i < 16; i++) {
        auto* item = &vb->sub_items[i];
        if (!item || !item->arr_ptr || item->item_count <= 0 || item->item_count > 256 || IsBadReadPtr(item->arr_ptr, (size_t)item->item_count * sizeof(StatescriptVar_VarBag::VarBag_Item))) continue;
        for (int j = 0; j < item->item_count; j++) {
            StatescriptVar_Base* rid = item->arr_ptr[j].ptr;
            if (!rid || IsBadReadPtr(rid, sizeof(StatescriptVar_Base))) continue;
            if (rid->is_cvd()) {
                auto* prim = (StatescriptVar_Primitive*)rid;
                if (prim->global_var.type == StatescriptPrimitive_FLT) {
                    out.append(std::format("[0x{:X}:flt={:.2f}] ", (unsigned long long)rid->var_id, *(float*)&prim->global_var.value));
                } else {
                    out.append(std::format("[0x{:X}:t{}=0x{:X}] ", (unsigned long long)rid->var_id, (int)prim->global_var.type, (unsigned long long)prim->global_var.value));
                }
            } else if (rid->is_varbag()) {
                out.append(std::format("[0x{:X}:bag={{", (unsigned long long)rid->var_id));
                AppendSingleVarBagEntries(out, (StatescriptVar_VarBag*)rid, depth + 1);
                out.append("}] ");
            } else if (rid->is_dictvar()) {
                out.append(std::format("[0x{:X}:dict] ", (unsigned long long)rid->var_id));
            } else if (rid->is_siref()) {
                out.append(std::format("[0x{:X}:siref] ", (unsigned long long)rid->var_id));
            } else {
                out.append(std::format("[0x{:X}:other] ", (unsigned long long)rid->var_id));
            }
            count++;
        }
    }
    if (count == 0) out += "(empty)";
}

static uintptr_t SafeRva(uintptr_t addr) {
    return (addr >= globals::gameBase && addr < globals::gameBase + 0x3000000ULL) ? (addr - globals::gameBase) : addr;
}

static std::string FmtRvaShort(uintptr_t addr) {
    if (!addr) return "0";
    uintptr_t rva = SafeRva(addr);
    if (rva == 0x5A0C20) return "ret0";
    return std::format("+0x{:X}", rva);
}

static const char* ResolveSSNodeName(uint32_t hash) {
    switch (hash) {
    case 0x2F937AE3: return "Entry:GameMsg";
    case 0x902C07FC: return "Entry:Start";
    case 0xC71FD31C: return "Entry:Event";
    case 0xE811ECBB: return "Cond:BoolSwitch";
    case 0xCD462C0E: return "Cond:CmpSwitch";
    case 0xD5411AC4: return "Cond:FilterSwitch";
    case 0x7555AC24: return "Act:SendEvent";
    case 0xF9EF59A6: return "Act:PlayScript";
    case 0x8CD9A3B0: return "Act:StopScript";
    case 0xAD6D8308: return "Act:ResetState";
    case 0xBA2A3B19: return "Act:WaitDelay";
    case 0x558B92CA: return "Act:ClientSimTick";
    case 0xA329F060: return "Act:SetVarClient";
    case 0xB3934B2D: return "Act:ModifyVar";
    case 0xFB58999F: return "Act:ClearVar";
    case 0xA1ABA697: return "Act:PlayAnim";
    case 0xB2301AC1: return "Act:PlaySound";
    case 0x4935CA8D: return "Act:CamFX";
    case 0x1CA9A2AC: return "Act:Rumble";
    case 0x4E83BC01: return "SRV_Act:SetVar";
    case 0x04CC5359: return "SRV_Act:DmgHeal";
    case 0x5C46D801: return "SRV_Act:DestroyEnt";
    case 0x1BB99C36: return "SRV_Act:CreateEntA";
    case 0x4CCED352: return "SRV_Act:CreateEntB";
    case 0x0E3689F3: return "SRV_Act:CreateEntC";
    case 0xEA036C1F: return "SRV_Act:CombatAmmo";
    case 0xC75B4AD2: return "SRV_Act:UltCharge";
    case 0xFC39E2F1: return "SRV_Act:RespawnTele";
    case 0xC5E146B4: return "SRV_Act:TeamObj";
    case 0xF53C5DA8: return "State:LogicalBtn";
    case 0xF7D6498C: return "State:StateStack(Mtx)";
    case 0x860EE5FC: return "State:WeaponVolley";
    case 0x6E81F6D4: return "State:Effect";
    case 0xCD396790: return "State:WaitTimer";
    case 0x3DB862C4: return "State:Subscript";
    case 0xC17B255D: return "State:MoveImpulse";
    case 0x2A0FB7E6: return "State:DirDash";
    case 0x619D9E95: return "State:MeleeSweep";
    case 0xC2A01675: return "State:TargetSweep";
    case 0x6A6C9D8B: return "State:ChannelHold";
    case 0x8A71D6B6: return "State:StatusMod";
    case 0x3E7B1C90: return "State:Barrier";
    case 0x72596FBE: return "State:RecallRewind";
    case 0xA6D7759E: return "State:BlinkTele";
    case 0x2E3B1980: return "State:HealTether";
    case 0xFD0D8424: return "State:Deployable";
    case 0x5F1C8A02: return "State:HookTether";
    case 0x237949CC: return "State:HealthWatch";
    default: break;
    }
    auto it = allmighty_hash_lib::hashes.find(hash);
    return (it != allmighty_hash_lib::hashes.end()) ? it->second.c_str() : "?";
}

static std::string DecodeConfigVarCompact(uintptr_t obj_addr, uint32_t chash) {
    if (!obj_addr || IsBadReadPtr((const void*)obj_addr, 0x18)) return "?";
    if (chash == 0xF8CC195D) { // STUConfigVarDynamic
        auto* cvd = (STUConfigVarDynamic*)obj_addr;
        std::string pstr;
        if (!IsBadReadPtr(cvd, sizeof(STUConfigVarDynamic)) && cvd->item_location.list() && !IsBadReadPtr(cvd->item_location.list(), 8)) {
            for (int pi = 0; pi < cvd->item_location.count() && pi < 4; pi++) {
                if (pi > 0) pstr += "/";
                pstr += std::format("0x{:X}", (unsigned long long)cvd->item_location.list()[pi].resource_id);
            }
        }
        return std::format("CVD({})", pstr);
    }
    if (chash == 0x92C535C6) { // STUStatescriptDataStoreBase
        uint64_t rid10 = SafeReadVal<uint64_t>(obj_addr + 0x10);
        uint64_t rid08 = SafeReadVal<uint64_t>(obj_addr + 0x08);
        return std::format("DS(0x{:X})", rid10 ? rid10 : rid08);
    }
    if (chash == 0xE476E0B4) { // STUConfigVarFloat
        return std::format("Flt({:.2f})", SafeReadVal<float>(obj_addr + 0x10));
    }
    if (chash == 0x636E86DD) { // STUConfigVarInt
        return std::format("Int({})", SafeReadVal<int32_t>(obj_addr + 0x10));
    }
    if (chash == 0xB233D29C) { // STUConfigVarBool
        return std::format("Bool({})", SafeReadVal<uint8_t>(obj_addr + 0x10));
    }
    uint64_t c10 = SafeReadVal<uint64_t>(obj_addr + 0x10);
    float cf10 = SafeReadVal<float>(obj_addr + 0x10);
    if ((c10 >> 48) != 0 && (c10 >> 48) < 0x1000) {
        return std::format("0x{:08X}(res=0x{:X})", chash, c10);
    }
    return std::format("0x{:08X}(+10=0x{:X}/{:.2f})", chash, c10, cf10);
}

static void AppendVarBagSummary(std::string& out, const char* prefix, Entity* ent, bool dump_script_nodes = false) {
    if (!ent) return;
    auto* ss = ent->getById<Component_23_Statescript>(0x23);
    if (!ss || !ss->ss_inner.rid_entity_varbag || IsBadReadPtr(ss->ss_inner.rid_entity_varbag, sizeof(StatescriptVar_VarBag))) return;
    auto* vb = ss->ss_inner.rid_entity_varbag;
    uintptr_t ss_core = (uintptr_t)ss + 0xC0;
    uint8_t auth_374 = SafeReadVal<uint8_t>(ss_core + 0x374);
    out.append(std::format("  - **{} `Component_23` VarBag (`0x{:X}`, `auth_374={}`, `is_aim={}`, `scope={}`)**: ",
        prefix, (unsigned long long)vb, auth_374, (int)ss->ss_inner.is_aim_ent, (int)ss->ss_inner.entity_admin_scope));
    AppendSingleVarBagEntries(out, vb, 0);
    out += "\n";

    if (dump_script_nodes) {
        // A. Dump 8-Bucket Ability Mutex Priority Queue (comp23 + 0x10, each bucket = 0x18 bytes)
        std::string mtx_out;
        for (int b = 0; b < 8; b++) {
            uintptr_t b_addr = (uintptr_t)ss + 0x10 + (uintptr_t)b * 0x18;
            uintptr_t entry = SafeReadVal<uintptr_t>(b_addr + 0x00);
            int guard = 0;
            while (entry && !IsBadReadPtr((const void*)entry, 0x20) && guard++ < 16) {
                uint64_t mkey = SafeReadVal<uint64_t>(entry + 0x00);
                uintptr_t head_st = SafeReadVal<uintptr_t>(entry + 0x10);
                uintptr_t tail_st = SafeReadVal<uintptr_t>(entry + 0x18);
                if (mkey || head_st || tail_st) {
                    mtx_out.append(std::format("      * `Bucket[{}]` `key=0x{:X}` `head=0x{:X}` `tail=0x{:X}`: ",
                        b, (unsigned long long)mkey, (unsigned long long)head_st, (unsigned long long)tail_st));
                    uintptr_t cur_st = head_st;
                    int sguard = 0;
                    while (cur_st && !IsBadReadPtr((const void*)cur_st, 0xC0) && sguard++ < 16) {
                        auto* st_obj = (StatescriptState*)cur_st;
                        auto* st_inst = (StatescriptInstance*)SafeReadVal<uintptr_t>(cur_st + 0x10);
                        int16_t st_iid = (st_inst && !IsBadReadPtr(st_inst, 0x260)) ? st_inst->instance_id : 0;
                        uint64_t st_scid = (st_inst && !IsBadReadPtr(st_inst, 0x260)) ? st_inst->script_id : 0;
                        STUStatescriptGraph* st_gr = (st_inst && !IsBadReadPtr(st_inst, 0x260)) ? st_inst->graph : nullptr;
                        STUStatescriptState* stu_s = (st_gr && !IsBadReadPtr(st_gr, sizeof(STUStatescriptGraph))) ? st_gr->STUFromState(st_obj) : nullptr;
                        int n_idx = (stu_s && !IsBadReadPtr(stu_s, sizeof(STUStatescriptBase))) ? stu_s->ss_base.idx_in_nodes : -1;
                        STUInfo* si_info = (stu_s && !IsBadReadPtr(stu_s, 8) && stu_s->ss_base.graph_node.base.vfptr && !IsBadReadPtr(stu_s->ss_base.graph_node.base.vfptr, 8))
                            ? stu_s->ss_base.graph_node.base.vfptr->GetSTUInfo() : nullptr;
                        uint32_t sh = (si_info && !IsBadReadPtr(si_info, sizeof(STUInfo))) ? si_info->name_hash : 0;
                        float prio = SafeReadVal<float>(cur_st + 0xB8);
                        bool is_tail = (cur_st == tail_st);
                        mtx_out.append(std::format("[{}inst={} sc=0x{:X} node={} stu=0x{:08X}({}) flg=0x{:X} prio={:.1f}] ",
                            is_tail ? "TAIL:" : "", st_iid, (unsigned long long)(st_scid & 0xFFFFULL), n_idx, sh, ResolveSSNodeName(sh),
                            (uint32_t)st_obj->state_flags, prio));
                        cur_st = SafeReadVal<uintptr_t>(cur_st + 0xA0);
                    }
                    mtx_out += "\n";
                }
                entry = SafeReadVal<uintptr_t>(entry + 0x08);
            }
        }
        if (!mtx_out.empty()) {
            out.append(std::format("    - **{} Ability Mutex Queue (`comp23+0x10`)**:\n{}", prefix, mtx_out));
        }

        // B. Dump comp23 + 0x1458 Child Visual/Weapon Slot Records (stride 0x250)
        uintptr_t arr_1458 = SafeReadVal<uintptr_t>((uintptr_t)ss + 0x1458);
        int32_t cnt_1458 = SafeReadVal<int32_t>((uintptr_t)ss + 0x1460);
        if (arr_1458 && cnt_1458 > 0 && cnt_1458 <= 32 && !IsBadReadPtr((const void*)arr_1458, (size_t)cnt_1458 * 0x250)) {
            std::string slots_str;
            for (int32_t si = 0; si < cnt_1458; si++) {
                uintptr_t rec = arr_1458 + (uintptr_t)si * 0x250;
                uint32_t e0 = SafeReadVal<uint32_t>(rec + 0x00);
                uint32_t e1 = SafeReadVal<uint32_t>(rec + 0x04);
                uint64_t key = SafeReadVal<uint64_t>(rec + 0x230);
                uint32_t fxh = SafeReadVal<uint32_t>(rec + 0x238);
                if ((e0 && e0 != 0xFFFFFFFF) || (e1 && e1 != 0xFFFFFFFF) || key || fxh) {
                    slots_str.append(std::format("[slot{}:e0=0x{:X},e1=0x{:X},key=0x{:X},fx=0x{:X}] ", si, e0, e1, (unsigned long long)key, fxh));
                }
            }
            if (!slots_str.empty()) {
                out.append(std::format("    - **{} Child Slots (`comp23+0x1458`, cnt={})**: {}\n", prefix, cnt_1458, slots_str));
            }
        }
    }

    // Also dump script-instance VarBags at ss_core + 0x120 (g3_arrWithIds: StatescriptInstance* has VarBags at +0x98, +0x270, +0x280)
    uintptr_t inst_arr = SafeReadVal<uintptr_t>(ss_core + 0x120);
    int32_t inst_cnt = SafeReadVal<int32_t>(ss_core + 0x128);
    if (inst_arr && inst_cnt > 0 && inst_cnt < 128 && !IsBadReadPtr((const void*)inst_arr, (size_t)inst_cnt * 16)) {
        for (int32_t k = 0; k < inst_cnt; k++) {
            uintptr_t sub_inst = SafeReadVal<uintptr_t>(inst_arr + (uintptr_t)k * 16 + 0x00);
            int16_t scope_id = SafeReadVal<int16_t>(inst_arr + (uintptr_t)k * 16 + 0x08);
            if (!sub_inst || IsBadReadPtr((const void*)sub_inst, 0x290)) continue;
            uint64_t script_id = SafeReadVal<uint64_t>(sub_inst + 0x150);
            uint32_t exec_flags = SafeReadVal<uint32_t>(sub_inst + 0x258);
            int16_t parent_id = SafeReadVal<int16_t>(sub_inst + 0x1A);
            const int vb_offs[3] = { 0x98, 0x270, 0x280 };
            for (int m = 0; m < 3; m++) {
                auto* sub_vb = (StatescriptVar_VarBag*)SafeReadVal<uintptr_t>(sub_inst + vb_offs[m]);
                if (!sub_vb || IsBadReadPtr(sub_vb, sizeof(StatescriptVar_VarBag)) || !sub_vb->base.is_varbag()) continue;
                out.append(std::format("    - **{} InstVarBag `inst_id={}` (`script=0x{:X}`, `parent={}`, `exec=0x{:X}`, `+0x{:X}`, `bag_id=0x{:X}`)**: ",
                    prefix, scope_id, (unsigned long long)script_id, parent_id, exec_flags, vb_offs[m], (unsigned long long)sub_vb->base.var_id));
                AppendSingleVarBagEntries(out, sub_vb, 0);
                out += "\n";
            }
            if (!dump_script_nodes) continue;
            auto* inst_obj = (StatescriptInstance*)sub_inst;
            STUStatescriptGraph* graph = (STUStatescriptGraph*)SafeReadVal<uintptr_t>(sub_inst + 0x160);
            if (!graph || IsBadReadPtr(graph, sizeof(STUStatescriptGraph))) continue;
            const auto& sc_state_map = statescript_logger::GetScriptState(inst_obj);

            // 1. Dump runtime state_impl_list (+0xA0)
            uintptr_t st_arr = SafeReadVal<uintptr_t>(sub_inst + 0xA0);
            int32_t st_cnt = SafeReadVal<int32_t>(sub_inst + 0xA8);
            if (st_arr && st_cnt > 0 && st_cnt < 128 && !IsBadReadPtr((const void*)st_arr, (size_t)st_cnt * 8)) {
                out.append(std::format("      - **Runtime `state_impl_list` (`inst_id={}`, `script=0x{:X}`, `count={}`)**:\n",
                    scope_id, (unsigned long long)script_id, st_cnt));
                for (int32_t si = 0; si < st_cnt; si++) {
                    auto* st = (StatescriptState*)SafeReadVal<uintptr_t>(st_arr + (uintptr_t)si * 8);
                    if (!st || IsBadReadPtr(st, 0xC0)) continue;
                    uintptr_t svt = SafeReadVal<uintptr_t>((uintptr_t)st);
                    uintptr_t fn110 = svt ? SafeReadVal<uintptr_t>(svt + 0x110) : 0;
                    uintptr_t fn170 = svt ? SafeReadVal<uintptr_t>(svt + 0x170) : 0;
                    uintptr_t fn178 = svt ? SafeReadVal<uintptr_t>(svt + 0x178) : 0;
                    uintptr_t fn188 = svt ? SafeReadVal<uintptr_t>(svt + 0x188) : 0;
                    uintptr_t fn190 = svt ? SafeReadVal<uintptr_t>(svt + 0x190) : 0;
                    uintptr_t fn260 = svt ? SafeReadVal<uintptr_t>(svt + 0x260) : 0;
                    STUStatescriptState* stu_st = graph->STUFromState(st);
                    int node_idx = (stu_st && !IsBadReadPtr(stu_st, sizeof(STUStatescriptBase))) ? stu_st->ss_base.idx_in_nodes : -1;
                    STUInfo* s_info = (stu_st && !IsBadReadPtr(stu_st, 8) && stu_st->ss_base.graph_node.base.vfptr && !IsBadReadPtr(stu_st->ss_base.graph_node.base.vfptr, 8))
                        ? stu_st->ss_base.graph_node.base.vfptr->GetSTUInfo() : nullptr;
                    uint32_t s_hash = (s_info && !IsBadReadPtr(s_info, sizeof(STUInfo))) ? s_info->name_hash : 0;
                    const char* s_name = ResolveSSNodeName(s_hash);
                    uintptr_t svt_rva = SafeRva(svt);
                    uintptr_t fn170_rva = SafeRva(fn170);
                    const char* vt_tag = (svt_rva == 0x15CCFC0 || svt_rva == 0x15CA730) ? "[STRIPPED_ST]"
                        : (fn170_rva == 0xF43450) ? "[GRP1_STUB]"
                        : (fn170_rva != 0x5A0C20 && fn170_rva != 0xF495C0) ? "[SUBCLASS_START]" : "";
                    uint64_t mkey = SafeReadVal<uint64_t>((uintptr_t)st + 0x90);
                    float mprio = SafeReadVal<float>((uintptr_t)st + 0xB8);
                    std::string mtx_tag = mkey ? std::format(" `mtx=0x{:X}(prio={:.1f})`", (unsigned long long)mkey, mprio) : "";
                    out.append(std::format("        * `state_impl[{}]`: `m_st={}` `node={}` `stu=0x{:08X}({})` `cO={}` `sO={}` `flags(+0x40)=0x{:X}` `f44={}` `f80={}`{} `vt=+0x{:X}`{} (`170={}`,`178={}`,`188={}`,`190={}`,`260={}`,`110={}`)\n",
                        si, st->idx_in_m_states, node_idx, s_hash, s_name,
                        stu_st ? (int)stu_st->ss_base.m_clientOnly : -1, stu_st ? (int)stu_st->ss_base.m_serverOnly : -1,
                        (uint32_t)st->state_flags, st->field_44, (int)st->field_80, mtx_tag,
                        svt_rva, vt_tag,
                        FmtRvaShort(fn170), FmtRvaShort(fn178), FmtRvaShort(fn188), FmtRvaShort(fn190), FmtRvaShort(fn260), FmtRvaShort(fn110)));
                }
            }

            // 2. Dump graph->m_nodes and runtime action_impl_list (+0xD0)
            uintptr_t act_arr = SafeReadVal<uintptr_t>(sub_inst + 0xD0);
            int32_t act_cnt = SafeReadVal<int32_t>(sub_inst + 0xD8);
            int node_cnt = graph->m_nodes.count();
            if (node_cnt > 0 && node_cnt < 256 && graph->m_nodes.list() && !IsBadReadPtr(graph->m_nodes.list(), (size_t)node_cnt * 8)) {
                out.append(std::format("      - **Graph `m_nodes` & `action_impl_list` (`inst_id={}`, `nodes={}`, `actions={}`)**:\n",
                    scope_id, node_cnt, act_cnt));
                for (int ni = 0; ni < node_cnt; ni++) {
                    STUStatescriptBase* node = graph->m_nodes.list()[ni];
                    if (!node || IsBadReadPtr(node, sizeof(STUStatescriptBase))) continue;
                    STUInfo* n_info = (node->graph_node.base.vfptr && !IsBadReadPtr(node->graph_node.base.vfptr, 8))
                        ? node->graph_node.base.vfptr->GetSTUInfo() : nullptr;
                    uint32_t n_hash = (n_info && !IsBadReadPtr(n_info, sizeof(STUInfo))) ? n_info->name_hash : 0;
                    const char* n_name = ResolveSSNodeName(n_hash);
                    uintptr_t act_ptr = (act_arr && ni < act_cnt && !IsBadReadPtr((const void*)(act_arr + (uintptr_t)ni * 8), 8))
                        ? SafeReadVal<uintptr_t>(act_arr + (uintptr_t)ni * 8) : 0;
                    uintptr_t avt = (act_ptr && !IsBadReadPtr((const void*)act_ptr, 8)) ? SafeReadVal<uintptr_t>(act_ptr) : 0;
                    uintptr_t fnF0 = avt ? SafeReadVal<uintptr_t>(avt + 0xF0) : 0;
                    uintptr_t fnF8 = avt ? SafeReadVal<uintptr_t>(avt + 0xF8) : 0;
                    uintptr_t fn100 = avt ? SafeReadVal<uintptr_t>(avt + 0x100) : 0;
                    auto sit = sc_state_map.find(node->idx_in_nodes);
                    const char* n_stat = (sit == sc_state_map.end()) ? "NEVER" : (sit->second.is_finish_state ? "FINISHED" : "ACTIVE");
                    int64_t n_ts = (sit != sc_state_map.end()) ? sit->second.change_timestamp : 0;

                    bool has_unreached_succ = false;
                    std::string links_str;
                    auto plug_map = node->get_output_plugs();
                    for (auto& pe : plug_map) {
                        STUArgumentInfo* parg = (n_info && !IsBadReadPtr(n_info, sizeof(STUInfo))) ? n_info->argumentByHash(pe.first) : nullptr;
                        uint32_t poff = (parg && !IsBadReadPtr(parg, sizeof(STUArgumentInfo))) ? parg->offset : 0;
                        const char* ptag = (poff == 0x58) ? "Beg" : (poff == 0x60) ? "End" : nullptr;
                        for (auto* plug : pe.second) {
                            if (!plug || IsBadReadPtr(plug, sizeof(STUGraphPlug))) continue;
                            for (auto* lnk : plug->m_links) {
                                if (!lnk || IsBadReadPtr(lnk, sizeof(STUGraphLink))) continue;
                                if (lnk->m_inputPlug && !IsBadReadPtr(lnk->m_inputPlug, sizeof(STUStatescriptInputPlug))) {
                                    auto* tgt = lnk->m_inputPlug->plug_base.m_parentNode;
                                    if (tgt && !IsBadReadPtr(tgt, sizeof(STUStatescriptBase))) {
                                        if (sc_state_map.find(tgt->idx_in_nodes) == sc_state_map.end()) has_unreached_succ = true;
                                        if (ptag) links_str += std::format("{}(+0x{:X})->{} ", ptag, poff, tgt->idx_in_nodes);
                                        else links_str += std::format("+0x{:X}->{} ", poff, tgt->idx_in_nodes);
                                    }
                                }
                            }
                        }
                    }

                    std::string diag_tags;
                    if (sit != sc_state_map.end() && (!sit->second.is_finish_state || has_unreached_succ)) {
                        diag_tags += "[FRONTIER] ";
                    }
                    if (!node->m_isState && avt == 0 && n_hash != 0x2F937AE3 && n_hash != 0x902C07FC && n_hash != 0xC71FD31C && n_hash != 0xE811ECBB && n_hash != 0xCD462C0E && n_hash != 0xD5411AC4) {
                        diag_tags += "[STRIPPED_ACT] ";
                    }
                    if (n_hash == 0xCD396790 && SafeReadVal<uintptr_t>((uintptr_t)node + 0x108) == 0) {
                        diag_tags += "[NULL_TIMER_0x108] ";
                    }

                    std::string fields_str;
                    if (n_info && !IsBadReadPtr(n_info, sizeof(STUInfo))) {
                        for (auto ap : *n_info) {
                            STUArgumentInfo* arg = ap.second;
                            if (!arg || IsBadReadPtr(arg, sizeof(STUArgumentInfo))) continue;
                            if (arg->offset < 0x78) continue; // Skip base STUGraphNode / STUStatescriptBase layout noise!
                            uintptr_t fa = (uintptr_t)node + arg->offset;
                            uint64_t fraw = SafeReadVal<uint64_t>(fa);
                            if (!fraw) continue;
                            STUConstraintType ct = (arg->constraint && !IsBadReadPtr(arg->constraint, 8))
                                ? arg->constraint->get_type_flag() : (STUConstraintType)-1;
                            if (ct == STU_ConstraintType_Object && fraw > 0x10000ULL && !IsBadReadPtr((const void*)fraw, 0x20)) {
                                STUBase<>* ch = (STUBase<>*)fraw;
                                STUInfo* ci = (ch->vfptr && !IsBadReadPtr(ch->vfptr, 8) && ch->vfptr->GetSTUInfo) ? ch->vfptr->GetSTUInfo() : nullptr;
                                uint32_t chash = (ci && !IsBadReadPtr(ci, sizeof(STUInfo))) ? ci->name_hash : 0;
                                if (chash == 0x3E490486 || chash == 0x11D7F052 || chash == 0x24AB1F0B || chash == 0xB8F35B20) continue; // skip plugs
                                fields_str += std::format("[+0x{:X}:{}] ", arg->offset, DecodeConfigVarCompact(fraw, chash));
                            } else if (ct == STU_ConstraintType_BSList_Object || ct == STU_ConstraintType_BSList_InlinedObject || ct == STU_ConstraintType_BSList_ResourceRef) {
                                auto* bs = (STUBullshitListFull<uint64_t>*)fa;
                                if (!IsBadReadPtr(bs, sizeof(*bs)) && bs->list_ptr && !IsBadReadPtr(bs->list_ptr, 16)) {
                                    int cnt = bs->count();
                                    if (cnt > 0) fields_str += std::format("[+0x{:X}:list(cnt={})] ", arg->offset, cnt);
                                }
                            } else if (ct == STU_ConstraintType_ResourceRef || ct == STU_ConstraintType_NonSTUResourceRef) {
                                uint64_t rid = SafeReadVal<uint64_t>(fa + 0x08);
                                if (rid) fields_str += std::format("[+0x{:X}:ref=0x{:X}] ", arg->offset, rid);
                            } else if (ct == STU_ConstraintType_Primitive || ct == STU_ConstraintType_Enum) {
                                uint32_t u32 = (uint32_t)fraw;
                                float fflt = SafeReadVal<float>(fa);
                                if (u32 != 0 && u32 != 0xFFFFFFFF) {
                                    if (fflt > -1e6f && fflt < 1e6f && (fflt < -1e-4f || fflt > 1e-4f))
                                        fields_str += std::format("[+0x{:X}:0x{:X}({:.2f})] ", arg->offset, u32, fflt);
                                    else
                                        fields_str += std::format("[+0x{:X}:0x{:X}] ", arg->offset, u32);
                                }
                            }
                        }
                    }

                    out.append(std::format("        * `node[{}]`: `stu=0x{:08X}({})` `isSt={}` `cO={}` `sO={}` `stat={}({})` `act_vt={}`(`F0={}`,`F8={}`,`100={}`) {}{}{}\n",
                        node->idx_in_nodes, n_hash, n_name,
                        (int)node->m_isState, (int)node->m_clientOnly, (int)node->m_serverOnly,
                        n_stat, n_ts, FmtRvaShort(avt), FmtRvaShort(fnF0), FmtRvaShort(fnF8), FmtRvaShort(fn100),
                        diag_tags, links_str, fields_str));
                }
            }
        }
    }
}

static void AppendHeroEntityForensics(std::string& out, const char* label, Entity* ctrl_ent, Entity* mdl_ent) {
    out.append(std::format("### {}\n", label));
    out.append(std::format("- **Controller**: `0x{:X}` (`entid=0x{:X}`), **Model**: `0x{:X}` (`entid=0x{:X}`)\n",
        (unsigned long long)ctrl_ent, ctrl_ent ? ctrl_ent->entity_id : 0,
        (unsigned long long)mdl_ent, mdl_ent ? mdl_ent->entity_id : 0));

    if (ctrl_ent) {
        std::string c_comps;
        for (int i = 0; i < ctrl_ent->component_list.num - 1; i++) {
            if (ctrl_ent->component_list.ptr[i]) {
                if (!c_comps.empty()) c_comps += ",";
                c_comps += std::format("{:02X}", (int)ctrl_ent->component_list.ptr[i]->component_id);
            }
        }
        out.append(std::format("  - **Controller Components**: `[{}]`\n", c_comps));
        if (auto* cc10 = ctrl_ent->getById<Component_10_FilterBits>(0x10)) {
            out.append(std::format("  - **Controller `Component_10_FilterBits`**: `f20=0x{:X}`, `f48=0x{:X}`, `f8C=0x{:X}`, `f8E=0x{:X}`, `pre=0x{:016X}`, `final=0x{:016X}`\n",
                cc10->field_20, cc10->field_48, (uint16_t)cc10->field_8C, (uint8_t)cc10->field_8E,
                (unsigned long long)cc10->pre_filter_bits, (unsigned long long)cc10->filter_bits));
        }
        if (ComponentBase* cc21 = ctrl_ent->getById(0x21)) {
            float f18 = SafeReadVal<float>((uintptr_t)cc21 + 0x18);
            out.append(std::format("  - **Controller `Component_21` (`0x{:X}`, `f18={:.3f}`)**: ", (unsigned long long)cc21, f18));
            for (int off = 0x18; off <= 0x58; off += 4) {
                out.append(std::format("+0x{:02X}=0x{:X} ", off, SafeReadVal<uint32_t>((uintptr_t)cc21 + off)));
            }
            out += "\n";
        }
        if (auto* cc2f = ctrl_ent->getById<Component_2F_LocalPlayer>(0x2F)) {
            uintptr_t ring = (uintptr_t)cc2f + 0x15E0;
            out.append(std::format("  - **Controller `Component_2F` (`0x{:X}`)**: `cap(+0x17E0)={}`, `head(+0x17F0)={}`, `cnt(+0x17F4)={}`, `e0:(ent=0x{:X}, ts=0x{:X}, dmg={}, flg=0x{:X})`\n",
                (unsigned long long)cc2f, cc2f->field_17E0, cc2f->field_17F0, cc2f->field_17F4,
                SafeReadVal<uint32_t>(ring + 0x00), SafeReadVal<uint32_t>(ring + 0x04),
                SafeReadVal<uint32_t>(ring + 0x08), SafeReadVal<uint32_t>(ring + 0x0C)));
        }
        if (auto* cc3f = ctrl_ent->getById<Component_3F_PlayerInfo>(0x3F)) {
            out.append(std::format("  - **Controller `Component_3F` (`0x{:X}`)**: `slot={}`, `hero=0x{:X}`, `sel={}`\n    ",
                (unsigned long long)cc3f, (int)cc3f->m_slot, (unsigned long long)cc3f->selected_heroid, (int)cc3f->hero_was_selected));
            for (int off = 0x18; off <= 0xC0; off += 8) {
                uint64_t v = SafeReadVal<uint64_t>((uintptr_t)cc3f + off);
                if (v) out.append(std::format("+0x{:02X}=0x{:X} ", off, v));
            }
            out += "\n";
        }
        AppendVarBagSummary(out, "Controller", ctrl_ent);
    }

    if (!mdl_ent) return;

    std::string m_comps;
    for (int i = 0; i < mdl_ent->component_list.num - 1; i++) {
        if (mdl_ent->component_list.ptr[i]) {
            if (!m_comps.empty()) m_comps += ",";
            m_comps += std::format("{:02X}", (int)mdl_ent->component_list.ptr[i]->component_id);
        }
    }
    out.append(std::format("  - **Model Components**: `[{}]`\n", m_comps));

    if (ComponentBase* c1c = mdl_ent->getById(0x1C)) {
        uint64_t da0 = SafeReadVal<uint64_t>((uintptr_t)c1c + 0xDA0);
        uint64_t p1018 = SafeReadVal<uint64_t>((uintptr_t)c1c + 0x1018);
        out.append(std::format("  - **Model `Component_1C` (3D World UI, `0x{:X}`)**: `res_DA0=0x{:X}`, `init_1018=0x{:X}`\n    ",
            (unsigned long long)c1c, da0, p1018));
        for (int off = 0x18; off <= 0x1030; off += 8) {
            uint64_t v = SafeReadVal<uint64_t>((uintptr_t)c1c + off);
            if (v) out.append(std::format("+0x{:03X}=0x{:X} ", off, v));
        }
        out += "\n";
    }
    if (ComponentBase* c1f = mdl_ent->getById(0x1F)) {
        out.append(std::format("  - **Model `Component_1F` (Backref/Nameplate, `0x{:X}`)**: `backref(+0x18)=0x{:X}`, `np_A0(+0xA0)=0x{:X}`\n    ",
            (unsigned long long)c1f, SafeReadVal<uint32_t>((uintptr_t)c1f + 0x18), SafeReadVal<uint64_t>((uintptr_t)c1f + 0xA0)));
        for (int off = 0x18; off <= 0x138; off += 8) {
            uint64_t v = SafeReadVal<uint64_t>((uintptr_t)c1f + off);
            if (v) out.append(std::format("+0x{:02X}=0x{:X} ", off, v));
        }
        out += "\n";
    }

    if (auto* c1 = mdl_ent->getById<Component_1_SceneRendering>(1)) {
        Vector4 wpos = SafeReadVal<Vector4>((uintptr_t)c1 + 0xC0);
        Vector4 ipos = SafeReadVal<Vector4>((uintptr_t)c1 + 0x170);
        uintptr_t c198 = SafeReadVal<uintptr_t>((uintptr_t)c1 + 0x198);
        uintptr_t c1c_link = SafeReadVal<uintptr_t>((uintptr_t)c1 + 0x1A8);
        uint64_t flags_1b8 = SafeReadVal<uint64_t>((uintptr_t)c1 + 0x1B8);
        uint64_t rflags_180 = SafeReadVal<uint64_t>((uintptr_t)c1 + 0x180);
        uintptr_t anim_inst = c1c_link ? SafeReadVal<uintptr_t>((c1c_link - 0x20) + 0x1F0) : 0;
        uintptr_t bone_mats = anim_inst ? SafeReadVal<uintptr_t>(anim_inst + 0x20) : 0;
        uint16_t bone_cnt = anim_inst ? SafeReadVal<uint16_t>(anim_inst + 0x40) : 0;
        Vector4 c1_aabb[2]{};
        uint8_t c1_aabb_ok = ((get_c1_world_aabb_fn)(globals::gameBase + 0xA02DB0))(c1, c1_aabb);
        out.append(std::format("  - **Model `Component_01` (`0x{:X}`)**: `pos(+0x60)=({:.2f},{:.2f},{:.2f})`, `world(+0xC0)=({:.2f},{:.2f},{:.2f})`, `ipos(+0x170)=({:.2f},{:.2f},{:.2f})`, `aabb(ok={})=({:.2f},{:.2f},{:.2f})..({:.2f},{:.2f},{:.2f})`\n",
            (unsigned long long)c1, c1->position.X, c1->position.Y, c1->position.Z,
            wpos.X, wpos.Y, wpos.Z, ipos.X, ipos.Y, ipos.Z,
            c1_aabb_ok, c1_aabb[0].X, c1_aabb[0].Y, c1_aabb[0].Z, c1_aabb[1].X, c1_aabb[1].Y, c1_aabb[1].Z));
        out.append(std::format("    `rflags(+0x180)=0x{:016X}`, `c198(+0x198)=0x{:X}`, `c1C_link(+0x1A8)=0x{:X}`, `filter(+0x1B8)=0x{:016X}`, `anim_inst=0x{:X}`, `bones=0x{:X}(cnt={})`\n",
            rflags_180, c198, c1c_link, flags_1b8, anim_inst, bone_mats, bone_cnt));
    }

    if (ComponentBase* c04 = mdl_ent->getById(0x04)) {
        out.append(std::format("  - **Model `Component_04` (Animation/Hitbox Skeleton, `0x{:X}`)**:\n    ", (unsigned long long)c04));
        for (int off = 0x18; off <= 0x120; off += 8) {
            uint64_t v = SafeReadVal<uint64_t>((uintptr_t)c04 + off);
            if (v) out.append(std::format("+0x{:02X}=0x{:X} ", off, v));
        }
        out += "\n";
    }

    const uint8_t extra_cids[] = { 0x0E, 0x13, 0x18, 0x1C, 0x20, 0x21, 0x22, 0x26, 0x40, 0x46, 0x48, 0x68, 0x7A };
    for (uint8_t cid : extra_cids) {
        if (ComponentBase* cb = mdl_ent->getById(cid)) {
            uintptr_t vt = SafeReadVal<uintptr_t>((uintptr_t)cb);
            uintptr_t vt_rva = (vt >= globals::gameBase && vt < globals::gameBase + globals::gameSize) ? (vt - globals::gameBase) : vt;
            out.append(std::format("  - **Model `Component_{:02X}` (`0x{:X}`, vt=`+0x{:X}`)**: ", cid, (unsigned long long)cb, (unsigned long long)vt_rva));
            for (int off = 0x18; off <= 0x90; off += 8) {
                uint64_t v = SafeReadVal<uint64_t>((uintptr_t)cb + off);
                if (v) out.append(std::format("+0x{:02X}=0x{:X} ", off, v));
            }
            out += "\n";
        }
    }

    if (ComponentBase* c46 = mdl_ent->getById(0x46)) {
        auto get_comp_stu = (STUBase<>*(__fastcall*)(ComponentBase*))(globals::gameBase + 0xF193A0);
        STUBase<>* stu_vc = get_comp_stu(c46);
        uint8_t b30 = SafeReadVal<uint8_t>((uintptr_t)c46 + 0x30);
        STUBase<>* vs_stu = nullptr;
        if (b30 != 0 || (stu_vc && !IsBadReadPtr(stu_vc, 0x18))) {
            auto get_voiceset_stu = (STUBase<>*(__fastcall*)(ComponentBase*))(globals::gameBase + 0x1019430);
            vs_stu = get_voiceset_stu(c46);
        }
        if ((!vs_stu || IsBadReadPtr(vs_stu, 0x58)) && stu_vc && !IsBadReadPtr(stu_vc, 0x18)) {
            auto* vd_ref = (STUResourceReference*)((uintptr_t)stu_vc + 0x08);
            vs_stu = vd_ref->get_STU();
            if (!vs_stu && vd_ref->resource_id) vs_stu = stu_resources::GetByID(vd_ref->resource_id);
        }
        if (vs_stu && !IsBadReadPtr(vs_stu, 0x58)) {
            uintptr_t bs_ptr = SafeReadVal<uintptr_t>((uintptr_t)vs_stu + 0x48);
            uint64_t bs_flag = SafeReadVal<uint64_t>((uintptr_t)vs_stu + 0x50);
            int32_t vl_count = 0;
            uintptr_t vl_arr = 0;
            if (bs_ptr && !IsBadReadPtr((const void*)bs_ptr, 0x10)) {
                if (bs_flag == 1) {
                    vl_arr = SafeReadVal<uintptr_t>(bs_ptr + 0x00);
                    vl_count = SafeReadVal<int32_t>(bs_ptr + 0x08);
                } else {
                    vl_count = SafeReadVal<int32_t>(bs_ptr + 0x00);
                    vl_arr = SafeReadVal<uintptr_t>(bs_ptr + 0x08);
                }
            }
            if (vl_arr && vl_count > 0 && vl_count < 4096 && !IsBadReadPtr((const void*)vl_arr, (size_t)vl_count * 8)) {
                struct StimForensic {
                    uint64_t stim_id = 0;
                    STUBase<>* stim_stu = nullptr;
                    int uncond_cnt = 0;
                    int cond_cnt = 0;
                    uint64_t sample_vl = 0;
                };
                std::vector<StimForensic> dead_stims;
                int total_vls = 0;
                for (int32_t i = 0; i < vl_count; i++) {
                    uintptr_t inst = SafeReadVal<uintptr_t>(vl_arr + (uintptr_t)i * 8);
                    if (!inst || IsBadReadPtr((const void*)inst, 0x30)) continue;
                    total_vls++;
                    if (SafeReadVal<uintptr_t>(inst + 0x28) == 0) continue;
                    auto* vl_ref = (STUResourceReference*)(inst + 0x08);
                    uint64_t vl_id = SafeReadVal<uint64_t>(inst + 0x10);
                    if (!vl_id) continue;
                    STUBase<>* vl_stu = vl_ref->get_STU();
                    if (!vl_stu) vl_stu = stu_resources::GetByID(vl_id);
                    if (!vl_stu || IsBadReadPtr(vl_stu, 0x40)) continue;
                    auto* stim_ref = (STUResourceReference*)((uintptr_t)vl_stu + 0x18);
                    uint64_t stim_id = SafeReadVal<uint64_t>((uintptr_t)vl_stu + 0x20);
                    uintptr_t crit_ptr = SafeReadVal<uintptr_t>((uintptr_t)vl_stu + 0x38);
                    if (!stim_id) continue;
                    STUBase<>* stim_stu = stim_ref->get_STU();
                    if (!stim_stu) stim_stu = stu_resources::GetByID(stim_id);
                    if (!stim_stu || IsBadReadPtr(stim_stu, 0x94)) continue;
                    if (SafeReadVal<uint8_t>((uintptr_t)stim_stu + 0x90) != 0) {
                        StimForensic* sf = nullptr;
                        for (auto& ds : dead_stims) {
                            if (ds.stim_id == stim_id) { sf = &ds; break; }
                        }
                        if (!sf) {
                            dead_stims.push_back(StimForensic{ stim_id, stim_stu, 0, 0, vl_id });
                            sf = &dead_stims.back();
                        }
                        if (crit_ptr == 0) {
                            sf->uncond_cnt++;
                            sf->sample_vl = vl_id;
                        } else {
                            sf->cond_cnt++;
                        }
                    }
                }
                out.append(std::format("    - **`STUVoiceSet` (`0x{:X}`, total_vls={}, allow_dead_stims={})**:\n",
                    (unsigned long long)vs_stu, total_vls, dead_stims.size()));
                for (const auto& ds : dead_stims) {
                    uint32_t f68 = SafeReadVal<uint32_t>((uintptr_t)ds.stim_stu + 0x68);
                    uint32_t u7c = SafeReadVal<uint32_t>((uintptr_t)ds.stim_stu + 0x7C);
                    uint8_t b90 = SafeReadVal<uint8_t>((uintptr_t)ds.stim_stu + 0x90);
                    uint8_t b91 = SafeReadVal<uint8_t>((uintptr_t)ds.stim_stu + 0x91);
                    uint8_t b92 = SafeReadVal<uint8_t>((uintptr_t)ds.stim_stu + 0x92);
                    out.append(std::format("      - DeadStim `0x{:X}` (`0x{:X}`): uncond={}, cond={}, sample_vl=`0x{:X}`, f68=`0x{:X}`, u7c=`{}`, b90/91/92=`{}/{}/{}`\n",
                        (unsigned long long)ds.stim_id, (unsigned long long)ds.stim_stu,
                        ds.uncond_cnt, ds.cond_cnt, (unsigned long long)ds.sample_vl,
                        f68, u7c, b90, b91, b92));
                }
            }
        }
    }

    if (auto* c10 = mdl_ent->getById<Component_10_FilterBits>(0x10)) {
        out.append(std::format("  - **Model `Component_10_FilterBits` (`0x{:X}`)**: `f20=0x{:X}`, `f48=0x{:X}`, `f8C=0x{:X}`, `f8E=0x{:X}`, `pre(+0x90)=0x{:016X}`, `final(+0x98)=0x{:016X}`\n",
            (unsigned long long)c10, c10->field_20, c10->field_48, (uint16_t)c10->field_8C, (uint8_t)c10->field_8E,
            (unsigned long long)c10->pre_filter_bits, (unsigned long long)c10->filter_bits));
    }

    if (auto* c12 = mdl_ent->getById<Component_12_STUMovementStateComponent>(0x12)) {
        Vector4 c12_min = SafeReadVal<Vector4>((uintptr_t)c12 + 0xA2D0);
        Vector4 c12_max = SafeReadVal<Vector4>((uintptr_t)c12 + 0xA2E0);
        out.append(std::format("  - **Model `Component_12` (`0x{:X}`)**: `mov_state.pos=({:.2f},{:.2f},{:.2f})`, `mov_flags(+0xA1F8)=0x{:X}`, `arr_used(+0xA054)={}`, `cf={}`, `aabb(+0xA2D0)=({:.2f},{:.2f},{:.2f})..({:.2f},{:.2f},{:.2f})`\n",
            (unsigned long long)c12, c12->mov_state.absolute_position.X, c12->mov_state.absolute_position.Y, c12->mov_state.absolute_position.Z,
            c12->mov_state.movement_flags, c12->history_1.movstate_arr_used, c12->mov_state.command_frame,
            c12_min.X, c12_min.Y, c12_min.Z, c12_max.X, c12_max.Y, c12_max.Z));
    }

    if (auto* c15 = mdl_ent->getById<Component_15_STUCharacterMoverComponent>(0x15)) {
        uintptr_t h_cc0 = SafeReadVal<uintptr_t>((uintptr_t)c15 + 0xCC0);
        uint32_t f_1070 = SafeReadVal<uint32_t>((uintptr_t)c15 + 0x1070);
        uint8_t f_1076 = SafeReadVal<uint8_t>((uintptr_t)c15 + 0x1076);
        out.append(std::format("  - **Model `Component_15` (`0x{:X}`)**: `ms1.pos=({:.2f},{:.2f},{:.2f})`, `ms1.flags=0x{:X}`, `ms2.flags=0x{:X}`, `havok_CC0=0x{:X}`, `filter_1070=0x{:X}`, `flg_1076={}`\n",
            (unsigned long long)c15,
            c15->movement_state_1.absolute_position.X, c15->movement_state_1.absolute_position.Y, c15->movement_state_1.absolute_position.Z,
            c15->movement_state_1.movement_flags, c15->movement_state_2.movement_flags,
            h_cc0, f_1070, f_1076));
    }

    if (ComponentBase* c16 = mdl_ent->getById(0x16)) {
        uint8_t b40 = SafeReadVal<uint8_t>((uintptr_t)c16 + 0x40);
        uint8_t b41 = SafeReadVal<uint8_t>((uintptr_t)c16 + 0x41);
        uint8_t b42 = SafeReadVal<uint8_t>((uintptr_t)c16 + 0x42);
        uint8_t b43 = SafeReadVal<uint8_t>((uintptr_t)c16 + 0x43);
        uint64_t h44 = SafeReadVal<uint64_t>((uintptr_t)c16 + 0x44);
        uint64_t h4C = SafeReadVal<uint64_t>((uintptr_t)c16 + 0x4C);
        out.append(std::format("  - **Model `Component_16` (Hitbox/Ragdoll Collider, `0x{:X}`)**: `b40={}`, `b41={}`, `b42={}`, `b43={}`, `h44=0x{:X}`, `h4C=0x{:X}`\n    ",
            (unsigned long long)c16, b40, b41, b42, b43, h44, h4C));
        for (int off = 0x18; off <= 0x88; off += 8) {
            out.append(std::format("+0x{:02X}=0x{:X} ", off, SafeReadVal<uint64_t>((uintptr_t)c16 + off)));
        }
        out += "\n";
    }

    if (ComponentBase* c27 = mdl_ent->getById(0x27)) {
        out.append(std::format("  - **Model `Component_27` (Damage Receiver, `0x{:X}`)**:\n    ", (unsigned long long)c27));
        for (int off = 0x18; off <= 0x78; off += 8) {
            out.append(std::format("+0x{:02X}=0x{:X} ", off, SafeReadVal<uint64_t>((uintptr_t)c27 + off)));
        }
        out += "\n";
    }

    if (auto* c28 = mdl_ent->getById<Component_28_STUHealthComponent>(0x28)) {
        uint8_t b484 = SafeReadVal<uint8_t>((uintptr_t)c28 + 0x484);
        uint8_t b485 = SafeReadVal<uint8_t>((uintptr_t)c28 + 0x485);
        out.append(std::format("  - **Model `Component_28_STUHealthComponent` (`0x{:X}`)**: `CurrHP={:.1f}`, `MaxHP={:.1f}`, `b484={}`, `b485(hb_vis)={}`, `norm0:(en={}, cur={:.1f}/{:.1f})`, `arm0:(en={}, cur={:.1f}/{:.1f})`, `shd0:(en={}, cur={:.1f}/{:.1f})`\n    ",
            (unsigned long long)c28, c28->CalcTotalCurrentHealth(), c28->CalcTotalMaxHealth(), b484, b485,
            c28->normal_health[0].part_enabled, c28->normal_health[0].curr_health, c28->normal_health[0].max_health,
            c28->armour_health[0].part_enabled, c28->armour_health[0].curr_health, c28->armour_health[0].max_health,
            c28->shield_health[0].part_enabled, c28->shield_health[0].curr_health, c28->shield_health[0].max_health));
        for (int off = 0x18; off <= 0x488; off += 8) {
            uint64_t v = SafeReadVal<uint64_t>((uintptr_t)c28 + off);
            if (v) out.append(std::format("+0x{:03X}=0x{:X} ", off, v));
        }
        out += "\n";

        // Dump the STUHealthComponent STU resource (+0xF193A0) and its child fields / arrays
        auto get_comp_stu = (STUBase<>*(__fastcall*)(ComponentBase*))(globals::gameBase + 0xF193A0);
        STUBase<>* stu_hc = get_comp_stu(&c28->base);
        if (stu_hc && !IsBadReadPtr(stu_hc, 0x40)) {
            STUInfo* hc_info = (stu_hc->vfptr && !IsBadReadPtr(stu_hc->vfptr, 8) && stu_hc->vfptr->GetSTUInfo)
                ? stu_hc->vfptr->GetSTUInfo() : nullptr;
            uint32_t hc_hash = (hc_info && !IsBadReadPtr(hc_info, sizeof(STUInfo))) ? hc_info->name_hash : 0;
            uint32_t hc_size = (hc_info && !IsBadReadPtr(hc_info, sizeof(STUInfo))) ? hc_info->instance_size : 0x50;
            out.append(std::format("    - **`STUHealthComponent` STU Resource (`+0xF193A0` = `0x{:X}`, hash=`0x{:08X}`, size=`0x{:X}`)**:\n      raw: ",
                (unsigned long long)stu_hc, hc_hash, hc_size));
            int dump_sz = (hc_size > 0 && hc_size <= 0x100) ? (int)hc_size : 0x60;
            for (int off = 0; off < dump_sz; off += 8) {
                uint64_t qw = SafeReadVal<uint64_t>((uintptr_t)stu_hc + off);
                float fl0 = SafeReadVal<float>((uintptr_t)stu_hc + off);
                float fl1 = SafeReadVal<float>((uintptr_t)stu_hc + off + 4);
                out.append(std::format("+0x{:02X}=0x{:X}({:.1f},{:.1f}) ", off, qw, fl0, fl1));
            }
            out += "\n";
            if (hc_info && !IsBadReadPtr(hc_info, sizeof(STUInfo))) {
                for (auto arg_pair : *hc_info) {
                    STUArgumentInfo* arg = arg_pair.second;
                    if (!arg || IsBadReadPtr(arg, sizeof(STUArgumentInfo))) continue;
                    uintptr_t f_addr = (uintptr_t)stu_hc + arg->offset;
                    uint64_t f_raw = SafeReadVal<uint64_t>(f_addr);
                    float f_flt = SafeReadVal<float>(f_addr);
                    STUConstraintType ctype = (arg->constraint && !IsBadReadPtr(arg->constraint, 8))
                        ? arg->constraint->get_type_flag() : (STUConstraintType)-1;
                    out.append(std::format("      - Field `0x{:08X}` (`{}`) at `+0x{:X}` (type={}): raw=`0x{:X}` flt=`{:.2f}`\n",
                        arg->name_hash, allmighty_hash_lib::hashes.count(arg->name_hash) ? allmighty_hash_lib::hashes[arg->name_hash] : "?",
                        arg->offset, (int)ctype, f_raw, f_flt));
                    if (ctype == STU_ConstraintType_BSList_Object || ctype == STU_ConstraintType_BSList_InlinedObject ||
                        ctype == STU_ConstraintType_BSList_Primitive || ctype == STU_ConstraintType_BSList_ResourceRef) {
                        auto* bs = (STUBullshitListFull<uint64_t>*)f_addr;
                        if (!IsBadReadPtr(bs, sizeof(*bs)) && bs->list_ptr && !IsBadReadPtr(bs->list_ptr, 16)) {
                            int cnt = bs->count();
                            out.append(std::format("        BSList count={}: ", cnt));
                            for (int k = 0; k < cnt && k < 16; k++) {
                                uintptr_t elem_addr = (uintptr_t)&bs->list()[k];
                                out.append(std::format("[{}:0x{:X}] ", k, SafeReadVal<uint64_t>(elem_addr)));
                            }
                            out += "\n";
                            if (ctype == STU_ConstraintType_BSList_Object || ctype == STU_ConstraintType_BSList_InlinedObject) {
                                STU_ObjectList obj_list(arg, (void*)f_addr);
                                if (obj_list.valid()) {
                                    for (int k = 0; k < cnt && k < 8; k++) {
                                        STU_Object elem = obj_list[k];
                                        if (elem.valid() && elem.value && !IsBadReadPtr(elem.value, 0x20)) {
                                            STU_Object root_e = elem.get_runtime_root();
                                            uint32_t eh = root_e.struct_info ? root_e.struct_info->name_hash : 0;
                                            uint32_t esz = root_e.struct_info ? root_e.struct_info->instance_size : 0x30;
                                            out.append(std::format("        elem[{}] (`0x{:X}`, hash=`0x{:08X}`, sz=`0x{:X}`): ",
                                                k, (unsigned long long)elem.value, eh, esz));
                                            for (int eo = 0; eo < (int)esz && eo < 0x48; eo += 4) {
                                                uint32_t dw = SafeReadVal<uint32_t>((uintptr_t)elem.value + eo);
                                                float ef = SafeReadVal<float>((uintptr_t)elem.value + eo);
                                                out.append(std::format("+0x{:02X}=0x{:X}({:.1f}) ", eo, dw, ef));
                                            }
                                            out += "\n";
                                        }
                                    }
                                }
                            }
                        }
                    } else if (ctype == STU_ConstraintType_Object && f_raw > 0x10000ULL && !IsBadReadPtr((const void*)f_raw, 0x20)) {
                        STUBase<>* child = (STUBase<>*)f_raw;
                        STUInfo* ci = (child->vfptr && !IsBadReadPtr(child->vfptr, 8) && child->vfptr->GetSTUInfo) ? child->vfptr->GetSTUInfo() : nullptr;
                        uint32_t ch = (ci && !IsBadReadPtr(ci, sizeof(STUInfo))) ? ci->name_hash : 0;
                        uint32_t csz = (ci && !IsBadReadPtr(ci, sizeof(STUInfo))) ? ci->instance_size : 0x30;
                        out.append(std::format("        child (`0x{:X}`, hash=`0x{:08X}`, sz=`0x{:X}`): ", f_raw, ch, csz));
                        for (int co = 0; co < (int)csz && co < 0x48; co += 4) {
                            uint32_t dw = SafeReadVal<uint32_t>(f_raw + co);
                            float cf = SafeReadVal<float>(f_raw + co);
                            out.append(std::format("+0x{:02X}=0x{:X}({:.1f}) ", co, dw, cf));
                        }
                        out += "\n";
                    }
                }
            }
        }
    }
    bool is_local_player = (label && strncmp(label, "LOCAL PLAYER", 12) == 0);
    AppendVarBagSummary(out, "Model", mdl_ent, is_local_player);
}

void WeaponHardpointDiag::DumpDummyCombatForensics(EntityAdminBase* game_ea, const char* trigger_reason) {
    std::string out;
    out.reserve(65536);
    out += "# Local Player vs Enemy Dummy Combat & Shot Raycast Forensics\n\n";
    out.append(std::format("- **Trigger**: `{}` | **Shot Raycasts (`+0xD354B0`)**: `{}` | **FireShot (`+0xD46060`)**: `{}`\n\n",
        trigger_reason ? trigger_reason : "Manual", s_shot_raycast_calls, s_fire_shot_calls));

    if (s_last_sys46) {
        uintptr_t lst_arr = SafeReadVal<uintptr_t>(s_last_sys46 + 0x50);
        int32_t lst_cnt = SafeReadVal<int32_t>(s_last_sys46 + 0x58);
        out.append(std::format("- **System #46 (`0x{:X}`) Combat Listeners (`+0x50`=`0x{:X}`, count=`{}`)**: ",
            (unsigned long long)s_last_sys46, (unsigned long long)lst_arr, lst_cnt));
        if (lst_arr && lst_cnt > 0 && lst_cnt < 64) {
            for (int32_t i = 0; i < lst_cnt; i++) {
                uintptr_t l_obj = SafeReadVal<uintptr_t>(lst_arr + (uintptr_t)i * 8);
                uintptr_t l_vt = l_obj ? SafeReadVal<uintptr_t>(l_obj) : 0;
                uintptr_t l_vt_rva = (l_vt >= globals::gameBase && l_vt < globals::gameBase + globals::gameSize) ? (l_vt - globals::gameBase) : l_vt;
                uintptr_t l_fn = l_vt ? SafeReadVal<uintptr_t>(l_vt) : 0;
                uintptr_t l_fn_rva = (l_fn >= globals::gameBase && l_fn < globals::gameBase + globals::gameSize) ? (l_fn - globals::gameBase) : l_fn;
                out.append(std::format("[{}:obj=0x{:X},vt=+0x{:X},fn0=+0x{:X}] ", i, (unsigned long long)l_obj, (unsigned long long)l_vt_rva, (unsigned long long)l_fn_rva));
            }
        }
        out += "\n\n";
    }

    if (game_ea) {
        Entity* local_ctrl = game_ea->getLocalEnt();
        uint32 local_ctrl_id = game_ea->local_entid;
        uint32 local_mdl_id = 0;
        Entity* local_mdl = nullptr;
        if (local_ctrl) {
            if (auto* pet = local_ctrl->getById<Component_20_ModelReference>(0x20)) {
                local_mdl_id = pet->cam_attach_entid ? pet->cam_attach_entid : (pet->aim_entid ? pet->aim_entid : pet->movement_attach_entid);
                if (local_mdl_id) local_mdl = game_ea->getEntById(local_mdl_id);
            }
        }
        out += "## 1. Side-by-Side Entity Component Comparison\n";
        AppendHeroEntityForensics(out, "LOCAL PLAYER (Team 0)", local_ctrl, local_mdl);

        int idx = 0;
        int sub_idx = 0;
        for (Entity* ent : *game_ea) {
            if (!ent || ent->entity_id == local_ctrl_id || ent->entity_id == local_mdl_id) continue;
            if (ent->hasComponent(0x15) && ent->hasComponent(0x28)) {
                uint32 ctrl_id = 0;
                if (auto* c1f = ent->getById<Component_1F_Backref>(0x1F)) ctrl_id = c1f->backref_entid;
                Entity* ctrl = ctrl_id ? game_ea->getEntById(ctrl_id) : nullptr;
                std::string lbl = std::format("ENEMY DUMMY #{} (entid=0x{:X})", idx++, ent->entity_id);
                AppendHeroEntityForensics(out, lbl.c_str(), ctrl, ent);
            } else if (local_ctrl_id && ent->entity_id > local_ctrl_id && sub_idx < 12 &&
                       (ent->hasComponent(0x23) || ent->hasComponent(0x2A))) {
                uintptr_t rl = SafeReadVal<uintptr_t>((uintptr_t)ent + 0x08);
                uint64_t res_id = (rl && !IsBadReadPtr((const void*)rl, 0x60)) ? SafeReadVal<uint64_t>(rl + 0x58) : 0;
                auto* c1 = ent->getById<Component_1_SceneRendering>(1);
                Vector4 wpos = c1 ? SafeReadVal<Vector4>((uintptr_t)c1 + 0xC0) : Vector4{};
                uintptr_t scene_nd = c1 ? SafeReadVal<uintptr_t>((uintptr_t)c1 + 0x148) : 0;
                out.append(std::format("### SPAWNED SUB-ENTITY #{} (`entid=0x{:X}`, `res=0x{:X}`, `wpos=({:.2f},{:.2f},{:.2f})`, `c1_scene(+0x148)=0x{:X}`)\n",
                    sub_idx++, ent->entity_id, (unsigned long long)res_id, wpos.X, wpos.Y, wpos.Z, (unsigned long long)scene_nd));
                AppendVarBagSummary(out, "SubEnt", ent, true);
            }
        }
    }

    if (!s_alive_hit_snapshot.empty()) {
        out += "\n## 1B. Captured Alive & Damaged Snapshot (Hit #1)\n";
        out += s_alive_hit_snapshot;
    }

    if (!statescript_logger::s_trace_history.empty()) {
        out += "\n## 1C. Captured Statescript Ability Trace (`Q` Pulse Bomb / `E` Recall)\n```text\n";
        for (const auto& tline : statescript_logger::s_trace_history) {
            out += tline;
            out += "\n";
        }
        out += "```\n";
    }

    out += "\n## 2. Recent Shot / Projectile Events (dedicated ring, last 100)\n```text\n";
    {
        LONG sh_seq = s_shot_diag_seq;
        LONG sh_start = (sh_seq > 100) ? (sh_seq - 100 + 1) : 1;
        for (LONG s = sh_start; s <= sh_seq; s++) {
            const WeaponDiagEntry& e = s_shot_diag_ring[(s - 1) & (SHOT_DIAG_RING_SIZE - 1)];
            if (e.seq == (uint32_t)s && e.text[0] != '\0') {
                out.append(std::format("[#{:04d}] {}\n", (int)e.seq, (const char*)e.text));
            }
        }
    }
    out += "```\n";

    out += "\n## 3. Recent General Weapon Events (last 8)\n```text\n";
    LONG cur_seq = s_weapon_diag_seq;
    LONG start_seq = (cur_seq > 8) ? (cur_seq - 8 + 1) : 1;
    for (LONG s = start_seq; s <= cur_seq; s++) {
        const WeaponDiagEntry& e = s_weapon_diag_ring[(s - 1) & (WEAPON_DIAG_RING_SIZE - 1)];
        if (e.seq == (uint32_t)s && e.text[0] != '\0') {
            out.append(std::format("[#{:04d}] {}\n", (int)e.seq, (const char*)e.text));
        }
    }
    out += "```\n";

    FILE* f = fopen("dummy_combat_diag.log", "wb");
    bool ok = false;
    if (f) {
        fwrite(out.data(), 1, out.size(), f);
        fclose(f);
        ok = true;
    }
    s_last_dummy_dump_status = std::format("Wrote dummy_combat_diag.log ({} bytes, raycasts={}, ok={})",
        out.size(), s_shot_raycast_calls, ok ? "YES" : "NO");
    printf("[DummyCombatDiag] %s\n", s_last_dummy_dump_status.c_str());
}

//void ConstructNewGameEAInPlace() {
//    memset((void*)(globals::gameBase + 0xbfe68a), 0x90, 5); //initialize random values
//    memset((void*)(globals::gameBase + 0xbfe696), 0x90, 5); //init entity admin creation info
//    memset((void*)(globals::gameBase + 0xbfe69b), 0x90, 5); //initialize lobby entity admin
//    memset((void*)(globals::gameBase + 0xbfe6ae), 0x90, 7); //set lobbyea EA_staticstru
//    memset((void*)(globals::gameBase + 0xbfe6c4), 0x90, 11); //lobbyea initialize
//    memset((void*)(globals::gameBase + 0xbfe6fb), 0x90, 5); //tankwindowmanager initialize, comp50 input stuff
//    memset((void*)(globals::gameBase + 0xbff1f8), 0x90, 5); //init ini settings
//    memset((void*)(globals::gameBase + 0xbff213), 0x90, 9); //init replay admin
//
//    ((char(*)(void))(globals::gameBase + 0xbfe670))();
//}
//
//void(*VigsTickOrig)(LARGE_INTEGER, LARGE_INTEGER);
//void testVIGSTickHook(LARGE_INTEGER a1, LARGE_INTEGER a2) {
//    if (globals::switchGameEA) {
//        globals::switchGameEA = false;
//        ConstructNewGameEAInPlace();
//
//    }
//    VigsTickOrig(a1, a2);
//}

__int64 (*System63_orig)(__int64);
__int64 System63_hook(__int64 sys) {
    if (!*(__int64*)(sys + 0x30)) { //should be component 2
        return ((__int64(*)(__int64, __int64))(globals::gameBase + 0xc43d00))(sys - 0x80, 0);
    }
    return System63_orig(sys);
}

void __cdecl StartHook(void*) {
    //system("pause");
    printf("The cavalry's here!\n");
    atexit(exit_handler);

    char orig[] = { 0x7E, 0x41, 0xDB, 0xB6, 0x8F, 0x68, 0x93, 0x42, 0x09, 0xC8, 0x5F, 0x4A };
    memcpy((void*)(globals::gameBase + Start_Addr), orig, sizeof(orig));
    printf("restored game start.\n");

    typedef int(__stdcall* _TlsCallbackFunc)(DWORD_PTR, DWORD, PVOID);
    ((_TlsCallbackFunc)(globals::gameBase + 0x137f000))(globals::gameBase, 1, 0);
    printf("called iat decryptor (TlsCallback1)\n");

    decryptPage(Start_Addr); //yes this is wanted
    decryptPage(Start_Addr); //yes this is wanted
    decryptPage(0x1000); //yes this is wanted
    //Encrypted pages are 0x1000 but sometimes it doesnt catch all of them
    //https://images.wikidexcdn.net/mwuploads/wikidex/thumb/4/46/latest/20130917005914/Pok%C3%A9mon_Gotta_catch_em_all_logo.png/600px-Pok%C3%A9mon_Gotta_catch_em_all_logo.png
    for (int i = 0x1000; i < /*0x137ec00 0x1382d58*/globals::gameSize; i += 0x100) {
        decryptPage(i);
        DWORD old;
        VirtualProtect((LPVOID)(globals::gameBase + i), 0x100, PAGE_EXECUTE_READWRITE, &old);
    }
    printf("called text decryptor (veh)\n");

    char verify[] = { 0x48, 0x83, 0xEC, 0x28, 0xE8, 0xFF, 0xD8, 0x00, 0x00 };
    for (int i = 0; i < sizeof(verify); i++) {
        if (*(char*)(globals::gameBase + Start_Addr + i) != verify[i]) {
            printf(".text decryption failed!\n");
            system("pause");
        }
    }

    memset((void*)(globals::gameBase + TlsCallback_Addr), 0xc3, 0x10);
    printf("patched tls callbacks\n");

    memset((void*)(globals::gameBase + 0x1133D81), 0x90, 0x4); //illegal instruction at end of crash_me__.
    printf("patched debugger trap 1 (illegal isntruction)\n");

    //ud2
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0x5abd00), DebuggerTrapHook, 0));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0x5abd00)));
    //basically same debugger trap here but __debugbreak()
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0x5abd30), DebuggerTrapHook, 0));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0x5abd30)));
    //ame but ud2 again
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0x808b90), DebuggerTrapHook, 0));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0x808b90)));
    //ud2
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0x7e1710), DebuggerTrapHook, 0));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0x7e1710)));
    //crash_me__
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0x1130960), DebuggerTrapHook, 0));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0x1130960)));

    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0x8fc240), createwindow_hook, (PVOID*)&createwindow_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0x8fc240)));

    char return_zero[] = { 0x31, 0xC0, 0xC3 };
    memcpy((void*)(globals::gameBase + 0x804740), return_zero, sizeof(return_zero));
    printf("patched debugger trap 2 (return value check)\n");

    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0x5ac130), regedit_initialize, (PVOID*)&regedit_initialize_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0x5ac130)));
    printf("patched regedit launch options\n");

    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0x80f7c0), emplace_entity_hook, (PVOID*)&emplace_entity_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0x80f7c0)));
    printf("emplace entity hook\n");

    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xf3eb10), afterdatapackload_hook, (PVOID*)&afterdatapackload_fn));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xf3eb10)));
    printf("after data pack load hook\n");

    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xa00450), componen1_oncreate_hook, (PVOID*)&component_1_oncreate_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xa00450)));

    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xa04000), SetPosRotation_hook, (PVOID*)&SetPosRotation_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xa04000)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xa049f0), SetWorldPosRotation_hook, (PVOID*)&SetWorldPosRotation_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xa049f0)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xa045c0), SetPosRotationInternal_hook, (PVOID*)&SetPosRotationInternal_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xa045c0)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xa03e60), SetPosition_hook, (PVOID*)&SetPosition_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xa03e60)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xd563d0), UpdateWeaponAttachments_hook, (PVOID*)&UpdateWeaponAttachments_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xd563d0)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xd560b0), ResolveWeaponOrParentEntity_hook, (PVOID*)&ResolveWeaponOrParentEntity_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xd560b0)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xd46060), SpawnWeaponFireEffect_hook, (PVOID*)&SpawnWeaponFireEffect_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xd46060)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xd58110), SpawnStatescriptEffect_hook, (PVOID*)&SpawnStatescriptEffect_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xd58110)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xa01160), AttachComponent1ToHardpoint_hook, (PVOID*)&AttachComponent1ToHardpoint_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xa01160)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xa01430), AttachComponent1Internal_hook, (PVOID*)&AttachComponent1Internal_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xa01430)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xcba3f0), TryPlayBarrelMuzzleFlash_hook, (PVOID*)&TryPlayBarrelMuzzleFlash_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xcba3f0)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xcba6d0), ProcessMuzzleFlashEntry_hook, (PVOID*)&ProcessMuzzleFlashEntry_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xcba6d0)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xd354b0), PerformBulletRaycast_hook, (PVOID*)&PerformBulletRaycast_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xd354b0)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xd45240), ExecuteShotRaycast_hook, (PVOID*)&ExecuteShotRaycast_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xd45240)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xc34be0), ProcessShotQueue_hook, (PVOID*)&ProcessShotQueue_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xc34be0)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xd37230), ProcessPelletRaycastResult_hook, (PVOID*)&ProcessPelletRaycastResult_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xd37230)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xf453c0), LookupStatescriptNode_hook, (PVOID*)&LookupStatescriptNode_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xf453c0)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0x1034300), TestRayAgainstCandidates_hook, (PVOID*)&TestRayAgainstCandidates_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0x1034300)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xd37730), TestPelletAgainstCandidates_hook, (PVOID*)&TestPelletAgainstCandidates_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xd37730)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xd3b720), ProcessPelletEntityHits_hook, (PVOID*)&ProcessPelletEntityHits_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xd3b720)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0x1038860), PerformPelletWorldAndHitboxRaycast_hook, (PVOID*)&PerformPelletWorldAndHitboxRaycast_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0x1038860)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xa45aa0), RayIntersectsAABB_hook, (PVOID*)&RayIntersectsAABB_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xa45aa0)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xd381f0), OnPelletHitboxHit_hook, (PVOID*)&OnPelletHitboxHit_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xd381f0)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xd3eee0), ProjectileSpawnImpactEffect_hook, (PVOID*)&ProjectileSpawnImpactEffect_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xd3eee0)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xd3dc50), ShouldDeferProjectileImpact_hook, (PVOID*)&ShouldDeferProjectileImpact_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xd3dc50)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xa01070), AttachComponent1World_hook, (PVOID*)&AttachComponent1World_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xa01070)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xca7200), UpdateHealthbarVisibility_hook, (PVOID*)&UpdateHealthbarVisibility_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xca7200)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xd8bc90), EvalComp28Byte485_hook, (PVOID*)&EvalComp28Byte485_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xd8bc90)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xd1d7f0), UpdateHealthbarState_hook, (PVOID*)&UpdateHealthbarState_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xd1d7f0)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0x1023e80), IsCriticalHitCapsule_hook, (PVOID*)&IsCriticalHitCapsule_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0x1023e80)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0x1027940), GetEntityDisplayName_hook, (PVOID*)&GetEntityDisplayName_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0x1027940)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xd67bf0), RecallPreRewindState_OnTimerTick_hook, (PVOID*)&RecallPreRewindState_OnTimerTick_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xd67bf0)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0x108a160), RecallHistoryRecorder_Update1Tick_hook, (PVOID*)&RecallHistoryRecorder_Update1Tick_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0x108a160)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xd667a0), RecallRewindState_Update1Tick_hook, (PVOID*)&RecallRewindState_Update1Tick_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xd667a0)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xd44d60), WeaponFiringState_Update1Tick_hook, (PVOID*)&WeaponFiringState_Update1Tick_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xd44d60)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xd423c0), TickProjectileManager_hook, (PVOID*)&TickProjectileManager_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xd423c0)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xd39500), OnProjectileDetonate_hook, (PVOID*)&OnProjectileDetonate_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xd39500)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xf43450), FindStatescriptStateImpl_hook, (PVOID*)&FindStatescriptStateImpl_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xf43450)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xf47740), StatescriptAction_ExecuteAndContinue_hook, (PVOID*)&StatescriptAction_ExecuteAndContinue_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xf47740)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xf495c0), StatescriptState_StartState_hook, (PVOID*)&StatescriptState_StartState_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xf495c0)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xd739a0), IsGraphNodeClientOnly_hook, (PVOID*)&IsGraphNodeClientOnly_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xd739a0)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xf4d1d0), CanAcquireAbilityMutex_hook, (PVOID*)&CanAcquireAbilityMutex_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xf4d1d0)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0x1096500), PlacementState_TickRay_hook, (PVOID*)&PlacementState_TickRay_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0x1096500)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xd6d270), FindGround_Begin_hook, (PVOID*)&FindGround_Begin_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xd6d270)));
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xf471f0), TraverseLink_hook, (PVOID*)&TraverseLink_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xf471f0)));
    statescript_logger::s_universal_state_entry_cb = OnUniversalStateEntry;
    statescript_logger::s_universal_state_exit_cb = OnUniversalStateExit;
    printf("hooked Component_1 transforms, weapon hardpoint fixes, shot/projectile impact, healthbar & Horizon 1 Stage H1.2+H1.3 hooks\n");

    /*MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xbff460), testVIGSTickHook, (PVOID*)&VigsTickOrig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xbff460)));*/

    memcpy((void*)(globals::gameBase + 0x000000000137F061), std::array<unsigned char, 5>{0xe9, 0x57, 0x20, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x000000000137F0D1), std::array<unsigned char, 5>{0xe9, 0xdf, 0x1f, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x000000000138119B), std::array<unsigned char, 2>{0xeb, 0x42}.data(), 0x2);
    memcpy((void*)(globals::gameBase + 0x0000000001380408), std::array<unsigned char, 5>{0xe9, 0x78, 0xc, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000005A50F7), std::array<unsigned char, 5>{0xe9, 0x92, 0x4, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000007CEE55), std::array<unsigned char, 5>{0xe9, 0x2d, 0x4, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000001130A17), std::array<unsigned char, 2>{0xeb, 0x70}.data(), 0x2);
    memcpy((void*)(globals::gameBase + 0x00000000005A59F1), std::array<unsigned char, 5>{0xe9, 0x7f, 0x0, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000001380E4D), std::array<unsigned char, 5>{0xe9, 0x33, 0x2, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000007CF3CA), std::array<unsigned char, 5>{0xe9, 0xdf, 0x4, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000001130F81), std::array<unsigned char, 5>{0xe9, 0x56, 0x2, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000005A640A), std::array<unsigned char, 5>{0xe9, 0xf1, 0x41, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000007E7C25), std::array<unsigned char, 5>{0xe9, 0x12, 0x2, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000007E1BA9), std::array<unsigned char, 2>{0xeb, 0x78}.data(), 0x2);
    memcpy((void*)(globals::gameBase + 0x000000000113215B), std::array<unsigned char, 5>{0xe9, 0x15, 0x1c, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000001131D1E), std::array<unsigned char, 5>{0xe9, 0xe3, 0x3, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000005AA74D), std::array<unsigned char, 5>{0xe9, 0xd2, 0xf9, 0xff, 0xff}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000005A6F3C), std::array<unsigned char, 5>{0xe9, 0x8d, 0x4, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000007E7EBC), std::array<unsigned char, 5>{0xe9, 0xd4, 0x0, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000005AA2E6), std::array<unsigned char, 5>{0xe9, 0x2, 0xa, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000007E80C3), std::array<unsigned char, 5>{0xe9, 0x7e, 0x4, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000007E2163), std::array<unsigned char, 5>{0xe9, 0x5a, 0x2, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000005AAE10), std::array<unsigned char, 5>{0xe9, 0x76, 0x3, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000001132579), std::array<unsigned char, 2>{0xeb, 0x7b}.data(), 0x2);
    memcpy((void*)(globals::gameBase + 0x00000000007D0C32), std::array<unsigned char, 5>{0xe9, 0x7f, 0x0, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000005AB1D8), std::array<unsigned char, 5>{0xe9, 0xea, 0x0, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000005A9357), std::array<unsigned char, 5>{0xe9, 0x41, 0xd0, 0xff, 0xff}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000005AB3C7), std::array<unsigned char, 5>{0xe9, 0x91, 0x3, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000007E8EB6), std::array<unsigned char, 5>{0xe9, 0x7e, 0x0, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000007E902F), std::array<unsigned char, 5>{0xe9, 0xcc, 0x3, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C3E713), std::array<unsigned char, 5>{0xe9, 0xcd, 0x0, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000007D9B64), std::array<unsigned char, 5>{0xe9, 0xda, 0x0, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000007D945F), std::array<unsigned char, 5>{0xe9, 0xd0, 0x0, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000007DADAB), std::array<unsigned char, 2>{0xeb, 0x7a}.data(), 0x2);
    memcpy((void*)(globals::gameBase + 0x00000000007E951F), std::array<unsigned char, 5>{0xe9, 0xa8, 0x3, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000007E2CB7), std::array<unsigned char, 5>{0xe9, 0xa6, 0xf2, 0xff, 0xff}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000001132AF1), std::array<unsigned char, 5>{0xe9, 0x56, 0x2, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C3E970), std::array<unsigned char, 5>{0xe9, 0x7, 0x4, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000007D9DF2), std::array<unsigned char, 5>{0xe9, 0xf, 0x7, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000007D95FC), std::array<unsigned char, 5>{0xe9, 0x93, 0x2, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000007D15D5), std::array<unsigned char, 5>{0xe9, 0x15, 0x37, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000001133995), std::array<unsigned char, 5>{0xe9, 0xdb, 0x3, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000009C2E08), std::array<unsigned char, 2>{0xeb, 0x1b}.data(), 0x2);
    memcpy((void*)(globals::gameBase + 0x00000000007D2192), std::array<unsigned char, 5>{0xe9, 0xe9, 0x3, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000009C2EEE), std::array<unsigned char, 5>{0xe9, 0xb3, 0x2, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000805177), std::array<unsigned char, 2>{0xeb, 0x7b}.data(), 0x2);
    memcpy((void*)(globals::gameBase + 0x00000000007DB658), std::array<unsigned char, 5>{0xe9, 0xe6, 0x2e, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000007FAA1B), std::array<unsigned char, 5>{0xe9, 0xef, 0x5, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000005A144D), std::array<unsigned char, 2>{0xeb, 0x77}.data(), 0x2);
    memcpy((void*)(globals::gameBase + 0x0000000000805213), std::array<unsigned char, 5>{0xe9, 0xc8, 0x3, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000008086E8), std::array<unsigned char, 5>{0xe9, 0xce, 0x0, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000007D3FA1), std::array<unsigned char, 5>{0xe9, 0xc5, 0xd4, 0xff, 0xff}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x000000000080577A), std::array<unsigned char, 5>{0xe9, 0x4c, 0x6, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000808883), std::array<unsigned char, 5>{0xe9, 0xc1, 0x2, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000007DC0C2), std::array<unsigned char, 5>{0xe9, 0x79, 0x3, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C38F24), std::array<unsigned char, 5>{0xe9, 0x52, 0x3, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C3B58D), std::array<unsigned char, 5>{0xe9, 0x21, 0x3, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000808E07), std::array<unsigned char, 2>{0xeb, 0x7d}.data(), 0x2);
    memcpy((void*)(globals::gameBase + 0x00000000007F7F13), std::array<unsigned char, 5>{0xe9, 0x1, 0x6, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C3D417), std::array<unsigned char, 2>{0xeb, 0x22}.data(), 0x2);
    memcpy((void*)(globals::gameBase + 0x00000000007DDB21), std::array<unsigned char, 5>{0xe9, 0xd5, 0xd9, 0xff, 0xff}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000005A1914), std::array<unsigned char, 5>{0xe9, 0xaa, 0x16, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C3BEEA), std::array<unsigned char, 5>{0xe9, 0xd9, 0x0, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C3D594), std::array<unsigned char, 5>{0xe9, 0x2e, 0x5, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C3C174), std::array<unsigned char, 5>{0xe9, 0xc4, 0x6, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000007EBE76), std::array<unsigned char, 2>{0xeb, 0x7c}.data(), 0x2);
    memcpy((void*)(globals::gameBase + 0x00000000005A1DA6), std::array<unsigned char, 5>{0xe9, 0x4, 0x2, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C5203D), std::array<unsigned char, 5>{0xe9, 0x59, 0x3, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000809770), std::array<unsigned char, 5>{0xe9, 0x35, 0x39, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000007EBFEA), std::array<unsigned char, 5>{0xe9, 0xd1, 0x3, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C24AAE), std::array<unsigned char, 5>{0xe9, 0xd3, 0x0, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C3CF5B), std::array<unsigned char, 5>{0xe9, 0xf0, 0x3, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C51A0E), std::array<unsigned char, 5>{0xe9, 0xac, 0x2, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x000000000080D19D), std::array<unsigned char, 5>{0xe9, 0x63, 0x3, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x000000000080A1C0), std::array<unsigned char, 5>{0xe9, 0x5b, 0x4, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000007EC593), std::array<unsigned char, 5>{0xe9, 0x2d, 0x7, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C24CC3), std::array<unsigned char, 5>{0xe9, 0x93, 0x4, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000005A2942), std::array<unsigned char, 5>{0xe9, 0xb8, 0xee, 0xff, 0xff}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C26431), std::array<unsigned char, 5>{0xe9, 0xcf, 0x0, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C83B5A), std::array<unsigned char, 5>{0xe9, 0xb7, 0x2, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C39CFC), std::array<unsigned char, 5>{0xe9, 0x5a, 0x3, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C265DB), std::array<unsigned char, 5>{0xe9, 0xc1, 0xfd, 0xff, 0xff}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C52C7D), std::array<unsigned char, 5>{0xe9, 0x6d, 0x5, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000007FD6A4), std::array<unsigned char, 5>{0xe9, 0x7d, 0x0, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x000000000080C41D), std::array<unsigned char, 5>{0xe9, 0x5, 0xd3, 0xff, 0xff}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000008305AA), std::array<unsigned char, 5>{0xe9, 0xd6, 0x0, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000008110F1), std::array<unsigned char, 5>{0xe9, 0x87, 0x4, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000008307E8), std::array<unsigned char, 5>{0xe9, 0xd4, 0x5, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C1EFD8), std::array<unsigned char, 5>{0xe9, 0xd1, 0x0, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C3A1D6), std::array<unsigned char, 2>{0xeb, 0x1b}.data(), 0x2);
    memcpy((void*)(globals::gameBase + 0x0000000000C1F1DA), std::array<unsigned char, 5>{0xe9, 0x8e, 0x4, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C80EC6), std::array<unsigned char, 5>{0xe9, 0x21, 0x3, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C3A2FE), std::array<unsigned char, 5>{0xe9, 0xdf, 0x3, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000F3EE5A), std::array<unsigned char, 5>{0xe9, 0xd6, 0x0, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C1F9B3), std::array<unsigned char, 5>{0xe9, 0xcd, 0x0, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000F3F064), std::array<unsigned char, 5>{0xe9, 0x2b, 0x3, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C05679), std::array<unsigned char, 5>{0xe9, 0x27, 0x9, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C1FB4C), std::array<unsigned char, 5>{0xe9, 0xcf, 0xfd, 0xff, 0xff}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C3ADD9), std::array<unsigned char, 5>{0xe9, 0x5d, 0x3, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000009D260A), std::array<unsigned char, 2>{0xeb, 0x22}.data(), 0x2);
    memcpy((void*)(globals::gameBase + 0x0000000000C383B1), std::array<unsigned char, 5>{0xe9, 0x1a, 0x4, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000007FEF06), std::array<unsigned char, 5>{0xe9, 0xd7, 0x0, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000007FDE07), std::array<unsigned char, 5>{0xe9, 0x3, 0x2, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000817C0A), std::array<unsigned char, 5>{0xe9, 0xfe, 0x3, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000009D275B), std::array<unsigned char, 5>{0xe9, 0x5e, 0x4, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000CB17C2), std::array<unsigned char, 5>{0xe9, 0x6b, 0x3, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C05A69), std::array<unsigned char, 5>{0xe9, 0xf2, 0xf7, 0xff, 0xff}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000007FF0D6), std::array<unsigned char, 5>{0xe9, 0x77, 0x3, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000818CEC), std::array<unsigned char, 5>{0xe9, 0x25, 0x6, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000008180DC), std::array<unsigned char, 5>{0xe9, 0x9e, 0x2, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000CB6B30), std::array<unsigned char, 5>{0xe9, 0xfa, 0x4, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000E630AB), std::array<unsigned char, 5>{0xe9, 0xd3, 0x0, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000E62596), std::array<unsigned char, 5>{0xe9, 0xd8, 0x0, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000007FEA60), std::array<unsigned char, 5>{0xe9, 0xfa, 0xef, 0xff, 0xff}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000E63C58), std::array<unsigned char, 5>{0xe9, 0xe8, 0x0, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000E632FA), std::array<unsigned char, 5>{0xe9, 0x26, 0xfc, 0xff, 0xff}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000E62779), std::array<unsigned char, 5>{0xe9, 0x98, 0x2, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x000000000101258C), std::array<unsigned char, 5>{0xe9, 0xaf, 0x2, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C06C22), std::array<unsigned char, 5>{0xe9, 0xd4, 0x3, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C75113), std::array<unsigned char, 5>{0xe9, 0xc7, 0xfe, 0xff, 0xff}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000E63F5C), std::array<unsigned char, 5>{0xe9, 0x76, 0xfb, 0xff, 0xff}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000CA2F7E), std::array<unsigned char, 5>{0xe9, 0x58, 0x5, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C9CD8A), std::array<unsigned char, 5>{0xe9, 0x38, 0xfe, 0xff, 0xff}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C9C381), std::array<unsigned char, 5>{0xe9, 0xd8, 0x0, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000A4B54D), std::array<unsigned char, 5>{0xe9, 0xf3, 0x4, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C9C570), std::array<unsigned char, 5>{0xe9, 0xfa, 0x3, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000A4BB85), std::array<unsigned char, 5>{0xe9, 0xd9, 0x4, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000CB5108), std::array<unsigned char, 5>{0xe9, 0xfe, 0xc, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000CA3CC3), std::array<unsigned char, 5>{0xe9, 0xf7, 0x8, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C9D8C4), std::array<unsigned char, 5>{0xe9, 0x1a, 0x3, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C75B53), std::array<unsigned char, 5>{0xe9, 0xce, 0x0, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000CB5493), std::array<unsigned char, 5>{0xe9, 0xd6, 0x0, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C76593), std::array<unsigned char, 5>{0xe9, 0xce, 0x0, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C75D6B), std::array<unsigned char, 5>{0xe9, 0x1c, 0x5, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000CB56E6), std::array<unsigned char, 5>{0xe9, 0x1d, 0xf8, 0xff, 0xff}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C767AB), std::array<unsigned char, 5>{0xe9, 0x1c, 0x5, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C94624), std::array<unsigned char, 2>{0xeb, 0x1b}.data(), 0x2);
    memcpy((void*)(globals::gameBase + 0x0000000000C1BE66), std::array<unsigned char, 5>{0xe9, 0xca, 0x0, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C94710), std::array<unsigned char, 5>{0xe9, 0xc1, 0x2, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C1C0A8), std::array<unsigned char, 5>{0xe9, 0xfc, 0x5, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C082B7), std::array<unsigned char, 5>{0xe9, 0xd8, 0x0, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C312C7), std::array<unsigned char, 5>{0xe9, 0xc9, 0x0, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C084E9), std::array<unsigned char, 5>{0xe9, 0x71, 0x5, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C3145F), std::array<unsigned char, 5>{0xe9, 0xc4, 0x1, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C093A5), std::array<unsigned char, 5>{0xe9, 0xce, 0x0, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C095D3), std::array<unsigned char, 5>{0xe9, 0x87, 0x5, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C9FE42), std::array<unsigned char, 5>{0xe9, 0xde, 0x0, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000CA009F), std::array<unsigned char, 5>{0xe9, 0xf4, 0x5, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000D4D705), std::array<unsigned char, 5>{0xe9, 0xcb, 0x0, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000D4D8C3), std::array<unsigned char, 5>{0xe9, 0xfc, 0xfb, 0xff, 0xff}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C2D9B4), std::array<unsigned char, 5>{0xe9, 0xcc, 0x0, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C2DB4F), std::array<unsigned char, 5>{0xe9, 0xfd, 0xfd, 0xff, 0xff}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C739C6), std::array<unsigned char, 5>{0xe9, 0x26, 0xfe, 0xff, 0xff}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C35A3F), std::array<unsigned char, 2>{0xeb, 0x22}.data(), 0x2);
    memcpy((void*)(globals::gameBase + 0x0000000000C35B33), std::array<unsigned char, 5>{0xe9, 0x93, 0x2, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C2E015), std::array<unsigned char, 5>{0xe9, 0xc3, 0x0, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C33CB7), std::array<unsigned char, 5>{0xe9, 0xce, 0x0, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C2E1EF), std::array<unsigned char, 5>{0xe9, 0xfe, 0x3, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C33E84), std::array<unsigned char, 5>{0xe9, 0x4c, 0x3, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C362D0), std::array<unsigned char, 5>{0xe9, 0xc8, 0xfe, 0xff, 0xff}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C2E998), std::array<unsigned char, 5>{0xe9, 0xcb, 0x0, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C2EC08), std::array<unsigned char, 5>{0xe9, 0x86, 0xfc, 0xff, 0xff}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000CBC913), std::array<unsigned char, 5>{0xe9, 0xd6, 0x0, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000CBCBE8), std::array<unsigned char, 5>{0xe9, 0x23, 0x8, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C78165), std::array<unsigned char, 5>{0xe9, 0xbd, 0x0, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C783AA), std::array<unsigned char, 5>{0xe9, 0xee, 0x5, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C78EDA), std::array<unsigned char, 5>{0xe9, 0xbd, 0x0, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000C790DE), std::array<unsigned char, 5>{0xe9, 0x48, 0xfd, 0xff, 0xff}.data(), 0x5);

    memcpy((void*)(globals::gameBase + 0x000000000113CD10), std::array<unsigned char, 2>{0xeb, 0x1e}.data(), 0x2);
    memcpy((void*)(globals::gameBase + 0x0000000001130A8C), std::array<unsigned char, 5>{0xe9, 0xd4, 0xc, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000005A5A78), std::array<unsigned char, 5>{0xe9, 0xcd, 0x3, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000007E1C26), std::array<unsigned char, 5>{0xe9, 0x5c, 0x1, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000011325F9), std::array<unsigned char, 5>{0xe9, 0xd0, 0xc, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000007D0CC1), std::array<unsigned char, 5>{0xe9, 0x36, 0x3, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000007DAE32), std::array<unsigned char, 5>{0xe9, 0xd5, 0x2, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000005A14C9), std::array<unsigned char, 5>{0xe9, 0x4b, 0x1, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x0000000000808E91), std::array<unsigned char, 5>{0xe9, 0x96, 0x3, 0x0, 0x0}.data(), 0x5);
    memcpy((void*)(globals::gameBase + 0x00000000007FD729), std::array<unsigned char, 5>{0xe9, 0x48, 0x1, 0x0, 0x0}.data(), 0x5);
    printf("patched all antidebug traps\n");

    memset((void*)(globals::gameBase + 0x805607), 0x85, 1);
    printf("patched veh set trap\n");

    printf("casc log hook\n");
    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xba00e0), cascLogHook, (LPVOID*)&cascLogHook_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xba00e0)));

    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xf4dff0), statescript_load_hook, (LPVOID*)&statescript_load_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xf4dff0)));

    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xb2ae70), stuux_string_hash_hook, (LPVOID*)&stuux_string_hash_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xb2ae70)));
    printf("stuux string hash uxlink\n");

    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0x9c3500), manager_14_init_hook, (LPVOID*)&manager_14_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0x9c3500)));
    printf("Manager 14 initialization hook\n");

    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xcd0590), dataflow_bugfix_fn, (LPVOID*)&dataflow_bugfix_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xcd0590)));
    printf("filterbits dataflow fix\n");

    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xc541c0), Construct_GameEntityAdmin_fn, (LPVOID*)&Construct_GameEntityAdmin_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xc541c0)));
    printf("gameEA initialize hook\n");

    char ret_1[] = { 0xB0, 0x01, 0xC3 };
    memcpy((void*)(globals::gameBase + 0xf4c920), ret_1, sizeof(ret_1)); //Debug Statescript log enabled
    memcpy((void*)(globals::gameBase + 0xcfd740), ret_1, sizeof(ret_1)); //Force Enable all heroes
    memset((void*)(globals::gameBase + 0x7e1f69), 0x90, 8);
    memset((void*)(globals::gameBase + 0xf44bba), 0x90, 0x13);

    //Return address checks from getHealth func
    memset((void*)(globals::gameBase + 0xcc6110), 0x90, 0xcc67f5 - 0xcc6110);
    memset((void*)(globals::gameBase + 0xcc57d3), 0x90, 0xcc5f92 - 0xcc57d3);

    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xc43a40), selectiveResLoad_hook, (PVOID*)&selectiveResLoad_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xc43a40)));
    printf("selectiveResLoad_hook\n");

    MH_VERIFY(MH_CreateHook((PVOID)(globals::gameBase + 0xc43ab0), System63_hook, (PVOID*)&System63_orig));
    MH_VERIFY(MH_EnableHook((PVOID)(globals::gameBase + 0xc43ab0)));
    printf("system63 bugfix\n"); 

    ((_TlsCallbackFunc)(globals::gameBase + TlsCallback2_Addr))(globals::gameBase, 3, 0);

    printf("Calling static prelaunch window initializers\n");
    window_manager::call_preStartInitialize();

    printf("Loading hashlib\n");
    allmighty_hash_lib::initialize();
    stu_resources::initialize();

    printf("Prepared to launch %p.\n", globals::gameBase + Start_Addr);
    //system("pause");
    void(*pFunc)() = (void(*)())(globals::gameBase + Start_Addr);
    pFunc();
}

__int64 AddVehHook(__int64 old, __int64 func) {
    if ((DWORD_PTR)_ReturnAddress() > globals::gameBase && (DWORD_PTR)_ReturnAddress() < globals::gameBase + globals::gameSize)
        return AddVeh_orig(old, func);
    printf("Caught RtlAddVectoredExceptionHandler %d %p ret: %p\n", old, func, _ReturnAddress());
    return 0;
}

int __stdcall VectoredExceptionHandler(EXCEPTION_POINTERS* exception) {
    if (exception->ExceptionRecord->ExceptionCode == EXCEPTION_BREAKPOINT ||
        exception->ExceptionRecord->ExceptionCode == EXCEPTION_SINGLE_STEP ||
        exception->ExceptionRecord->ExceptionCode == EXCEPTION_GUARD_PAGE)
        return EXCEPTION_CONTINUE_SEARCH;
    if (exception->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
        if (exception->ContextRecord->Rip > (DWORD64)std::addressof(IsBadReadPtr) && exception->ContextRecord->Rip - (DWORD64)std::addressof(IsBadReadPtr) < 0x100)
            return EXCEPTION_CONTINUE_SEARCH;
        if (exception->ContextRecord->Rip >= globals::modBase && exception->ContextRecord->Rip < globals::modBase + globals::modSize) {
            return EXCEPTION_CONTINUE_SEARCH;
        }
        if (exception->ExceptionRecord->NumberParameters >= 2 &&
            exception->ExceptionRecord->ExceptionInformation[1] >= globals::modBase &&
            exception->ExceptionRecord->ExceptionInformation[1] < globals::modBase + globals::modSize) {
            return EXCEPTION_CONTINUE_SEARCH;
        }
    }
    if (exception->ExceptionRecord->ExceptionCode == 0x40010006 ||
        exception->ExceptionRecord->ExceptionCode == 0x406D1388 ||
        exception->ExceptionRecord->ExceptionCode == 0xE06D7363 ||
        (exception->ExceptionRecord->ExceptionCode & 0xC0000000) != 0xC0000000)
        return EXCEPTION_CONTINUE_SEARCH;
    if (exception->ExceptionRecord->ExceptionCode == 0xc0000008)
        return EXCEPTION_CONTINUE_EXECUTION;

    DWORD64 rip = exception->ContextRecord->Rip;
    DWORD64 rva = (rip >= globals::gameBase && rip < globals::gameBase + globals::gameSize) ? (rip - globals::gameBase) : rip;
    DWORD64 mod_rva = (rip >= globals::modBase && rip < globals::modBase + globals::modSize) ? (rip - globals::modBase) : 0;
    std::string fmt = ExceptionFormatter::FormatException(exception);
    printf("New exception: %d 0x%x (gameRVA=+0x%llX modRVA=+0x%llX)\n", GetCurrentThreadId(), GetCurrentThreadId(),
        (unsigned long long)rva, (unsigned long long)mod_rva);
    printf("%s\n", fmt.c_str());

    ShotDiagLogFmt("[CRASH_VEH] code=0x%08X rip=0x%llX gameRVA=+0x%llX modRVA=+0x%llX rcx=0x%llX rdx=0x%llX r8=0x%llX r9=0x%llX rbx=0x%llX rsi=0x%llX rdi=0x%llX",
        exception->ExceptionRecord->ExceptionCode,
        (unsigned long long)rip, (unsigned long long)rva, (unsigned long long)mod_rva,
        (unsigned long long)exception->ContextRecord->Rcx,
        (unsigned long long)exception->ContextRecord->Rdx,
        (unsigned long long)exception->ContextRecord->R8,
        (unsigned long long)exception->ContextRecord->R9,
        (unsigned long long)exception->ContextRecord->Rbx,
        (unsigned long long)exception->ContextRecord->Rsi,
        (unsigned long long)exception->ContextRecord->Rdi);

    // Dump stack return addresses in GameClientApp.exe / prometheus.dll
    DWORD64 rsp = exception->ContextRecord->Rsp;
    std::string stack_rvas;
    for (int i = 0; i < 48; i++) {
        DWORD64 val = 0;
        if (!IsBadReadPtr((const void*)(rsp + i * 8), 8)) {
            val = *(const DWORD64*)(rsp + i * 8);
            if (val >= globals::gameBase && val < globals::gameBase + globals::gameSize) {
                stack_rvas += std::format("+0x{:X} ", val - globals::gameBase);
            } else if (val >= globals::modBase && val < globals::modBase + globals::modSize) {
                stack_rvas += std::format("dll+0x{:X} ", val - globals::modBase);
            }
        }
    }
    ShotDiagLogFmt("[CRASH_STACK] %s", stack_rvas.c_str());

    // Write dummy_combat_diag.log from VEH when shots have been fired or a local player entity is active in GameEntityAdmin
    static bool s_in_veh_dump = false;
    bool in_live_session = (WeaponHardpointDiag::s_shot_raycast_calls > 0) || (GameEntityAdmin() && GameEntityAdmin()->local_entid != 0);
    if (in_live_session && !s_in_veh_dump) {
        s_in_veh_dump = true;
        // Write raw crash exception info FIRST before calling DumpDummyCombatForensics so recursive faults can never lose the crash log
        if (FILE* f = fopen("dummy_combat_diag.log", "wb")) {
            fprintf(f, "## CRASH EXCEPTION DUMP (PRE-FORENSICS)\n```text\ngameRVA=+0x%llX modRVA=+0x%llX\nstack: %s\n%s\n```\n",
                (unsigned long long)rva, (unsigned long long)mod_rva, stack_rvas.c_str(), fmt.c_str());
            fflush(f);
            fclose(f);
        }
        WeaponHardpointDiag::DumpDummyCombatForensics(GameEntityAdmin(), "AUTO_CRASH_VEH_DUMP");
        if (FILE* f = fopen("dummy_combat_diag.log", "ab")) {
            fprintf(f, "\n## 4. CRASH EXCEPTION DUMP\n```text\ngameRVA=+0x%llX modRVA=+0x%llX\nstack: %s\n%s\n```\n",
                (unsigned long long)rva, (unsigned long long)mod_rva, stack_rvas.c_str(), fmt.c_str());
            fflush(f);
            fclose(f);
        }
        s_in_veh_dump = false;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

bool CheckDebuggerPresentHook() {
    printf("CheckDebuggerPresent %p\n", _ReturnAddress());
    return false;
}

//https://stackoverflow.com/questions/557081/how-do-i-get-the-hmodule-for-the-currently-executing-code
HMODULE GetCurrentModule()
{ // NB: XP+ solution!
    HMODULE hModule = NULL;
    GetModuleHandleEx(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
        (LPCTSTR)GetCurrentModule,
        &hModule);

    return hModule;
}


BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    static std::once_flag entrypoint_mutex;
    std::call_once(entrypoint_mutex, [] {
        globals::ensure_console_allocated();
        printf("Cheers love!\n");

        printf("Adding VEH: %p\n", AddVectoredExceptionHandler(true, (PVECTORED_EXCEPTION_HANDLER)VectoredExceptionHandler));
        globals::gameBase = (DWORD_PTR)GetModuleHandleA(nullptr);
        globals::modBase = (DWORD_PTR)GetCurrentModule();
        const auto pe = Pe::PeNative::fromModule(GetModuleHandleA(NULL));
        globals::gameSize = pe.imageSize();
        const auto modpe = Pe::PeNative::fromModule(GetCurrentModule());
        globals::modSize = modpe.imageSize();
        printf("Module Base: %p %p\n", globals::modBase, globals::modSize);
        printf("Game Base: %p %p\n", globals::gameBase, globals::gameSize);
        mainThreadId = GetCurrentThreadId();
        printf("Main Thread: %d (%x)\n", mainThreadId, mainThreadId);
        printf("Patching back TLSCallback0 at %p!\n");
        DWORD_PTR tlsCallback = TlsCallback_Addr + globals::gameBase;
        DWORD old;
        VirtualProtect((void*)(tlsCallback), sizeof(TlsCallback_0), PAGE_EXECUTE_READWRITE, &old);
        memcpy((void*)tlsCallback, TlsCallback_0, sizeof(TlsCallback_0));

        MH_VERIFY(MH_Initialize());

        MH_VERIFY(MH_CreateHook(ExitProcess, ExitProcessHook, (LPVOID*)&ExitProcess_orig));
        MH_VERIFY(MH_EnableHook(ExitProcess));

        auto addVeh = GetProcAddress(LoadLibraryA("ntdll.dll"), "RtlAddVectoredExceptionHandler");
        MH_VERIFY(MH_CreateHook(addVeh, AddVehHook, (LPVOID*)&AddVeh_orig));
        MH_VERIFY(MH_EnableHook(addVeh));

        MH_VERIFY(MH_CreateHook(IsDebuggerPresent, CheckDebuggerPresentHook, 0));
        MH_VERIFY(MH_EnableHook(IsDebuggerPresent));

        MH_VERIFY(MH_CreateHook(CheckRemoteDebuggerPresent, CheckDebuggerPresentHook, 0));
        MH_VERIFY(MH_EnableHook(CheckRemoteDebuggerPresent));

        printf("Creating WinMain Hook...\n");
        char starthook[] = {
            0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //mov rax,0
            0xFF, 0xE0 //jmp rax
        };
        VirtualProtect((void*)(globals::gameBase + Start_Addr), sizeof(starthook), PAGE_EXECUTE_READWRITE, &old);
        memcpy((void*)(globals::gameBase + Start_Addr), starthook, sizeof(starthook));
        DWORD_PTR thatAddr = (DWORD_PTR)&StartHook;
        memcpy((void*)(globals::gameBase + Start_Addr + 2), (void*)&thatAddr, 8);
        });
    return TRUE;
}


