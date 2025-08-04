/**
* @author Txt_Text
*                    Demo
* ###########################################
* #                                         #
* #             Wave Visualizer             #
* #                                         #
* #                音量示波器                #
* #                                         #
* #               By Txt_Text               #
* #                                         #
* ###########################################
*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl/GL.h>
#include <tchar.h>
#include <vector>
#include <cmath>

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

// Globals
HDC hDC;
HGLRC hRC;
HWND hWnd;
ma_device device;
std::vector<float> waveform(512, 0.0f);
bool showBars = false;//显示额外的示波器
bool enableGlow = false;//开启发光
bool visible = true;//可见
int width = GetSystemMetrics(SM_CXSCREEN) + 100;//获取主显示器屏幕宽度
int height = 200;


bool dragging = false;
POINT lastCursor = { 0 };

//回调音频
void audio_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    const float* in = (const float*)pInput;
    for (int i = 0; i < 512 && i < frameCount; ++i) {
        waveform[i] = in[i];
    }
}

//启用 OpenGL
void SetupOpenGL() {
    PIXELFORMATDESCRIPTOR pfd = { sizeof(PIXELFORMATDESCRIPTOR), 1, PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, PFD_TYPE_RGBA, 32 };
    int pf = ChoosePixelFormat(hDC, &pfd);
    SetPixelFormat(hDC, pf, &pfd);
    hRC = wglCreateContext(hDC);
    wglMakeCurrent(hDC, hRC);
}

//绘制波形线条
void DrawWaveform(float intensity) {
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i < width; ++i) {
        //修正采样：从 0 到 511 平均映射 width 个像素
        float pos = ((float)i + 0.5f) * 512.0f / width;
        int i0 = (int)pos;
        int i1 = min(i0 + 1, 511);//防止越界
        float frac = pos - i0;
        float sample = waveform[i0] * (1.0f - frac) + waveform[i1] * frac;

        float x = ((float)i / (width - 1)) * 2.0f - 1.0f;
        
        // 1.0 是 OpenGL 顶部，sample 往下偏移
        float y = 1.0f - fabsf(sample) * intensity * 2.0f;
        //float y = sample * 0.8f * intensity;

        glColor4f(0.0f, 1.0f, 0.0f, intensity * 0.2f);
        glVertex2f(x, y);
    }
    glEnd();
}
//void DrawWaveform(float intensity) {
//    glBegin(GL_LINE_STRIP);
//    for (int i = 0; i < width; ++i) {
//        //插值采样：将 i 均匀映射到 waveform 的 0~511 范围中
//        float pos = (float)i / (width - 1) * 511.0f;
//        int i0 = (int)pos;
//        int i1 = min(i0 + 1, 511);//防止越界
//        float frac = pos - i0;
//        float sample = waveform[i0] * (1.0f - frac) + waveform[i1] * frac;
//
//        float x = ((float)i / (width - 1)) * 2.0f - 1.0f;
//        float y = sample * 0.8f * intensity;
//
//        // 绿色波形
//        //glColor4f(0.0f, 1.0f, 0.0f, intensity * 0.2f);
//
//        int colorBand = i / 100 % 3;
//        switch (colorBand) {
//        case 0: glColor4f(1.0f, 0.0f, 0.0f, 1.0f); break;
//        case 1: glColor4f(0.0f, 1.0f, 0.0f, 1.0f); break;
//        case 2: glColor4f(0.0f, 0.0f, 1.0f, 1.0f); break;
//        }
//
//        glVertex2f(x, y);
//    }
//    glEnd();
//}

//void DrawWaveform(float intensity) {
//    glBegin(GL_LINE_STRIP);
//    for (int i = 0; i < width; ++i) {
//        int index = (i * 512) / width;
//        float x = ((float)i / (width - 1)) * 2.0f - 1.0f;
//        float y = waveform[index] * 0.8f * intensity;
//        /*float r = (float)i / width;//彩色
//        float g = 1.0f - r;
//        glColor4f(r, g, 0.6f, intensity * 0.2f);*/
//        glColor4f(0.0f, 1.0f, 0.0f, intensity * 0.2f);//绿色
//
//        glVertex2f(x, y);
//    }
//    glEnd();
//}

// Draw bar spectrum
void DrawBars() {
    glBegin(GL_QUADS);
    for (int i = 0; i < width; ++i) {
        int index = (i * 512) / width;
        float x = ((float)i / (width - 1)) * 2.0f - 1.0f;
        float barWidth = 2.0f / width;
        float y = fabsf(waveform[index]);
        float r = (float)i / width;
        float g = 1.0f - r;
        glColor4f(r, g, 0.4f, 1.0f);
        glVertex2f(x, -1.0f);
        glVertex2f(x + barWidth, -1.0f);
        glVertex2f(x + barWidth, -1.0f + y * 2.0f);
        glVertex2f(x, -1.0f + y * 2.0f);
    }
    glEnd();
}

// Render frame
void Render() {
    if (!visible) return;
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    if (enableGlow) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        for (float i = 0.2f; i <= 1.0f; i += 0.2f) DrawWaveform(i);
        glDisable(GL_BLEND);
    }
    else {
        glColor3f(0.0f, 1.0f, 0.0f);
        DrawWaveform(1.0f);
    }
    if (showBars) DrawBars();
    SwapBuffers(hDC);
}

// Win32 Window Proc
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_DESTROY: PostQuitMessage(0); break;
    case WM_KEYDOWN:
        if (wParam == VK_SPACE) showBars = !showBars;
        if (wParam == 'G') enableGlow = !enableGlow;
        if (wParam == 'V') visible = !visible;
        break;
    case WM_LBUTTONDOWN: dragging = true; GetCursorPos(&lastCursor); break;
    case WM_LBUTTONUP: dragging = false; break;
    case WM_MOUSEMOVE:
        if (dragging) {
            POINT pt; GetCursorPos(&pt);
            int dx = pt.x - lastCursor.x, dy = pt.y - lastCursor.y;
            RECT rect; GetWindowRect(hwnd, &rect);
            MoveWindow(hwnd, rect.left + dx, rect.top + dy, width, height, TRUE);
            lastCursor = pt;
        }
        break;
    default: return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

//主窗口入口点相关
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = _T("WaveformVisualizer");
    RegisterClass(&wc);
    hWnd = CreateWindowEx(//透明窗口，置顶
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
        wc.lpszClassName, _T("Visualizer"), WS_POPUP,
        0, 0, width, height, NULL, NULL, hInstance, NULL
    );
    SetLayeredWindowAttributes(hWnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
    ShowWindow(hWnd, SW_SHOW);

    hDC = GetDC(hWnd);
    SetupOpenGL();

    ma_device_config config = ma_device_config_init(ma_device_type_loopback);
    config.capture.format = ma_format_f32;
    config.capture.channels = 1;
    config.sampleRate = 48000;
    config.dataCallback = audio_callback;
    if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) return -1;
    ma_device_start(&device);

    MSG msg = { 0 };
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            Render();
            Sleep(16);
        }
    }

    ma_device_uninit(&device);
    wglDeleteContext(hRC);
    ReleaseDC(hWnd, hDC);
    return 0;
}
