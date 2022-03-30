#ifndef UNICODE
#define UNICODE
#endif  
#define MAX_LOADSTRING 100

#include <windows.h>
#include <strsafe.h>
#include <stdio.h>
#include <assert.h>
#include <string>
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono> 
#include "../include/tobii.h"
#include "../include/tobii_streams.h"
#include <mutex>
#include <condition_variable>

#include <magnification.h>

// Global Variables:
HINSTANCE hInstance;
WCHAR szWindowClass[MAX_LOADSTRING] = L"Calibration"; 
HWND hWnd;                                      
WNDCLASS wc = { };
std::ofstream myfile;
bool is_positive = true;
bool is_zoom = false;
int scroll;
bool scroll_message = true;


std::mutex m;
std::condition_variable cv;
bool is_safe = true;

// Forward declarations of functions included in this code module:
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void url_receiver(char const* url, void* user_data);
void gaze_point_callback(tobii_gaze_point_t const* gaze_point, void* /* user_data*/);
DWORD WINAPI  tobii(_In_ LPVOID lpParameter);
void Initialization();


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = szWindowClass;
    RegisterClass(&wc);


    DWORD tobiiID;
    HANDLE hTobii = CreateThread(NULL, 0, tobii, NULL, 0, &tobiiID);
    assert(hTobii != 0);
    
    Initialization();

    CloseHandle(hTobii);
    
    return 0;

}


void Initialization()
{
    hWnd = CreateWindowEx(0, szWindowClass, L"Calibration", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);

    if (hWnd == NULL) return;
    ShowWindow(hWnd, SW_MAXIMIZE);
        
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}


DWORD WINAPI  tobii(_In_ LPVOID lpParameter)
{   
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

    //std::unique_lock<std::mutex> lk(m);
    //cv.wait(lk, [] {return !is_safe; }); // negate it?*/
    //Update window to display initial message
    //InvalidateRect(hWnd, NULL, TRUE);
    //UpdateWindow(hWnd);
    //lk.unlock();
    //cv.notify_all();

    for (int i = 0; i < 5; i++)
    {
        if (i == 0) scroll = 1;
        else if (i == 1) scroll = -1;
        else if (i == 2) scroll = 0;
        else if (i == 3) is_zoom = true;
        else is_positive = false;

        if (myfile.is_open()) myfile.close();
        
        InvalidateRect(hWnd, NULL, TRUE);
        UpdateWindow(hWnd);
        
        if (!is_zoom)
        {
            // Subscribe to gaze data
            result = tobii_gaze_point_subscribe(device, gaze_point_callback, 0);
            assert(result == TOBII_ERROR_NO_ERROR);
        }
        else
        {
            result = tobii_gaze_point_subscribe(device, gaze_point_callback, 0);
            assert(result == TOBII_ERROR_NO_ERROR);
        }


        for (int i = 0; i <= 100; i++)
        {
            // Optionally block this thread until data is available.
            result = tobii_wait_for_callbacks(1, &device);
            assert(result == TOBII_ERROR_NO_ERROR || result == TOBII_ERROR_TIMED_OUT);

            // Process callbacks if data is available
            result = tobii_device_process_callbacks(device);
            assert(result == TOBII_ERROR_NO_ERROR);
        }
        if (!is_zoom)
        {
            result = tobii_gaze_point_unsubscribe(device);
            assert(result == TOBII_ERROR_NO_ERROR);
        }
        else
        {
            result = tobii_gaze_point_unsubscribe(device);
            assert(result == TOBII_ERROR_NO_ERROR);
        }        
    }

    result = tobii_device_destroy(device);
    assert(result == TOBII_ERROR_NO_ERROR);
    result = tobii_api_destroy(api);
    assert(result == TOBII_ERROR_NO_ERROR);

    PostMessageW(hWnd, WM_DESTROY, NULL, NULL);
    return 0;
}

void gaze_point_callback(tobii_gaze_point_t const* gaze_point, void* data)
{
    // Check that the data is valid before using it
    if (gaze_point->validity == TOBII_VALIDITY_VALID)
    {
        //myfile << std::to_string(gaze_point->position_xy[0]) + "," + std::to_string(gaze_point->position_xy[1]) + "\n";
    }
}

void url_receiver(char const* url, void* user_data)
{
    char* buffer = (char*)user_data;
    if (*buffer != '\0') return; // only keep first value

    if (strlen(url) < 256)  strcpy_s(buffer, 256, url);
}





BOOL SetZoom(float magnificationFactor)
{
    // A magnification factor less than 1.0 is not valid.
    if (magnificationFactor < 1.0)
    {
        return FALSE;
    }

    // Calculate offsets such that the center of the magnified screen content 
    // is at the center of the screen. The offsets are relative to the 
    // unmagnified screen content.
    int xDlg = (int)((float)GetSystemMetrics(
        SM_CXSCREEN) * (1.0 - (1.0 / magnificationFactor)) / 2.0);
    int yDlg = (int)((float)GetSystemMetrics(
        SM_CYSCREEN) * (1.0 - (1.0 / magnificationFactor)) / 2.0);

    return MagSetFullscreenTransform(magnificationFactor, xDlg, yDlg);
}






LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
       
    switch (message)
    {
         case WM_PAINT:
             hdc = BeginPaint(hWnd, &ps);

             //Explanation to user
             if (scroll_message) //TODO: Fix this!!
             {
                 //std::lock_guard<std::mutex> lk(m);
                 //SetTextColor(hdc, 0x00FFFFFF);
                 //SetBkColor(hdc, 0x00000000);
                 DrawText(hdc, TEXT("Look around the painted section of the screen"), -1, &clientRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                 //std::this_thread::sleep_for(std::chrono::seconds(2));
                 scroll_message = false;
                 is_safe = true;
                 //cv.notify_all();
             }
             else
             {

                 FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
                 std::this_thread::sleep_for(std::chrono::seconds(2));

                 if (!is_zoom)
                 {
                     if (scroll == 1)
                     {
                         //myfile.open("Outputs/scroll_up.csv");
                         //assert(myfile.is_open() == true);
                         //myfile << "x,y\n";

                         //Top section
                         FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
                         ps.rcPaint.bottom /= 3;
                         FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_HOTLIGHT + 1));
                     }
                     else if (scroll == -1)
                     {
                         //myfile.open("Outputs/scroll_down.csv");
                         //assert(myfile.is_open() == true);
                         //myfile << "x,y\n";

                         //Botton section
                         FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
                         ps.rcPaint.top = ps.rcPaint.bottom;
                         ps.rcPaint.top /= 1.7;
                         FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_HOTLIGHT + 1));
                     }
                     else
                     {
                         //myfile.open("Outputs/no_scroll.csv");
                         //assert(myfile.is_open() == true);
                         //myfile << "x,y\n";

                         //Middle section
                         FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
                         ps.rcPaint.bottom /= 1.5;
                         ps.rcPaint.top = ps.rcPaint.bottom;
                         ps.rcPaint.top /= 2;
                         FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_HOTLIGHT + 1));
                     }
                 }
                 else
                 {
                     //Explanation to user

                     if (is_positive)
                     {
                         //myfile.open("Outputs/zoom_in.csv");
                         //assert(myfile.is_open() == true);
                         //myfile << "x,y\n"; //CHANGEEEEE

                         FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_HOTLIGHT + 1));
                     }
                     else
                     {
                         //myfile.open("Outputs/no_zoom.csv");
                         //assert(myfile.is_open() == true);
                         //myfile << "x,y\n"; //CHANGEEEE

                         FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
                     }
                 }
             }
            EndPaint(hWnd, &ps);
            
            /*MagInitialize();
            std::this_thread::sleep_for(std::chrono::seconds(2));
            SetZoom(3);
            std::this_thread::sleep_for(std::chrono::seconds(6));
            SetZoom(1);
            MagUninitialize();*/
                        
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}