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
#include <mutex>
#include <condition_variable>
#include <math.h>


// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
HWND hWnd;                                      // main window handle
std::ifstream parameters;                       //to read from output of SVM
std::string coord;                              //tmp variable

double epsilon = pow(10, -7);
std::mutex m;
std::condition_variable cv;

struct TrackerInfo {
    float old_gaze_y = 0;
    float gaze_y = 0;
    float old_gaze_x = 0;
    float gaze_x = 0;
    float head_z = 0;
};
struct GlobalInfo {
    TrackerInfo tracker = {};
    bool working = true;
    bool safe = true;
    bool scroll_down = true;
    bool scroll_up = true;
    bool zoom = true;
    int lines_scroll = 40;
    int speed_scroll = 240;
    double zoom_factor = 1.65;
}Info;

struct SVM_param {
    double w[2];
    double b;
    double mean[2];
    double std[2];
}svm_down, svm_up, svm_zoom;




// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
void url_receiver(char const* url, void* user_data);
void gaze_point_callback(tobii_gaze_point_t const* gaze_point, void* /* user_data*/);
void head_pose_callback(tobii_head_pose_t const* head_pose, void* user_data);
void Parameter_parsing();
DWORD WINAPI  tobii(_In_ LPVOID lpParameter);
DWORD WINAPI  scroll(_In_ LPVOID lpParameter);
DWORD WINAPI  zoom(_In_ LPVOID lpParameter);




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

    Info = {};

    std::thread hTobii(tobii, &Info);
    std::thread hScroll(scroll, &Info);
    std::thread hZoom(zoom, &Info);

    // Perform application initialization:
    //if (!InitInstance (hInstance, nCmdShow))
    {
        //return FALSE;
    }

    //HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WINDOWSPROJECT1));

    //MSG msg;

    Parameter_parsing();

    /*/ Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }//*/


    MessageBox(NULL, L"Click to quit", L"Click to quit", MB_OK);
    Info.working = FALSE;

    hTobii.join();
    hScroll.join();
    hZoom.join();

    return 0;
}




void Parameter_parsing()
{
    parameters.open("../Calibration/Outputs/parameters.txt");
    if (parameters.is_open())
    {
        try
        {
            parameters >> coord;
            svm_down.w[0] = std::atof(coord.c_str());
            parameters >> coord;
            svm_down.w[1] = std::atof(coord.c_str());
            parameters >> coord;
            svm_down.b = std::atof(coord.c_str());
            parameters >> coord;
            svm_down.mean[0] = std::atof(coord.c_str());
            parameters >> coord;
            svm_down.mean[1] = std::atof(coord.c_str());
            parameters >> coord;
            svm_down.std[0] = std::atof(coord.c_str());
            parameters >> coord;
            svm_down.std[1] = std::atof(coord.c_str());

            parameters >> coord;
            svm_up.w[0] = std::atof(coord.c_str());
            parameters >> coord;
            svm_up.w[1] = std::atof(coord.c_str());
            parameters >> coord;
            svm_up.b = std::atof(coord.c_str());
            parameters >> coord;
            svm_up.mean[0] = std::atof(coord.c_str());
            parameters >> coord;
            svm_up.mean[1] = std::atof(coord.c_str());
            parameters >> coord;
            svm_up.std[0] = std::atof(coord.c_str());
            parameters >> coord;
            svm_up.std[1] = std::atof(coord.c_str());


            parameters >> coord;
            svm_zoom.w[0] = std::atof(coord.c_str());
            parameters >> coord;
            svm_zoom.b = std::atof(coord.c_str());
            parameters >> coord;
            svm_zoom.mean[0] = std::atof(coord.c_str());
            parameters >> coord;
            svm_zoom.std[0] = std::atof(coord.c_str());

            parameters.close();
        }
        catch (...)
        {
            exit(15);
        }
    }
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

    while (info->working)
    {
        // Subscribe to gaze data
        result = tobii_gaze_point_subscribe(device, gaze_point_callback, info);
        assert(result == TOBII_ERROR_NO_ERROR);

        // Optionally block this thread until data is available.
        result = tobii_wait_for_callbacks(1, &device);
        assert(result == TOBII_ERROR_NO_ERROR || result == TOBII_ERROR_TIMED_OUT);

        // Process callbacks if data is available
        result = tobii_device_process_callbacks(device);
        assert(result == TOBII_ERROR_NO_ERROR);

        result = tobii_gaze_point_unsubscribe(device);
        assert(result == TOBII_ERROR_NO_ERROR);



        result = tobii_head_pose_subscribe(device, head_pose_callback, info);
        assert(result == TOBII_ERROR_NO_ERROR);

        // Optionally block this thread until data is available.
        result = tobii_wait_for_callbacks(1, &device);
        assert(result == TOBII_ERROR_NO_ERROR || result == TOBII_ERROR_TIMED_OUT);

        // Process callbacks if data is available
        result = tobii_device_process_callbacks(device);
        assert(result == TOBII_ERROR_NO_ERROR);

        result = tobii_head_pose_unsubscribe(device);
        assert(result == TOBII_ERROR_NO_ERROR);
    }

    // Cleanup
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
        if (info->tracker.old_gaze_x == 0 && info->tracker.old_gaze_y == 0)
        {
            info->tracker.old_gaze_x = gaze_point->position_xy[0];
            info->tracker.gaze_x = gaze_point->position_xy[0];
            info->tracker.old_gaze_y = gaze_point->position_xy[1];
            info->tracker.gaze_y = gaze_point->position_xy[1];
        }
        else
        {
            info->tracker.old_gaze_x = info->tracker.gaze_x;
            info->tracker.gaze_x = gaze_point->position_xy[0];
            info->tracker.old_gaze_y = info->tracker.gaze_y;
            info->tracker.gaze_y = gaze_point->position_xy[1];
        }
    }
}



void head_pose_callback(tobii_head_pose_t const* head_pose, void* user_data)
{
    if (head_pose->position_validity == TOBII_VALIDITY_VALID)
    {
        GlobalInfo* info = reinterpret_cast<GlobalInfo*>(user_data);
        info->tracker.head_z = head_pose->position_xyz[2];
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
    POINT client_down;
    POINT client_up;

    int time_to_read = 200; // (0.2 sec)
    int line = 40; //maybe depends on screen?

    while (info->working)
    {
        //Screen-to-client-area conversion
        hCurrentWind = GetForegroundWindow();
        client_down.x = static_cast<LONG>(svm_down.w[0] * GetSystemMetrics(SM_CXSCREEN));
        client_down.y = static_cast<LONG>(svm_down.w[1] * GetSystemMetrics(SM_CYSCREEN));
        client_up.x = static_cast<LONG>(svm_up.w[0] * GetSystemMetrics(SM_CXSCREEN));
        client_up.y = static_cast<LONG>(svm_up.w[1] * GetSystemMetrics(SM_CYSCREEN));

        ScreenToClient(hCurrentWind, &client_down);
        ScreenToClient(hCurrentWind, &client_up);

        client_down.x /= GetSystemMetrics(SM_CXSCREEN);
        client_down.y /= GetSystemMetrics(SM_CYSCREEN);
        client_up.x /= GetSystemMetrics(SM_CXSCREEN);
        client_up.y /= GetSystemMetrics(SM_CYSCREEN);

        double x_down = (info->tracker.gaze_x - svm_down.mean[0]) / svm_down.std[0];
        double y_down = (info->tracker.gaze_y - svm_down.mean[1]) / svm_down.std[1];
        double x_up = (info->tracker.gaze_x - svm_up.mean[0]) / svm_up.std[0];
        double y_up = (info->tracker.gaze_y - svm_up.mean[1]) / svm_up.std[1];

        if ((x_down * client_down.x + y_down * client_down.y + svm_down.b > 0) && (info->scroll_down))
        {
            std::unique_lock<std::mutex> lk(m);
            cv.wait(lk, [&] {return !info->safe; });

            mouse_event(MOUSEEVENTF_WHEEL, 0, 0, -1 * info->lines_scroll, 0); //Not the best, but works for now
            std::this_thread::sleep_for(std::chrono::milliseconds(info->speed_scroll));

            lk.unlock();
            cv.notify_all();
        }

        if ((x_up * client_up.x + y_up * client_up.y + svm_up.b > 0) && (info->scroll_up))
        {
            std::unique_lock<std::mutex> lk(m);
            cv.wait(lk, [&] {return !info->safe; });

            mouse_event(MOUSEEVENTF_WHEEL, 0, 0, info->lines_scroll, 0); //Not the best, but works for now
            std::this_thread::sleep_for(std::chrono::milliseconds(info->speed_scroll));

            lk.unlock();
            cv.notify_all();
        }
    }
    return 0;
}




DWORD WINAPI  zoom(_In_ LPVOID lpParameter)
{
    GlobalInfo* info = reinterpret_cast<GlobalInfo*>(lpParameter);
    try
    {
        MagInitialize();
        while (info->working)
        {
            std::lock_guard<std::mutex> lk(m);
            double x = (info->tracker.head_z - svm_zoom.mean[0]) / svm_zoom.std[0];
            if ((svm_zoom.w[0] * x + svm_zoom.b > 0) && (info->zoom))
            {
                info->safe = true;
                if ((abs(info->tracker.gaze_x - info->tracker.old_gaze_x) > epsilon) && (abs(info->tracker.gaze_y - info->tracker.old_gaze_y) > epsilon))
                {
                    int dx = (int)((float)GetSystemMetrics(SM_CXSCREEN) * (1.0 - (1.0 / info->zoom_factor)) * info->tracker.gaze_x);
                    int dy = (int)((float)GetSystemMetrics(SM_CYSCREEN) * (1.0 - (1.0 / info->zoom_factor)) * info->tracker.gaze_y);

                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    MagSetFullscreenTransform(info->zoom_factor, dx, dy);
                }

            }
            else
            {
                info->safe = false;
                int dx = (int)((float)GetSystemMetrics(SM_CXSCREEN) * (1.0 - (1.0 / 1)) / 2.0);
                int dy = (int)((float)GetSystemMetrics(SM_CYSCREEN) * (1.0 - (1.0 / 1)) / 2.0);

                MagSetFullscreenTransform(1, dx, dy);

            }
            cv.notify_all();
        }
        MagUninitialize();
    }
    catch (const std::exception&)
    {
        exit(200);
    }

    return 0;
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
