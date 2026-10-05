#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include "recipe_parser.h"

class RecipeManager {
private:
    std::vector<Recipe> recipes;
    std::string databasePath;

public:
    RecipeManager(const std::string& dbPath = "recipes.dat") : databasePath(dbPath) {
        loadFromDatabase();
    }

    // Load recipes from database file
    bool loadFromDatabase() {
        recipes = RecipeParser::loadRecipes(databasePath);
        return !recipes.empty();
    }

    // Save recipes to database file
    bool saveToDatabase() {
        return RecipeParser::saveRecipes(databasePath, recipes);
    }

    // Import recipes from a folder of .txt files
    int importFromFolder(const std::string& folderPath) {
        std::vector<Recipe> importedRecipes = RecipeParser::parseRecipeFolder(folderPath);
        int count = 0;

        for (const auto& recipe : importedRecipes) {
            if (addRecipe(recipe)) {
                count++;
            }
        }

        return count;
    }

    // Add a single recipe
    bool addRecipe(const Recipe& recipe) {
        if (recipe.name.empty() || recipe.name == "Error: Cannot open file") {
            return false;
        }

        // Check for duplicates by name
        for (const auto& existing : recipes) {
            if (RecipeParser::toLower(existing.name) == RecipeParser::toLower(recipe.name)) {
                return false; // Recipe already exists
            }
        }

        recipes.push_back(recipe);
        return true;
    }

    // Get all recipes
    const std::vector<Recipe>& getAllRecipes() const {
        return recipes;
    }

    // Get recipe by index
    Recipe* getRecipeByIndex(size_t index) {
        if (index < recipes.size()) {
            return &recipes[index];
        }
        return nullptr;
    }

    // Get recipe by name
    Recipe* getRecipeByName(const std::string& name) {
        for (auto& recipe : recipes) {
            if (RecipeParser::toLower(recipe.name) == RecipeParser::toLower(name)) {
                return &recipe;
            }
        }
        return nullptr;
    }

    // Search recipes by name or ingredient
    std::vector<Recipe*> searchRecipes(const std::string& keyword) {
        std::vector<Recipe*> results;
        std::string lowerKeyword = RecipeParser::toLower(keyword);

        for (auto& recipe : recipes) {
            bool matchName = RecipeParser::toLower(recipe.name).find(lowerKeyword) != std::string::npos;
            bool matchIngredient = false;

            for (const auto& ingredient : recipe.ingredients) {
                if (RecipeParser::toLower(ingredient).find(lowerKeyword) != std::string::npos) {
                    matchIngredient = true;
                    break;
                }
            }

            if (matchName || matchIngredient) {
                results.push_back(&recipe);
            }
        }

        return results;
    }

    // Remove recipe by index
    bool removeRecipeByIndex(size_t index) {
        if (index < recipes.size()) {
            recipes.erase(recipes.begin() + index);
            return true;
        }
        return false;
    }

    // Remove recipe by name
    bool removeRecipeByName(const std::string& name) {
        for (size_t i = 0; i < recipes.size(); ++i) {
            if (RecipeParser::toLower(recipes[i].name) == RecipeParser::toLower(name)) {
                recipes.erase(recipes.begin() + i);
                return true;
            }
        }
        return false;
    }

    // Get recipe count
    size_t getRecipeCount() const {
        return recipes.size();
    }

    // Clear all recipes
    void clearAllRecipes() {
        recipes.clear();
    }

    // Sort recipes alphabetically
    void sortRecipes() {
        std::sort(recipes.begin(), recipes.end(), [](const Recipe& a, const Recipe& b) {
            return RecipeParser::toLower(a.name) < RecipeParser::toLower(b.name);
        });
    }
};
