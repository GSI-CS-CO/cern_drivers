/*****************************************************************/
/* Counters test                                                 */
/*****************************************************************/

CountersTest() {
StFault *f; short i,n,nn,v,conf,delay;

  /* Enable Bus Monitor (BME) & Set BMT=64 Clocks */
  sim->SimProtect = Tg8ENABLE_BUSMONITOR;
  bus_int = 0;

  dpm->Info.TestProg = T_COUNTERS;
  dpm->Info.Counter.Bad = 0; /* Clear the mask of bad counters */

  /* Read/write counter delay */

  for (i=0; i<Tg8COUNTERS; i++) {
    v = xlx->XlxDelay[i];
    xlx->XlxDelay[i] = ~i;
    if (bus_int) CounterFailed(i,CI_DELAY);
  };

  if (!(dpm->Info.FaultType & T_MS)) { /* MS interrupts are OK */

    for (nn=1,delay = 2; delay<=4; nn++,delay++) {

      for (i=0,f= dpm->Info.Counter.Err; i<Tg8COUNTERS; i++,f++) {

	xlx->XlxDelay[i] = f->Templ = delay;
	tpu_int[i+1] = 0;
	tpu_int[0] = 0; while (!tpu_int[0]); /* Wait for the next ms */
	tpu_int[0] = 0;
	xlx->XlxConfig[i] = /** ConfINTERRUPT | ConfOUTPUT |**/
                         ConfNORMAL;                /* Just start Counting */
	if (bus_int) CounterFailed(i,CI_CONFIG);
	else {
	  /* Wait for 'delay' OR 'counter interrupt'*/
	  while (tpu_int[0] < delay) ;
	  dpm->XilinxStatus = f->Dir = xlx->WClrRerr &0xFF;
	  if (!tpu_int[i+1]) /* No interrupt from this counter */
	    CounterFailed(i,CO_BLOCKED);
	  else
	    dpm->Info.Counter.Delay = f->Actual = tpu_int[0];
	};
      };
    };
  };
  if (!(dpm->Info.FaultType & T_COUNTERS)) dpm->Info.TestPassed |= T_COUNTERS;
}

/******************************************************/
/* Local procedure to set up fault code for a counter */
/******************************************************/

CounterFailed(int cnt,int cond) {
StFault *f = &dpm->Info.Counter.Err[cnt];
  if (cond >= N_OF_CONT_ER) { /* BUS interrupt occured */
    f->BusInt = cond;
    f->N_of_bus += bus_int;
    dpm->Info.N_of_bus += bus_int;
    bus_int = 0;
  };
  f->Code = cond;
  dpm->Info.Counter.Bad |= (1<<cnt);
  dpm->Info.FaultType |= T_COUNTERS;
  dpm->Info.FaultCnt++;
}

/* eof */
