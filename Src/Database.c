#include <PalmOS.h>
#include "Quartermaster.h"
#include "Quartermaster_Rsc.h"

/*********************************************************************
 * Internal Structures
 *********************************************************************/

typedef struct {
    Char name[32];
    UInt8 numIngredients;
} RecipeHeader;
//A minimal header for recipe records

/*********************************************************************
 * External Variables
 *********************************************************************/

DmOpenRef gRecipeDB;
DmOpenRef gIngredientDB;
DmOpenRef gUnitDB;
DmOpenRef gPantryDB;
DmOpenRef gGroceryDB;

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
   return StrCompare(((RecipeHeader *)rec1)->name, ((RecipeHeader *)rec2)->name);
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
    // Formerly used TxtCaselessCompare - removed for compatibility
}


/***********************************************************************
 *
 * FUNCTION:     DBIngredientIDCompare
 *
 * DESCRIPTION:  For DmFindSortPosition - assumes rec1 and rec2 are IDs of database
 *				 entries which consist of strings
 *
 * PARAMETERS:   two UInt32 pointers
 *
 * RETURNED:     0 if parameters match, positive number if rec1 sorts after
 *				 rec2 alphabetically, negative number if the reverse
 *
 ***********************************************************************/
static Int16 DBIngredientIDCompare(void *rec1, void *rec2, Int16 other,
                        SortRecordInfoPtr rec1SortInfo,
                        SortRecordInfoPtr rec2SortInfo,
                        MemHandle appInfoH)
{
    UInt32 id1, id2;
    UInt16 index1, index2;
    MemHandle recH1 = NULL, recH2 = NULL;
    Char *str1 = NULL, *str2 = NULL;
    Int16 result = 0;


    if (!rec1 || !rec2)
        return 0;

    id1 = *(UInt32 *)rec1;
    id2 = *(UInt32 *)rec2;

    if (DmFindRecordByID(gIngredientDB, id1, &index1) != errNone)
        return 0;
    if (DmFindRecordByID(gIngredientDB, id2, &index2) != errNone)
        return 0;

    recH1 = DmQueryRecord(gIngredientDB, index1);
    recH2 = DmQueryRecord(gIngredientDB, index2);
    if (!recH1 || !recH2)
        return 0;

    str1 = (Char *)MemHandleLock(recH1);
    str2 = (Char *)MemHandleLock(recH2);

	result = StrCompare(str1, str2);

    MemHandleUnlock(recH1);
    MemHandleUnlock(recH2);

    return result;
}

/***********************************************************************
 *
 * FUNCTION:     DBIntCompare
 *
 * DESCRIPTION:  For DmFindSortPosition - compares database entries as integers
 *
 * PARAMETERS:   two UInt32 pointers
 *
 * RETURNED:     0 if parameters match, positive number if rec1 sorts after
 *				 rec2 alphabetically, negative number if the reverse
 *
 ***********************************************************************/
/* static Int16 DBIntCompare(void *rec1, void *rec2, Int16 other,
                        SortRecordInfoPtr rec1SortInfo,
                        SortRecordInfoPtr rec2SortInfo,
                        MemHandle appInfoH)
{
    if (*(UInt32 *)rec1 < *(UInt32 *)rec2)
        return -1;
    else if (*(UInt32 *)rec1 > *(UInt32 *)rec2)
        return 1;
    else
        return 0;
} */

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
static Boolean FindIfUsed(UInt8 dbase, UInt32 itemId) {
	UInt16 dbaseSize = DmNumRecords(gRecipeDB);
	MemHandle recH;
	MemPtr *recP;
	RecipeRecord recipe;
	UInt16 i;
	UInt16 j;
	
	for (i = 0; i < dbaseSize; i++) {
		recH = DmQueryRecord(gRecipeDB, i);
		if (!recH) continue;
		recP = MemHandleLock(recH);
		recipe = RecipeGetRecord(recP);
		
		if (dbase == 0) {
			for (j = 0; j < recipeMaxIngredients && recipe.ingredientIDs[j] != 0; j++) {
				if (recipe.ingredientIDs[j] == itemId) {
					MemHandleUnlock(recH); 
					return true;
				}
			}
		}
		else if (dbase == 1) {
			for (j = 0; j < recipeMaxIngredients && recipe.ingredientUnits[j] != 0; j++) {
				if (recipe.ingredientUnits[j] == itemId) {
					MemHandleUnlock(recH); 
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
 
 /*********************************************************************
 * General DB Functions
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
    
    dbID = DmFindDatabase(0, databasePantryName);
    if (!dbID) {
        DmCreateDatabase(0, databasePantryName, databaseCreatorID, 'Data', false);
        dbID = DmFindDatabase(0, databasePantryName);
        if (!dbID) return dmErrCantOpen;
    }
    gPantryDB = DmOpenDatabase(0, dbID, dmModeReadWrite);
    if (!gPantryDB) return DmGetLastErr();
    
    dbID = DmFindDatabase(0, databaseGroceryName);
    if (!dbID) {
        DmCreateDatabase(0, databaseGroceryName, databaseCreatorID, 'Data', false);
        dbID = DmFindDatabase(0, databaseGroceryName);
        if (!dbID) return dmErrCantOpen;
    }
    gGroceryDB = DmOpenDatabase(0, dbID, dmModeReadWrite);
    if (!gGroceryDB) return DmGetLastErr();

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
    if (gRecipeDB)     DmCloseDatabase(gRecipeDB);
    if (gIngredientDB) DmCloseDatabase(gIngredientDB);
    if (gUnitDB)       DmCloseDatabase(gUnitDB);
    if (gPantryDB)	   DmCloseDatabase(gPantryDB);
    if (gGroceryDB)    DmCloseDatabase(gGroceryDB);
}

/***********************************************************************
 *
 * FUNCTION:     IDFromIndex
 *
 * DESCRIPTION:  Utility function to convert between index and entry ID
 *
 * PARAMETERS:   database, index
 *
 * RETURNED:     id (or 0 if index is invalid)
 *
 ***********************************************************************/
UInt32 IDFromIndex(DmOpenRef dbase, UInt16 index)
{
    UInt32 id = 0;
    Err err;
    err = DmRecordInfo(dbase, index, NULL, &id, NULL);

    if (err != errNone)
        return 0;

    return id;
}

/***********************************************************************
 *
 * FUNCTION:     IndexFromID
 *
 * DESCRIPTION:  Utility function to convert between index and entry ID
 *
 * PARAMETERS:   database, id
 *
 * RETURNED:     index (or 0xFFFF if index is invalid)
 *
 ***********************************************************************/
UInt16 IndexFromID(DmOpenRef dbase, UInt32 id)
{
    UInt16 index = 0xFFFF;
    Err err;
    err = DmFindRecordByID(dbase, id, &index);

    if (err != errNone)
        return 0xFFFF;

    return index;
}

/***********************************************************************
 *
 * FUNCTION:     AddIdToDatabase
 *
 * DESCRIPTION:  Checks if an identical entry already exists.
 *				 If not, creates new database entry consisting of id
 *
 * PARAMETERS:   database, id
 *
 * RETURNED:     errNone if either entry is found or entry creation succeeds
 *
 ***********************************************************************/
Err AddIdToDatabase(DmOpenRef dbase, UInt32 id)
{
    UInt16 index;
    Err err;
    MemHandle recH;
    UInt32 *recP;

    if (!dbase)
        return dmErrInvalidParam;

    index = DmFindSortPosition(dbase, &id, 0, (DmComparF *) DBIngredientIDCompare, 0);

	if (index > 0) {
		recH = DmQueryRecord(dbase, index - 1);
		recP = MemHandleLock(recH);
		
		if (*recP == id) {
			MemHandleUnlock(recH);
			return errNone;
			//id already in database, 
		}
		MemHandleUnlock(recH);
		recP = NULL, recH = NULL;
	}

    recH = DmNewRecord(dbase, &index, sizeof(UInt32));
    if (!recH)
        return dmErrMemError;
	recP = MemHandleLock(recH);
	DmWrite(recP, 0, &id, sizeof(UInt32));
    MemHandleUnlock(recH);

    err = DmReleaseRecord(dbase, index, true);

    return err;
}

/***********************************************************************
 *
 * FUNCTION:     EntryInDatabase
 *
 * DESCRIPTION:  Checks if an entry already exists.
 *
 * PARAMETERS:   database, entry (id)
 *
 * RETURNED:     boolean
 *
 ***********************************************************************/
Boolean EntryInDatabase(DmOpenRef dbase, UInt32 id)
{
    UInt16 index;
    MemHandle recH;
    UInt32 *recP;

    if (!dbase)
        return false;

    index = DmFindSortPosition(dbase, &id, 0, (DmComparF *) DBIngredientIDCompare, 0);

	if (index > 0) {
		recH = DmQueryRecord(dbase, index - 1);
		recP = MemHandleLock(recH);
		
		if (*recP == id) {
			MemHandleUnlock(recH);
			return true; 
		}
		MemHandleUnlock(recH);
	}

	return false;
}

/***********************************************************************
 *
 * FUNCTION:     IndexOfEntry
 *
 * DESCRIPTION:  IDInDatabase but it returns the index instead of a boolean
 *
 * PARAMETERS:   database, entry (id)
 *
 * RETURNED:     UInt16 index or 0xFFFF if not exists
 *
 ***********************************************************************/
UInt16 IndexOfEntry(DmOpenRef dbase, UInt32 id)
{
    UInt16 index;
    MemHandle recH;
    UInt32 *recP;

    if (!dbase)
        return false;

    index = DmFindSortPosition(dbase, &id, 0, (DmComparF *) DBIngredientIDCompare, 0);

	if (index > 0) {
		recH = DmQueryRecord(dbase, index - 1);
		recP = MemHandleLock(recH);
		
		if (*recP == id) {
			MemHandleUnlock(recH);
			return index - 1; 
		}
		MemHandleUnlock(recH);
	}

	return 0xFFFF;
}


/*********************************************************************
 * Recipe DB Functions
 *********************************************************************/

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
    RecipeHeader recipe;
	MemHandle recH;
	MemPtr recP;
	UInt16 stepsLen;
	UInt16 ingredientsLen;
	UInt32 totalSize;
    UInt32 ingredientIDs[recipeMaxIngredients];
    UInt32 ingredientUnits[recipeMaxIngredients];
	UInt16 recordIndex;
    UInt16 offset = 0;
    UInt16 i;
    Err err;
    
    err = MemSet(&recipe, sizeof(recipe), 0);
    if (err != errNone) return err;
    
    // constructs recipe header
    stepsLen  = StrLen(recipeSteps) + 1;
    ingredientsLen = numIngredients * (3 * sizeof(UInt8) + 2 * sizeof(UInt32));
    totalSize = sizeof(RecipeHeader) + ingredientsLen + stepsLen;
	
	StrNCopy(recipe.name, recipeName, 31);
	recipe.name[31] = '\0';
	recipe.numIngredients = numIngredients;
	
	// builds arrays of ingredient IDs
    for (i = 0; i < numIngredients; i++) {
        ingredientIDs[i] = IngredientIDByName(ingredientNames[i]);
        if (ingredientIDs[i] == -1) return dmErrResourceNotFound; 

        ingredientUnits[i] = UnitIDByName(unitNames[i]);
        if (ingredientUnits[i] == -1) return dmErrResourceNotFound;
    }	
	
	// writes entry
	recordIndex = DmFindSortPosition(gRecipeDB, &recipe, 0, (DmComparF *) CompareRecipeNames, 0);
	
	recH = DmNewRecord(gRecipeDB, &recordIndex, totalSize);
	if (!recH) return dmErrMemError;
	recP = MemHandleLock(recH);    
	
	DmSet(recP, 0, totalSize, 0);
	DmWrite(recP, 0, &recipe, sizeof(recipe));         
    offset += sizeof(RecipeHeader);
	DmWrite(recP, offset, counts, numIngredients * sizeof(UInt8)); 
	offset += numIngredients * sizeof(UInt8);
	DmWrite(recP, offset, fracs, numIngredients * sizeof(UInt8)); 
	offset += numIngredients * sizeof(UInt8);
	DmWrite(recP, offset, denoms, numIngredients * sizeof(UInt8)); 
	offset += numIngredients * sizeof(UInt8);
	DmWrite(recP, offset, ingredientIDs, numIngredients * sizeof(UInt32));
	offset += numIngredients * sizeof(UInt32);
	DmWrite(recP, offset, ingredientUnits, numIngredients * sizeof(UInt32));
	offset += numIngredients * sizeof(UInt32);

	// It seems that DmWrite errors are always fatal, so code handling it wouldn't do anything
	// offset should now be equivalent to sizeof(RecipeHeader) + ingredientsLen - numIngredients * sizeof(UInt32)
	DmWrite(recP, offset, recipeSteps, stepsLen); 
	
	MemHandleUnlock(recH);
	err = DmReleaseRecord(gRecipeDB, recordIndex, true);
	
	return err;
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
	MemPtr recP;
	RecipeRecord recipe;
	Err err;
	UInt16 index;
	UInt16 i;
	
	// Removes recipe from database but gets MemHandle to data
	err = DmDetachRecord(gRecipeDB, recipeIndex, &recH); 
	if (!(err == errNone)) return err;

	recP = MemHandleLock(recH);
	recipe = RecipeGetRecord(recP);
	MemHandleUnlock(recH);
		
	for (i = 0; i < recipeMaxIngredients && recipe.ingredientIDs[i] != 0; i++) {
		RemoveIngredient(recipe.ingredientIDs[i]);
	}
		
	for (i = 0; i < recipeMaxIngredients && recipe.ingredientUnits[i] != 0; i++) {
		if (!(FindIfUsed(1, recipe.ingredientUnits[i]))) {
			err = DmFindRecordByID(gUnitDB, recipe.ingredientUnits[i], &index);
			if (err == errNone) DmRemoveRecord(gUnitDB, index);
		} 
	}
	
	MemHandleFree(recH);
	return errNone;
}

/***********************************************************************
 *
 * FUNCTION:     RecipeGetRecord
 *
 * DESCRIPTION:  Reconstructs recipe header from recipe database entry
 *
 * PARAMETERS:   MemPtr to gRecipeDB entry
 *
 * RETURNED:     RecipeRecord
 *
 ***********************************************************************/
RecipeRecord RecipeGetRecord(MemPtr recP)
{
	RecipeRecord recipe;
	RecipeHeader* header = recP;
	UInt8 *counts = (UInt8*)header + sizeof(RecipeHeader);
	UInt8 *fracs  = counts + header->numIngredients;
	UInt8 *denoms = fracs  + header->numIngredients;
    UInt8 *nameRaw = denoms + header->numIngredients; //Reads name and unit arrays as UInt8 arrays to prevent alignment issues
    UInt8 *unitRaw = nameRaw + header->numIngredients * sizeof(UInt32); 
	Err err;
	UInt8 i;
	
	err = MemSet(&recipe, sizeof(RecipeRecord), 0);
	StrNCopy(recipe.name, header->name, 31);
	recipe.name[31] = '\0';
	recipe.numIngredients = header->numIngredients;

    for (i = 0; i < header->numIngredients; i++) {
        recipe.ingredientCounts[i] = counts[i];
        recipe.ingredientFracs[i]  = fracs[i];
        recipe.ingredientDenoms[i] = denoms[i];
        
        recipe.ingredientIDs[i]=
        	((UInt32)nameRaw[i * 4]     << 24) |
        	((UInt32)nameRaw[i * 4 + 1] << 16) |
        	((UInt32)nameRaw[i * 4 + 2] << 8)  |
        	((UInt32)nameRaw[i * 4 + 3]);
        
    	recipe.ingredientUnits[i] =
        	((UInt32)unitRaw[i * 4]     << 24) |
        	((UInt32)unitRaw[i * 4 + 1] << 16) |
        	((UInt32)unitRaw[i * 4 + 2] << 8)  |
        	((UInt32)unitRaw[i * 4 + 3]);
    }

	return recipe;
}


/***********************************************************************
 *
 * FUNCTION:     RecipeGetStepsPtr
 *
 * DESCRIPTION:  Returns pointer to recipe steps
 *
 * PARAMETERS:   MemPtr to gRecipeDB entry
 *
 * RETURNED:     Pointer to steps portion of recipe entry
 *
 ***********************************************************************/
Char* RecipeGetStepsPtr(MemPtr recP)
{
	UInt16 ingredientsLen = ((RecipeHeader*)recP)->numIngredients * (3 * sizeof(UInt8) + 2 * sizeof(UInt32));
	return (Char*)recP + sizeof(RecipeHeader) + ingredientsLen;
}

/*********************************************************************
 * Ingredient DB Functions
 *********************************************************************/

/***********************************************************************
 *
 * FUNCTION:     RemoveIngredient
 *
 * DESCRIPTION:  Removes ingredient specified by id from ingredient DB,
 *				 pantry DB, and grocery list DB
 * 
 * PARAMETERS:   Ingredient ID
 *
 * RETURNED:     errNone if successful, err if ID can't be found or if
 *				 ingredient used 
 *
 ***********************************************************************/
Err RemoveIngredient(UInt32 ingId) {
	UInt16 index;
	Err err;

	if (!(FindIfUsed(0, ingId))) {
	
		index = IndexOfEntry(gPantryDB, ingId);
		if (index != 0xFFFF)
			err = DmRemoveRecord(gPantryDB, index);
			
		index = IndexOfEntry(gGroceryDB, ingId);
		if (index != 0xFFFF)
			err = DmRemoveRecord(gGroceryDB, index);
	
		// Note: ingredient must be deleted from pantry/grocery dbs first
		// because of how DbIngredientIDCompare works
	
		err = DmFindRecordByID(gIngredientDB, ingId, &index);
		if (err == errNone) 
			DmRemoveRecord(gIngredientDB, index);
		else
			return err;
			
		// If speed is a concern, switch this to using DmDeleteRecord and
		// manually pack later. But I anticipate deleting records on-device
		// to be rare

		return errNone;
	} else {
		return errIngredInUse;
	}
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
    UInt16 index;
    
    index = DmFindSortPosition(gIngredientDB, (void *) ingredientName, 0, (DmComparF *) DBStringCompare, 0);

    // Old code - linear search
 /*   for (i = 0; i < numRecords; i++) {
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
    } */
    
    if (index > 0) {
    	recH = DmQueryRecord(gIngredientDB, index - 1);
    	recP = MemHandleLock(recH);
	    if (StrCompare(recP, ingredientName) == 0) {
	    	DmRecordInfo(gIngredientDB, index - 1, NULL, &entryID, NULL);
	        MemHandleUnlock(recH);
	        return entryID;
	    }
        MemHandleUnlock(recH);
    }

	// If none found, creates new record
    recH = DmNewRecord(gIngredientDB, &index, StrLen(ingredientName) + 1);
    if (!recH) {
        return -1;
    }

    recP = MemHandleLock(recH);
    DmWrite(recP, 0, ingredientName, StrLen(ingredientName) + 1);
    MemHandleUnlock(recH);
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
Err IngredientNameByID(Char* buffer, UInt8 len, UInt32 entryID)
{
	MemHandle recH;
	Char* recP;
	UInt16 index;
	
	if (DmFindRecordByID(gIngredientDB, entryID, &index) == errNone) {
		recH = DmQueryRecord(gIngredientDB, index);
		if (recH) {
			recP = MemHandleLock(recH);
			StrNCopy(buffer, recP, len-1);
			buffer[len-1] = '\0';
			MemHandleUnlock(recH);
			return errNone;
		}
	}
	
	return dmErrCantFind;
}

/*********************************************************************
 * Unit DB Functions
 *********************************************************************/

/***********************************************************************
 *
 * FUNCTION:     UnitIDByName
 *
 * DESCRIPTION:  Returns database identifier of the unit with the
 *				 specified name, or creates a new entry
 *
 * PARAMETERS:   unit name string
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
    UInt16 index;
    
    index = DmFindSortPosition(gUnitDB, (void *) unitName, 0, (DmComparF *) DBStringCompare, 0);
    
    if (index > 0) {
    	recH = DmQueryRecord(gUnitDB, index - 1);
    	recP = MemHandleLock(recH);
	    if (StrCompare(recP, unitName) == 0) {
	    	DmRecordInfo(gUnitDB, index - 1, NULL, &entryID, NULL);
	        MemHandleUnlock(recH);
	        return entryID;
	    }
        MemHandleUnlock(recH);
        recP = NULL, recH = NULL;
    }

    // If no match found creates new record
    recH = DmNewRecord(gUnitDB, &index, StrLen(unitName) + 1);
    if (!recH) {
        return -1;
    }

    recP = MemHandleLock(recH);
    DmWrite(recP, 0, unitName, StrLen(unitName) + 1); //includes null terminator
    MemHandleUnlock(recH);
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
 * PARAMETERS:   buffer to write to, buffer length, entry ID
 *
 * RETURNED:     Err
 *
 ***********************************************************************/
Err UnitNameByID(Char* buffer, UInt8 len, UInt32 entryID)
{
	MemHandle recH;
	Char* recP;
	UInt16 index;
	
	if (DmFindRecordByID(gUnitDB, entryID, &index) == errNone) {
		recH = DmQueryRecord(gUnitDB, index);
		if (recH) {
			recP = MemHandleLock(recH);
			StrNCopy(buffer, recP, len-1);
			buffer[len-1] = '\0';
			MemHandleUnlock(recH);
			return errNone;
		}
	}
	
	return dmErrCantFind;
}

/*********************************************************************
 * Pantry DB Functions
 *********************************************************************/

/***********************************************************************
 *
 * FUNCTION:     PantryFuzzySearch
 *
 * DESCRIPTION:  Queries recipe database to find recipes that any ingredient
 *				 in the pantry
 *
 * PARAMETERS:   MemHandle pointer to store returned list of recipes
 *
 * RETURNED:     number of recipes that match
 *
 ***********************************************************************/
UInt16 PantryFuzzySearch(MemHandle* ret) {
	UInt16 numRecipes = DmNumRecords(gRecipeDB);
	UInt16* results;
	MemHandle recH;
	MemPtr recP;
	RecipeRecord recipe;
	UInt16 i;
	UInt16 j;
	UInt16 idx = 0;
	
	if (numRecipes == 0)
		return 0;
	
	*ret = MemHandleNew(numRecipes * sizeof(UInt16));
	results = MemHandleLock(*ret);
	
	if (!results) {
		displayError(memErrNotEnoughSpace);
		return 0;
	} //Could use improvement
	
	for (i = 0; i < numRecipes; i++) {
		recH = DmQueryRecord(gRecipeDB, i);
		if (!recH) continue;
		recP = MemHandleLock(recH);
		recipe = RecipeGetRecord(recP);
		MemHandleUnlock(recH); 
		
		for (j = 0; j < recipe.numIngredients; j++) {
			if (EntryInDatabase(gPantryDB, recipe.ingredientIDs[j])) { // checks in this manner because pantryDB has ingredients in sorted order
				results[idx] = i;
				idx++;
				break; // only affects innermost loop
			}
		}
	}
	
	
	MemHandleUnlock(*ret);
	MemHandleResize(*ret, idx * sizeof(UInt16));
	return idx;
}

/***********************************************************************
 *
 * FUNCTION:     PantryStrictSearch
 *
 * DESCRIPTION:  Queries recipe database to find recipes that can
 *				 be made with ingredients in pantry
 *
 * PARAMETERS:   MemHandle pointer to store returned list of recipes
 *
 * RETURNED:     number of recipes that match
 *
 ***********************************************************************/
UInt16 PantryStrictSearch(MemHandle* ret) {
	UInt16 numRecipes = DmNumRecords(gRecipeDB);
	UInt16* results;
	MemHandle recH;
	MemPtr recP;
	RecipeRecord recipe;
	Boolean CanBeMade;
	UInt16 i;
	UInt16 j;
	UInt16 idx = 0;
	
	if (numRecipes == 0)
		return 0;
	
	*ret = MemHandleNew(numRecipes * sizeof(UInt16));
	results = MemHandleLock(*ret);
	
	if (!results) {
		displayError(memErrNotEnoughSpace);
		return 0;
	} 
	
	for (i = 0; i < numRecipes; i++) {
		recH = DmQueryRecord(gRecipeDB, i);
		if (!recH) continue;
		recP = MemHandleLock(recH);
		recipe = RecipeGetRecord(recP);
		MemHandleUnlock(recH); 
		
		CanBeMade = true;
		
		for (j = 0; j < recipe.numIngredients; j++) {
			if (!EntryInDatabase(gPantryDB, recipe.ingredientIDs[j]))
				CanBeMade = false;
		}
		
		if (CanBeMade) {
			results[idx] = i;
			idx++;
		}
	}
	
	
	MemHandleUnlock(*ret);
	MemHandleResize(*ret, idx * sizeof(UInt16));
	return idx;
}