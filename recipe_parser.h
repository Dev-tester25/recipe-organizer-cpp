#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <filesystem>

struct Recipe {
    std::string name;
    std::string prepTime;
    std::string cookTime;
    std::string servings;
    std::string notes;
    std::vector<std::string> ingredients;
    std::vector<std::string> instructions;
};

class RecipeParser {
public:
    static std::string trim(const std::string& input) {
        const std::string whitespace = " \t\r\n";
        size_t start = input.find_first_not_of(whitespace);
        if (start == std::string::npos) return "";
        size_t end = input.find_last_not_of(whitespace);
        return input.substr(start, end - start + 1);
    }

    static std::string toLower(std::string text) {
        std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        return text;
    }

    static std::string stripLeadingNumberOrBullet(const std::string& text) {
        std::string s = trim(text);
        if (s.empty()) return s;

        // Remove common markdown/bullet prefixes: "1. ", "2) ", "- ", "* "
        size_t pos = 0;
        while (pos < s.size() && (s[pos] == '-' || s[pos] == '*' || s[pos] == '>' || s[pos] == ' ' || s[pos] == '\t')) pos++;
        if (pos < s.size()) {
            // If it starts with digits + punctuation then drop it
            if (s[pos] >= '0' && s[pos] <= '9') {
                size_t i = pos;
                while (i < s.size() && s[i] >= '0' && s[i] <= '9') i++;
                if (i < s.size() && (s[i] == '.' || s[i] == ')')) {
                    s = trim(s.substr(i + 1));
                }
            }
        }

        return trim(s);
    }

    static bool startsWithIgnoreCase(const std::string& text, const std::string& prefix) {
        if (prefix.size() > text.size()) return false;
        return toLower(text.substr(0, prefix.size())) == toLower(prefix);
    }

    static std::string extractAfterColon(const std::string& line) {
        size_t pos = line.find(':');
        if (pos == std::string::npos) return "";
        return trim(line.substr(pos + 1));
    }

    static Recipe parseRecipeFile(const std::string& filePath) {
        Recipe recipe;
        recipe.name = "Untitled Recipe";
        recipe.prepTime = "Not specified";
        recipe.cookTime = "Not specified";
        recipe.servings = "Not specified";

        std::ifstream input(filePath);
        if (!input.is_open()) {
            recipe.name = "Error: Cannot open file";
            return recipe;
        }

        std::string line;
        std::string currentSection = "";
        bool titleSet = false;

        while (std::getline(input, line)) {
            std::string trimmed = trim(line);
            if (trimmed.empty()) continue;

            if (startsWithIgnoreCase(trimmed, "Ingredients:")) {
                currentSection = "Ingredients";
                continue;
            }
            if (startsWithIgnoreCase(trimmed, "Instructions:")) {
                currentSection = "Instructions";
                continue;
            }
            if (startsWithIgnoreCase(trimmed, "Notes:")) {
                currentSection = "Notes";
                std::string noteText = extractAfterColon(trimmed);
                if (!noteText.empty()) {
                    if (!recipe.notes.empty()) recipe.notes += " ";
                    recipe.notes += noteText;
                }
                continue;
            }
            if (startsWithIgnoreCase(trimmed, "Prep Time:")) {
                recipe.prepTime = extractAfterColon(trimmed);
                continue;
            }
            if (startsWithIgnoreCase(trimmed, "Prep time:")) {
                recipe.prepTime = extractAfterColon(trimmed);
                continue;
            }
            if (startsWithIgnoreCase(trimmed, "Cook Time:")) {
                recipe.cookTime = extractAfterColon(trimmed);
                continue;
            }
            if (startsWithIgnoreCase(trimmed, "Cook time:")) {
                recipe.cookTime = extractAfterColon(trimmed);
                continue;
            }
            if (startsWithIgnoreCase(trimmed, "Servings:")) {
                recipe.servings = extractAfterColon(trimmed);
                continue;
            }
            if (startsWithIgnoreCase(trimmed, "Language:")) {
                continue;
            }
            if (startsWithIgnoreCase(trimmed, "Title:")) {
                recipe.name = extractAfterColon(trimmed);
                titleSet = true;
                continue;
            }

            if (!titleSet) {
                recipe.name = trimmed;
                titleSet = true;
                continue;
            }

            if (currentSection == "Ingredients") {
                recipe.ingredients.push_back(stripLeadingNumberOrBullet(trimmed));
            }
            else if (currentSection == "Instructions") {
                recipe.instructions.push_back(stripLeadingNumberOrBullet(trimmed));
            }
            else if (currentSection == "Notes") {
                if (!recipe.notes.empty()) recipe.notes += " ";
                recipe.notes += stripLeadingNumberOrBullet(trimmed);
            }
        }

        input.close();
        if (recipe.name == "Untitled Recipe" || recipe.name == "Error: Cannot open file") {
            recipe.name = trim(recipe.name);
        }

        return recipe;
    }

    static std::vector<Recipe> parseRecipeFolder(const std::string& folderPath) {
        std::vector<Recipe> recipes;
        std::filesystem::path folder(folderPath);
        if (!std::filesystem::exists(folder) || !std::filesystem::is_directory(folder)) {
            return recipes;
        }

        for (const auto& entry : std::filesystem::directory_iterator(folder)) {
            if (entry.is_regular_file()) {
                auto ext = entry.path().extension();
                if (ext == ".txt" || ext == ".TXT") {
                    Recipe recipe = parseRecipeFile(entry.path().string());
                    if (!recipe.name.empty() && recipe.name != "Error: Cannot open file") {
                        recipes.push_back(recipe);
                    }
                }
            }
        }

        std::sort(recipes.begin(), recipes.end(), [](const Recipe& a, const Recipe& b) {
            return toLower(a.name) < toLower(b.name);
        });

        return recipes;
    }

    static bool saveRecipes(const std::string& filePath, const std::vector<Recipe>& recipes) {
        std::ofstream out(filePath, std::ios::out | std::ios::trunc);
        if (!out.is_open()) return false;

        out << recipes.size() << "\n";
        for (const auto& recipe : recipes) {
            out << "[RECIPE]\n";
            out << recipe.name << "\n";
            out << recipe.prepTime << "\n";
            out << recipe.cookTime << "\n";
            out << recipe.servings << "\n";
            out << recipe.notes << "\n";
            out << recipe.ingredients.size() << "\n";
            for (const auto& ingredient : recipe.ingredients) {
                out << ingredient << "\n";
            }
            out << recipe.instructions.size() << "\n";
            for (const auto& step : recipe.instructions) {
                out << step << "\n";
            }
        }

        out.close();
        return true;
    }

    static std::vector<Recipe> loadRecipes(const std::string& filePath) {
        std::vector<Recipe> recipes;
        std::ifstream in(filePath);
        if (!in.is_open()) return recipes;

        size_t count = 0;
        in >> count;
        in.ignore(10000, '\n');

        for (size_t i = 0; i < count; ++i) {
            std::string marker;
            std::getline(in, marker);
            if (marker != "[RECIPE]") {
                continue;
            }

            Recipe recipe;
            recipe.prepTime = "Not specified";
            recipe.cookTime = "Not specified";
            recipe.servings = "Not specified";

            std::getline(in, recipe.name);
            std::getline(in, recipe.prepTime);
            std::getline(in, recipe.cookTime);
            std::getline(in, recipe.servings);
            std::getline(in, recipe.notes);

            size_t ingredientCount = 0;
            in >> ingredientCount;
            in.ignore(10000, '\n');
            for (size_t j = 0; j < ingredientCount; ++j) {
                std::string ingredient;
                std::getline(in, ingredient);
                recipe.ingredients.push_back(ingredient);
            }

            size_t instructionCount = 0;
            in >> instructionCount;
            in.ignore(10000, '\n');
            for (size_t j = 0; j < instructionCount; ++j) {
                std::string instruction;
                std::getline(in, instruction);
                recipe.instructions.push_back(instruction);
            }

            recipes.push_back(recipe);
        }

        in.close();
        return recipes;
    }
};
