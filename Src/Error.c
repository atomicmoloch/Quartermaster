#include <PalmOS.h>
#include "Quartermaster.h"
#include "Quartermaster_Rsc.h"



void displayError(Err code) {
	Char buf[64];
	
	SysErrString(code, buf, 64);
	FrmCustomAlert(ErrorAlert, buf, NULL, NULL);
}
 	
void displayFatalError(Err code) {
	Char buf[64];
	
	SysErrString(code, buf, 64);
	FrmCustomAlert(ErrorAlert, buf, "Fatal ", NULL);
	
	AppLaunchWithCommand(
		sysFileCDefaultApp, 
		sysAppLaunchCmdNormalLaunch, NULL);
}