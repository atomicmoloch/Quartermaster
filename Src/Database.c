#include <PalmOS.h>
#include "Quartermaster.h"
#include "Quartermaster_Rsc.h"

/*********************************************************************
 * External Variables
 *********************************************************************/

DmOpenRef gRecipeDB;
DmOpenRef gIngredientDB;
DmOpenRef gUnitDB;

/*********************************************************************
 * Internal Functions
 *********************************************************************/

/***********************************************************************
 *
 * FUNCTION:     CompareRecipeNames
 *
 * DESCRIPTION:  Internal function - compares recipe names 
 *               alphabetically
 *
 * PARAMETERS:   two recipe pointers
 *
 * RETURNED:     0 if strings match, positive number if rec1 sorts after
 *				 rec2 alphabetically, negative number if the reverse
 *
 ***********************************************************************/
static Int16 CompareRecipeNames(void *rec1, void *rec2, Int16 other,
                        SortRecordInfoPtr rec1SortInfo,
                        SortRecordInfoPtr rec2SortInfo,
                        MemHandle appInfoH)
{
    RecipeRecord *r1 = (RecipeRecord *)rec1;
    RecipeRecord *r2 = (RecipeRecord *)rec2;
    return StrCompare(r1->name, r2->name);
}

static Int16 DBStringCompare(void *rec1, void *rec2, Int16 other,
                        SortRecordInfoPtr rec1SortInfo,
                        SortRecordInfoPtr rec2SortInfo,
                        MemHandle appInfoH)
{
    return StrCompare((Char *)rec1, (Char *)rec2);
}



/*********************************************************************
 * External Functions
 *********************************************************************/

/***********************************************************************
 *
 * FUNCTION:     DatabaseOpen
 *
 * DESCRIPTION:  Opens Pantry and Recipe databases
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     errNone or dmErrCantOpen
 *
 ***********************************************************************/
Err DatabaseOpen() {
    LocalID dbID;
    
    dbID = DmFindDatabase(0, databaseRecipeName);
    if (!dbID) {
        DmCreateDatabase(0, databaseRecipeName, databaseCreatorID, 'Data', false);
        dbID = DmFindDatabase(0, databaseRecipeName);
        if (!dbID) return dmErrCantOpen;
    }
    gRecipeDB = DmOpenDatabase(0, dbID, dmModeReadWrite);
    if (!gRecipeDB) return DmGetLastErr();

    dbID = DmFindDatabase(0, databaseIngredientName);
    if (!dbID) {
        DmCreateDatabase(0, databaseIngredientName, databaseCreatorID, 'Data', false);
        dbID = DmFindDatabase(0, databaseIngredientName);
        if (!dbID) return dmErrCantOpen;
    }
    gIngredientDB = DmOpenDatabase(0, dbID, dmModeReadWrite);
    if (!gIngredientDB) return DmGetLastErr();
    
    dbID = DmFindDatabase(0, databaseUnitName);
    if (!dbID) {
        DmCreateDatabase(0, databaseUnitName, databaseCreatorID, 'Data', false);
        dbID = DmFindDatabase(0, databaseUnitName);
        if (!dbID) return dmErrCantOpen;
    }
    gUnitDB = DmOpenDatabase(0, dbID, dmModeReadWrite);
    if (!gUnitDB) return DmGetLastErr();

    return errNone;
}


/***********************************************************************
 *
 * FUNCTION:     DatabaseClose
 *
 * DESCRIPTION:  Closes all databases
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     nothing
 *
 ***********************************************************************/
void DatabaseClose() {
    if (gRecipeDB) DmCloseDatabase(gRecipeDB);
    if (gIngredientDB) DmCloseDatabase(gIngredientDB);
    if (gUnitDB) DmCloseDatabase(gUnitDB);
}

/***********************************************************************
 *
 * FUNCTION:     AddRecipe
 *
 * DESCRIPTION:  Adds new recipe to database
 *
 * PARAMETERS:   Name, ingredients, units, amounts, number of ingredients
 *				 and steps of recipe
 *
 * RETURNED:     Err
 *
 ***********************************************************************/

Err AddRecipe(
    const Char *recipeName,
    const Char *ingredientNames[],
    const Char *unitNames[],
    UInt16 numIngredients,
    const UInt16 counts[],
    const Char *recipeSteps)
{
    RecipeRecord recipe;
    Err err = MemSet(&recipe, sizeof(recipe), 0);
	UInt16 stepsLen = StrLen(recipeSteps) + 1;
	UInt32 totalSize = sizeof(RecipeRecord) + stepsLen;
	MemHandle recH;
	RecipeRecord *recP;
	UInt16 recordIndex;
    UInt16 i;
    
    if (err != errNone) return err;
	
	StrNCopy(recipe.name, recipeName, sizeof(recipe.name)-1);
    for (i = 0; i < numIngredients; i++) {
        recipe.ingredientCounts[i] = counts[i];

        recipe.ingredientIDs[i] = IngredientIDByName(ingredientNames[i]);
        if (recipe.ingredientIDs[i] == -1) return dmErrResourceNotFound; 

        recipe.ingredientUnits[i] = UnitIDByName(unitNames[i]);
        if (recipe.ingredientUnits[i] == -1) return dmErrResourceNotFound;
    }	
	//Keeps database sorted alphabetically
	recordIndex = DmFindSortPosition(gRecipeDB, &recipe, 0, (DmComparF *) CompareRecipeNames, 0);
	
	recH = DmNewRecord(gRecipeDB, &recordIndex, totalSize);
	recP = MemHandleLock(recH);    

	DmWrite(recP, 
        0,  
        &recipe,
        sizeof(recipe));       

	DmWrite(recP, 
        sizeof(recipe), 
        recipeSteps,
        stepsLen);       
        
	MemHandleUnlock(recH);
	DmReleaseRecord(gRecipeDB, recordIndex, true);

	return errNone;
}

/***********************************************************************
 *
 * FUNCTION:     QueryRecipes
 *
 * DESCRIPTION:  
 *
 * PARAMETERS:   
 *
 * RETURNED:     (Note: must be freed by calling function!)
 *
 ***********************************************************************/
Boolean* QueryRecipes(UInt32 ingId) {
	UInt16 numRecipes = DmNumRecords(gRecipeDB);
	Boolean *results;
	UInt16 i;
	MemHandle recH;
	RecipeRecord *recP;
	UInt16 j;
	UInt16 numIngredients;

	results = MemPtrNew(numRecipes * sizeof(Boolean));
	
/*	if (!results) {
		return memErrNotEnoughSpace;
	}*/
//Must figure out error handling another way
	
	for (i = 0; i < numRecipes; i++) {
		results[i] = false;
		
		recH = DmQueryRecord(gRecipeDB, i);
		if (!recH) continue;
		recP = MemHandleLock(recH);
		
		numIngredients = sizeof(recP->ingredientIDs) / sizeof(UInt32);
		
		for (j = 0; j < numIngredients; j++) {
			if (recP->ingredientIDs[j] == ingId) results[i] = true;
		}
		
		MemHandleUnlock(recH); 
	}
	
	return results;

}



/***********************************************************************
 *
 * FUNCTION:     IngredientIDByName
 *
 * DESCRIPTION:  Returns database identifier of the ingredient with the
 *				 specified name, or creates a new entry
 *
 * PARAMETERS:   name of ingredient
 *
 * RETURNED:     IngredientDB ID of ingredient, or -1 if error
 *
 ***********************************************************************/
UInt32 IngredientIDByName(const Char *ingredientName)
{
    UInt16 numRecords = DmNumRecords(gIngredientDB);
    UInt16 i;
    MemHandle recH;
    Char *recP;
    UInt32 entryID = 0;
    UInt16 index;
    MemHandle newRecH;
    Char *newRecP;

    // Search existing records for a match
    for (i = 0; i < numRecords; i++) {
        recH = DmQueryRecord(gIngredientDB, i);
        if (recH) {
	        recP = MemHandleLock(recH);
	        if (StrCompare(recP, ingredientName) == 0) {
	            DmRecordInfo(gIngredientDB, i, NULL, &entryID, NULL);
	            MemHandleUnlock(recH);
	            return entryID;
	        }
        	MemHandleUnlock(recH);
        }
    }

    // No match found - create a new record
    index = DmFindSortPosition(gIngredientDB, (void *) ingredientName, 0, (DmComparF *) DBStringCompare, 0);
    newRecH = DmNewRecord(gIngredientDB, &index, StrLen(ingredientName) + 1);
    if (!newRecH) {
        return -1;
    }

    newRecP = MemHandleLock(newRecH);
    DmWrite(newRecP, 0, ingredientName, StrLen(ingredientName) + 1);
    MemHandleUnlock(newRecH);
    DmReleaseRecord(gIngredientDB, index, true);

    DmRecordInfo(gIngredientDB, index, NULL, &entryID, NULL);
    return entryID;
}

/***********************************************************************
 *
 * FUNCTION:     IngredientNameByID
 *
 * DESCRIPTION:  Gets ingredient name from DB identifier, writes to buffer
 *
 * PARAMETERS:   DB id, buffer to write to
 *
 * RETURNED:     Err
 *
 ***********************************************************************/
Err IngredientNameByID(UInt32 entryID, Char* buffer)
{
	UInt16 index;
	MemHandle rech;
	Char* recp;
	
	if (DmFindRecordByID(gIngredientDB, entryID, &index) == errNone) {
		rech = DmQueryRecord(gIngredientDB, index);
		if (rech) {
			recp = MemHandleLock(rech);
			StrCopy(buffer, recp);
			MemHandleUnlock(rech);
			return errNone;
		}
	}
	
	return dmErrCantFind;
}

/***********************************************************************
 *
 * FUNCTION:     UnitIDByName
 *
 * DESCRIPTION:  Returns database identifier of the unit with the
 *				 specified name, or creates a new entry
 *
 * PARAMETERS:   unit as string
 *
 * RETURNED:     UnitDB ID of ingredient, or -1 if error
 *
 ***********************************************************************/
UInt32 UnitIDByName(const Char *unitName)
{
    UInt16 numRecords = DmNumRecords(gUnitDB);
    UInt16 i;
    MemHandle recH;
    Char *recP;
    UInt32 entryID = 0;
    UInt16 index;
    MemHandle newRecH;
    Char *newRecP;

    // Search existing records for a match
    for (i = 0; i < numRecords; i++) {
        recH = DmQueryRecord(gUnitDB, i);
        if (recH) {
	        recP = MemHandleLock(recH);
	        if (StrCompare(recP, unitName) == 0) {
	            DmRecordInfo(gUnitDB, i, NULL, &entryID, NULL);
	            MemHandleUnlock(recH);
	            return entryID;
	        }
        	MemHandleUnlock(recH);
        }
    }

    // No match found - create a new record
    index = DmFindSortPosition(gRecipeDB, (void *) unitName, 0, (DmComparF *) DBStringCompare, 0);
    newRecH = DmNewRecord(gUnitDB, &index, StrLen(unitName) + 1);
    if (!newRecH) {
        return -1;
    }

    newRecP = MemHandleLock(newRecH);
    DmWrite(newRecP, 0, unitName, StrLen(unitName) + 1);
    MemHandleUnlock(newRecH);
    DmReleaseRecord(gUnitDB, index, true);

    DmRecordInfo(gUnitDB, index, NULL, &entryID, NULL);
    return entryID;
}

/***********************************************************************
 *
 * FUNCTION:     UnitNameByID
 *
 * DESCRIPTION:  Gets unit name from DB identifier, writes to buffer
 *
 * PARAMETERS:   DB id, buffer to write to
 *
 * RETURNED:     Err
 *
 ***********************************************************************/
Err UnitNameByID(UInt32 entryID, char* buffer)
{
	UInt16 index;
	MemHandle rech;
	Char* recp;
	
	if (DmFindRecordByID(gUnitDB, entryID, &index) == errNone) {
		rech = DmQueryRecord(gUnitDB, index);
		if (rech) {
			recp = MemHandleLock(rech);
			StrCopy(buffer, recp);
			MemHandleUnlock(rech);
			return errNone;
		}
	}
	
	return dmErrCantFind;
}
