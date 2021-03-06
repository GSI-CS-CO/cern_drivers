/**********************************************************************/
/* Tg8's Mail Box Protocol implementation Routine.		      */
/*             1992-1994                                              */
/* Lewis J., Kovaltsov V.                                             */
/* Last edition: 20 Sep 1994                                          */
/* Vladimir Kovaltsov for the SL Version, February, 1997	      */

/* Fri 10th June Julian Lewis: Modifications as follows...            */

/*    Removed statically initialized memory so that program can be    */
/*    downloaded accross the VME bus.                                 */

/*    Added static memory initialization to the SoftInit routine for  */
/*    download capability                                             */

/*    Implemented SPS telegram support                                */

/*    Removed all logic to do with the old date/time formats for the  */
/*    legacy events for tg3, and PS tg8                               */

/*    Implemented full UTC time support based on the Tgv events       */

/*    The handling of the SUPER-CYCLE number in the old 0x20 header   */
/*    now returns the 32 bit UTC time instead of the 24 bit payload   */

/*    Some general tidy up of the code, in particular so that the O3  */
/*    optimization can be turned on hardware addresses are declared   */
/*    as volatile. This makes the code much faster.                   */

/**********************************************************************/

/**********************************************************************/
/* Poll the mail box and carry out any requests for the host DSC      */
/**********************************************************************/

void MbxProcess() {

Tg8Action *a; Tg8User *ad;
Tg8PpmUser *ppm; Tg8PpmLine *ppml; Tg8Gate *g;
int j,cnt,pos; short cw,err;

  /*************************************/
  /* The main loop of the mailbox task */
  /*************************************/

  for (;;) {

#if 0
   /******************/
   /* Watchdog reset */
   /******************/

   sim->SimServ = 0x55; /* Magic sequence to satisfy the */
   sim->SimServ = 0xaa; /* Software Watchdog */

   /********************************/
   /* Make an edge on the OK light */
   /********************************/

   sim->SimDataF1 |= OkLed;
   sim->SimDataF1 &= ~OkLed;
   sim->SimDataF1 |= OkLed;

   /********************************/
   /* Trace the receiver errors    */
   /********************************/

   if (err = xlx->WClrRerr) { /* Read the current receiver error */
     rcv_error.Err = err;
     /* Copy RcvErr & Hour bytes to the DPRAM */
     *(Word*)&dpm->At.aDt.aRcvErr = *(Word*)&rcv_error;
   };

#else
   /******************/
   /* Watchdog reset */
   /******************/

   /* Optimized => use assembler code */

   asm volatile ("movew #0x55,SimServ
		  movew #0xAA,SimServ");

   /********************************/
   /* Make an edge on the OK light */
   /********************************/

   /* Optimized => use assembler code */

   asm volatile ("oriw  #OkLed,SimDataF1
		  andiw #~OkLed,SimDataF1
		  oriw  #OkLed,SimDataF1");

   /********************************/
   /* Trace the receiver errors    */
   /********************************/

   if (err = xlx->WClrRerr) { /* Read the current receiver error */

      rcv_error.Err = err;

      /* Copy RcvErr & Hour bytes to the DPRAM */

      *(Word *) &dpm->At.aDt.aRcvErr = *(Word *) &rcv_error;
   }

#endif

   /*********************************************/
   /* Provoke an interrupt to the Host computer */
   /*********************************************/

   if (eprog.IntSrc && !dpm->At.aIntSrc) {
     /* The DSC is ready to proceed the new interrupts if any */
     /* Set up the interr. source mask */
     /* Issue the real VME BUS interrupt right now */
     asm volatile (ISM_SET_TRAP);
   };

   /* Check the Mailbox request */

   switch (dpm->At.aMbox) {

     /**********************************************************/
     /* No Command                                             */
     /**********************************************************/

   case Tg8OP_NO_COMMAND:
     continue;         /* Wait for the next request to appear */

     /**********************************************************/
     /* Ping the module                                        */
     /**********************************************************/

   case Tg8OP_PING_MODULE:
     break;

     /**********************************************************/
     /* Ping the module                                        */
     /**********************************************************/

   case Tg8OP_SIMULATE_PULSE: {
     Tg8IntAction  uu;
     Tg8Interrupt *ip;
     j = dpm->BlockData.SimPulse.Mask;
     for (cnt=0; j; cnt++,j>>=1) {
       if (j&1) { /* Start counter number 'cnt' [0-7] */
	 xlx->XlxDelay [cnt] = 1;
	 xlx->XlxConfig[cnt] = ConfOUTPUT|ConfNORMAL;

	 /* Fill the counter's interrupt table entry in */
	 ip = (Tg8Interrupt *) &dpm->It.CntInter[cnt];
	 eprog.iFired[cnt] = ip;        /* TPU int.handler uses this value */
	 eprog.rFired[cnt] = &sim_rec;  /* --"-- */

	 /* Fill the interrupt table entry in. Here the 'ip' points to that entry. */
	 /* This information is used by the driver only. */

	 ip->iOut = 0xFFFFFFFF;     /* Output time is unknown yet */
	 ip->iEvent.Long = 0;       /* Take the pseudo trigger code */
	 ip->iSc = dpm->At.aSc;     /* The Super Cycle number */
	 ip->iOcc= dpm->At.aScTime; /* Occurence time from the start of the last S-Cycle */
	 uu.iRcvErr = rcv_error.Err;/* Place the receiver error code here */
	 uu.iOvwrAct = 0;           /* No problems about of action overwritting */
	 uu.iAct = 0;               /* Place the number of the pseudo action */
	 uu.iSem = 1;               /* Block the data */
	 ip->iExt= uu;              /* Insert the new data into the interr. table */
       };
     };
     break;
   };

     /**********************************************************/
     /* Read the firmware statuse                              */
     /**********************************************************/

   case Tg8OP_GET_STATUS: {   /* Get the firmware status words */
     Tg8StatusBlock *s = (Tg8StatusBlock *) &dpm->BlockData.StatusBlock;
     s->Fw = dpm->At.aFwStat;
     s->Am = dpm->At.aAlarms; ResetAlarms();
     s->Cc = dpm->At.aCoco;
     s->Evo= dpm->ExceptionVector;
     s->Epc= dpm->ExceptionPC;
     s->ScTime = dpm->At.aScTime;
     s->Dt = dpm->At.aDt;
     dpm->At.aMaxQueueSize = 0;
     break;
   };

     /**********************************************************/
     /* Read the selftest result                               */
     /**********************************************************/

   case Tg8OP_SELFTEST_RESULT:
     dpm->BlockData.SelfTestResult = info;
     break;

     /**********************************/
     /* "Read user table" operation.   */
     /**********************************/

   case Tg8OP_GET_USER_TABLE: {
     Tg8MbxRwAction *tab = (Tg8MbxRwAction *) &dpm->BlockData.RwAction;
     ins.Addr = tab->Hdr.Row &0xFF;
     cnt = tab->Hdr.Number;
     if (ins.Addr+cnt > Tg8ACTIONS) cnt = Tg8ACTIONS - ins.Addr;
     for (ad=tab->Actions,a=act.Pointers[ins.Addr]; cnt>0;
	  cnt--,ins.Addr++,ad++,a++) {
       *ad = a->User;
       if (ad->uDelay) --(ad->uDelay);
     };
     tab->Hdr.Row = ins.Addr;
     break;
   };

     /*************************************/
     /* "Read recording table" operation. */
     /*************************************/

   case Tg8OP_GET_RECORDING_TABLE: {
     Tg8MbxRecording *tab = (Tg8MbxRecording *) &dpm->BlockData.Recording;
     Tg8Recording *r;
     ins.Addr = tab->Hdr.Row &0xFF;
     cnt = tab->Hdr.Number;
     if (ins.Addr+cnt > Tg8ACTIONS) cnt = Tg8ACTIONS - ins.Addr;
     for (r=tab->Recs,a=act.Pointers[ins.Addr]; cnt>0;
	  cnt--,ins.Addr++,r++,a++) *r = a->Rec;
     tab->Hdr.Row = ins.Addr;
     break;
   };

     /*************************************/
     /* "Read events history" operation.  */
     /*************************************/

   case Tg8OP_GET_HISTORY_TABLE: {
     Tg8MbxHistory *tab = (Tg8MbxHistory *) &dpm->BlockData.HistoryBlock;
     Tg8History *ah,*d;
     cnt = tab->Hdr.Number;
     pos = tab->Hdr.Row;   /* >0 gets the number of the last received entry */
     if (!pos) pos = hist.History_i; /* Read the newest records */
     if (pos >= Tg8HISTORIES) pos = Tg8HISTORIES-1;
     ah  = &hist.Histories[pos];
     for (j=0,d=tab->Hist; j<cnt; j++,d++) {
       if (--pos <0) {
	 pos = Tg8HISTORIES-1;
	 ah  = &hist.Histories[pos];
       } else
	 --ah;
       *d = *ah;
     };
     tab->Hdr.Row = pos;
     break;
   };
   
     /*************************************/
     /* "Read clock table" operation.     */
     /*************************************/

   case Tg8OP_GET_CLOCK_TABLE: {
     Tg8MbxClock *tab = (Tg8MbxClock *) &dpm->BlockData.ClockBlock;
     Tg8Clock *cc,*d;
     cnt = tab->Hdr.Number;
     pos = tab->Hdr.Row;   /* >0 gets the number of the last received entry */
     if (!pos) pos = clk.Clock_i; /* The newest entry index */
     if (pos >= Tg8CLOCKS) pos = Tg8CLOCKS-1;
     cc  = &clk.Clocks[pos];
     for (j=0,d=tab->Clks; j<cnt; j++,d++) {
       if (--pos <0) {
	 pos = Tg8CLOCKS-1;
	 cc  = &clk.Clocks[pos];
       } else
	 --cc;
       *d = *cc;
     };
     tab->Hdr.Row = pos;
     break;
   };

     /**********************************/
     /* "Clear Actions" operation */
     /**********************************/

   case Tg8OP_CLEAR_USER_TABLE: {
     Tg8MbxClearAction *tab = (Tg8MbxClearAction *) &dpm->BlockData.ClearAction;
     ins.Addr = pos = tab->Hdr.Row &0xFF;
     cnt = tab->Hdr.Number;
     if (ins.Addr+cnt > Tg8ACTIONS) cnt = Tg8ACTIONS - ins.Addr;
     ins.Event.Long = 0;
     ins.Last_word.Short = 0;
     for (a=act.Pointers[ins.Addr]; cnt>0 && ins.Addr<dpm->At.aNumOfAct;
	  cnt--,a++,ins.Addr++) {
       /* Clear the CAM's item and action table entry */
       memset((volatile short *) a,0,sizeof(*a));
       asm(Insert_CAM_Trap);
     };
     if (ins.Addr == dpm->At.aNumOfAct) {
       dpm->At.aNumOfAct = pos;
       if (pos == 0) {/* The user table is empty. */
	 memset((volatile short *) &dpm->It,0,sizeof(Tg8InterruptTable)); /* Remove pending interrupts */
	 memset((volatile short *) &in_use,0,sizeof(in_use));             /* Clear the triggers usage info */
       };
     };
     break;
   };

   /****************************************************/
   /* Rewrite the set of actions. Gate shall be reset. */
   /****************************************************/

   case Tg8OP_SET_USER_TABLE: {
     Tg8MbxRwAction *tab = (Tg8MbxRwAction *) &dpm->BlockData.RwAction;
     ins.Addr = tab->Hdr.Row &0xFF;
     cnt = tab->Hdr.Number;
     if (ins.Addr+cnt > Tg8ACTIONS) cnt = Tg8ACTIONS - ins.Addr;
     for (ad=tab->Actions; cnt>0; cnt--,ad++)
       if ((j = InsertAction(ad,0/*NotPpm*/))<0) {
	 dpm->At.aTrace= ins.Addr;
	 dpm->At.aCoco = j;
	 goto done;
       };
     tab->Hdr.Row = ins.Addr;
     break;
   };

     /*************************************************/
     /* Rewrite the set of the PPM actions.           */
     /*************************************************/

   case Tg8OP_SET_PPM_TIMING: {
     Tg8MbxRwPpmAction *tab = (Tg8MbxRwPpmAction *) &dpm->BlockData.RwPpmAction;
     ins.Addr = tab->Hdr.Row &0xFF;
     cnt = tab->Hdr.Number;
     if (ins.Addr+cnt > Tg8ACTIONS) cnt = Tg8ACTIONS - ins.Addr;
     for (ppm=tab->Actions; cnt>0; cnt--,ppm++)
       if ((j = InsertAction((Tg8User*)ppm,1/*Ppm*/))<0) {
	 dpm->At.aTrace= ins.Addr;
	 dpm->At.aCoco = j;
	 goto done;
       };
     tab->Hdr.Row = ins.Addr;
     break;
   };

   /***********************************/
   /* "Read PPM user table" operation.*/
   /***********************************/

   case Tg8OP_GET_PPM_LINE: {
     Tg8MbxRwPpmLine *tab = (Tg8MbxRwPpmLine *) &dpm->BlockData.RwPpmLine;
     ins.Addr = tab->Hdr.Row &0xFF;
     cnt = tab->Hdr.Number;
     if (ins.Addr+cnt > Tg8ACTIONS) cnt = Tg8ACTIONS - ins.Addr;
     for (ppml=tab->Lines,a=act.Pointers[ins.Addr]; cnt>0;
	  cnt--,ins.Addr++,ppml++,a++) {
       ppml->Action = a->User;
       if (ppml->Action.uDelay) --(ppml->Action.uDelay);
       ppml->Gate = a->Gate;
     };
     tab->Hdr.Row = ins.Addr;
     break;
   };

     /*************************************************/
     /* Rewrite gate part of the set of actions.      */
     /*************************************************/

   case Tg8OP_SET_GATE: {
     Tg8MbxRwGate *tab = (Tg8MbxRwGate *) &dpm->BlockData.RwGate;
     ins.Addr = tab->Hdr.Row &0xFF;
     cnt = tab->Hdr.Number;
     if (ins.Addr+cnt > Tg8ACTIONS) cnt = Tg8ACTIONS - ins.Addr;
     for (g=tab->Gates,a=act.Pointers[ins.Addr]; cnt>0;
	  cnt--,g++,a++,ins.Addr++) {
       a->Gate = *g;
     };
     tab->Hdr.Row = ins.Addr;
     break;
   };

     /*************************************************/
     /* Rewrite dimension part of the set of actions. */
     /*************************************************/

   case Tg8OP_SET_DIM: {
     Tg8MbxRwDim *tab = (Tg8MbxRwDim *) &dpm->BlockData.RwDim; Byte *p;
     ins.Addr = tab->Hdr.Row &0xFF;
     cnt = tab->Hdr.Number;
     if (ins.Addr+cnt > Tg8ACTIONS) cnt = Tg8ACTIONS - ins.Addr;
     for (p=tab->Dims,a=act.Pointers[ins.Addr]; cnt>0;
	  cnt--,p++,a++,ins.Addr++) {
       a->Enabled &= 1; /* Keep the Enable/Disable state */
       a->Enabled |= (*p<<1);
     };
     tab->Hdr.Row = ins.Addr;
     break;
   };

   /*************************************/
   /* "Set Action's Delay" operation    */
   /*************************************/

   case Tg8OP_SET_DELAY: {
     Tg8MbxActionState *tab = (Tg8MbxActionState *) &dpm->BlockData.ActionState;
     ins.Addr = tab->Hdr.Row &0xFF;
     cnt = tab->Hdr.Number;
     if (ins.Addr+cnt > Tg8ACTIONS) cnt = Tg8ACTIONS - ins.Addr;
     for (a=act.Pointers[ins.Addr]; cnt>0; cnt--,a++,ins.Addr++) {
       if (a->CntControl &ConfOUTPUT) {
	 if (tab->State == 0xFFFF) {
	   dpm->At.aTrace= ins.Addr;
	   dpm->At.aCoco = Tg8ERR_WRONG_DELAY;
	   goto done;
	 };
	 if (tab->Aux) { /* Set the clock type too */
	   cw = a->CntControl & ~(ConfEXT_CLOCK_1|ConfEXT_CLOCK_2|ConfCABLE_CLOCK);
	   switch(tab->Aux &= 0x3) {
	   case Tg8CLK_MILLISECOND:
	     break;
	   case Tg8CLK_X1:
	     cw |= ConfEXT_CLOCK_1;
	     break;
	   case Tg8CLK_X2:
	     cw |= ConfEXT_CLOCK_2;
	     break;
	   default:
	     dpm->At.aTrace= ins.Addr;
	     dpm->At.aCoco = Tg8ERR_WRONG_CLOCK;
	     goto done;
	   };
	   Tg8CW_CLOCK_Clr(a->User.uControl);
	   Tg8CW_CLOCK_Set(a->User.uControl,tab->Aux);
	   a->CntControl = cw;
	 };
	 a->User.uDelay = tab->State+1;
       };
     };
     tab->Hdr.Row = ins.Addr;
     break;
   };

   /*************************************/
   /* "Set Action's State" operation    */
   /*************************************/

   case Tg8OP_SET_STATE: {
     Tg8MbxActionState *tab = (Tg8MbxActionState *) &dpm->BlockData.ActionState;
     ins.Addr = tab->Hdr.Row &0xFF;
     cnt = tab->Hdr.Number;
     if (ins.Addr+cnt > Tg8ACTIONS) cnt = Tg8ACTIONS - ins.Addr;
     for (a=act.Pointers[ins.Addr]; cnt>0; cnt--,a++,ins.Addr++) {
       switch (tab->State) {
       case Tg8ENABLED:
	 if (Tg8CW_PPML_Get(a->User.uControl)) goto st_en; /* Except of lines of PPM action's */
       case Tg8PPMT_ENABLED:
	 ins.Event.Long = a->User.uEvent.Long;
	 ins.Last_word.Short = (a->CntControl &ConfOUTPUT)? 0xFF/*out*/: 0x00/*imm*/;
	 asm(Insert_CAM_Trap);
st_en:
	 a->Enabled |= Tg8ENABLED;
	 Tg8CW_STATE_Clr(a->User.uControl);
	 break;
       case Tg8DISABLED:
	 if (Tg8CW_PPML_Get(a->User.uControl)) goto st_dis; /* Except of lines of PPM action's */
       case Tg8PPMT_DISABLED:
	 ins.Event.Long = 0;
	 ins.Last_word.Short = 0;
	 asm(Insert_CAM_Trap);
st_dis:
	 a->Enabled &= ~Tg8ENABLED;
	 Tg8CW_STATE_Set(a->User.uControl,1);
	 break;
       case Tg8INTERRUPT: /* Enable interrupts */
	 a->CntControl |= ConfINTERRUPT; /* Set the counter's configuration */
	 Tg8CW_INT_Set(a->User.uControl,Tg8DO_INTERRUPT);
	 break;
       case Tg8NO_INTERRUPT: /* Disable interrupts */
	 a->CntControl &= ~ConfINTERRUPT; /* Set the counter's configuration */
	 a->User.uControl &= ~(Tg8DO_INTERRUPT<<Tg8CW_INT_BITN);
	 break;
       };
     };
     break;
   };

   /*************************************/
   /* "Set SSC Header" operation.       */
   /*************************************/

   case Tg8OP_SET_SSC_HEADER:
     j = xlx->WClrRerr;        /* Read the reciever error */
     xlx->WClrRerr = 0;        /* and clear it. */
     xlx->XWsscRframe1 = dpm->At.aSscHeader; /* Set the new SSC header code */
     break;

     /**********************************************************/
     /* Unrecognized mailbox operation                         */
     /**********************************************************/

   default:
     dpm->At.aCoco = Tg8ERR_ILLEGAL_OPERATION;
     goto done;
   };
   
   dpm->At.aCoco = Tg8ERR_OK;

done: /* Send out the response to the host */

   dpm->At.aMbox = Tg8OP_NO_COMMAND;   /* Clean the opcode location */

   /* If the previous command did not completed, send an alarm */
   if (dpm->At.aIntSrc &Tg8ISM_MAILBOX) SignalAlarm(Tg8ALARM_MBX_BUSY);

   SignalToHost(Tg8IS_MAILBOX); /* Send an acknowledge */

 }; /*for ever*/

} /* <<MbxProcess>> */

/* eof mbx.c */
