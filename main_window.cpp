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
int g_selectedRecipeIndex = -1;

// Forward declarations
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void OnPaint(HWND hwnd);
void OnImportFolder(HWND hwnd);
void OnSaveDatabase(HWND hwnd);
void OnLoadDatabase(HWND hwnd);
void OnSearch(HWND hwnd);
void RefreshRecipeList(HWND hwnd, const std::vector<Recipe*>& recipes);
void DisplayRecipeDetail(HWND hwnd, int index);

std::string WideToString(const wchar_t* wide) {
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
    std::string str(size_needed - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, &str[0], size_needed, NULL, NULL);
    return str;
}

std::wstring StringToWide(const std::string& str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    std::wstring wstr(size_needed - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size_needed);
    return wstr;
}

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

    // Draw background
    RECT rect;
    GetClientRect(hwnd, &rect);
    HBRUSH hBrush = CreateSolidBrush(WinUIHelper::COLOR_DARK_BG);
    FillRect(hdc, &rect, hBrush);
    DeleteObject(hBrush);

    // Draw header bar
    RECT headerRect = { 0, 0, rect.right, g_layout->headerHeight };
    hBrush = CreateSolidBrush(WinUIHelper::COLOR_PANEL_BG);
    FillRect(hdc, &headerRect, hBrush);
    DeleteObject(hBrush);

    // Draw header title
    HFONT hFont = WinUIHelper::createSegoeUIFont(16, true);
    WinUIHelper::drawText(hdc, "Recipe Organizer", 15, 15, WinUIHelper::COLOR_ACCENT, hFont);
    DeleteObject(hFont);

    // Draw left panel border
    WinUIHelper::drawHorizontalLine(hdc, g_layout->leftPanelX + g_layout->leftPanelWidth, 
                                     g_layout->leftPanelY, 1, WinUIHelper::COLOR_BORDER);

    EndPaint(hwnd, &ps);
}

void OnImportFolder(HWND hwnd) {
    std::string folderPath = BrowseForFolder(hwnd);
    if (folderPath.empty()) return;

    int count = g_manager->importFromFolder(folderPath);
    
    std::string message = "Imported " + std::to_string(count) + " recipe(s)";
    MessageBoxA(hwnd, message.c_str(), "Import Complete", MB_OK | MB_ICONINFORMATION);

    // Refresh list
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
        InvalidateRect(g_hwndDetailView, nullptr, TRUE);
        return;
    }

    g_selectedRecipeIndex = index;
    InvalidateRect(g_hwndDetailView, nullptr, TRUE);
}

LRESULT CALLBACK DetailViewProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rect;
        GetClientRect(hwnd, &rect);

        // Draw background
        HBRUSH hBrush = CreateSolidBrush(WinUIHelper::COLOR_DARK_BG);
        FillRect(hdc, &rect, hBrush);
        DeleteObject(hBrush);

        if (g_selectedRecipeIndex >= 0 && g_selectedRecipeIndex < (int)g_manager->getAllRecipes().size()) {
            const auto& recipe = g_manager->getAllRecipes()[g_selectedRecipeIndex];
            int y = 20;

            // Recipe name
            HFONT hFont = WinUIHelper::createSegoeUIFont(14, true);
            WinUIHelper::drawText(hdc, recipe.name, 20, y, WinUIHelper::COLOR_ACCENT, hFont);
            DeleteObject(hFont);
            y += 40;

            // Prep time, cook time, servings
            hFont = WinUIHelper::createSegoeUIFont(11, false);
            std::string prepText = "Prep: " + recipe.prepTime;
            WinUIHelper::drawText(hdc, prepText, 20, y, WinUIHelper::COLOR_TEXT, hFont);
            y += 25;

            std::string cookText = "Cook: " + recipe.cookTime;
            WinUIHelper::drawText(hdc, cookText, 20, y, WinUIHelper::COLOR_TEXT, hFont);
            y += 25;

            std::string servText = "Servings: " + recipe.servings;
            WinUIHelper::drawText(hdc, servText, 20, y, WinUIHelper::COLOR_TEXT, hFont);
            y += 40;

            // Ingredients header
            hFont = WinUIHelper::createSegoeUIFont(12, true);
            WinUIHelper::drawText(hdc, "Ingredients:", 20, y, WinUIHelper::COLOR_YELLOW, hFont);
            DeleteObject(hFont);
            y += 25;

            hFont = WinUIHelper::createSegoeUIFont(10, false);
            for (size_t i = 0; i < recipe.ingredients.size() && y < rect.bottom - 100; ++i) {
                std::string ingText = std::to_string(i + 1) + ". " + recipe.ingredients[i];
                if (ingText.length() > 80) ingText = ingText.substr(0, 80) + "...";
                WinUIHelper::drawText(hdc, ingText, 40, y, WinUIHelper::COLOR_TEXT, hFont);
                y += 20;
            }

            y += 15;

            // Instructions header
            hFont = WinUIHelper::createSegoeUIFont(12, true);
            WinUIHelper::drawText(hdc, "Instructions:", 20, y, WinUIHelper::COLOR_YELLOW, hFont);
            DeleteObject(hFont);
            y += 25;

            hFont = WinUIHelper::createSegoeUIFont(10, false);
            for (size_t i = 0; i < recipe.instructions.size() && y < rect.bottom - 50; ++i) {
                std::string stepText = std::to_string(i + 1) + ". " + recipe.instructions[i];
                if (stepText.length() > 80) stepText = stepText.substr(0, 80) + "...";
                WinUIHelper::drawText(hdc, stepText, 40, y, WinUIHelper::COLOR_TEXT, hFont);
                y += 20;
            }

            DeleteObject(hFont);

            // Notes
            if (!recipe.notes.empty()) {
                y += 15;
                hFont = WinUIHelper::createSegoeUIFont(12, true);
                WinUIHelper::drawText(hdc, "Notes:", 20, y, WinUIHelper::COLOR_YELLOW, hFont);
                DeleteObject(hFont);
                y += 25;

                hFont = WinUIHelper::createSegoeUIFont(9, false);
                std::string noteText = recipe.notes;
                if (noteText.length() > 80) noteText = noteText.substr(0, 80) + "...";
                WinUIHelper::drawText(hdc, noteText, 40, y, WinUIHelper::COLOR_TEXT, hFont);
                DeleteObject(hFont);
            }
        } else {
            HFONT hFont = WinUIHelper::createSegoeUIFont(12, false);
            WinUIHelper::drawText(hdc, "Select a recipe to view details", 20, 20, WinUIHelper::COLOR_TEXT, hFont);
            DeleteObject(hFont);
        }

        EndPaint(hwnd, &ps);
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
                g_layout->searchBoxWidth, g_layout->searchBoxHeight,
                hwnd, (HMENU)1001, nullptr, nullptr);

            // Recipe list
            g_hwndRecipeList = CreateWindowA("LISTBOX", "",
                WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
                g_layout->recipeListX, g_layout->recipeListY,
                g_layout->recipeListWidth, g_layout->recipeListHeight,
                hwnd, (HMENU)1002, nullptr, nullptr);

            // Detail view
            WNDCLASSA detailViewClass = {};
            detailViewClass.lpfnWndProc = DetailViewProc;
            detailViewClass.hbrBackground = (HBRUSH)CreateSolidBrush(WinUIHelper::COLOR_DARK_BG);
            detailViewClass.lpszClassName = "DetailViewClass";
            RegisterClassA(&detailViewClass);

            g_hwndDetailView = CreateWindowA("DetailViewClass", "",
                WS_CHILD | WS_VISIBLE,
                g_layout->mainPanelX, g_layout->mainPanelY,
                g_layout->mainPanelWidth, g_layout->mainPanelHeight,
                hwnd, (HMENU)1003, nullptr, nullptr);

            // Import button
            CreateWindowA("BUTTON", "Import Folder",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                g_layout->recipeListX, g_layout->recipeListY + g_layout->recipeListHeight + 10,
                g_layout->buttonWidth, g_layout->buttonHeight,
                hwnd, (HMENU)2001, nullptr, nullptr);

            // Save button
            CreateWindowA("BUTTON", "Save",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                g_layout->recipeListX + g_layout->buttonWidth + 10, g_layout->recipeListY + g_layout->recipeListHeight + 10,
                g_layout->buttonWidth / 2, g_layout->buttonHeight,
                hwnd, (HMENU)2002, nullptr, nullptr);

            // Load button
            CreateWindowA("BUTTON", "Load",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                g_layout->recipeListX + g_layout->buttonWidth + g_layout->buttonWidth / 2 + 15, g_layout->recipeListY + g_layout->recipeListHeight + 10,
                g_layout->buttonWidth / 2, g_layout->buttonHeight,
                hwnd, (HMENU)2003, nullptr, nullptr);

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
    g_layout = new UILayout(1200, 800);

    // Register window class
    WNDCLASSA wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "RecipeOrganizerClass";
    wc.hbrBackground = (HBRUSH)CreateSolidBrush(WinUIHelper::COLOR_DARK_BG);
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
        g_layout->windowWidth, g_layout->windowHeight,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!hwnd) {
        MessageBoxA(nullptr, "Failed to create window", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Message loop
    MSG msg = {};
    while (GetMessageA(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    delete g_manager;
    delete g_layout;

    return (int)msg.wParam;
}
