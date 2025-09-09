#ifndef DATABASE_H
#define DATABASE_H

#include <PalmOS.h>

// --- Constants ---
#define databaseCreatorID      'WOEM'
//#define DB_TYPE_RECIPE     'DATA'
#define databaseRecipeName     "RecipesDB-WOEM"
#define databasePantryName     "PantryDB-WOEM"

#define recipeMaxIngredients    32
#define MAX_STEPS          10

// --- Recipe Record Structure ---
typedef struct {
    Char name[32];
    UInt16 ingredientCounts[MAX_INGREDIENTS];
    UInt16 ingredientIDs[MAX_INGREDIENTS];
    Char steps[512]; // Could also be stored separately
} RecipeRecord;

// --- Ingredient Structure ---
typedef struct {
    UInt16 id;
    Char name[32];
} Ingredient;

// --- Pantry Record Structure ---
// UInt16 array of ingredient IDs

// --- Database Functions ---
Err DatabaseOpen();
void DatabaseClose();

Err AddRecipe(const RecipeRecord* newRecipePtr);
//Err GetRecipe(UInt16 index, RecipeRecord* outRecipe);
//UInt16 GetRecipeCount();

//Err SavePantry(const UInt16* pantry);
//Err LoadPantry(UInt16* outPantry);

#endif