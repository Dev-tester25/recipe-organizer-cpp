#pragma once

#include <windows.h>
#include <string>
#include <vector>

class WinUIHelper {
public:
    // Modern Windows 11 colors
    static constexpr COLORREF COLOR_DARK_BG = RGB(32, 32, 32);        // Dark background
    static constexpr COLORREF COLOR_LIGHT_BG = RGB(242, 242, 242);    // Light background
    static constexpr COLORREF COLOR_PANEL_BG = RGB(45, 45, 45);       // Panel background
    static constexpr COLORREF COLOR_ACCENT = RGB(0, 120, 215);        // Windows blue accent
    static constexpr COLORREF COLOR_ACCENT_LIGHT = RGB(230, 240, 255); // Light accent
    static constexpr COLORREF COLOR_TEXT = RGB(255, 255, 255);        // White text
    static constexpr COLORREF COLOR_TEXT_DARK = RGB(0, 0, 0);         // Dark text
    static constexpr COLORREF COLOR_BORDER = RGB(60, 60, 60);         // Border color
    static constexpr COLORREF COLOR_HOVER = RGB(55, 55, 55);          // Hover state
    static constexpr COLORREF COLOR_SUCCESS = RGB(16, 124, 16);       // Green
    static constexpr COLORREF COLOR_WARNING = RGB(240, 180, 40);      // Orange

    static HFONT createSegoeUIFont(int size, bool bold = false) {
        return CreateFontA(
            size,                          // Height
            0,                             // Width
            0,                             // Escapement
            0,                             // Orientation
            bold ? FW_BOLD : FW_NORMAL,   // Weight
            FALSE,                         // Italic
            FALSE,                         // Underline
            FALSE,                         // StrikeOut
            DEFAULT_CHARSET,              // CharSet
            OUT_DEFAULT_PRECIS,           // OutPrecision
            CLIP_DEFAULT_PRECIS,          // ClipPrecision
            CLEARTYPE_QUALITY,            // Quality
            FF_DONTCARE,                  // PitchAndFamily
            "Segoe UI"                    // FaceName
        );
    }

    static void drawRoundedRect(HDC hdc, int x, int y, int width, int height, int radius, COLORREF fillColor) {
        HBRUSH hBrush = CreateSolidBrush(fillColor);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);

        RoundRect(hdc, x, y, x + width, y + height, radius * 2, radius * 2);

        SelectObject(hdc, hOldBrush);
        DeleteObject(hBrush);
    }

    static void drawText(HDC hdc, const std::string& text, int x, int y, COLORREF color, HFONT font = nullptr) {
        SetTextColor(hdc, color);
        SetBkMode(hdc, TRANSPARENT);

        if (font) {
            SelectObject(hdc, font);
        }

        TextOutA(hdc, x, y, text.c_str(), (int)text.length());
    }

    static void drawHorizontalLine(HDC hdc, int x, int y, int width, COLORREF color) {
        HPEN hPen = CreatePen(PS_SOLID, 1, color);
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

        MoveToEx(hdc, x, y, nullptr);
        LineTo(hdc, x + width, y);

        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);
    }
};

// Struct for UI element dimensions and positions
struct UILayout {
    // Window dimensions
    int windowWidth = 1200;
    int windowHeight = 800;

    // Header
    int headerHeight = 60;
    int headerX = 0;
    int headerY = 0;

    // Left panel (recipe list)
    int leftPanelX = 0;
    int leftPanelY = headerHeight;
    int leftPanelWidth = 300;
    int leftPanelHeight;

    // Main detail panel
    int mainPanelX;
    int mainPanelY = headerHeight;
    int mainPanelWidth;
    int mainPanelHeight;

    // Search box
    int searchBoxX = 10;
    int searchBoxY = headerHeight + 10;
    int searchBoxWidth = leftPanelWidth - 20;
    int searchBoxHeight = 35;

    // Recipe list box
    int recipeListX = 10;
    int recipeListY = searchBoxY + searchBoxHeight + 10;
    int recipeListWidth = leftPanelWidth - 20;
    int recipeListHeight;

    // Button dimensions
    int buttonWidth = 140;
    int buttonHeight = 40;
    int buttonMargin = 10;

    // Detail panel elements
    int detailMargin = 20;
    int detailTitleSize = 24;
    int detailTextSize = 14;

    UILayout(int w, int h) : windowWidth(w), windowHeight(h) {
        leftPanelHeight = windowHeight - headerHeight;
        mainPanelX = leftPanelX + leftPanelWidth + 1;
        mainPanelWidth = windowWidth - mainPanelX;
        mainPanelHeight = windowHeight - headerHeight;
        recipeListHeight = recipeListY + (windowHeight - headerHeight - recipeListY - buttonHeight - 30);
    }
};
