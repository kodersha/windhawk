// ==WindhawkMod==
// @id              desktop-acrylic-blur
// @name            Desktop Acrylic Blur
// @description     Acrulic blur for the Windows desktop
// @version         1.0
// @author          kodersha
// @include         explorer.exe
// @compilerOptions -ldwmapi
// @license         MIT
// ==/WindhawkMod==

#include <windows.h>
#include <dwmapi.h>

#include <cstdint>

#pragma comment(lib, "dwmapi.lib")

constexpr DWORD WCA_ACCENT_POLICY_VALUE = 19;

struct ACCENT_POLICY_LOCAL {
  int AccentState;
  int AccentFlags;
  DWORD GradientColor;
  int AnimationId;
};

struct WINDOWCOMPOSITIONATTRIBDATA_LOCAL {
  DWORD Attribute;
  PVOID Data;
  SIZE_T SizeOfData;
};

using SetWindowCompositionAttributeFn =
  BOOL(WINAPI*)(HWND, WINDOWCOMPOSITIONATTRIBDATA_LOCAL*);

static SetWindowCompositionAttributeFn g_setCompositionAttribute = nullptr;
static UINT_PTR g_timer = 0;

static bool ApplyAeroBlur(HWND hwnd) {
  if (!IsWindow(hwnd) || !g_setCompositionAttribute) {
    return false;
  }

  ACCENT_POLICY_LOCAL policy = {};
  policy.AccentState = 4;
  policy.AccentFlags = 0;
  policy.GradientColor = 0x01000000;
  policy.AnimationId = 0;

  WINDOWCOMPOSITIONATTRIBDATA_LOCAL data = {};
  data.Attribute = WCA_ACCENT_POLICY_VALUE;
  data.Data = &policy;
  data.SizeOfData = sizeof(policy);

  return g_setCompositionAttribute(hwnd, &data) != FALSE;
}

static BOOL CALLBACK EnumDesktopWindows(HWND hwnd, LPARAM) {
  wchar_t className[128] = {};

  if (!GetClassNameW(hwnd, className, ARRAYSIZE(className))) {
    return TRUE;
  }

  if (lstrcmpW(className, L"Progman") == 0 ||
    lstrcmpW(className, L"WorkerW") == 0) {
    bool success = ApplyAeroBlur(hwnd);

    Wh_Log(
      L"Aero blur: class=%s HWND=%p result=%d",
      className,
      hwnd,
      success ? 1 : 0
    );
  }

  return TRUE;
}

static void ApplyDesktopBlur() {
  EnumWindows(EnumDesktopWindows, 0);
}

static VOID CALLBACK TimerProc(HWND, UINT, UINT_PTR, DWORD) {
  ApplyDesktopBlur();
}

BOOL Wh_ModInit() {
  HMODULE user32 = GetModuleHandleW(L"user32.dll");

  if (user32) {
    g_setCompositionAttribute =
      reinterpret_cast<SetWindowCompositionAttributeFn>(
        GetProcAddress(user32, "SetWindowCompositionAttribute")
      );
  }

  if (!g_setCompositionAttribute) {
    Wh_Log(L"SetWindowCompositionAttribute is unavailable");
    return FALSE;
  }

  ApplyDesktopBlur();

  g_timer = SetTimer(nullptr, 0, 2000, TimerProc);

  if (!g_timer) {
    Wh_Log(L"Failed to create refresh timer");
    return FALSE;
  }

  return TRUE;
}

void Wh_ModBeforeUninit() {
  if (g_timer) {
    KillTimer(nullptr, g_timer);
    g_timer = 0;
  }
}
