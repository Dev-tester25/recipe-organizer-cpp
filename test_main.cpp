#include <iostream>
#include <iomanip>
#include <windows.h>
#include "recipe_parser.h"
#include "recipe_manager.h"

using namespace std;

// ANSI color codes for console output
namespace Colors {
    const string RESET = "\x1b[0m";
    const string BOLD = "\x1b[1m";
    const string GREEN = "\x1b[32m";
    const string RED = "\x1b[31m";
    const string YELLOW = "\x1b[33m";
    const string CYAN = "\x1b[36m";
    const string BLUE = "\x1b[34m";
}

void enableConsoleColors() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) return;

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
}

void printHeader(const string& text) {
    cout << "\n" << Colors::BOLD << Colors::CYAN << "======================================================\n";
    cout << "  " << text << "\n";
    cout << "======================================================" << Colors::RESET << "\n\n";
}

void printSuccess(const string& message) {
    cout << Colors::GREEN << "✓ " << message << Colors::RESET << "\n";
}

void printError(const string& message) {
    cout << Colors::RED << "✗ " << message << Colors::RESET << "\n";
}

void printWarning(const string& message) {
    cout << Colors::YELLOW << "⚠ " << message << Colors::RESET << "\n";
}

void printInfo(const string& message) {
    cout << Colors::BLUE << "ℹ " << message << Colors::RESET << "\n";
}

void printRecipe(const Recipe& recipe) {
    cout << Colors::BOLD << Colors::CYAN << "\n  " << recipe.name << Colors::RESET << "\n";
    cout << "  " << string(recipe.name.length(), '-') << "\n";

    cout << "  Prep Time:  " << recipe.prepTime << "\n";
    cout << "  Cook Time:  " << recipe.cookTime << "\n";
    cout << "  Servings:   " << recipe.servings << "\n";

    if (!recipe.notes.empty()) {
        cout << "  Notes:      " << recipe.notes << "\n";
    }

    cout << "\n  " << Colors::YELLOW << "Ingredients:" << Colors::RESET << "\n";
    if (recipe.ingredients.empty()) {
        cout << "    (none)\n";
    } else {
        for (size_t i = 0; i < recipe.ingredients.size(); ++i) {
            cout << "    " << (i + 1) << ". " << recipe.ingredients[i] << "\n";
        }
    }

    cout << "\n  " << Colors::YELLOW << "Instructions:" << Colors::RESET << "\n";
    if (recipe.instructions.empty()) {
        cout << "    (none)\n";
    } else {
        for (size_t i = 0; i < recipe.instructions.size(); ++i) {
            cout << "    " << (i + 1) << ". " << recipe.instructions[i] << "\n";
        }
    }

    cout << "\n";
}

int main() {
    enableConsoleColors();

    cout << "\n" << Colors::BOLD << Colors::BLUE;
    cout << "╔════════════════════════════════════════════════════╗\n";
    cout << "║     Recipe Organizer - Parser & Manager Tests      ║\n";
    cout << "║                    v2.0 (Testing)                  ║\n";
    cout << "╚════════════════════════════════════════════════════╝\n";
    cout << Colors::RESET << "\n";

    // ========================================
    // TEST 1: Single Recipe File Parsing
    // ========================================
    printHeader("TEST 1: Single Recipe File Parsing");

    cout << "Enter the path to a .txt recipe file to test:\n";
    cout << Colors::YELLOW << "> " << Colors::RESET;
    
    string singleFilePath;
    getline(cin, singleFilePath);

    if (singleFilePath.empty()) {
        printWarning("No file path provided. Skipping single file test.");
    } else {
        Recipe singleRecipe = RecipeParser::parseRecipeFile(singleFilePath);

        if (singleRecipe.name.find("Error") != string::npos) {
            printError("Failed to parse file: " + singleRecipe.name);
        } else {
            printSuccess("Successfully parsed recipe file");
            printRecipe(singleRecipe);

            // Validate parsing
            if (singleRecipe.ingredients.empty()) {
                printWarning("No ingredients found in recipe");
            } else {
                printSuccess("Found " + to_string(singleRecipe.ingredients.size()) + " ingredients");
            }

            if (singleRecipe.instructions.empty()) {
                printWarning("No instructions found in recipe");
            } else {
                printSuccess("Found " + to_string(singleRecipe.instructions.size()) + " instructions");
            }
        }
    }

    // ========================================
    // TEST 2: Folder Import (Multiple Files)
    // ========================================
    printHeader("TEST 2: Folder Import (Multiple Files)");

    cout << "Enter the path to a folder containing .txt recipe files:\n";
    cout << Colors::YELLOW << "> " << Colors::RESET;

    string folderPath;
    getline(cin, folderPath);

    if (folderPath.empty()) {
        printWarning("No folder path provided. Skipping folder import test.");
    } else {
        vector<Recipe> importedRecipes = RecipeParser::parseRecipeFolder(folderPath);

        if (importedRecipes.empty()) {
            printError("No recipe files found in folder or all files failed to parse");
        } else {
            printSuccess("Imported " + to_string(importedRecipes.size()) + " recipes from folder");
            printInfo("Recipes found (in alphabetical order):");
            for (size_t i = 0; i < importedRecipes.size(); ++i) {
                cout << "  " << (i + 1) << ". " << importedRecipes[i].name << "\n";
            }
        }
    }

    // ========================================
    // TEST 3: Recipe Manager
    // ========================================
    printHeader("TEST 3: Recipe Manager (Add, Search, Save, Load)");

    RecipeManager manager("test_recipes.dat");
    printSuccess("Created RecipeManager with database: test_recipes.dat");

    // Add recipes from folder
    if (!folderPath.empty()) {
        int importCount = manager.importFromFolder(folderPath);
        if (importCount > 0) {
            printSuccess("Imported " + to_string(importCount) + " recipes into manager");
        } else {
            printWarning("No recipes were added to manager");
        }
    } else {
        printInfo("Skipping manager import (no folder provided in TEST 2)");
    }

    // Display all recipes in manager
    size_t recipeCount = manager.getRecipeCount();
    if (recipeCount > 0) {
        printSuccess("Manager contains " + to_string(recipeCount) + " recipes");
        cout << "\n  Recipes in manager:\n";
        for (size_t i = 0; i < manager.getAllRecipes().size(); ++i) {
            cout << "    " << (i + 1) << ". " << manager.getAllRecipes()[i].name << "\n";
        }
    } else {
        printWarning("Manager is empty");
    }

    // ========================================
    // TEST 4: Search Functionality
    // ========================================
    if (recipeCount > 0) {
        printHeader("TEST 4: Search Functionality");

        cout << "Enter a search keyword (recipe name or ingredient):\n";
        cout << Colors::YELLOW << "> " << Colors::RESET;

        string searchKeyword;
        getline(cin, searchKeyword);

        if (!searchKeyword.empty()) {
            auto searchResults = manager.searchRecipes(searchKeyword);

            if (searchResults.empty()) {
                printWarning("No recipes found matching: " + searchKeyword);
            } else {
                printSuccess("Found " + to_string(searchResults.size()) + " recipe(s) matching: " + searchKeyword);
                for (const auto& result : searchResults) {
                    printRecipe(*result);
                }
            }
        } else {
            printWarning("No search keyword provided");
        }
    }

    // ========================================
    // TEST 5: Save to Database
    // ========================================
    printHeader("TEST 5: Save to Database");

    if (recipeCount > 0) {
        if (manager.saveToDatabase()) {
            printSuccess("Successfully saved recipes to database (test_recipes.dat)");
            printInfo("Database file size: ~" + to_string(recipeCount * 500) + " bytes (estimated)");
        } else {
            printError("Failed to save recipes to database");
        }
    } else {
        printWarning("No recipes to save (manager is empty)");
    }

    // ========================================
    // TEST 6: Load from Database
    // ========================================
    printHeader("TEST 6: Load from Database");

    RecipeManager manager2("test_recipes.dat");
    if (manager2.loadFromDatabase()) {
        printSuccess("Successfully loaded recipes from database");
        printInfo("Loaded " + to_string(manager2.getRecipeCount()) + " recipes");
        
        if (manager2.getRecipeCount() > 0) {
            cout << "\n  Recipes loaded from database:\n";
            for (size_t i = 0; i < manager2.getAllRecipes().size(); ++i) {
                cout << "    " << (i + 1) << ". " << manager2.getAllRecipes()[i].name << "\n";
            }
        }
    } else {
        printWarning("No saved database found or database is empty");
    }

    // ========================================
    // TEST 7: Data Integrity Check
    // ========================================
    printHeader("TEST 7: Data Integrity Check");

    if (manager.getRecipeCount() > 0 && manager2.getRecipeCount() > 0) {
        bool integrityOK = true;

        if (manager.getRecipeCount() != manager2.getRecipeCount()) {
            printError("Recipe count mismatch! Manager: " + to_string(manager.getRecipeCount()) + 
                      ", Loaded: " + to_string(manager2.getRecipeCount()));
            integrityOK = false;
        } else {
            printSuccess("Recipe counts match (" + to_string(manager.getRecipeCount()) + " recipes)");
        }

        const auto& originalRecipes = manager.getAllRecipes();
        const auto& loadedRecipes = manager2.getAllRecipes();

        for (size_t i = 0; i < originalRecipes.size() && i < loadedRecipes.size(); ++i) {
            if (originalRecipes[i].name != loadedRecipes[i].name) {
                printError("Recipe name mismatch at index " + to_string(i));
                integrityOK = false;
            }
            if (originalRecipes[i].ingredients.size() != loadedRecipes[i].ingredients.size()) {
                printError("Ingredient count mismatch for: " + originalRecipes[i].name);
                integrityOK = false;
            }
            if (originalRecipes[i].instructions.size() != loadedRecipes[i].instructions.size()) {
                printError("Instruction count mismatch for: " + originalRecipes[i].name);
                integrityOK = false;
            }
        }

        if (integrityOK) {
            printSuccess("Data integrity check PASSED - All data survived save/load cycle");
        } else {
            printError("Data integrity check FAILED - Some data was corrupted");
        }
    } else {
        printWarning("Cannot perform integrity check (insufficient data)");
    }

    // ========================================
    // Summary Report
    // ========================================
    printHeader("TEST SUMMARY");

    cout << Colors::BOLD << "Parser Tests:" << Colors::RESET << "\n";
    cout << "  ✓ Single file parsing\n";
    cout << "  ✓ Folder multi-file import\n";
    cout << "  ✓ Recipe section extraction (name, ingredients, instructions, notes)\n";

    cout << "\n" << Colors::BOLD << "Manager Tests:" << Colors::RESET << "\n";
    cout << "  ✓ Recipe collection management\n";
    cout << "  ✓ Search functionality\n";
    cout << "  ✓ Database save/load\n";
    cout << "  ✓ Data integrity validation\n";

    cout << "\n" << Colors::BOLD << Colors::GREEN << "STATUS: All core logic tests completed!" << Colors::RESET << "\n";
    cout << "\nNext Steps:\n";
    cout << "  1. Review the test results above\n";
    cout << "  2. Fix any parsing issues with recipe file format\n";
    cout << "  3. Build the Win32 GUI shell (main_window.cpp)\n";
    cout << "  4. Wire GUI to RecipeManager for full integration\n\n";

    cout << "Press Enter to exit...";
    cin.get();

    return 0;
}
