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
#define appPrefVersionNum		0x01

#define databaseCreatorID      'WOEM'
#define databaseRecipeName     "RecipesDB-WOEM"
#define databaseIngredientName "IngredientDB-WOEM"
#define databaseUnitName 	   "UnitsDB-WOEM"
#define recipeMaxIngredients    32


/*********************************************************************
 * Structures
 *********************************************************************/
 
// Recipe Record Structure
typedef struct {
    Char name[32];
    UInt16 ingredientCounts[recipeMaxIngredients];
    UInt32 ingredientIDs[recipeMaxIngredients];
    UInt32 ingredientUnits[recipeMaxIngredients];
} RecipeRecord;

/*********************************************************************
 * Global variables
 *********************************************************************/
 
//static UInt32 PantryIngredients[recipeMaxIngredients];
 
// Database handles 
extern DmOpenRef gRecipeDB;
extern DmOpenRef gIngredientDB;
extern DmOpenRef gUnitDB;

/*********************************************************************
 * Quartermaster.c functions
 *********************************************************************/

void * GetObjectPtr(UInt16 objectID);
 
/*********************************************************************
 * Database.c functions
 *********************************************************************/
 
Err DatabaseOpen();
void DatabaseClose();

Err AddRecipe(const Char *recipeName, const Char *ingredientNames[],
    const Char *unitNames[], UInt16 numIngredients, const UInt16 counts[],
    const Char *recipeSteps);
UInt32 IngredientIDByName(const Char *ingredientName);
UInt32 UnitIDByName(const Char *ingredientName);
Err IngredientNameByID(UInt32 entryID, char* buffer);
Err UnitNameByID(UInt32 entryID, char* buffer);
Boolean* QueryRecipes(UInt32 ingId);

/*********************************************************************
 * Main.c functions
 *********************************************************************/
Boolean MainFormDoCommand(UInt16 command);
Boolean MainFormHandleEvent(EventType * eventP);

/*********************************************************************
 * Recipes.c functions
 *********************************************************************/
 
 Boolean RecipeListHandleEvent(EventPtr eventP);
 
/*********************************************************************
 * ViewRecipe.c functions
 *********************************************************************/
Boolean ViewRecipeHandleEvent(EventPtr eventP); 
void OpenRecipeForm(MemHandle recipe);
UInt16 DrawRecipe(FormType *form);

/*********************************************************************
 * Error.c functions
 *********************************************************************/

void displayError(Err code);
void displayFatalError(Err code);

#endif /* QUARTERMASTER_H_ */
