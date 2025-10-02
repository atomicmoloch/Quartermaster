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
 * DESCRIPTION:  For DmFindSortPosition - compares recipe names 
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
    return StrCompare(((RecipeRecord *)rec1)->name, ((RecipeRecord *)rec2)->name);
}

/***********************************************************************
 *
 * FUNCTION:     DBStringCompare
 *
 * DESCRIPTION:  For DmFindSortPosition - compares database entries as strings
 *
 * PARAMETERS:   two Char* pointers
 *
 * RETURNED:     0 if strings match, positive number if rec1 sorts after
 *				 rec2 alphabetically, negative number if the reverse
 *
 ***********************************************************************/
static Int16 DBStringCompare(void *rec1, void *rec2, Int16 other,
                        SortRecordInfoPtr rec1SortInfo,
                        SortRecordInfoPtr rec2SortInfo,
                        MemHandle appInfoH)
{
    return StrCompare((Char *)rec1, (Char *)rec2);
}

/***********************************************************************
 *
 * FUNCTION:     FindIfUsed
 *
 * DESCRIPTION:  Checks if an ingredient/unit is used in any recipes
 *				 (or if it can be removed)
 *
 * PARAMETERS:   bitflag (0 = ingredient id, 1 = unit id), item ID
 *				 (bitflag identifies what the item ID is a reference to)
 *
 * RETURNED:     true if item appears in any recipe entry, false otherwise
 *
 ***********************************************************************/
Boolean FindIfUsed(UInt8 dbase, UInt32 itemId) {
	UInt16 dbaseSize = DmNumRecords(gRecipeDB);
	UInt16 listSize;
	MemHandle recH;
	RecipeRecord *recP;
	UInt16 i;
	UInt16 j;
	
	for (i = 0; i < dbaseSize; i++) {
		recH = DmQueryRecord(gRecipeDB, i);
		if (!recH) continue;
		recP = MemHandleLock(recH);
		
		if (dbase == 0) {
			listSize = sizeof(recP->ingredientIDs) / sizeof(UInt32);
			for (j = 0; j < listSize; j++) {
				if (recP->ingredientIDs[j] == itemId) {
					return true;
				}
			}
		}
		else if (dbase == 1) {
			listSize =  sizeof(recP->ingredientUnits) / sizeof(UInt32);
			for (j = 0; j < listSize; j++) {
				if (recP->ingredientUnits[j] == itemId) {
					return true;
				}
			}
		}
		MemHandleUnlock(recH); 
	}
	
	return false;
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
    const UInt8 counts[],
    const UInt8 fracs[],
    const UInt8 denoms[],
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
        recipe.ingredientFracs[i] = fracs[i];
        recipe.ingredientDenoms[i] = denoms[i];

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
MemHandle QueryRecipes(UInt32 ingId) {
	Boolean *results;
	MemHandle ret;
	UInt16 numIngredients;
	UInt16 numRecipes = DmNumRecords(gRecipeDB);
	MemHandle recH;
	RecipeRecord *recP;
	UInt16 i;
	UInt16 j;
	
	ret = MemHandleNew(numRecipes * sizeof(Boolean));
	results = MemHandleLock(ret);
	
	if (!results) {
		displayError(memErrNotEnoughSpace);
		return NULL;
	} //Could use improvement
	
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
	
	MemHandleLock(ret);
	return ret;
}


/***********************************************************************
 *
 * FUNCTION:     RemoveRecipe
 *
 * DESCRIPTION:  Removes recipe from database, and removes any ingredients/units
 *				 from respective databases that only appear in deleted recipe
 *
 * PARAMETERS:   Index of recipe to remove
 *
 * RETURNED:     errNone or error
 *
 ***********************************************************************/
Err RemoveRecipe(UInt16 recipeIndex) {
	MemHandle recH;
	RecipeRecord* recP;
	Err err;
	UInt16 listSize;
	UInt16 index;
	UInt16 j;
	
	// Removes recipe from database but gets MemHandle to data
	err = DmDetachResource(gRecipeDB, recipeIndex, &recH); 
	if (!(err == errNone)) return err;

	recP = MemHandleLock(recH);
	
	listSize = sizeof(recP->ingredientIDs) / sizeof(UInt32);
		
	for (j = 0; j < listSize; j++) {
		if (!(FindIfUsed(0, recP->ingredientIDs[j]))) {
			err = DmFindRecordByID(gIngredientDB, recP->ingredientIDs[j], &index);
			if (!(err == errNone)) return err;
			DmDeleteRecord(gIngredientDB, index);
		} 
	}
		
	listSize = sizeof(recP->ingredientUnits) / sizeof(UInt32);
		
	for (j = 0; j < listSize; j++) {
		if (!(FindIfUsed(1, recP->ingredientUnits[j]))) {
			err = DmFindRecordByID(gUnitDB, recP->ingredientUnits[j], &index);
			if (!(err == errNone)) return err;
			DmDeleteRecord(gUnitDB, index);
		} 
	}

	MemHandleFree(recH);
	return errNone;
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
    UInt32 entryID = 0;
    MemHandle recH;
    Char *recP;
    MemHandle newrecH;
    Char *newrecP;
    UInt16 index;
    UInt16 i;

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
    newrecH = DmNewRecord(gIngredientDB, &index, StrLen(ingredientName) + 1);
    if (!newrecH) {
        return -1;
    }

    newrecP = MemHandleLock(newrecH);
    DmWrite(newrecP, 0, ingredientName, StrLen(ingredientName) + 1);
    MemHandleUnlock(newrecH);
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
	MemHandle recH;
	Char* recP;
	UInt16 index;
	
	if (DmFindRecordByID(gIngredientDB, entryID, &index) == errNone) {
		recH = DmQueryRecord(gIngredientDB, index);
		if (recH) {
			recP = MemHandleLock(recH);
			StrCopy(buffer, recP);
			MemHandleUnlock(recH);
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
    UInt32 entryID = 0;
    MemHandle recH;
    Char *recP;
    MemHandle newrecH;
    Char *newrecP;
    UInt16 index;
    UInt16 i;

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
    newrecH = DmNewRecord(gUnitDB, &index, StrLen(unitName) + 1);
    if (!newrecH) {
        return -1;
    }

    newrecP = MemHandleLock(newrecH);
    DmWrite(newrecP, 0, unitName, StrLen(unitName) + 1);
    MemHandleUnlock(newrecH);
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
	MemHandle recH;
	Char* recP;
	UInt16 index;
	
	if (DmFindRecordByID(gUnitDB, entryID, &index) == errNone) {
		recH = DmQueryRecord(gUnitDB, index);
		if (recH) {
			recP = MemHandleLock(recH);
			StrCopy(buffer, recP);
			MemHandleUnlock(recH);
			return errNone;
		}
	}
	
	return dmErrCantFind;
}
