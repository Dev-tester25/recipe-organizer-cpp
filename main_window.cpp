#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <string>
#include <vector>
#include "recipe_manager.h"
#include "winui_helper.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")

// Global variables
RecipeManager* g_manager = nullptr;
UILayout* g_layout = nullptr;
HWND g_hwndRecipeList = nullptr;
HWND g_hwndSearchBox = nullptr;
HWND g_hwndDetailView = nullptr;
HWND g_hwndButtonImport = nullptr;
HWND g_hwndButtonSave = nullptr;
HWND g_hwndButtonLoad = nullptr;
int g_selectedRecipeIndex = -1;
int g_detailScrollOffset = 0;

// Forward declarations
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void OnPaint(HWND hwnd);
void OnImportFolder(HWND hwnd);
void OnSaveDatabase(HWND hwnd);
void OnLoadDatabase(HWND hwnd);
void OnSearch(HWND hwnd);
void RefreshRecipeList(HWND hwnd, const std::vector<Recipe*>& recipes);
void DisplayRecipeDetail(HWND hwnd, int index);

std::string BrowseForFolder(HWND hwnd) {
    BROWSEINFOA bi = {};
    bi.hwndOwner = hwnd;
    bi.lpszTitle = "Select a folder containing recipe files:";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
    if (!pidl) return "";

    char path[MAX_PATH];
    if (!SHGetPathFromIDListA(pidl, path)) {
        CoTaskMemFree(pidl);
        return "";
    }

    CoTaskMemFree(pidl);
    return std::string(path);
}

void OnPaint(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    RECT rect;
    GetClientRect(hwnd, &rect);
    
    // Draw background
    HBRUSH hBrush = CreateSolidBrush(RGB(242, 242, 242));
    FillRect(hdc, &rect, hBrush);
    DeleteObject(hBrush);

    // Draw header bar
    RECT headerRect = { 0, 0, rect.right, g_layout->headerHeight };
    hBrush = CreateSolidBrush(RGB(0, 120, 215));
    FillRect(hdc, &headerRect, hBrush);
    DeleteObject(hBrush);

    // Draw header title
    HFONT hFont = WinUIHelper::createSegoeUIFont(18, true);
    SetTextColor(hdc, RGB(255, 255, 255));
    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, hFont);
    TextOutA(hdc, 15, 15, "Recipe Organizer", 16);
    DeleteObject(hFont);

    // Draw left panel border
    HPEN hPen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    MoveToEx(hdc, g_layout->leftPanelX + g_layout->leftPanelWidth, g_layout->leftPanelY, nullptr);
    LineTo(hdc, g_layout->leftPanelX + g_layout->leftPanelWidth, rect.bottom);
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);

    EndPaint(hwnd, &ps);
}

void OnImportFolder(HWND hwnd) {
    std::string folderPath = BrowseForFolder(hwnd);
    if (folderPath.empty()) return;

    int count = g_manager->importFromFolder(folderPath);
    
    std::string message = "Imported " + std::to_string(count) + " recipe(s)";
    MessageBoxA(hwnd, message.c_str(), "Import Complete", MB_OK | MB_ICONINFORMATION);

    const auto& recipes = g_manager->getAllRecipes();
    std::vector<Recipe*> recipeRefs;
    for (auto& recipe : recipes) {
        recipeRefs.push_back(const_cast<Recipe*>(&recipe));
    }
    RefreshRecipeList(hwnd, recipeRefs);
}

void OnSaveDatabase(HWND hwnd) {
    if (g_manager->saveToDatabase()) {
        MessageBoxA(hwnd, "Recipes saved successfully!", "Save Complete", MB_OK | MB_ICONINFORMATION);
    } else {
        MessageBoxA(hwnd, "Failed to save recipes!", "Error", MB_OK | MB_ICONERROR);
    }
}

void OnLoadDatabase(HWND hwnd) {
    if (g_manager->loadFromDatabase()) {
        const auto& recipes = g_manager->getAllRecipes();
        std::vector<Recipe*> recipeRefs;
        for (auto& recipe : recipes) {
            recipeRefs.push_back(const_cast<Recipe*>(&recipe));
        }
        RefreshRecipeList(hwnd, recipeRefs);
        MessageBoxA(hwnd, "Recipes loaded successfully!", "Load Complete", MB_OK | MB_ICONINFORMATION);
    } else {
        MessageBoxA(hwnd, "No saved recipes found!", "Info", MB_OK | MB_ICONINFORMATION);
    }
}

void OnSearch(HWND hwnd) {
    char searchText[256] = {};
    GetWindowTextA(g_hwndSearchBox, searchText, sizeof(searchText));

    if (strlen(searchText) == 0) {
        const auto& recipes = g_manager->getAllRecipes();
        std::vector<Recipe*> recipeRefs;
        for (auto& recipe : recipes) {
            recipeRefs.push_back(const_cast<Recipe*>(&recipe));
        }
        RefreshRecipeList(hwnd, recipeRefs);
    } else {
        auto results = g_manager->searchRecipes(searchText);
        RefreshRecipeList(hwnd, results);
    }
}

void RefreshRecipeList(HWND hwnd, const std::vector<Recipe*>& recipes) {
    SendMessageA(g_hwndRecipeList, LB_RESETCONTENT, 0, 0);
    
    for (size_t i = 0; i < recipes.size(); ++i) {
        SendMessageA(g_hwndRecipeList, LB_ADDSTRING, 0, (LPARAM)recipes[i]->name.c_str());
    }
}

void DisplayRecipeDetail(HWND hwnd, int index) {
    const auto& recipes = g_manager->getAllRecipes();
    if (index < 0 || index >= (int)recipes.size()) {
        g_selectedRecipeIndex = -1;
        g_detailScrollOffset = 0;
        InvalidateRect(g_hwndDetailView, nullptr, TRUE);
        return;
    }

    g_selectedRecipeIndex = index;
    g_detailScrollOffset = 0;
    InvalidateRect(g_hwndDetailView, nullptr, TRUE);
}

LRESULT CALLBACK DetailViewProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rect;
        GetClientRect(hwnd, &rect);

        // Draw background
        HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(hdc, &rect, hBrush);
        DeleteObject(hBrush);

        if (g_selectedRecipeIndex >= 0 && g_selectedRecipeIndex < (int)g_manager->getAllRecipes().size()) {
            const auto& recipe = g_manager->getAllRecipes()[g_selectedRecipeIndex];
            int y = 20 - g_detailScrollOffset;

            // Recipe name
            HFONT hFont = WinUIHelper::createSegoeUIFont(16, true);
            SetTextColor(hdc, RGB(0, 120, 215));
            SetBkMode(hdc, TRANSPARENT);
            SelectObject(hdc, hFont);
            TextOutA(hdc, 20, y, recipe.name.c_str(), (int)recipe.name.length());
            DeleteObject(hFont);
            y += 35;

            // Prep time, cook time, servings
            hFont = WinUIHelper::createSegoeUIFont(12, false);
            SetTextColor(hdc, RGB(64, 64, 64));
            SelectObject(hdc, hFont);
            
            std::string prepText = "Prep: " + recipe.prepTime;
            TextOutA(hdc, 20, y, prepText.c_str(), (int)prepText.length());
            y += 26;

            std::string cookText = "Cook: " + recipe.cookTime;
            TextOutA(hdc, 20, y, cookText.c_str(), (int)cookText.length());
            y += 26;

            std::string servText = "Servings: " + recipe.servings;
            TextOutA(hdc, 20, y, servText.c_str(), (int)servText.length());
            y += 35;

            // Ingredients header
            hFont = WinUIHelper::createSegoeUIFont(13, true);
            SetTextColor(hdc, RGB(0, 120, 215));
            SelectObject(hdc, hFont);
            TextOutA(hdc, 20, y, "Ingredients:", 12);
            DeleteObject(hFont);
            y += 28;

            hFont = WinUIHelper::createSegoeUIFont(11, false);
            SetTextColor(hdc, RGB(64, 64, 64));
            SelectObject(hdc, hFont);
            
            for (size_t i = 0; i < recipe.ingredients.size() && y < rect.bottom - 50; ++i) {
                std::string ingText = std::to_string(i + 1) + ". " + recipe.ingredients[i];
                if (ingText.length() > 90) ingText = ingText.substr(0, 87) + "...";
                TextOutA(hdc, 40, y, ingText.c_str(), (int)ingText.length());
                y += 24;
            }

            y += 18;

            // Instructions header
            hFont = WinUIHelper::createSegoeUIFont(13, true);
            SetTextColor(hdc, RGB(0, 120, 215));
            SelectObject(hdc, hFont);
            TextOutA(hdc, 20, y, "Instructions:", 13);
            DeleteObject(hFont);
            y += 28;

            hFont = WinUIHelper::createSegoeUIFont(11, false);
            SetTextColor(hdc, RGB(64, 64, 64));
            SelectObject(hdc, hFont);
            
            int stepCount = 0;
            for (size_t i = 0; i < recipe.instructions.size() && y < rect.bottom - 20 && stepCount < 8; ++i) {
                std::string stepText = std::to_string(i + 1) + ". " + recipe.instructions[i];
                if (stepText.length() > 90) stepText = stepText.substr(0, 87) + "...";
                TextOutA(hdc, 40, y, stepText.c_str(), (int)stepText.length());
                y += 24;
                stepCount++;
            }

            DeleteObject(hFont);
        } else {
            HFONT hFont = WinUIHelper::createSegoeUIFont(14, false);
            SetTextColor(hdc, RGB(128, 128, 128));
            SetBkMode(hdc, TRANSPARENT);
            SelectObject(hdc, hFont);
            TextOutA(hdc, 20, 20, "Select a recipe to view details", 31);
            DeleteObject(hFont);
        }

        EndPaint(hwnd, &ps);
        return 0;
    }
    else if (msg == WM_MOUSEWHEEL) {
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        if (delta > 0) {
            g_detailScrollOffset = max(0, g_detailScrollOffset - 30);
        } else {
            g_detailScrollOffset += 30;
        }
        InvalidateRect(hwnd, nullptr, TRUE);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            // Search box
            g_hwndSearchBox = CreateWindowA("EDIT", "", 
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                g_layout->searchBoxX, g_layout->searchBoxY,
                g_layout->searchBoxWidth, 32,
                hwnd, (HMENU)1001, nullptr, nullptr);

            // Set search box font
            HFONT hFont = WinUIHelper::createSegoeUIFont(12, false);
            SendMessageA(g_hwndSearchBox, WM_SETFONT, (WPARAM)hFont, TRUE);

            // Recipe list
            g_hwndRecipeList = CreateWindowA("LISTBOX", "",
                WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
                g_layout->recipeListX, g_layout->recipeListY,
                g_layout->recipeListWidth, 300,
                hwnd, (HMENU)1002, nullptr, nullptr);

            SendMessageA(g_hwndRecipeList, WM_SETFONT, (WPARAM)hFont, TRUE);

            // Detail view (scrollable)
            WNDCLASSA detailViewClass = {};
            detailViewClass.lpfnWndProc = DetailViewProc;
            detailViewClass.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(255, 255, 255));
            detailViewClass.lpszClassName = "DetailViewClass";
            RegisterClassA(&detailViewClass);

            g_hwndDetailView = CreateWindowA("DetailViewClass", "",
                WS_CHILD | WS_VISIBLE,
                g_layout->mainPanelX, g_layout->mainPanelY,
                g_layout->mainPanelWidth, g_layout->mainPanelHeight,
                hwnd, (HMENU)1003, nullptr, nullptr);

            // Buttons with proper sizing and positioning
            int buttonY = g_layout->recipeListY + 310;
            int buttonX = g_layout->recipeListX;

            g_hwndButtonImport = CreateWindowA("BUTTON", "Import Folder",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                buttonX, buttonY, 130, 36,
                hwnd, (HMENU)2001, nullptr, nullptr);
            SendMessageA(g_hwndButtonImport, WM_SETFONT, (WPARAM)hFont, TRUE);

            g_hwndButtonSave = CreateWindowA("BUTTON", "Save",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                buttonX + 140, buttonY, 75, 36,
                hwnd, (HMENU)2002, nullptr, nullptr);
            SendMessageA(g_hwndButtonSave, WM_SETFONT, (WPARAM)hFont, TRUE);

            g_hwndButtonLoad = CreateWindowA("BUTTON", "Load",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                buttonX + 220, buttonY, 75, 36,
                hwnd, (HMENU)2003, nullptr, nullptr);
            SendMessageA(g_hwndButtonLoad, WM_SETFONT, (WPARAM)hFont, TRUE);

            // Load initial recipes
            g_manager->loadFromDatabase();
            const auto& recipes = g_manager->getAllRecipes();
            std::vector<Recipe*> recipeRefs;
            for (auto& recipe : recipes) {
                recipeRefs.push_back(const_cast<Recipe*>(&recipe));
            }
            RefreshRecipeList(hwnd, recipeRefs);

            break;
        }

        case WM_PAINT:
            OnPaint(hwnd);
            break;

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            int notif = HIWORD(wParam);

            if (id == 1001 && notif == EN_CHANGE) {
                OnSearch(hwnd);
            }
            else if (id == 1002 && notif == LBN_SELCHANGE) {
                int index = SendMessageA(g_hwndRecipeList, LB_GETCURSEL, 0, 0);
                DisplayRecipeDetail(hwnd, index);
            }
            else if (id == 2001) {
                OnImportFolder(hwnd);
            }
            else if (id == 2002) {
                OnSaveDatabase(hwnd);
            }
            else if (id == 2003) {
                OnLoadDatabase(hwnd);
            }
            break;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProcA(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    InitCommonControls();

    // Initialize manager
    g_manager = new RecipeManager("recipes.dat");
    g_layout = new UILayout(900, 700);

    // Register window class
    WNDCLASSA wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "RecipeOrganizerClass";
    wc.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(242, 242, 242));
    wc.hCursor = LoadCursorA(nullptr, IDC_ARROW);

    if (!RegisterClassA(&wc)) {
        MessageBoxA(nullptr, "Failed to register window class", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Create window
    HWND hwnd = CreateWindowA(
        "RecipeOrganizerClass",
        "Recipe Organizer v2.0",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        900, 700,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!hwnd) {
        MessageBoxA(nullptr, "Failed to create window", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessageA(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    delete g_manager;
    delete g_layout;

    return (int)msg.wParam;
}
