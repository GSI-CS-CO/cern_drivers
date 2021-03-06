/***************************************************************************/
/* Tg8 Device driver.                                                      */
/* November 1993. CERN/PS Version. Julian Lewis.			   */
/* Vladimir Kovaltsov for ROCS Project, February, 1997			   */
/***************************************************************************/

#include <dldd.h>

#include <conf.h>
#include <kernel.h>
#include <file.h>
#include <errno.h>
#include <io.h>
#include <ioctl.h>
#include <time.h>
#include <absolute.h>
#include <mem.h>
#include <file.h>

#include "local.c"	/* Local var/proc. */
#include "low.c"	/* Low level procedures (dpram access) */

#include "isr.c"	/* Interrupt Service Routine codes */
#include "opencl.c"     /* Open/Close functions */
#include "rdwr.c"       /* Select/Read/Write functions */
#include "ioctl.c"      /* Ioctl functions */

/***************************************************************************/
/* INSTALL                                                                 */
/***************************************************************************/

char * Tg8DrvrInstall(info)
Tg8DrvrInfoTable * info; {     /* Driver info table */
int i,j; Tg8DrvrModuleAddress moad;

   /* Dont allow more than one single call to install. */

   if (wa) {
      pseterr(EEXIST);          /* File exists */
      return((char *) SYSERR);
   };

   /* Allocate the driver working area. */

   wa = (Tg8DrvrWorkingArea *) sysbrk(sizeof(Tg8DrvrWorkingArea));
   if (!wa) {
      cprintf("Tg8DrvrInstall: NOT ENOUGH MEMORY(WorkingArea)\n");
      pseterr(ENOMEM);          /* Not enough core */
      return((char *) SYSERR);
   };

   /****************************************/
   /* Initialize the driver's working area */
   /****************************************/

   bzero((void *) wa,sizeof(Tg8DrvrWorkingArea));           /* Clear working area */

   wa->InfoTable = *info;
   wa->FirmwareTimeout = Tg8DrvrFIRMWARE_TIMEOUT;
   wa->DebugOn = 0;

   /* Allocate and initialize a module context for each non null module. */

   for (i=j=0; i<16 && j<Tg8DrvrMODULES; i++) {
     moad.VMEAddress = (Tg8Dpm*) (info->Address + i*info->Increment);	/* Base address */
     moad.InterruptVector = 0;	         /* Interrupt vector number */
     moad.InterruptLevel  = info->Level; /* Interrupt level (2 usually) */
     moad.SwitchSettings  = -1;	         /* Switch settings */
     moad.SscHeader = info->SscHeader;   /* SSC Header code */
     if (AddModule(j,&moad,1) == OK) j++;
   };

   return((char*)wa);
} /* <<Tg8DrvrInstall>> */

/***************************************************************************/
/* UNINSTALL                                                               */
/***************************************************************************/

Tg8DrvrUninstall(wa)
Tg8DrvrWorkingArea * wa; {      /* Drivers working area pointer */

int i;                          /* Loop index */
Tg8DrvrModuleContext  * mcon;   /* Module context */
Tg8DrvrModuleAddress  * moad;   /* Module address descriptor */

   if (!wa) return OK;

   for (i=0; i<Tg8DrvrMODULES; i++) {

      mcon = wa->ModuleContexts[i];
      if (!mcon) continue;

      /* Reset the module ignoring bus interrupts */

      if (recoset()) {
	 noreco();
	 continue;                 /* Ignore bus interrupts */
      };
      ResetModule(mcon);           /* Reset the module */
      noreco();                    /* Remove bus interrupt trap */

      /* Clear interrupt handler */

      moad = &mcon->ModuleAddress;
      iointclr((int) ((moad->InterruptVector) << 2));
   };

   FreeWorkingArea();
   return OK;

} /* <<Tg8DrvrUninstall>> */


/*************************************************************/
/* Dynamic loading information for driver install routine.   */
/*************************************************************/

struct dldd entry_points = {
   Tg8DrvrOpen,
   Tg8DrvrClose,
   Tg8DrvrRead,
   Tg8DrvrWrite,
   Tg8DrvrSelect,
   Tg8DrvrIoctl,
   Tg8DrvrInstall,
   Tg8DrvrUninstall
};

/* eof drvr.c */
