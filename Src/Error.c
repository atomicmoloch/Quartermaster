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