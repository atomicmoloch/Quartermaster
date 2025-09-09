#include <PalmOS.h>
#include "Quartermaster.h"
#include "Quartermaster_Rsc.h"

/*********************************************************************
 * Internal Variables
 *********************************************************************/

//static DmOpenRef gPantryDB = NULL;

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
        dbID = DmCreateDatabase(0, databaseRecipeName, databaseCreatorID, 'Data', false);
        if (!dbID) return dmErrCantOpen;
    }
    gRecipeDB = DmOpenDatabase(0, dbID, dmModeReadWrite);

    dbID = DmFindDatabase(0, databaseIngredientName);
    if (!dbID) {
        dbID = DmCreateDatabase(0, databaseIngredientName, databaseCreatorID, 'Data', false);
        if (!dbID) return dmErrCantOpen;
    }
    gIngredientDB = DmOpenDatabase(0, dbID, dmModeReadWrite);
    
    dbID = DmFindDatabase(0, databaseUnitName);
    if (!dbID) {
        dbID = DmCreateDatabase(0, databaseUnitName, databaseCreatorID, 'Data', false);
        if (!dbID) return dmErrCantOpen;
    }
    gUnitDB = DmOpenDatabase(0, dbID, dmModeReadWrite);

    return errNone;
}


/***********************************************************************
 *
 * FUNCTION:     DatabaseOpen
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
 * PARAMETERS:   Pointer to recipe object
 *
 * RETURNED:     
 *
 ***********************************************************************/

Boolean AddRecipe(
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
    Char *stepsP;
	
	StrNCopy(recipe.name, recipeName, sizeof(recipe.name)-1);
	
	//Keeps database sorted alphabetically
	recordIndex = DmFindSortPosition(gRecipeDB, &recipe, 0, (DmComparF *) CompareRecipeNames, 0);
	
	recH = DmNewRecord(gRecipeDB, &recordIndex, totalSize);
	recP = MemHandleLock(recH);
	
	StrNCopy(recP->name, recipeName, sizeof(recP->name)-1);
    for (i = 0; i < numIngredients; i++) {
        recP->ingredientCounts[i] = counts[i];

        recP->ingredientIDs[i] = IngredientIDByName(ingredientNames[i]);
        if (recP->ingredientIDs[i] == -1) return false; // failed to add/lookup ingredient

        recP->ingredientUnits[i] = UnitIDByName(unitNames[i]);
        if (recP->ingredientUnits[i] == -1) return false; // failed to add/lookup unit
    }
    
	stepsP = (Char *)recP + sizeof(RecipeRecord);
	StrCopy(stepsP, recipeSteps);
	
	MemHandleUnlock(recH);
	DmReleaseRecord(gRecipeDB, recordIndex, true);

	return true;
}



/***********************************************************************
 *
 * FUNCTION:     
 *
 * DESCRIPTION:  
 *
 * PARAMETERS:   
 *
 * RETURNED:     
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
	            // Found a match: get its unique ID
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
        return -1; // out of memory or another error
    }

    newRecP = MemHandleLock(newRecH);
    StrCopy(newRecP, ingredientName);
    MemHandleUnlock(newRecH);
    DmReleaseRecord(gIngredientDB, index, true);

    // Get the unique ID of the new record
    DmRecordInfo(gIngredientDB, index, NULL, &entryID, NULL);
    return entryID;
}

Boolean IngredientNameByID(UInt32 entryID, char* buffer)
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
			return true;
		}
	}
	
	return false;
}

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
	            // Found a match: get its unique ID
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
        return -1; // out of memory or another error
    }

    newRecP = MemHandleLock(newRecH);
    StrCopy(newRecP, unitName);
    MemHandleUnlock(newRecH);
    DmReleaseRecord(gUnitDB, index, true);

    // Get the unique ID of the new record
    DmRecordInfo(gUnitDB, index, NULL, &entryID, NULL);
    return entryID;
}


Boolean UnitNameByID(UInt32 entryID, char* buffer)
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
			return true;
		}
	}
	
	return false;
}
