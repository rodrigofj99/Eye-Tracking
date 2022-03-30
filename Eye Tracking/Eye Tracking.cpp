// Eye Tracking.cpp : Defines the entry point for the application.
//
#ifndef UNICODE
#define UNICODE
#endif  
#define MAX_LOADSTRING 100


#include <windows.h>
#include <thread>
#include <chrono>
#include <strsafe.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <fstream>
#include "../include/tobii.h"
#include "../include/tobii_streams.h"
#include <magnification.h>
#include "framework.h"
#include "Eye Tracking.h"



// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
HWND hWnd;                                      // main window handle
std::ifstream parameters;                       //to read from output of SVM
std::string coord;                              //tmp variable

struct GlobalInfo {                             // for the working environment
    //POINT gaze;
    float gaze_x;
    float gaze_y;
    bool working;
    //headpose

}Info;

struct SVM_param {
    double w[2];
    double b;
}SVM;




// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
void url_receiver(char const* url, void* user_data);
void gaze_point_callback(tobii_gaze_point_t const* gaze_point, void* /* user_data*/);
void Parameter_parsing();
BOOL SetZoom(float zoomFactor);
DWORD WINAPI  tobii(_In_ LPVOID lpParameter);
DWORD WINAPI  scroll(_In_ LPVOID lpParameter);
//DWORD WINAPI  zoom(_In_ LPVOID lpParameter);




int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_EYETRACKING, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);


    DWORD tobiiID;
    DWORD scrollID;
    //DWORD zoomID;
    Info.working = true;

    HANDLE hTobii = CreateThread(NULL, 0, tobii, &Info, 0, &tobiiID);
    assert(hTobii != NULL);
    HANDLE hScroll = CreateThread(NULL, 0, scroll, &Info, 0, &scrollID);
    assert(hScroll != NULL);
    //HANDLE hZoom = CreateThread(NULL, 0, zoom, &global, 0, &zoomID);


    /*/ Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_EYETRACKING));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;*/


    Parameter_parsing();


    MessageBox(NULL, L"Click to quit", L"Click to quit", MB_OK);
    Info.working = FALSE;

    bool close = CloseHandle(hTobii);
    assert(close != 0);
    close = CloseHandle(hScroll);
    assert(close != 0);
    //close = CloseHandle(hZoom);
    //assert(close != 0);

    return 0;
}




void Parameter_parsing()
{
    parameters.open("../Calibration/Outputs/parameters.txt");
    parameters >> coord;
    SVM.w[0] = std::atof(coord.c_str());
    parameters >> coord;
    SVM.w[1] = std::atof(coord.c_str());
    parameters >> coord;
    SVM.b = std::atof(coord.c_str());
}




DWORD WINAPI  tobii(_In_ LPVOID lpParameter)
{
    GlobalInfo* info = reinterpret_cast<GlobalInfo*>(lpParameter);

    // Create API
    tobii_api_t* api = NULL;
    tobii_error_t result = tobii_api_create(&api, NULL, NULL);
    assert(result == TOBII_ERROR_NO_ERROR);

    // Enumerate devices to find connected eye trackers, keep the first
    char url[256] = { 0 };
    result = tobii_enumerate_local_device_urls(api, url_receiver, url);
    assert(result == TOBII_ERROR_NO_ERROR);
    if (*url == '\0')
    {
        printf("Error: No device found\n");
        return 1;
    }

    // Connect to the first tracker found
    tobii_device_t* device = NULL;
    result = tobii_device_create(api, url, TOBII_FIELD_OF_USE_INTERACTIVE, &device);
    assert(result == TOBII_ERROR_NO_ERROR);

    // Subscribe to gaze data
    result = tobii_gaze_point_subscribe(device, gaze_point_callback, info);
    assert(result == TOBII_ERROR_NO_ERROR);

    while (info->working)
    {
        // Optionally block this thread until data is available.
        result = tobii_wait_for_callbacks(1, &device);
        assert(result == TOBII_ERROR_NO_ERROR || result == TOBII_ERROR_TIMED_OUT);

        // Process callbacks if data is available
        result = tobii_device_process_callbacks(device);
        assert(result == TOBII_ERROR_NO_ERROR);
    }

    // Cleanup
    result = tobii_gaze_point_unsubscribe(device);
    assert(result == TOBII_ERROR_NO_ERROR);
    result = tobii_device_destroy(device);
    assert(result == TOBII_ERROR_NO_ERROR);
    result = tobii_api_destroy(api);
    assert(result == TOBII_ERROR_NO_ERROR);
    return 0;
}




void gaze_point_callback(tobii_gaze_point_t const* gaze_point, void* data)
{
    // Check that the data is valid before using it
    if (gaze_point->validity == TOBII_VALIDITY_VALID)
    {
        GlobalInfo* info = reinterpret_cast<GlobalInfo*>(data);
        info->gaze_x = gaze_point->position_xy[0];
        info->gaze_y = gaze_point->position_xy[1];
    }
}




void url_receiver(char const* url, void* user_data)
{
    char* buffer = (char*)user_data;
    if (*buffer != '\0') return; // only keep first value

    if (strlen(url) < 256)  strcpy_s(buffer, 256, url);
}





DWORD WINAPI  scroll(_In_ LPVOID lpParameter)
{
    GlobalInfo* info = reinterpret_cast<GlobalInfo*>(lpParameter);
    HWND hCurrentWind;
    int time_to_read = 200000000; //preliminary (0.2 sec)
    int line = 40; //maybe depends on screen?
    //double down = 0.8; //preliminary
    //double up = 0.2; //preliminary

    while (info->working)
    {
        hCurrentWind = GetForegroundWindow();

        //Need a transformation to denormalize
        //ScreenToClient(hCurrentWind, &info->gaze);

        if (info->gaze_x * SVM.w[0] + info->gaze_y * SVM.w[1] + SVM.b > 0)
        {
            mouse_event(MOUSEEVENTF_WHEEL, 0, 0, -1 * line, 0); //Not the best, but works for now
            std::this_thread::sleep_for(std::chrono::nanoseconds(time_to_read));
        }

        //PREVIOUS METHOD
        /*if (info->gaze_y >= down)
        {
            mouse_event(MOUSEEVENTF_WHEEL, 0, 0, -1 * line, 0); //Not the best, but works for now
            std::this_thread::sleep_for(std::chrono::nanoseconds(time_to_read));
            //if ((GetWindowLongW(wind, GWL_STYLE) and WS_VSCROLL))
            {
                //bool result = PostMessageW(wind, WM_VSCROLL, MAKEWPARAM(SB_LINEDOWN, NULL), 0);
                //if (!result) exit(0);
            }
        }
        else if (info->gaze_y <= up)
        {
            mouse_event(MOUSEEVENTF_WHEEL, 0, 0, 1 * line, 0); //Not the best, but works for now
            std::this_thread::sleep_for(std::chrono::nanoseconds(time_to_read));
        }*/
    }
    return 0;
}






BOOL SetZoom(float zoomFactor)
{
    if (zoomFactor < 1.0)
    {
        return FALSE;
    }

    // Calculate offsets such that the center of the magnified screen content 
    // is at the center of the screen. The offsets are relative to the 
    // unmagnified screen content.
    int dx = (int)((float)GetSystemMetrics(SM_CXSCREEN) * (1.0 - (1.0 / zoomFactor)) / 2.0);
    int dy = (int)((float)GetSystemMetrics(SM_CYSCREEN) * (1.0 - (1.0 / zoomFactor)) / 2.0);

    return MagSetFullscreenTransform(zoomFactor, dx, dy);
}





//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_EYETRACKING));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_EYETRACKING);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
