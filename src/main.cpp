#define NANOSVG_ALL_COLOR_KEYWORDS
#define NANOSVG_IMPLEMENTATION
#include "utils.h"
#include <Windowsx.h>
#include <commctrl.h>
#include <gdiplus.h>
#include "nanosvg.h"
#include "resource.h"

using namespace Gdiplus;

static const wchar_t CAPTION[] = L"SVG to EMF+ Converter";
struct NSVGimage *svgImage = nullptr;
float scale = 0.95f;
HFONT hFont = nullptr;
HWND hwndPreview = nullptr;
HWND hwndLabelWidth = nullptr;
HWND hwndWidthEdit = nullptr;
HWND hwndWidthUpDown = nullptr;
HWND hwndLabelHeight = nullptr;
HWND hwndHeightEdit = nullptr;
HWND hwndHeightUpDown = nullptr;
HWND hwndKeepArCheckbox = nullptr;
HWND hwndConvertBtn = nullptr;
bool keepAR = true;

static void OpenSvgFile(const std::wstring &filePath)
{
    const std::string path = Utils::WStrToUtf8(filePath);
    if (svgImage) {
        nsvgDelete(svgImage);
        svgImage = nullptr;
    }
    svgImage = nsvgParseFromFile(path.c_str(), "px", 96);
    if (!svgImage || svgImage->width <= 0 || svgImage->height <= 0) {
        HWND hWnd = GetForegroundWindow();
        MessageBox(hWnd, L"Cannot open file!", CAPTION, MB_OK | MB_ICONERROR);
    } else {
        InvalidateRect(hwndPreview, nullptr, FALSE);
        int imgWidth = (int)svgImage->width;
        int imgHeight = (int)svgImage->height;
        SendMessage(hwndWidthUpDown, UDM_SETPOS32, 0, imgWidth);
        SendMessage(hwndHeightUpDown, UDM_SETPOS32, 0, imgHeight);
    }
}

static void DrawSVGImage(Graphics &gr, NSVGimage *svgImage, float x, float y, float w, float h)
{
    if (!svgImage)
        return;

    float scaleX = (float)(w) / svgImage->width;
    float scaleY = (float)(h) / svgImage->height;
    Matrix mtx;
    mtx.Scale(scaleX, scaleY);
    mtx.Translate(x / scaleX, y / scaleY); // Important: Translate after Scale
    gr.SetTransform(&mtx);
    for (NSVGshape* shape = svgImage->shapes; shape != NULL; shape = shape->next) {
        GraphicsPath path;
        for (NSVGpath* svgPath = shape->paths; svgPath != NULL; svgPath = svgPath->next) {
            for (int i = 0; i < svgPath->npts - 1; i += 3) {
                float* p = &svgPath->pts[i * 2];
                PointF pt1(p[0], p[1]);
                PointF pt2(p[2], p[3]);
                PointF pt3(p[4], p[5]);
                PointF pt4(p[6], p[7]);
                path.AddBezier(pt1, pt2, pt3, pt4);
            }
            if (svgPath->closed) {
                path.CloseFigure();
            }
        }

        if (shape->fill.type == NSVG_PAINT_COLOR) {
            unsigned int c = shape->fill.color;
            BYTE a = (c >> 24) & 0xFF;
            BYTE b = (c >> 16) & 0xFF;
            BYTE g = (c >> 8) & 0xFF;
            BYTE r = c & 0xFF;
            Color fillColor(a, r, g, b);
            SolidBrush brush(fillColor);
            gr.FillPath(&brush, &path);

        // } else
        // if (shape->fill.type == NSVG_PAINT_LINEAR_GRADIENT) {
        //     NSVGgradient* grad = shape->fill.gradient;
        //     float x1 = grad->xform[4];
        //     float y1 = grad->xform[5];
        //     float x2 = x1 + grad->xform[0];
        //     float y2 = y1 + grad->xform[1];
        //     PointF start(x1, y1);
        //     PointF end(x2, y2);
        //     LinearGradientBrush lgb(start, end, Color(), Color());

        //     int numStops = grad->nstops;
        //     Color* colors = new Color[numStops];
        //     REAL* positions = new REAL[numStops];
        //     for (int i = 0; i < numStops; ++i) {
        //         uint32_t c = grad->stops[i].color;
        //         BYTE a = (c >> 24) & 0xFF;
        //         BYTE b = (c >> 16) & 0xFF;
        //         BYTE g = (c >> 8) & 0xFF;
        //         BYTE r = c & 0xFF;
        //         colors[i] = Color(a, r, g, b);
        //         positions[i] = grad->stops[i].offset;
        //     }
        //     lgb.SetInterpolationColors(colors, positions, numStops);
        //     gr.FillPath(&lgb, &path);
        //     delete[] colors;
        //     delete[] positions;

        // } else
        // if (shape->fill.type == NSVG_PAINT_RADIAL_GRADIENT) {
        //     NSVGgradient* grad = shape->fill.gradient;
        //     PointF center(grad->fx, grad->fy);
        //     RectF bounds;
        //     path.GetBounds(&bounds);
        //     PointF surroundPoint(bounds.GetRight(), bounds.GetBottom());
        //     PathGradientBrush pgBrush(&path);
        //     pgBrush.SetCenterPoint(center);

        //     int numStops = grad->nstops;
        //     Gdiplus::Color* colors = new Gdiplus::Color[numStops];
        //     for (int i = 0; i < numStops; i++) {
        //         uint32_t c = grad->stops[i].color;
        //         BYTE a = (c >> 24) & 0xFF;
        //         BYTE b = (c >> 16) & 0xFF;
        //         BYTE g = (c >> 8) & 0xFF;
        //         BYTE r = c & 0xFF;
        //         colors[i] = Gdiplus::Color(a, r, g, b);
        //     }
        //     pgBrush.SetSurroundColors(colors, &numStops);
        //     gr.FillPath(&pgBrush, &path);
        //     delete[] colors;
        }

        if (shape->stroke.type != NSVG_PAINT_NONE) {
            unsigned int strokeColorValue = shape->stroke.color;
            BYTE sa = (strokeColorValue >> 24) & 0xFF;
            BYTE sb = (strokeColorValue >> 16) & 0xFF;
            BYTE sg = (strokeColorValue >> 8) & 0xFF;
            BYTE sr = strokeColorValue & 0xFF;
            Color strokeColor(sa, sr, sg, sb);
            Pen pen(strokeColor, shape->strokeWidth);
            gr.DrawPath(&pen, &path);
        }
    }
    gr.ResetTransform();
}

static void ConvertAndSaveSvgToEmfPlus(HDC hdc, LPCWSTR emfPath, NSVGimage *img, float x, float y, float w, float h)
{
    const RectF rc(x, y, w, h);
    Metafile mf(emfPath, hdc, rc, Gdiplus::MetafileFrameUnitPixel, Gdiplus::EmfTypeEmfPlusDual, L"EMFplus");
    Gdiplus::Graphics gr(&mf);
    gr.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
    gr.SetInterpolationMode(Gdiplus::InterpolationModeHighQuality);
    gr.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
    DrawSVGImage(gr, img, x, y, w, h);
}

LRESULT CALLBACK StaticSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
{
    switch (msg) {
    case WM_PAINT: {
        RECT rc;
        GetClientRect(hWnd, &rc);
        PAINTSTRUCT ps = { 0 };
        HDC hdc = BeginPaint(hWnd, &ps );

        Graphics gr(hdc);
        gr.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
        gr.SetInterpolationMode(Gdiplus::InterpolationModeHighQuality);
        gr.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
        gr.Clear(Color(0xea, 0xea, 0xea));
        Gdiplus::Pen pen(Gdiplus::Color(0xad, 0xad, 0xad));
        gr.DrawRectangle(&pen, 0, 0, rc.right - rc.left, rc.bottom - rc.top);

        if (!svgImage) {
            WCHAR str[] = L"Drag & Drop SVG";
            LOGFONTW logFont = {0};
            GetObject(hFont, sizeof(LOGFONTW), &logFont);
            Gdiplus::Font font(hdc, &logFont);
            Gdiplus::RectF rcF(float(rc.left), float(rc.top), float(rc.right - rc.left), float(rc.bottom - rc.top));
            Gdiplus::StringAlignment h_algn = Gdiplus::StringAlignmentCenter, v_algn = Gdiplus::StringAlignmentCenter;
            Gdiplus::StringFormat strFmt;
            strFmt.SetAlignment(h_algn);
            strFmt.SetLineAlignment(v_algn);
            Gdiplus::SolidBrush brush(Gdiplus::Color(50, 50, 50));
            gr.DrawString(str, -1, &font, rcF, &strFmt, &brush);
            EndPaint(hWnd, &ps);
            return TRUE;
        }

        int rectWidth = rc.right - rc.left;
        int rectHeight = rc.bottom - rc.top;

        float imgWidth = svgImage->width;
        float imgHeight = svgImage->height;
        float imgAspect = imgWidth / imgHeight;
        float drawWidth, drawHeight;
        if ((float)rectWidth / rectHeight > imgAspect) {
            drawHeight = (float)rectHeight;
            drawWidth = drawHeight * imgAspect;
        } else {
            drawWidth = (float)rectWidth;
            drawHeight = drawWidth / imgAspect;
        }
        drawWidth *= scale;
        drawHeight *= scale;
        float offsetX = (rectWidth - drawWidth) / 2.0f;
        float offsetY = (rectHeight - drawHeight) / 2.0f;

        Gdiplus::SolidBrush brush(Gdiplus::Color(0xff, 0xff, 0xff));
        gr.FillRectangle(&brush, offsetX, offsetY, drawWidth, drawHeight);
        gr.DrawRectangle(&pen, offsetX, offsetY, drawWidth, drawHeight);
        DrawSVGImage(gr, svgImage, offsetX, offsetY, drawWidth, drawHeight);
        EndPaint(hWnd, &ps);
        return TRUE;
    }
    case WM_DROPFILES: {
        HDROP hDrop = (HDROP)wParam;
        UINT fileCount = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
        if (fileCount > 0) {
            TCHAR filePath[MAX_PATH];
            DragQueryFile(hDrop, 0, filePath, MAX_PATH);
            OpenSvgFile(filePath);
        }
        DragFinish(hDrop);
        return FALSE;
    }
    default:
        break;
    }
    return DefSubclassProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE: {
        HMODULE hInst = GetModuleHandle(NULL);
        hwndPreview = CreateWindow(L"STATIC", L"SVG Preview", WS_CHILD | WS_VISIBLE | SS_CENTER,
                                       6, 36, 380, 150, hWnd, nullptr, hInst, nullptr);
        DragAcceptFiles(hwndPreview, TRUE);
        SetWindowSubclass(hwndPreview, StaticSubclassProc, 1, 0);

        HWND hwndOpenBtn = CreateWindow(L"BUTTON", L"Open SVG", WS_CHILD | WS_VISIBLE | WS_TABSTOP /*| BS_ICON*/ | BS_LEFT,
                                            6, 2, 100, 32, hWnd, (HMENU)IDC_BUTTON_OPEN, hInst, nullptr);
        HICON hIcon = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON_OPEN), IMAGE_ICON,
                                           16, 16, LR_COPYFROMRESOURCE | LR_DEFAULTCOLOR | LR_SHARED);
        HIMAGELIST hImgList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 1, 1);
        ImageList_AddIcon(hImgList, hIcon);
        BUTTON_IMAGELIST imglist = {};
        imglist.himl = hImgList;
        imglist.uAlign = BUTTON_IMAGELIST_ALIGN_LEFT;
        imglist.margin.left = 4;
        imglist.margin.right = 4;
        SendMessage(hwndOpenBtn, BCM_SETIMAGELIST, 0, (LPARAM)&imglist);

        hwndLabelWidth = CreateWindow(L"STATIC", L"Width (px)", WS_CHILD | WS_VISIBLE,
                                          20, 220, 80, 20, hWnd, nullptr, hInst, nullptr);

        hwndWidthEdit = CreateWindow(L"EDIT", L"16", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_NUMBER,
                                         100, 248, 80, 20, hWnd, (HMENU)IDC_EDIT_WIDTH, hInst, nullptr);

        hwndWidthUpDown = CreateWindow(UPDOWN_CLASS, nullptr, WS_CHILD | WS_VISIBLE | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_SETBUDDYINT,
                                           0, 0, 0, 0, hWnd, (HMENU)IDC_UPDOWN_WIDTH, hInst, nullptr);
        SendMessage(hwndWidthUpDown, UDM_SETBUDDY, (WPARAM)hwndWidthEdit, 0);
        SendMessage(hwndWidthUpDown, UDM_SETRANGE32, 8, 7680);
        SendMessage(hwndWidthUpDown, UDM_SETPOS32, 0, 16);

        hwndLabelHeight = CreateWindow(L"STATIC", L"Height (px)", WS_CHILD | WS_VISIBLE,
                                           250, 220, 80, 20, hWnd, nullptr, hInst, nullptr);

        hwndHeightEdit = CreateWindow(L"EDIT", L"16", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_NUMBER,
                                          250, 248, 80, 20, hWnd, (HMENU)IDC_EDIT_HEIGHT, hInst, nullptr);

        hwndHeightUpDown = CreateWindow(UPDOWN_CLASS, nullptr, WS_CHILD | WS_VISIBLE | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_SETBUDDYINT,
                                            0, 0, 0, 0, hWnd, (HMENU)IDC_UPDOWN_HEIGHT, hInst, nullptr);
        SendMessage(hwndHeightUpDown, UDM_SETBUDDY, (WPARAM)hwndHeightEdit, 0);
        SendMessage(hwndHeightUpDown, UDM_SETRANGE32, 8, 7680);
        SendMessage(hwndHeightUpDown, UDM_SETPOS32, 0, 16);

        hwndKeepArCheckbox = CreateWindow(L"BUTTON", L"Keep aspect ratio", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                              30, 300, 140, 20, hWnd, (HMENU)IDC_CHECK_KEEP_AR, hInst, nullptr);
        SendMessage(hwndKeepArCheckbox, BM_SETCHECK, BST_CHECKED, 0);

        hwndConvertBtn = CreateWindow(L"BUTTON", L"Save EMF+", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                          6, 320, 90, 30, hWnd, (HMENU)IDC_BUTTON_SAVE, hInst, nullptr);

        SendMessage(hwndOpenBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hwndLabelWidth, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hwndWidthEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hwndLabelHeight, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hwndHeightEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hwndKeepArCheckbox, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hwndConvertBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
        return FALSE;
    }
    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case IDC_BUTTON_OPEN: {
            WCHAR filePath[MAX_PATH] = {};
            if (!Utils::FileDialog(hWnd, filePath, ARRAYSIZE(filePath), L"SVG Files (*.svg)\0*.svg\0All Files (*.*)\0*.*\0", NULL, Utils::Open)) {
                break;
            }
            OpenSvgFile(filePath);
            break;
        }
        case IDC_BUTTON_SAVE: {
            if (!svgImage || svgImage->width <= 0 || svgImage->height <= 0) {
                MessageBox(hWnd, L"Nothing to save!", CAPTION, MB_OK | MB_ICONERROR);
                break;
            }
            wchar_t wBuf[16], hBuf[16];
            GetWindowText(hwndWidthEdit, wBuf, 16);
            GetWindowText(hwndHeightEdit, hBuf, 16);
            if (wcslen(wBuf) == 0 || wcslen(hBuf) == 0) {
                MessageBox(hWnd, L"Error: set width and height first!", CAPTION, MB_OK | MB_ICONWARNING);
                break;
            }
            WCHAR filePath[MAX_PATH] = {};
            swprintf_s(filePath, ARRAYSIZE(filePath), L"%s", L"out.emf");
            if (!Utils::FileDialog(hWnd, filePath, ARRAYSIZE(filePath), L"EMF Plus (*.emf)\0*.emf\0", L"emf", Utils::Save)) {
                break;
            }
            float width = (float)_wtoi(wBuf);
            float height = (float)_wtoi(hBuf);
            HDC hdc = GetDC(hWnd);
            ConvertAndSaveSvgToEmfPlus(hdc, filePath, svgImage, 0, 0, width, height);
            ReleaseDC(hWnd, hdc);
            break;
        }
        case IDC_CHECK_KEEP_AR: {
            if (HIWORD(wParam) == BN_CLICKED) {
                keepAR = (SendMessage((HWND)lParam, BM_GETCHECK, 0, 0) == BST_CHECKED);
            }
            break;
        }
        default:
            break;
        }
        break;
    }
    case WM_NOTIFY: {
        NMUPDOWN *pNMUpDown = (NMUPDOWN*)lParam;
        if (keepAR && svgImage && pNMUpDown && pNMUpDown->hdr.code == UDN_DELTAPOS) {
            int val = pNMUpDown->iPos + pNMUpDown->iDelta;
            if (val < 8 || val > 7680) {
                return TRUE;
            }
            float imgWidth = svgImage->width;
            float imgHeight = svgImage->height;
            if (imgHeight > 0) {
                float imgAspect = imgWidth / imgHeight;
                if (pNMUpDown->hdr.hwndFrom == hwndWidthUpDown) {
                    SendMessage(hwndHeightUpDown, UDM_SETPOS32, 0, (LPARAM)round(val / imgAspect));
                } else
                if (pNMUpDown->hdr.hwndFrom == hwndHeightUpDown) {
                    SendMessage(hwndWidthUpDown, UDM_SETPOS32, 0, (LPARAM)round(val * imgAspect));
                }
            }
        }
        break;
    }
    case WM_MOUSEWHEEL: {
        POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        ScreenToClient(hWnd, &pt);
        RECT rc;
        GetWindowRect(hwndPreview, &rc);
        POINT ptLeft{ rc.left, rc.top };
        POINT ptRight{ rc.right, rc.bottom };
        ScreenToClient(hWnd, &ptLeft);
        ScreenToClient(hWnd, &ptRight);
        RECT clientRc{ ptLeft.x, ptLeft.y, ptRight.x, ptRight.y };
        if (!svgImage || !PtInRect(&clientRc, pt))
            break;

        float scaleStep = 0.05f;
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        if (delta > 0)
            scale += scaleStep;
        else
            scale = max(0.05f, scale - scaleStep);
        InvalidateRect(hwndPreview, nullptr, FALSE);
        return FALSE;
    }
    case WM_SIZE: {
        int w = LOWORD(lParam), h = HIWORD(lParam);
        SetWindowPos(hwndPreview, NULL, 0, 0, w - 12, h - 77, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOACTIVATE);
        SetWindowPos(hwndLabelWidth, NULL, 20, h - 32, 0, 0, SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOACTIVATE);
        SetWindowPos(hwndWidthEdit, NULL, 100, h - 32, 0, 0, SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOACTIVATE);
        SetWindowPos(hwndWidthUpDown, NULL, 164, h - 32, 0, 0, SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOACTIVATE);
        SetWindowPos(hwndLabelHeight, NULL, 200, h - 32, 0, 0, SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOACTIVATE);
        SetWindowPos(hwndHeightEdit, NULL, 280, h - 32, 0, 0, SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOACTIVATE);
        SetWindowPos(hwndHeightUpDown, NULL, 344, h - 32, 0, 0, SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOACTIVATE);
        SetWindowPos(hwndKeepArCheckbox, NULL, 380, h - 32, 0, 0, SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOACTIVATE);
        SetWindowPos(hwndConvertBtn, NULL, w - 94, h - 36, 0, 0, SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOACTIVATE);
        break;
    }
    // case WM_CTLCOLORSTATIC: {
    //     HDC hdc = (HDC)wParam;
    //     SetBkMode(hdc, TRANSPARENT);
    //     return (INT_PTR)GetStockObject(NULL_BRUSH);
    // }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        break;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR lpCmdLine, int nCmdShow)
{
    ULONG_PTR gdiplusToken;
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

    const wchar_t CLASS_NAME[] = L"SvgToEmfPlusWindow";

    WNDCLASSEX wcx;
    memset(&wcx, 0, sizeof(wcx));
    wcx.cbSize = sizeof(WNDCLASSEX);
    wcx.style = CS_HREDRAW | CS_VREDRAW;
    wcx.hInstance = hInstance;
    wcx.lpfnWndProc = WndProc;
    wcx.cbClsExtra	= 0;
    wcx.cbWndExtra	= 0;
    wcx.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON_MAIN));
    wcx.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON_MAIN));
    wcx.lpszClassName = CLASS_NAME;
    wcx.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wcx.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassEx(&wcx);

    hFont = CreateFont(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                           CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    HWND hWnd = CreateWindowEx(0, CLASS_NAME, CAPTION, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                                   800, 600, nullptr, nullptr, hInstance, nullptr);
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);    

    MSG msg;
    BOOL ret;
    while ((ret = GetMessage(&msg, NULL, 0, 0)) != 0) {
        if (ret == -1)
            break;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (svgImage) {
        nsvgDelete(svgImage);
        svgImage = nullptr;
    }
    if (hFont)
        DeleteObject(hFont);
    GdiplusShutdown(gdiplusToken);
    return 0;
}
