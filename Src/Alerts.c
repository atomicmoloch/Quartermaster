#include <PalmOS.h>
#include "Quartermaster.h"
#include "Quartermaster_Rsc.h"


/***********************************************************************
 *
 * FUNCTION:     displayError
 *
 * DESCRIPTION:  Displays non-fatal error message
 *
 * PARAMETERS:   Error code
 *
 * RETURNED:     nothing
 *
 ***********************************************************************/
void displayError(Err code) {
	Char buf[64];
	SysErrString(code, buf, 64);
	FrmCustomAlert(ErrorAlert, buf, NULL, NULL);
}

/***********************************************************************
 *
 * FUNCTION:     displayErrorIf
 *
 * DESCRIPTION:  Displays non-fatal error message if error is present
 *
 * PARAMETERS:   Error code
 *
 * RETURNED:     nothing
 *
 ***********************************************************************/
void displayErrorIf(Err code) {
	if (code != errNone) displayError(code);
}
 	
/***********************************************************************
 *
 * FUNCTION:     displayFatalError
 *
 * DESCRIPTION:  Displays fatal error message and exits application
 *
 * PARAMETERS:   Error code
 *
 * RETURNED:     nothing
 *
 ***********************************************************************/
void displayFatalError(Err code) {
	Char buf[64];
	SysErrString(code, buf, 64);
	FrmCustomAlert(ErrorAlert, buf, "Fatal ", NULL);
	AppStop();
	AppLaunchWithCommand(sysFileCDefaultApp, sysAppLaunchCmdNormalLaunch, NULL);
}

/***********************************************************************
 *
 * FUNCTION:     displayCustomError
 *
 * DESCRIPTION:  Displays non-fatal error message with custom message
 *
 * PARAMETERS:   Error code
 *
 * RETURNED:     nothing
 *
 ***********************************************************************/
void displayCustomError(UInt8 code) {
	Char buf[64];
	
	switch(code) {
		case 10:
			StrCopy(buf, "recipe name cannot be blank");
			break;
			
		case 11:
			StrCopy(buf, "max number of ingredients reached");
			break;
			
		case 12:
			StrCopy(buf, "silly billy, your ingredient must have a name");
			break;
			
		case 30:
			StrCopy(buf, "no recipes match search criteria");
			break;
			
		default:
			StrCopy(buf, "[no dialogue specified]");
			break;
	
	}

	FrmCustomAlert(ErrorAlert, buf, NULL, NULL);
}

/***********************************************************************
 *
 * FUNCTION:     confirmChoice
 *
 * DESCRIPTION:  Displays confirmation dialogue and returns user response
 *
 * PARAMETERS:   choice of confirmation dialogue string
 *
 * RETURNED:     true if choice confirmed, false if cancelled
 *
 ***********************************************************************/
Boolean confirmChoice(UInt8 dialogC) {
	Char buf[64];
	UInt16 userChoice;
	
	switch(dialogC) {
		case 0:
			StrCopy(buf, "delete recipe");
			break;
			
		case 1:
			StrCopy(buf, "delete ingredient");
			break;
			
		case 10:
			StrCopy(buf, "cancel and exit");
			break;
			
		default:
			StrCopy(buf, "[error: no dialogue specified]");
			break;
	
	}
	userChoice = FrmCustomAlert(ConfirmationAlert, buf, NULL, NULL);
	
	return (userChoice == 0);
}