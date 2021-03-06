/***************************************************************************/
/* Tg8 Device driver - ioctl functions.                                    */
/* November 1993. CERN/PS Version. Julian Lewis.			   */
/* Vladimir Kovaltsov for the SL Version, February, 1997		   */
/***************************************************************************/

/***************************************************************************/
/* IOCTL                                                                   */
/***************************************************************************/

int Tg8DrvrIoctl(wa, flp, cm, arg)
Tg8DrvrWorkingArea   * wa;      /* Working area */
struct file	     * flp;     /* File pointer */
Tg8DrvrControlFunction cm;      /* IOCTL command */
Tg8IoBlock	     * arg; {	/* Data for the IOCTL */

int an,mnum,dnum,dm,cw; /* Action/Module/Device/DevMask/ControlWord numbers */
int *mt;                /* Devices Masks Table address (part of the user table) */
int i, j, k;            /* For loops */
int rcnt, wcnt;         /* Readable, Writable byte counts at arg address */
int ps;                 /* Processor status */
int msk;		/* Mask */

Uint row,cnt,rows;      /* Range specifiers, rows per a transaction */
int  oid, *oi;          /* Object's id; its pointer */

Tg8DrvrModuleContext     * mcon;        /* Module context */
Tg8DrvrDeviceContext     * dcon;        /* Device context */
Tg8DrvrQueue             * queue;       /* Events queue */
Tg8DrvrEvent             * conn;        /* Queue item */
Tg8Clock                 * f_clock;     /* Firmware clock table data */
Tg8History               * f_hist;      /* Firmware events history data */
Tg8Recording             * f_rec;       /* Firmware recording data */
Tg8PpmUser               * ppm;         /* Firmware/PPM User action description */
Tg8PpmLine               * ppml;        /* Firmware/PPM line description */
Tg8Object                * obj;         /* Timing object */
Tg8Gate                  * g;           /* Gate info */
Tg8User                  * f_adesc, *a; /* Firmware/User action description */
Tg8StatusBlock             f_hwst;      /* Firmware status */
Tg8DrvrUserTable         * tab;         /* User table */

   /* Check argument contains a valid address for reading or writing. */
   /* We can not allow bus errors to occur inside the driver due to   */
   /* the caller providing a garbage address in "arg". So if arg is   */
   /* not null set "rcnt" and "wcnt" to contain the byte counts which */
   /* can be read or written to without error. */

   if (arg) {
      rcnt = rbounds((unsigned long) arg);      /* Number of readable bytes without error */
      wcnt = wbounds((unsigned long) arg);      /* Number of writable bytes without error */
      if (rcnt < sizeof(int)) { /* We at least need to read one int */
	 pseterr(EINVAL);       /* Invalid argument */
	 return(SYSERR);
      };
   } else {
      rcnt = 0; wcnt = 0;       /* Null arg = zero read/write counts */
   };

   if (cm == Tg8DrvrINSTALL_MODULE) { /* Install a module. */
     CheckConfiguration();
     if (rcnt<sizeof(arg->InsModule)) {
       pseterr(EINVAL);       /* Invalid argument */
       return(SYSERR);
     };
     return AddModule(arg->InsModule.Index,
		     (Tg8DrvrModuleAddress*)&arg->InsModule.Address,1);
   };

   if (cm == Tg8DrvrGET_CONFIGURATION) { /* Get the current configuration. */
     CheckConfiguration();
     if (wcnt<sizeof(arg->GetConfig)) {
       pseterr(EINVAL);       /* Invalid argument */
       return(SYSERR);
     };
     for (i=0; i<Tg8DrvrMODULES; i++) {
       if (mcon = wa->ModuleContexts[i]) {
	 arg->GetConfig.DevMasks[i] = mcon->DevMask;
	 arg->GetConfig.Addresses[i] = *(Tg8ModuleAddress*)&mcon->ModuleAddress;
       } else
	 arg->GetConfig.DevMasks[i] = -1; /* No such module */
     };
     return (OK);
   };

   /**** Get module and device descriptors ****/

   dnum = minor(flp->dev) -1;
   mnum = dnum / Tg8DrvrDEVICES; /* Get the target module number */
   if (dnum < 0 || mnum >= Tg8DrvrMODULES) {
      pseterr(EBADF);           /* Bad file number */
      return(SYSERR);
   };

   /* We can't control a file which is not open. */

   mcon = wa->ModuleContexts[mnum];
   if (!mcon) {
      pseterr(ENODEV);          /* No such device */
      return(SYSERR);
   };

   dnum %= Tg8DrvrDEVICES;      /* Get the target device (e.g. client) number */
   dm = 1<<dnum;                /* Its bit mask */
   dcon = &mcon->Device[dnum];  /* Its definition */
   tab = &mcon->UserTable;      /* User table address */
   queue = &mcon->Queue;        /* Queue address */

   /*************************************/
   /* Decode callers command and do it. */
   /*************************************/

   switch (cm) {

      /*******************************/
      /* Switch driver debugging ON. */

      case Tg8DrvrDEBUG_ON:
	wa->DebugOn = 1;
      return(OK);

      /********************************/
      /* Switch driver debugging OFF. */

      case Tg8DrvrDEBUG_OFF:
	  wa->DebugOn = 0;
      return(OK);

      /********************************/
      /* Get the driver version date. */

      case Tg8DrvrGET_DRI_VERSION:
	if (wcnt<sizeof(arg->Ver)) break;
	strcpy(arg->Ver,version);
      return(OK);

      /*****************************/
      /* Get the firmware version. */

      case Tg8DrvrGET_FIRMWARE_VERSION:
	if (wcnt<sizeof(arg->Ver)) break;
	ReadCreationDate(mcon,arg->Ver);
      return(OK);

      /*********************************************/
      /* Set the timeout value for events waiting. */

      case Tg8DrvrSET_TIME_OUT:
	if (arg->TimeOut < 0) break;
	mcon->TimeOut = arg->TimeOut;
      return(OK);

      /****************************************/
      /* Set the owner's pid and signal code. */

      case Tg8DrvrSET_SIGNAL:
	if (rcnt<sizeof(arg->Signal)) break;
	dcon->Pid = arg->Signal.Pid;
	dcon->Signal = arg->Signal.Signal;
      return(OK);

      /****************************************/
      /* Get the owner's pid and signal code. */

      case Tg8DrvrGET_SIGNAL:
	if (wcnt<sizeof(arg->Signal)) break;
	arg->Signal.Pid = dcon->Pid;
	arg->Signal.Signal = dcon->Signal;
      return(OK);

      /****************************/
      /* Set the SSC Header code. */

      case Tg8DrvrSET_SSC_HEADER:
	mcon->ModuleAddress.SscHeader = arg->SscHeader;
	SetSscHeader(mcon,arg->SscHeader);
      return(OK);

      /*******************************************************/
      /* Enable the module synchronously with the SSC frame. */
      /*******************************************************/

      case Tg8DrvrENABLE_SYNC:
	EnableSync(mcon);
	return (OK);
      break;

      /*******************************************************/
      /* Disable the module synchronously with the SSC frame.*/
      /*******************************************************/

      case Tg8DrvrDISABLE_SYNC:
	DisableSync(mcon);
	return (OK);
      break;

      /***********************/
      /* Enable the  module. */

      case Tg8DrvrENABLE_MODULE:
	EnableModule(mcon);
	return (OK);
      break;

      /***********************/
      /* Disable the module. */

      case Tg8DrvrDISABLE_MODULE:
	DisableModule(mcon);
	return (OK);
      break;

      /**********************/
      /* Reset the module.  */

      case Tg8DrvrRESET_MODULE:
	ResetModule(mcon);
	ReloadUserTable(mcon);
	/* Examine the current module's status */
	CheckDriverStatus(mcon);
      return (OK);

      /****************************/
      /* Simulate a set of pulses.*/

      case Tg8DrvrSIMULATE_PULSE:
	if (mcon->Status & Tg8DrvrMODULE_ENABLED) {
	  pseterr(EPERM);       /* Operation not permitted */
	  return(SYSERR);
	};
	if (SimulatePulse(mcon,arg->SimPulseMask) != Tg8ERR_OK) {
	  pseterr(EIO);       /* IO error of some description */
	  return(SYSERR);
	};
      return (OK);

      /*************************/
      /* Get a raw Tg8 status. */

      case Tg8DrvrGET_RAW_STATUS:
	if (wcnt<sizeof(arg->RawStatus)) break;
	if (recoset())
	  PrError(mcon,Tg8ERR_BUS_ERR);
	else
	  ReadRawStatus(mcon,&arg->RawStatus.Sb);
	noreco();
#if Tg8RESIDENT_V /* Resident firmware version */
	ReadSelfTestResult(mcon);
#endif
	arg->RawStatus.Res= mcon->SelfTestRes;
	arg->RawStatus.Cm = dcon->CloseMode;
	arg->RawStatus.ErrCnt = mcon->ErrCnt;
	arg->RawStatus.ErrCode= mcon->ErrCode;
	strcpy(arg->RawStatus.ErrMsg,mcon->ErrMsg);
      return (OK);


      /***************************************/
      /* Get the status of a module via mbx. */

      case Tg8DrvrGET_STATUS:
	if (wcnt < sizeof(arg->Status)) break;
	CheckDriverStatus(mcon);
	GetStatus(mcon,&f_hwst); /* Clear the fw alarms */
	arg->Status.Status = mcon->Status;
	arg->Status.Alarms = mcon->Alarms; mcon->Alarms = 0;
      return(OK);

      /*********************************************/
      /* Reload the firmware program for a module. */

      case Tg8DrvrRELOAD_FIRMWARE:
	if (rcnt < sizeof(Tg8FirmwareObject)) break; /* Garbage argument */
	if (arg->FirmwareObject.Size < 0 ||
	    arg->FirmwareObject.Size > Tg8FIRMWARE_OBJECT_SIZE) {
	  PrError(mcon,Tg8ERR_BAD_OBJECT);
	  pseterr(EFAULT); /* Bad address */
	  return(SYSERR);
	};
	mcon->Status = 0; /* Clear the module's status word */
	if (DownLoadFirmware(mcon,&arg->FirmwareObject)<0) {
	  pseterr(EIO);    /* IO error of some description */
	  return(SYSERR);
	};
	mcon->Status = Tg8DrvrMODULE_FIRMWARE_LOADED;
	ReloadUserTable(mcon);
	/* Set up the SSC Header required for the given TG8 module */
	SetSscHeader(mcon,mcon->ModuleAddress.SscHeader);
      return(OK);

      /*****************************************************/
      /* Reload a module's action table after a crash etc. */

      case Tg8DrvrRELOAD_ACTIONS:
      return ReloadUserTable(mcon);

      /***************************/
      /* Set the action's state. */

      case Tg8DrvrSET_ACTION_STATE:
	if (rcnt < sizeof(arg->ActState)) break;
	row = arg->Range.Row;
	rows= arg->Range.Cnt;
	cnt = row+rows-1;
	if (row<=0 || row>=Tg8ACTIONS || cnt>=Tg8ACTIONS) break;
	row--;
      return ChangeActionState(mcon,row,rows,arg->ActState.State);

      /*****************************/
      /* Clear the set of actions. */

      case Tg8DrvrCLEAR_ACTION:
	if (rcnt < sizeof(arg->Range)) break;
	row = arg->Range.Row;
	rows= arg->Range.Cnt;
	cnt = row+rows-1;
	if (row<=0 || row>=Tg8ACTIONS || cnt>=Tg8ACTIONS) break;
	row--;
      return ClearUserTable(mcon,row,rows);

      /***********************************/
      /* Get a part of the user table    */

      case Tg8DrvrUSER_TABLE:
	if (rcnt < sizeof(arg->Range)) break;
	row = arg->Range.Row;
	rows= arg->Range.Cnt;
	cnt = row+rows-1;
	if (row<=0 || row>=Tg8ACTIONS || cnt>=Tg8ACTIONS ||
	    wcnt < sizeof(arg->Range)+sizeof(Tg8User)*rows) break;
	f_adesc = arg->UserTable.Table;
	row--;
	if (GetAction(mcon,f_adesc,row,rows) != Tg8ERR_OK) {
	  pseterr(EIO);    /* IO error of some description */
	  return(SYSERR);
	};
      return(OK);

      /**************************************/
      /* Get a part of the recording table. */

      case Tg8DrvrRECORDING_TABLE:
	if (rcnt < sizeof(arg->Range)) break;
	row = arg->Range.Row;
	rows= arg->Range.Cnt;
	cnt = row+rows-1;
	if (row<=0 || row>=Tg8ACTIONS || cnt>=Tg8ACTIONS ||
	    wcnt < sizeof(arg->Range)+sizeof(Tg8Recording)*rows) break;
	f_rec = arg->RecTable.Table;
	row--;
	if (GetRecording(mcon,f_rec,row,rows) != Tg8ERR_OK) {
	  pseterr(EIO);    /* IO error of some description */
	  return(SYSERR);
	};
      return(OK);

      /*******************************************/
      /* Get the content of the interrupt table. */

      case Tg8DrvrINTERRUPT_TABLE:
	if (wcnt<sizeof(Tg8InterruptTable)) break;
	GetInterruptTable(mcon,&arg->IntTable);
      return(OK);

      /************************************/
      /* Get a part of the history table. */

      case Tg8DrvrHISTORY_TABLE:
	cnt= arg->HistTable.Cnt;
	if (wcnt < sizeof(int)+sizeof(Tg8History)*cnt) break;
	f_hist = arg->HistTable.Table;
	if (GetEventHistory(mcon,f_hist,cnt) != Tg8ERR_OK) {
	  pseterr(EIO);    /* IO error of some description */
	  return(SYSERR);
	};
      return(OK);

      /**********************************/
      /* Get a part of the clock table. */

      case Tg8DrvrCLOCK_TABLE:
	cnt= arg->ClockTable.Cnt;
	if (wcnt < sizeof(int)+sizeof(Tg8Clock)*cnt) break;
	f_clock = arg->ClockTable.Table;
	if (GetClock(mcon,f_clock,cnt) != Tg8ERR_OK) {
	  pseterr(EIO);    /* IO error of some description */
	  return(SYSERR);
	};
      return(OK);

      /*******************************************/
      /* Get the content of the auxiliary table. */

      case Tg8DrvrAUX_TABLE:
	if (wcnt<sizeof(Tg8Aux)) break;
	GetAuxTable(mcon,&arg->AuxTable);
      return(OK);

      /*******************************/
      /* Firmware break point trace. */

      case Tg8DrvrTRACE_FIRMWARE:
	if (wcnt<sizeof(int)) break;
	TraceFirmware(mcon,&arg->Trace);
      return(OK);

      /********************************************/
      /* Read the current module's date and time. */

      case Tg8DrvrDATE_TIME:
	if (wcnt<sizeof(Tg8DateTime)) break;
        ReadDateTime(mcon,&arg->DateTime);
      return (OK);

      /***************************/
      /* Read the DPRAM content. */

      case Tg8DrvrGET_DPRAM:
	if (wcnt<sizeof(Tg8Dpram)) break;
        GetDpram(mcon,&arg->Dpram);
      return (OK);

      /****************************/
      /* Read the SC information. */

      case Tg8DrvrSC_INFO:
	if (wcnt<sizeof(Tg8SuperCycleInfo)) break;
        ReadScInfo(mcon,&arg->ScInfo);
      return (OK);

      /***********************************/
      /* Set the safe device close mode. */

      case Tg8DrvrON_CLOSE:
	if (rcnt < sizeof(arg->CloseMode)) break; /* Garbage address in arg */
	if (arg->CloseMode.Mode>Tg8ONCL_DISABLE_SYNC ||
	    (arg->CloseMode.SimPulseMask &= 0xFF) &&
	    arg->CloseMode.Mode!=Tg8ONCL_DISABLE &&
	    arg->CloseMode.Mode!=Tg8ONCL_CLEAR) break; /* Bad mode */
	dcon->CloseMode = arg->CloseMode;
      return(OK);

      /******************************/
      /* Get the events queue size. */

      case Tg8DrvrGET_QUEUE_LENGTH:
	if (wcnt < sizeof(int)) break; /* Garbage address in arg */
	arg->QueueLength = dcon->AppSemaphore;
      return(OK);

      /************************************/
      /* Purge the client's events queue. */

      case Tg8DrvrPURGE_QUEUE:
        dcon->Tail = queue->Head; /* End of the queue */
        sreset(&dcon->AppSemaphore);
      return(OK);

      /*******************************************************/
      /* Wait for the given event. Wildcards can not be used */

      case Tg8DrvrWAIT_EVENT:
      if (wcnt < sizeof(Tg8DrvrEvent)) break; /* Bad byte count */

      if (!tab->Size) {
	arg->Event.Event.Long = 0; /* Clear the event code */
	return (OK);
      };
      if (!dcon->AppSemaphore) { /* Has to wait */
	if (dcon->Mode &FNDELAY) {
	  pseterr(EWOULDBLOCK);
	  return(SYSERR);
	};
      };
      if (dcon->AppSemaphore>Tg8DrvrQUEUE_SIZE)
	dcon->AppSemaphore = Tg8DrvrQUEUE_SIZE;

#if 0
      /* Start the timer */
      CancelTimeout(&dcon->Timer);
      dcon->Timer = timeout(HandleTimeout, dcon, mcon->TimeOut);
      if (dcon->Timer <= 0) {
	 dcon->Timer = 0;
	 pseterr(EAGAIN);       /* No more processes */
	 return(SYSERR);
      };
#endif
      /* Wait for the given event. No wildcard allowed */

      for (;;) {
wait_ev:
	if (swait(&dcon->AppSemaphore, SEM_SIGABORT)) {
	  /*** CancelTimeout(&dcon->Timer); ***/
	  pseterr(EINTR); /* Interrupted */
	  return (SYSERR);
	};
#if 0
	if (!dcon->Timer) {
	  /* The queue is empty - time out */
	  arg->Event.Event.Long = 0; /* Clear the event code */
	  break;
	};
#endif
	/* Scan the queue for the appropriate event */
	for (;;) {
	  if (dcon->Tail == queue->Head) {
	    /*** sreset(&dcon->AppSemaphore); */
	    goto wait_ev;
	  };
	  conn = &queue->Queue[dcon->Tail++];
	  if (dcon->Tail >= Tg8DrvrQUEUE_SIZE) dcon->Tail = 0;
	  if (conn->DevMask &dm) break; /* Found */
	};
	if (conn->Inter.iEvent.Long == arg->Event.Event.Long) {
	  /* The event that is waiting for */
	  arg->Event = *conn; /* Read in it */
	  break;
	};
	/* Wait for the next event forever ... */
      };
      /*** CancelTimeout(&dcon->Timer); ***/
      return(OK);

      /************************************************************************/
      /* Set up the filter event. It will open connections to existent events */
      /* automatically. Wildcards can be used as well. */

      case Tg8DrvrFILTER_EVENT:
	if (rcnt<sizeof(arg->Filter) || wcnt<sizeof(arg->Filter)) break;

	dcon->Filter.Long = arg->Filter.Event.Long;
	for (an=j=0,a=tab->Table,mt=tab->DevMask; an<tab->Size; an++,a++,mt++) {
	  if (a->uEvent.Long == 0) continue;
	  if (Match(dcon->Filter.Long,a->uEvent.Long)) {
	    /* Events are matching - set the device usage bit for it */
	    *mt |= dm;
	    /* Set the VME bus interrupt for that action */
	    EnableBusInterrupt(mcon,a,an);
	    j++;
	  } else {
	    *mt &= ~dm;
	    if (!*mt) ClearBusInterrupt(mcon,a,an);
	  };
	};
	arg->Filter.Matches = j;
	/* Purge the client's queue */
	dcon->Tail = mcon->Queue.Head;
	sreset(&dcon->AppSemaphore);
      return(OK);

      /*******************************************/
      /* Read the telegram for the given machine */

      case Tg8DrvrTELEGRAM:
	if (wcnt<sizeof(arg->Telegram)) break;
	if (ReadTelegram(mcon,arg->Telegram.Machine,arg->Telegram.Data) !=
	    Tg8ERR_OK) break;
      return (OK);

      /******************/
      /* Read ppm lines */

      case Tg8DrvrGET_PPM_LINE:
	if (rcnt < sizeof(arg->Range)) break;
	row = arg->Range.Row;
	rows= arg->Range.Cnt;
	cnt = row+rows-1;
	if (row<=0 || row>=Tg8ACTIONS || cnt>=Tg8ACTIONS ||
	    wcnt < sizeof(arg->Range)+sizeof(Tg8PpmLine)*rows) break;
	ppml = arg->PpmLineTable.Table;
	row--;
	if (GetPpmLine(mcon,ppml,row,rows) != Tg8ERR_OK) {
	  pseterr(EIO);    /* IO error of some description */
	  return(SYSERR);
	};
      return(OK);

      /*****************************/
      /* Insert the timing object. */

      case Tg8DrvrSET_OBJECT:
	if (rcnt < sizeof(arg->Object)) break;
	if (ObjById(mcon,arg->Object.Id) >= 0) break; /* Already presents */

	ppm = &arg->Object.Timing;
	if (!ppm->Machine) { /* Simple object, no PLS condition. Its dimension will be 1. */
	  ppm->Dim = k = 1;
	  ppm->GroupNum = ppm->GroupType = 0;
	} else {
	  if (ppm->Machine >= Tg8MACHINES) break;
	  if (ppm->GroupType > Tg8GT_BIT) break;
	  k = ppm->Dim;
	};
	if (k>Tg8PPM_LINES || k<=0) break; /* Bad Object's dimension */

	/* Look for free space in the user table to insert the object into */

	for (i=an=0,j=-1,a=tab->Table; i<tab->Size; i++,a++) {
	  if (a->uEvent.Long) { /* Used item */
	    an= 0;
	    j = -1;
	    continue;
	  };
	  if (j<0) j= i;
	  if (++an == k) break;
	};
	if (an<k) { /* Not enough, reserve the rest */
	  if (an) an= k-an;
	  else {
	    an= k;
	    j = i; /* size */
	  };
	  if (j+k>Tg8ACTIONS) {
	    pseterr(ENOSPC);    /* No space to locate the object */
	    return(SYSERR);
	  };
	};
	oid = arg->Object.Id;
	row = j;
	if (SetPpmAction(mcon,row,1,ppm) != Tg8ERR_OK) {
	  pseterr(EIO);    /* IO error of some description */
	  return(SYSERR);
	};

	/* Define now the ppm lines for the driver side */

	a = &tab->Table[row];
	mt= &tab->DevMask[row];
	g = &tab->Gate[row];
	oi= &tab->Id[row];
	tab->Dim[row] = k;
	for (i=0; i<k; i++,a++,mt++,g++,oi++) {
	  a->uEvent.Long = ppm->Trigger.Long;
	  a->uControl = ppm->LineCw[i];
	  a->uDelay   = ppm->LineDelay[i];
	  g->Machine  = (ppm->Machine<<4) | ppm->GroupType;
	  g->GroupNum = ppm->GroupNum;
	  g->GroupVal = ppm->LineGv[i];
	  *mt= dm;
	  *oi= oid;
	  Tg8CW_PPML_Clr(a->uControl);
	  if (i>0)
	    Tg8CW_PPML_Set(a->uControl,Tg8TM_LINE);
	  else
	    Tg8CW_PPML_Set(a->uControl,Tg8TM_PPM);
	};
	row += k;
	if (row > mcon->UserTable.Size) mcon->UserTable.Size = row;
      return(OK);

      case Tg8DrvrREMOVE_OBJECT:
	an = ObjById(mcon,arg->ObjectId);
	if (an<0) break; /* No such object */
        return ClearUserTable(mcon,an,tab->Dim[an]);

      /*******************************/
      /* Get the timing object by Id */

      case Tg8DrvrGET_OBJECT:
	if (wcnt < sizeof(arg->ObjectDescr)) break;

	obj= &arg->ObjectDescr.Object;
	an = ObjById(mcon,obj->Id);
	if (an<0) { /* No such object */
	  arg->ObjectDescr.Dim = 0;
	  obj->Id = 0;
	  break;
	};

	ppm = &obj->Timing;
	a = &tab->Table[an];
	g = &tab->Gate[an];
	k = tab->Dim[an];
	ppm->Trigger.Long = a->uEvent.Long;
	ppm->Machine= g->Machine>>4;
	ppm->GroupType = g->Machine &0xF;
	ppm->GroupNum = g->GroupNum;
	ppm->Dim = k;
	for (i=0; i<k; i++,a++,g++) {
	  ppm->LineCw[i] = a->uControl;
	  ppm->LineDelay[i] = a->uDelay;
	  ppm->LineGv[i] = g->GroupVal;
	};
	arg->ObjectDescr.Act = an+1;
	arg->ObjectDescr.Dim = k;
      return(OK);

      /*************************************************/
      /* Open connection to the timing object's lines. */

      case Tg8DrvrCONNECT:
	if (rcnt<sizeof(arg->ObjectConnect)|| wcnt<sizeof(arg->ObjectConnect)) break;

	an = ObjById(mcon,arg->ObjectConnect.Id);
	if (an<0) { /* No such object */
	  arg->ObjectConnect.Mask = 0;
	  break;
	};

	a = &tab->Table[an];
	mt= &tab->DevMask[an];
	k = tab->Dim[an]; /* Dimension */
	msk = arg->ObjectConnect.Mask;
	for (i=0,j=1,cnt=0; i<k; i++,an++,a++,mt++,j<<=1) {
	  if (!(msk &j)) {
	    *mt &= ~dm;
	    if (!*mt) ClearBusInterrupt(mcon,a,an);
	    continue; /* skip a line */
	  };
	  cnt |= j;
	  /* Set the device usage bit for this line */
	  *mt |= dm;
	  /* Set the VME bus interrupt for that line */
	  EnableBusInterrupt(mcon,a,an);
	};
	arg->ObjectConnect.Mask = cnt;
      return(OK);

      /**********************************/
      /* Change some object's parameter */

      case Tg8DrvrOBJECT_PARAM:
	if (rcnt<sizeof(arg->ObjectParam)) break;

	an = ObjById(mcon,arg->ObjectParam.Id);
	if (an<0) { /* No such object */
	  arg->ObjectParam.Id = 0;
	  break;
	};

	k = tab->Dim[an]; /* Dimension */
	i = arg->ObjectParam.Line;
	if (i>k) break; /* Bad line number */
	if (i) an += (i-1);
	a = &tab->Table[an];
	msk = arg->ObjectParam.Value;
	j   = arg->ObjectParam.Aux;
	switch (arg->ObjectParam.Sel) {
	case Tg8SEL_STATE: /* Change en/dis state */
	  return ChangeActionState(mcon,an,1,
				  (msk? (i? Tg8ENABLED : Tg8PPMT_ENABLED):
				        (i? Tg8DISABLED: Tg8PPMT_DISABLED)));
	case Tg8SEL_DELAY: /* Change delay and/or clock type */
	  if (SetActionDelay(mcon,an,1,msk,j) != Tg8ERR_OK) {
	    pseterr(EIO);    /* IO error of some description */
	    return(SYSERR);
	  };
	  a->uDelay = msk;
	  if (j) {
	    Tg8CW_CLOCK_Clr(a->uControl);
	    Tg8CW_CLOCK_Set(a->uControl,(j&0x3));
	  };
	  return(OK);
	default:
	  break;
	};
	break; /* On error */

      /*****************************************/
      /* Get the list of all known object IDs. */

      case Tg8DrvrOBJECTS_LIST:
	if (wcnt<sizeof(arg->ObjectsList)) break;
	k = arg->ObjectsList.Length;
	for (i=j=0; i<Tg8ACTIONS && j<k; ) {
	  if (an = tab->Dim[i]) { /* An object OR ppm timing */
	    arg->ObjectsList.Id[j++] = tab->Id[i];
	    i += an;
	  } else /* Low level timing unit (action) */
	    i++;
	};
	arg->ObjectsList.Length = j;
      return (OK);

      /*******************/
      /* Test the DPRAM. */

      case Tg8DrvrTEST_DPRAM:
	if (wcnt<sizeof(arg->TestDpram)) break;
	TestTheDpram(mcon,&arg->TestDpram);
      return (OK);

      /**********************/
      /* Do the card tests. */

      case Tg8DrvrCARD_TEST:
	if (wcnt<sizeof(arg->AutoTest)) break;
	AutoTest(mcon,&arg->AutoTest);
      return (OK);

   };
   /***********************************/
   /* End of switch                   */
   /***********************************/

   pseterr(EINVAL);       /* Invalid argument */
   return(SYSERR);

} /* <<Tg8DrvrIoctl>> */

/*eof ioctl.c */
