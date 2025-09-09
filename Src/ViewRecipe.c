#include <PalmOS.h>
#include "Quartermaster.h"
#include "Quartermaster_Rsc.h"


typedef struct {
    MemHandle recipe;  // pointer to the recipe to display
    Int16 scrollPos;       // vertical scroll position in pixels
    Int16 contentHeight;   // total height of rendered content
} RecipeFormContext;

static RecipeFormContext ctx;

static void DrawRecipe(FormType *form)
{
    RectangleType r;
    UInt16 y;
    Char buf[64];
    Char namebuf[32];
    Char unitbuf[16];
    RecipeRecord *recipeP = MemHandleLock(ctx.recipe);
    UInt16 i;
    Char *lineStart;
    
    // Get drawing rectangle
    FrmGetObjectBounds(form, FrmGetObjectIndex(form, ViewRecipeScrollbar), &r);
    r.extent.x = 160 - r.extent.x; // leave space for scrollbar - todo remove hardcoded dimensions

    WinEraseRectangle(&r, 0);

    y = 0 - ctx.scrollPos; // start at top minus scroll offset

    // Draw recipe name (bold)
    FntSetFont(boldFont);
    WinDrawChars(recipeP->name, StrLen(recipeP->name), 0, y);
    y += FntLineHeight() + 2;

    // Draw ingredients
    FntSetFont(stdFont);
    for (i = 0; i < recipeMaxIngredients && recipeP->ingredientIDs[i] != 0; i++) {
        // Format: "count unit ingredient"
        IngredientNameByID(recipeP->ingredientIDs[i], namebuf);
        UnitNameByID(recipeP->ingredientUnits[i], unitbuf);
        StrPrintF(buf, "%u %s %s", 
                recipeP->ingredientCounts[i],
                unitbuf,
                namebuf);
        WinDrawChars(buf, StrLen(buf), 0, y);
        y += FntLineHeight();
    }
    y += 4;

    // Draw steps (multi-line)
    lineStart = (Char*)(recipeP + sizeof(RecipeRecord));
    while (*lineStart) {
        Char *lineEnd = lineStart;
        Int16 len = 0;
        while (*lineEnd && len < 50 && *lineEnd != '\n') { lineEnd++; len++; }

        WinDrawChars(lineStart, len, 0, y);
        y += FntLineHeight();
        if (*lineEnd == '\n') lineEnd++;
        lineStart = lineEnd;
    }

    ctx.contentHeight = y; // total content height for scrollbar
    MemHandleUnlock(ctx.recipe);
}

Boolean ViewRecipeHandleEvent(EventPtr eventP) {
	Boolean handled = false;
	FormPtr frmP = FrmGetActiveForm();
	RectangleType r;
	Coord displayHeight;
	Int16 maxScroll = 0;

	FrmGetFormBounds(frmP, &r);
	displayHeight = r.extent.y;
	if (ctx.contentHeight > displayHeight) {
		maxScroll = ctx.contentHeight - displayHeight;
	}

	switch (eventP->eType) {
		case frmOpenEvent:
			FrmDrawForm (frmP);
			DrawRecipe(frmP);
            SclSetScrollBar(FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, ViewRecipeScrollbar)),
                            0, maxScroll, 0, displayHeight/FntLineHeight());
			handled = true;
			break;
        case sclRepeatEvent:
            ctx.scrollPos += eventP->data.sclRepeat.newValue - eventP->data.sclRepeat.value;
            if (ctx.scrollPos < 0) ctx.scrollPos = 0;
            if (ctx.scrollPos > maxScroll) ctx.scrollPos = maxScroll;
            FrmUpdateForm(formViewRecipe, 0);
            return true;
        case frmUpdateEvent:
            DrawRecipe(frmP);
            return true;
		default:		
			break;
	}
	return handled;
}

void OpenRecipeForm(MemHandle recipe) {
    ctx.recipe = recipe;
    ctx.scrollPos = 0;
    ctx.contentHeight = 0;
    FrmGotoForm(formViewRecipe);
}