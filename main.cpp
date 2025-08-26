#include <iostream>
#include <windows.h>
#include <WinUser.h>
#include <stdint.h>
#include <xinput.h>

#define internal static
#define global_variable static
#define local_persist static

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

struct win32_offscreen_buffer {
    BITMAPINFO Info;
    void* Memory;
    int Width;
    int Height;
    int BytesPerPixel;
    int Pitch;
};

struct win32_window_dimensions {
    int Width;
    int Height;
};

// global for now.
global_variable bool Running;
global_variable win32_offscreen_buffer GlobalBackbuffer;

// XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub) {
    return 0;
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;

// XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub) {
    return 0;
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;

internal void Win32LoadXInput() {
    HMODULE XInputLibrary = LoadLibraryA("xinput1_3.dll");
    if (XInputLibrary) {
        XInputGetState_ = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
        XInputSetState_ = (x_input_set_state * )GetProcAddress(XInputLibrary, "XInputSetState");
    }
}

internal void RenderWeirdGradient(win32_offscreen_buffer *Buffer, int XOffset, int YOffset) {
    uint8* Row = (uint8*)Buffer->Memory;
    for (int Y = 0; Y < Buffer->Height; ++Y) {
        uint32* Pixel = (uint32*)Row;
        for (int X = 0; X < Buffer->Width; ++X) {
            // Pixel in memory: BB GG RR xx
            //BLUE
            uint8 Blue = (X + XOffset) ;
            uint8 Green = (Y + YOffset);

            *Pixel++ = (Green << 8 | Blue);
        }
        Row += Buffer->Pitch;
    }
}

internal win32_window_dimensions Win32GetWindowDimensions(HWND Window) {
    RECT Rect;
    GetClientRect(Window, &Rect);
    win32_window_dimensions Dimensions{};
    Dimensions.Width = Rect.right - Rect.left;
    Dimensions.Height = Rect.bottom - Rect.top;
    return Dimensions;
}

internal void Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
    if (Buffer->Memory) {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    Buffer->Width = Width;
    Buffer->Height = Height;
    Buffer->BytesPerPixel = 4;

    // free our DIBSection
    // when biheight field is negative this is the clue to windows treat this bitmap as top-down meaning the first three bytes of the iamge are the colour for the top left pixel
    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    //Bullet-proof this maybe don't free first free after then free first if that fails.
    int BitmapMemorySize = (Buffer->Width*Buffer->Height) * Buffer->BytesPerPixel;
    Buffer->Memory = VirtualAlloc(nullptr, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
    Buffer->Pitch = Width*Buffer->BytesPerPixel;
}

internal void Win32DisplayBufferInWindow(HDC DeviceContext, int WindowWidth, int WindowHeight, win32_offscreen_buffer *Buffer, int X, int Y, int Width, int Height)
{
    StretchDIBits(DeviceContext,
        0, 0, WindowWidth, WindowHeight,
        0, 0, Buffer->Width, Buffer->Height,
        Buffer->Memory, &Buffer->Info, DIB_RGB_COLORS, SRCCOPY);
}


internal LRESULT CALLBACK Win32MainWindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam) {
    LRESULT Result = 0;
    switch (Message) {
    case WM_SIZE:
    {
    } break;
    case WM_CLOSE:
    case WM_DESTROY:
    {
        // handle as an error - recreate window?
        Running = false;
    } break;
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
        uint32 VKCode = WParam;
        bool WasDown = (LParam & 1 << 30) != 0;
        bool IsDown = (LParam & 1 << 31) == 0;
        if (WasDown != IsDown) {
            if (VKCode == 'W') {
                OutputDebugStringA("W");
            }
            else if (VKCode == 'S') {
                OutputDebugStringA("S");
            }
            else if (VKCode == 'A') {
                OutputDebugStringA("A");
            }
            else if (VKCode == 'D') {
                OutputDebugStringA("D");
            }
            else if (VKCode == 'Q') {
                OutputDebugStringA("Q");
            }
            else if (VKCode == 'E') {
                OutputDebugStringA("E");
            }
            else if (VKCode == VK_UP) {
                OutputDebugStringA("U");
            }
            else if (VKCode == VK_LEFT) {
                OutputDebugStringA("L");
            }
            else if (VKCode == VK_RIGHT) {
                OutputDebugStringA("R");
            }
            else if (VKCode == VK_DOWN) {
                OutputDebugStringA("D");
            }
            else if (VKCode == VK_SPACE) {
                OutputDebugStringA("SPACE");
            }
            else if (VKCode == VK_ESCAPE) {
                if (IsDown) {
                    OutputDebugStringA("ESCAPE DOWN");
                }
                if (WasDown) {
                    OutputDebugStringA("ESCAPE WAS DOWN");
                }
            }
        }
    } break;
    case WM_PAINT:
    {
        PAINTSTRUCT Paint;
        win32_window_dimensions Dimensions = Win32GetWindowDimensions(Window);
        HDC DeviceContext = BeginPaint(Window, &Paint);
        int X = Paint.rcPaint.left;
        int Y = Paint.rcPaint.top;
        int Width = Paint.rcPaint.right - Paint.rcPaint.left;
        int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
        Win32DisplayBufferInWindow(DeviceContext,
            Dimensions.Width, Dimensions.Height, &GlobalBackbuffer, X, Y, Width, Height);
        EndPaint(Window, &Paint);
    }
    case WM_ACTIVATEAPP:
    {
        OutputDebugString(reinterpret_cast<LPCSTR>(L"WM_ACTIVEATEAPP\n"));
    } break;
    default:
    {
            Result = DefWindowProc(Window, Message, WParam, LParam);
    } break;
    }
    return Result;
}

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
{
    Win32LoadXInput();
    WNDCLASSA WindowClass = {};

    Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720);

    WindowClass.style = CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = reinterpret_cast<LPCSTR>(L"HandmadeHeroWindowClass");

    if (RegisterClass(&WindowClass))
    {
        HWND Window = CreateWindowEx(
            0,
            WindowClass.lpszClassName,
            reinterpret_cast<LPCSTR>(L"Handmade Hero"),
            WS_OVERLAPPEDWINDOW|WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            nullptr,
            nullptr,
            Instance,
            nullptr);
        if (Window)
        {
            int XOffset = 0;
            int YOffset = 0;
            Running = true;
            MSG Message;
            while  (Running) {
                while (PeekMessageA(&Message, nullptr, 0, 0, PM_REMOVE)) {
                        TranslateMessage(&Message);
                        DispatchMessageA(&Message);
                        if (Message.message == WM_QUIT) {
                            Running = false;
                        }
                }
                // TODO Should we pull this more frequently?
                for (DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ControllerIndex++) {
                    XINPUT_STATE ControllerState;
                    if (XInputGetState_(ControllerIndex, &ControllerState) == ERROR_SUCCESS) {
                        // controller is plugged in
                        // dwPacket number might increment too rapidly, take a look
                        XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;
                        bool Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                        bool Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                        bool Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                        bool Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                        bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
                        bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
                        bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
                        bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
                        bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
                        bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);
                        bool LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                        bool RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);

                        int16 StickX = Pad->sThumbLX;
                        int16 StickY = Pad->sThumbLY;

                        if (AButton) {
                            YOffset += 2;
                            XINPUT_VIBRATION Vibration;
                            Vibration.wLeftMotorSpeed = 60000;
                            Vibration.wRightMotorSpeed = 60000;
                            XInputSetState_(ControllerIndex, &Vibration);
                        }
                        else {
                            XINPUT_VIBRATION Vibration;
                            Vibration.wLeftMotorSpeed = 0;
                            Vibration.wRightMotorSpeed = 0;
                            XInputSetState_(ControllerIndex, &Vibration);
                        }
                    } else {
                       // controller not available
                    }

                }

                HDC DeviceContext = GetDC(Window);
                win32_window_dimensions Dimensions = Win32GetWindowDimensions(Window);
                RenderWeirdGradient(&GlobalBackbuffer, XOffset, YOffset);
                Win32DisplayBufferInWindow(DeviceContext, Dimensions.Width, Dimensions.Height, &GlobalBackbuffer, 0, 0, Dimensions.Width, Dimensions.Height);
                ReleaseDC(Window, DeviceContext);
                ++XOffset;
           }
        }
        else
        {
            // TODO: logging
        }
    }
    else
    {
        // TODO: logging
    }


	return 0;
}