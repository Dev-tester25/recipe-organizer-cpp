#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>
#include <limits>
#include <windows.h>

using namespace std;

// ------------------------
// ANSI COLORS
// ------------------------
namespace ANSI {
    const string RESET = "\x1b[0m";
    const string BOLD = "\x1b[1m";
    const string DIM = "\x1b[2m";
    const string FG_RED = "\x1b[31m";
    const string FG_GREEN = "\x1b[32m";
    const string FG_YELLOW = "\x1b[33m";
    const string FG_BLUE = "\x1b[34m";
    const string FG_CYAN = "\x1b[36m";
    const string FG_MAGENTA = "\x1b[35m";
    const string FG_WHITE = "\x1b[37m";
    const string BG_RED = "\x1b[41m";
    const string BG_GREEN = "\x1b[42m";
    const string BG_YELLOW = "\x1b[43m";
    const string BG_BLUE = "\x1b[44m";
    const string BG_CYAN = "\x1b[46m";
}

void setColor(const string& code) {
    cout << code;
}

void resetColor() {
    cout << ANSI::RESET;
}

void clearScreen() {
    system("cls");
}

void enableAnsiWin32() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) return;

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
    SetConsoleTitleA("Recipe Organizer");
}

string trim(const string& s) {
    size_t start = 0;
    while (start < s.length() && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r' || s[start] == '\n')) start++;
    size_t end = s.length();
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r' || s[end - 1] == '\n')) end--;
    return s.substr(start, end - start);
}

vector<string> splitString(const string& text, char delim) {
    vector<string> parts;
    stringstream ss(text);
    string item;
    while (getline(ss, item, delim)) {
        parts.push_back(item);
    }
    return parts;
}

string toLower(string s) {
    for (char& c : s) {
        c = static_cast<char>(tolower((unsigned char)c));
    }
    return s;
}

struct Recipe {
    int id = 0;
    string name;
    string cuisine;
    int prepMinutes = 0;
    int cookMinutes = 0;
    string difficulty;
    vector<string> ingredients;
    vector<string> steps;
};

vector<Recipe> recipes;
const string FILE_NAME = "recipes_data.txt";

void printHeader() {
    clearScreen();
    setColor(ANSI::FG_CYAN + ANSI::BOLD);
    cout << "====================================================\n";
    cout << "                 RECIPE ORGANIZER                   \n";
    cout << "====================================================\n";
    resetColor();
}

void pauseForEnter() {
    cout << "\nPress Enter to continue...";
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

string promptLine(const string& prompt) {
    string value;
    cout << prompt;
    getline(cin, value);
    return trim(value);
}

int promptInt(const string& prompt, int minValue = 0) {
    string input;
    int value = minValue;
    while (true) {
        cout << prompt;
        getline(cin, input);
        try {
            value = stoi(input);
            if (value >= minValue) return value;
        } catch (...) {
            // ignore
        }
        cout << ANSI::FG_RED << "Invalid number. Try again.\n" << ANSI::RESET;
    }
}

void printMenu() {
    printHeader();
    setColor(ANSI::FG_GREEN + ANSI::BOLD);
    cout << "1. Add Recipe\n";
    cout << "2. View All Recipes\n";
    cout << "3. View One Recipe\n";
    cout << "4. Search Recipes\n";
    cout << "5. Remove Recipe\n";
    cout << "6. Save Recipes\n";
    cout << "7. Load Recipes\n";
    cout << "8. Exit\n";
    cout << "----------------------------------------------------\n";
    resetColor();
    cout << "Choose an option: ";
}

bool saveRecipesToFile(const string& filePath) {
    ofstream out(filePath, ios::out | ios::trunc);
    if (!out.is_open()) {
        cout << ANSI::FG_RED << "Unable to open file for saving.\n" << ANSI::RESET;
        return false;
    }

    out << recipes.size() << "\n";
    for (const auto& recipe : recipes) {
        out << recipe.id << "|"
            << recipe.name << "|"
            << recipe.cuisine << "|"
            << recipe.prepMinutes << "|"
            << recipe.cookMinutes << "|"
            << recipe.difficulty << "\n";

        out << recipe.ingredients.size() << "\n";
        for (const auto& ing : recipe.ingredients) {
            out << ing << "\n";
        }

        out << recipe.steps.size() << "\n";
        for (const auto& step : recipe.steps) {
            out << step << "\n";
        }
    }

    out.close();
    cout << ANSI::FG_GREEN << "Recipes saved to " << filePath << "\n" << ANSI::RESET;
    return true;
}

bool loadRecipesFromFile(const string& filePath) {
    ifstream in(filePath);
    if (!in.is_open()) {
        cout << ANSI::FG_YELLOW << "No saved file found. Starting fresh.\n" << ANSI::RESET;
        return false;
    }

    recipes.clear();

    size_t count = 0;
    in >> count;
    in.ignore(numeric_limits<streamsize>::max(), '\n');

    for (size_t i = 0; i < count; ++i) {
        string line;
        getline(in, line);
        if (line.empty()) {
            // skip empty line if needed
            getline(in, line);
        }

        vector<string> parts = splitString(line, '|');
        if (parts.size() < 6) continue;

        Recipe recipe;
        recipe.id = stoi(parts[0]);
        recipe.name = parts[1];
        recipe.cuisine = parts[2];
        recipe.prepMinutes = stoi(parts[3]);
        recipe.cookMinutes = stoi(parts[4]);
        recipe.difficulty = parts[5];

        size_t ingredientCount = 0;
        in >> ingredientCount;
        in.ignore(numeric_limits<streamsize>::max(), '\n');

        for (size_t j = 0; j < ingredientCount; ++j) {
            string ingredient;
            getline(in, ingredient);
            recipe.ingredients.push_back(trim(ingredient));
        }

        size_t stepCount = 0;
        in >> stepCount;
        in.ignore(numeric_limits<streamsize>::max(), '\n');

        for (size_t j = 0; j < stepCount; ++j) {
            string step;
            getline(in, step);
            recipe.steps.push_back(trim(step));
        }

        recipes.push_back(recipe);
    }

    in.close();
    cout << ANSI::FG_GREEN << "Recipes loaded from " << filePath << "\n" << ANSI::RESET;
    return true;
}

void addRecipe() {
    Recipe recipe;
    recipe.id = (recipes.empty() ? 1 : recipes.back().id + 1);

    cout << ANSI::FG_YELLOW << "\n=== Add New Recipe ===\n" << ANSI::RESET;

    recipe.name = promptLine("Recipe name: ");
    if (recipe.name.empty()) {
        cout << ANSI::FG_RED << "Recipe name cannot be empty.\n" << ANSI::RESET;
        return;
    }

    recipe.cuisine = promptLine("Cuisine: ");
    recipe.prepMinutes = promptInt("Prep time (minutes): ", 0);
    recipe.cookMinutes = promptInt("Cook time (minutes): ", 0);
    recipe.difficulty = promptLine("Difficulty (Easy/Medium/Hard): ");

    cout << "Add ingredients (type 'done' when finished):\n";
    while (true) {
        string ingredient = promptLine("Ingredient: ");
        if (toLower(ingredient) == "done") break;
        if (!ingredient.empty()) recipe.ingredients.push_back(ingredient);
    }

    cout << "Add steps (type 'done' when finished):\n";
    int stepNo = 1;
    while (true) {
        string step = promptLine("Step " + to_string(stepNo) + ": ");
        if (toLower(step) == "done") break;
        if (!step.empty()) {
            recipe.steps.push_back(step);
            stepNo++;
        }
    }

    recipes.push_back(recipe);
    cout << ANSI::FG_GREEN << "\nRecipe added successfully.\n" << ANSI::RESET;
    pauseForEnter();
}

void displayRecipe(const Recipe& recipe) {
    cout << ANSI::FG_CYAN << ANSI::BOLD;
    cout << "\n============================================\n";
    cout << recipe.name << "\n";
    cout << "============================================\n";
    resetColor();

    cout << "Cuisine: " << recipe.cuisine << "\n";
    cout << "Prep: " << recipe.prepMinutes << " min | Cook: " << recipe.cookMinutes << " min\n";
    cout << "Difficulty: " << recipe.difficulty << "\n\n";

    cout << ANSI::FG_YELLOW << "Ingredients:\n" << ANSI::RESET;
    if (recipe.ingredients.empty()) {
        cout << "  None\n";
    } else {
        for (size_t i = 0; i < recipe.ingredients.size(); ++i) {
            cout << "  " << (i + 1) << ". " << recipe.ingredients[i] << "\n";
        }
    }

    cout << ANSI::FG_YELLOW << "\nSteps:\n" << ANSI::RESET;
    if (recipe.steps.empty()) {
        cout << "  None\n";
    } else {
        for (size_t i = 0; i < recipe.steps.size(); ++i) {
            cout << "  " << (i + 1) << ". " << recipe.steps[i] << "\n";
        }
    }
    cout << "\n";
}

void viewAllRecipes() {
    printHeader();
    if (recipes.empty()) {
        cout << ANSI::FG_RED << "No recipes saved yet.\n" << ANSI::RESET;
        pauseForEnter();
        return;
    }

    cout << ANSI::FG_GREEN << "All Recipes\n" << ANSI::RESET;
    for (const auto& recipe : recipes) {
        cout << "--------------------------------------------\n";
        cout << ANSI::FG_CYAN << recipe.name << ANSI::RESET << " | "
             << recipe.cuisine << " | "
             << recipe.difficulty << " | "
             << recipe.prepMinutes << "m prep | "
             << recipe.cookMinutes << "m cook\n";
    }
    cout << "--------------------------------------------\n";
    pauseForEnter();
}

void viewOneRecipe() {
    if (recipes.empty()) {
        cout << ANSI::FG_RED << "No recipes to view.\n" << ANSI::RESET;
        pauseForEnter();
        return;
    }

    cout << ANSI::FG_YELLOW << "\n=== View Recipe ===\n" << ANSI::RESET;
    for (size_t i = 0; i < recipes.size(); ++i) {
        cout << i + 1 << ". " << recipes[i].name << "\n";
    }

    int index = promptInt("Enter recipe number to view: ", 1);
    if (index < 1 || index > static_cast<int>(recipes.size())) {
        cout << ANSI::FG_RED << "Invalid recipe number.\n" << ANSI::RESET;
        pauseForEnter();
        return;
    }

    displayRecipe(recipes[index - 1]);
    pauseForEnter();
}

void searchRecipes() {
    if (recipes.empty()) {
        cout << ANSI::FG_RED << "No recipes to search.\n" << ANSI::RESET;
        pauseForEnter();
        return;
    }

    string keyword = promptLine("Search by recipe name or ingredient: ");
    if (keyword.empty()) {
        cout << ANSI::FG_RED << "Search keyword cannot be empty.\n" << ANSI::RESET;
        pauseForEnter();
        return;
    }

    string lowerKeyword = toLower(keyword);

    bool found = false;
    for (const auto& recipe : recipes) {
        bool matchName = toLower(recipe.name).find(lowerKeyword) != string::npos;
        bool matchIngredient = false;

        for (const auto& ing : recipe.ingredients) {
            if (toLower(ing).find(lowerKeyword) != string::npos) {
                matchIngredient = true;
                break;
            }
        }

        if (matchName || matchIngredient) {
            found = true;
            displayRecipe(recipe);
        }
    }

    if (!found) {
        cout << ANSI::FG_RED << "No recipes found matching: " << keyword << "\n" << ANSI::RESET;
    }

    pauseForEnter();
}

void removeRecipe() {
    if (recipes.empty()) {
        cout << ANSI::FG_RED << "No recipes to remove.\n" << ANSI::RESET;
        pauseForEnter();
        return;
    }

    cout << ANSI::FG_YELLOW << "\n=== Remove Recipe ===\n" << ANSI::RESET;
    for (size_t i = 0; i < recipes.size(); ++i) {
        cout << i + 1 << ". " << recipes[i].name << "\n";
    }

    int index = promptInt("Enter recipe number to remove: ", 1);
    if (index < 1 || index > static_cast<int>(recipes.size())) {
        cout << ANSI::FG_RED << "Invalid recipe number.\n" << ANSI::RESET;
        pauseForEnter();
        return;
    }

    recipes.erase(recipes.begin() + (index - 1));
    cout << ANSI::FG_GREEN << "Recipe removed successfully.\n" << ANSI::RESET;
    pauseForEnter();
}

void showWelcome() {
    clearScreen();
    setColor(ANSI::FG_MAGENTA + ANSI::BOLD);
    cout << "====================================================\n";
    cout << "        Welcome to Recipe Organizer v1.0           \n";
    cout << "====================================================\n";
    resetColor();
    cout << "A simple Win32 + ANSI terminal recipe manager.\n";
    cout << "Use the menu to add, search, view, and save recipes.\n\n";
    pauseForEnter();
}

int main() {
    enableAnsiWin32();
    showWelcome();

    while (true) {
        printMenu();
        string choiceLine;
        getline(cin, choiceLine);
        int choice = 0;

        try {
            choice = stoi(choiceLine);
        } catch (...) {
            cout << ANSI::FG_RED << "Invalid menu choice.\n" << ANSI::RESET;
            pauseForEnter();
            continue;
        }

        switch (choice) {
            case 1:
                addRecipe();
                break;
            case 2:
                viewAllRecipes();
                break;
            case 3:
                viewOneRecipe();
                break;
            case 4:
                searchRecipes();
                break;
            case 5:
                removeRecipe();
                break;
            case 6:
                saveRecipesToFile(FILE_NAME);
                pauseForEnter();
                break;
            case 7:
                loadRecipesFromFile(FILE_NAME);
                pauseForEnter();
                break;
            case 8:
                cout << ANSI::FG_GREEN << "Saving before exit...\n" << ANSI::RESET;
                saveRecipesToFile(FILE_NAME);
                cout << "Goodbye!\n";
                return 0;
            default:
                cout << ANSI::FG_RED << "Invalid option.\n" << ANSI::RESET;
                pauseForEnter();
                break;
        }
    }

    return 0;
}
