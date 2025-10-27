/*
 * Quartermaster.h
 *
 * header file for Quartermaster
 *
 */
 
#include <PalmOS.h>
 
#ifndef QUARTERMASTER_H_
#define QUARTERMASTER_H_

/*********************************************************************
 *  Constants
 *********************************************************************/
 
#define appFileCreator			'WOEM'
#define appName					"Quartermaster"
#define appVersionNum			0x01
#define appPrefID				0x00
#define appPrefVersionNum	    0x01

#define databaseCreatorID       'WOEM'
#define databaseRecipeName      "QMRecipes"
#define databaseIngredientName  "QMIngredients"
#define databaseUnitName 	    "QMUnits"
#define databasePantryName 	    "QMPantry"
#define databaseGroceryName	    "QMGrocList"
#define recipeMaxIngredients    32

// Custom errors
#define errRecipeNameBlank		(appErrorClass | 11)
#define errRecipeMaxIngreds		(appErrorClass | 12)
#define errIngredNameBlank		(appErrorClass | 21)
#define errIngredInUse          (appErrorClass | 22)
#define errSearchNoMatch		(appErrorClass | 31)
			


/*********************************************************************
 * Structures
 *********************************************************************/
 
// Recipe Record Structure
typedef struct {
    Char name[32];
    UInt8 numIngredients;
    UInt8 ingredientCounts[recipeMaxIngredients];
    UInt8 ingredientFracs[recipeMaxIngredients];
    UInt8 ingredientDenoms[recipeMaxIngredients];
    UInt32 ingredientIDs[recipeMaxIngredients];
    UInt32 ingredientUnits[recipeMaxIngredients];
} RecipeRecord;

/*********************************************************************
 * Global variables
 *********************************************************************/
 
// Database handles 
extern DmOpenRef gRecipeDB;
extern DmOpenRef gIngredientDB;
extern DmOpenRef gUnitDB;
extern DmOpenRef gPantryDB;
extern DmOpenRef gGroceryDB;

/*********************************************************************
 * Quartermaster.c functions
 *********************************************************************/
 
void AppStop();
//Misc shared functions
void DrawIngredientList(Int16 itemNum, RectanglePtr bounds, Char** data);
void FormatQuantity(Char *out, UInt16 count, UInt8 frac, UInt8 denom);
Boolean MainMenuDoCommand(UInt16 command);
 
/*********************************************************************
 * Database.c functions
 *********************************************************************/
 
Err DatabaseOpen();
void DatabaseClose();
Boolean EntryInDatabase(DmOpenRef dbase, UInt32 id);
UInt16 IndexOfEntry(DmOpenRef dbase, UInt32 id);
Err AddIdToDatabase(DmOpenRef dbase, UInt32 id);
UInt16 IndexFromID(DmOpenRef dbase, UInt32 id);
UInt32 IDFromIndex(DmOpenRef dbase, UInt16 index);
RecipeRecord RecipeGetRecord(MemPtr recP);

Err AddRecipe(const Char *recipeName, const Char *ingredientNames[],
    const Char *unitNames[], UInt16 numIngredients, const UInt8 counts[],
    const UInt8 fracs[], const UInt8 denoms[], const Char *recipeSteps);
Err RemoveRecipe(UInt16 recipeIndex);
Char* RecipeGetStepsPtr(MemPtr recP); 
    
UInt32 IngredientIDByName(const Char *ingredientName);
Err IngredientNameByID(Char* buffer, UInt8 len, UInt32 entryID);
Err RemoveIngredient(UInt32 ingId);

UInt32 UnitIDByName(const Char *ingredientName);
Err UnitNameByID(Char* buffer, UInt8 len, UInt32 entryID);

UInt16 PantryFuzzySearch(MemHandle* ret);
UInt16 PantryStrictSearch(MemHandle* ret);

/*********************************************************************
 * RecipeList.c functions
 *********************************************************************/
 Boolean RecipeListHandleEvent(EventPtr eventP);
 void OpenRecipeList(MemHandle results, UInt16 num);
 //Err PopulateRecipeList(ListType* list);
 
/*********************************************************************
 * ViewRecipe.c functions
 *********************************************************************/
Boolean ViewRecipeHandleEvent(EventPtr eventP); 
void OpenRecipeForm(UInt16 selection);
//UInt16 DrawRecipe(FormType *form);

/*********************************************************************
 * EditRecipe.c functions
 *********************************************************************/
Boolean EditRecipeHandleEvent(EventPtr eventP);
Err OpenEditRecipeForm(UInt16 recipeIndex, Boolean isNew);
Boolean AddIngredientHandleEvent(EventPtr eventP);
Err AddIngredientForm();

/*********************************************************************
 * Pantry.c functions
 *********************************************************************/
Boolean PantryHandleEvent(EventPtr eventP);

/*********************************************************************
 * GroceryList.c functions
 *********************************************************************/
Boolean GroceryHandleEvent(EventPtr eventP);

/*********************************************************************
 * Alerts.c functions
 *********************************************************************/

void displayError(Err code);
void displayErrorIf(Err code);
void displayFatalError(Err code);
Boolean confirmChoice(UInt8 dialogC);

#endif /* QUARTERMASTER_H_ */
