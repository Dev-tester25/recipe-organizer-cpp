#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cctype>

struct Recipe {
    std::string name;
    std::vector<std::string> ingredients;
    std::vector<std::string> instructions;
    std::string notes;
    std::string prepTime;
    int id;
};

class RecipeParser {
public:
    static Recipe parseRecipeFile(const std::string& filePath) {
        Recipe recipe;
        recipe.id = 0;
        recipe.prepTime = "Not specified";

        std::ifstream file(filePath);
        if (!file.is_open()) {
            recipe.name = "Error: Could not open file";
            return recipe;
        }

        std::string line;
        std::string section;
        bool inIngredients = false;
        bool inInstructions = false;
        bool inNotes = false;

        // Get recipe name from first line
        if (std::getline(file, line)) {
            recipe.name = trim(line);
        }

        while (std::getline(file, line)) {
            line = trim(line);

            // Skip empty lines
            if (line.empty()) continue;

            // Detect section headers
            if (line == "Ingredients:") {
                inIngredients = true;
                inInstructions = false;
                inNotes = false;
                continue;
            }
            else if (line == "Instructions:") {
                inIngredients = false;
                inInstructions = true;
                inNotes = false;
                continue;
            }
            else if (line == "Notes:") {
                inIngredients = false;
                inInstructions = false;
                inNotes = true;
                continue;
            }

            // Parse content based on current section
            if (inIngredients) {
                recipe.ingredients.push_back(line);
            }
            else if (inInstructions) {
                recipe.instructions.push_back(line);
            }
            else if (inNotes) {
                if (!recipe.notes.empty()) {
                    recipe.notes += " ";
                }
                recipe.notes += line;
            }
        }

        file.close();
        return recipe;
    }

    static std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }
};
