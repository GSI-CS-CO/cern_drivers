/**********************************************************************/
/* Public declarations for everybody.                                 */
/* V. Kovaltsov for the SL Timing, April, 1997                        */

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

/***********************************************/

#ifndef TG8PUB_H
#define TG8PUB_H

#define _MVME_167_ 0  /* 1=MVME; 0=PPC */

#define _PS_       0  /* 1=the PS version; 0=the SL version */

#define DEBUG      1  /* 1=debugg mode ON; 0=debugg mode OFF */

/*************************************************************************/
/* General type definitions                                              */
/*************************************************************************/

#ifndef TG8_TYPES
#define TG8_TYPES
typedef unsigned char  Uchar;
typedef unsigned char  Byte;

typedef unsigned short Ushort;
typedef unsigned short Word;

typedef unsigned int   Uint;
typedef unsigned int   DWord;
#endif

/***************************************************************/
/* Tg8's accelerators (machines) with their own timing systems */
/***************************************************************/

typedef enum { Tg8NO_MACHINE=0,
	       Tg8LHC,  /* 1 */
	       Tg8SPS,	/* 2 */
	       Tg8CPS,	/* 3 */
	       Tg8PSB,	/* 4 */
	       Tg8LEA,  /* 5 */
	       Tg8MACHINES	/* Total number of machines +1 */
	     } Tg8Machine;


/***** The Tg8's Events and headers (as used by the SL timing system) ****/

typedef enum {
	Tg8MILLISECOND_HEADER = 0x01,
	Tg8SECOND_HEADER = 0x02,
	Tg8MINUTE_HEADER = 0x03,
	Tg8HOUR_HEADER   = 0x04,
	Tg8DAY_HEADER    = 0x05,
	Tg8MONTH_HEADER  = 0x06,
	Tg8YEAR_HEADER   = 0x07,
	Tg8psTIME_HEADER = 0x08,
	Tg8psDATE_HEADER = 0x09,
	TgvUTC_LOW_HEADER  = 0xB5,   /* Least significant 16 bits of UTC time */
	TgvUTC_HIGH_HEADER = 0xB6   /* Most significant 16 bits of UTC time */
  } Tg8EventHeader;

typedef enum {
	Tg8SSC_HEADER           = 0, /* Start Super Cycle for machine */
	Tg8SUPER_CYCLE_HEADER   = 2, /* Programmable event for the LHC or */
	Tg8TELEGRAM_HEADER      = 3, /* Machine telegram event */
	Tg8SIMPLE_EVENT_HEADER  = 4, /* Programmable event for the SPS (Simple event) */
	Tg8psTCU_EVENT_HEADER   = 5  /* The PS acc. TCU (Timing Control Unit) event */
  } Tg8MachineEventHeader; /* Real event header looks like <Machine><Header> byte value */

/* Start Super Cycle headers */

#define Tg8SPS_SSC_HEADER ((Tg8SPS << 4) | Tg8SSC_HEADER)

/* Simple Events */

#define Tg8LHC_SIMPLE_HEADER ((Tg8LHC << 4) | Tg8SIMPLE_EVENT_HEADER)
#define Tg8SPS_SIMPLE_HEADER ((Tg8SPS << 4) | Tg8SIMPLE_EVENT_HEADER)
#define Tg8CPS_SIMPLE_HEADER ((Tg8CPS << 4) | Tg8SIMPLE_EVENT_HEADER)
#define Tg8PSB_SIMPLE_HEADER ((Tg8PSB << 4) | Tg8SIMPLE_EVENT_HEADER)
#define Tg8LEA_SIMPLE_HEADER ((Tg8LEA << 4) | Tg8SIMPLE_EVENT_HEADER)

/* TCU Events */

#define Tg8CPS_TCU_HEADER ((Tg8CPS << 4) | Tg8psTCU_EVENT_HEADER)
#define Tg8PSB_TCU_HEADER ((Tg8PSB << 4) | Tg8psTCU_EVENT_HEADER)
#define Tg8LEA_TCU_HEADER ((Tg8LEA << 4) | Tg8psTCU_EVENT_HEADER)

/* Telegram events */

#define Tg8LHC_TELEGRAM_HEADER ((Tg8LHC << 4) | Tg8TELEGRAM_HEADER)
#define Tg8SPS_TELEGRAM_HEADER ((Tg8SPS << 4) | Tg8TELEGRAM_HEADER)
#define Tg8CPS_TELEGRAM_HEADER ((Tg8CPS << 4) | Tg8TELEGRAM_HEADER)
#define Tg8PSB_TELEGRAM_HEADER ((Tg8PSB << 4) | Tg8TELEGRAM_HEADER)
#define Tg8LEA_TELEGRAM_HEADER ((Tg8LEA << 4) | Tg8TELEGRAM_HEADER)

#define Tg8READY_TELEGRAM 0xFE  /* End of the new telegram (RPLS) */
#define Tg8GROUPS           24  /* Number of groups in a telegram */

/* Event structutes. */
/* Any event contains a header followed by the next 3 bytes */

typedef long Tg8LongEvent; /* Events are 32 bits long */

typedef struct {
   short Event_half1;
   short Event_half2; } Tg8ShortEvent;

typedef struct {
   unsigned char Header;
   unsigned char Byte_2;
   unsigned char Byte_3;
   unsigned char Byte_4; } Tg8AnyEvent;

typedef struct {
   unsigned char Header;
   unsigned char Event_code;
   unsigned char Cycle_type;
   unsigned char Cycle_number; } Tg8SimpleEvent;

typedef struct {
   unsigned char Header;
   unsigned char Scn_low;
   unsigned char Scn_mid;
   unsigned char Scn_msb; } Tg8SscEvent;

typedef struct {
   unsigned char Header;
   unsigned char Ticks_1ms;
   unsigned char Dcare_1;
   unsigned char Dcare_2;} Tg8MillisecondEvent; /* 01 ms FF FF */

typedef struct  {
   unsigned char  Header;
   unsigned char  Tg8_second;
   unsigned char  Byte_3;
   unsigned char  Byte_4; } Tg8SecondEvent;

typedef struct  {
   unsigned char  Header;
   unsigned char  Tg8_minute;
   unsigned char  Byte_3;
   unsigned char  Byte_4; } Tg8MinuteEvent;

typedef struct  {
   unsigned char  Header;
   unsigned char  Tg8_hour;
   unsigned char  Byte_3;
   unsigned char  Byte_4; } Tg8HourEvent;

typedef struct  {
   unsigned char  Header;
   unsigned char  Tg8_day;
   unsigned char  Byte_3;
   unsigned char  Byte_4; } Tg8DayEvent;

typedef struct  {
   unsigned char  Header;
   unsigned char  Tg8_month;
   unsigned char  Byte_3;
   unsigned char  Byte_4; } Tg8MonthEvent;

typedef struct  {
   unsigned char  Header;
   unsigned char  Tg8_year;
   unsigned char  Byte_3;
   unsigned char  Byte_4; } Tg8YearEvent;

typedef struct  { /* PS timing event */
   unsigned char  Header;
   unsigned char  psYear;
   unsigned char  psMonth;
   unsigned char  psDay; } Tg8psDateEvent;

typedef struct  { /* PS timing event */
   unsigned char  Header;
   unsigned char  psHour;
   unsigned char  psMinute;
   unsigned char  psSecond; } Tg8psTimeEvent;

typedef  struct {
   unsigned char  Header;
   unsigned char  Group_number;
   short          Group_value; } Tg8TelegramEvent;

typedef struct {
   unsigned char Header;
   unsigned char Junk;
   unsigned short UtcWord;
 } TgvUtc;

/* Now make a union of all the different event types. */

typedef union {
   Tg8AnyEvent           Any;
   Tg8LongEvent          Long;
   Tg8ShortEvent         Short;
   Tg8SimpleEvent        Simple;
   Tg8SscEvent           Ssc;
   Tg8MillisecondEvent   Millisecond;
   Tg8SecondEvent        Tg8Second;
   Tg8MinuteEvent        Tg8Minute;
   Tg8HourEvent          Tg8Hour;
   Tg8DayEvent           Tg8Day;
   Tg8MonthEvent         Tg8Month;
   Tg8YearEvent          Tg8Year;
   Tg8psDateEvent        psDate;
   Tg8psTimeEvent        psTime;
   Tg8TelegramEvent      Telegram;
   TgvUtc                Utc;
} Tg8Event;

/* Wild card for event patterns */

#define Tg8DONT_CARE (unsigned char) 0xFF

/******************************************/
/* Action state used by SET_STATE command */
/******************************************/

typedef enum {	Tg8DISABLED,	 /* Action is in disable state (==0) */
		Tg8ENABLED,	 /* Action enabled (==1) */
		Tg8INTERRUPT,    /* Action will produce an interrupt */
		Tg8NO_INTERRUPT, /* Action will not produce an interrupt */
		Tg8PPMT_DISABLED,/* Disable the PPM timing */
		Tg8PPMT_ENABLED  /* Enable the PPM timing */
} Tg8ActionState;

/**************************/
/* Telegram's group types */
/**************************/

typedef enum {
  Tg8GT_NUM = 1,       /* Numeric group type */
  Tg8GT_EXC = 2,       /* Exclusive group type */
  Tg8GT_BIT = 3        /* Bit pattern group type */
} Tg8GroupType;

/**************************************************************************/
/* User table description						  */
/**************************************************************************/

#define Tg8ACTIONS   256       /* Maximal number of actions per the Tg8 card */
#define Tg8PPM_LINES Tg8GROUPS /* Maximal number of lines per PPM timing unit */

/***************************************************/
/* Action parameters set. The most important data. */
/***************************************************/

/* Control word format : */

#define Tg8CW_INT_BITN 14
#define Tg8CW_INT_BITM 0x3
/* 15-14
   INT/PULSE:
   0  0  Neither Output pulse nor VME-bus interrupt; this line is disabled
   0  1  An Output pulse is required with no VME-bus interrupt
   1  0  A VME-bus interrupt is required with no Output pulse
   1  1  A VME-bus interrupt is required AND an Output pulse is required
*/
typedef enum {
  Tg8DO_NOTHING,              /* Line is disabled */
  Tg8DO_OUTPUT,       	      /* The counter produces the output */
  Tg8DO_INTERRUPT,	      /* The counter provokes the VME Bus interrupt */
  Tg8DO_OUTPUT_AND_INTERRUPT  /* The counter produces both the output and interrupt */
} Tg8ResultMode;

#define Tg8CW_CNT_BITN 8
#define Tg8CW_CNT_BITM 0x7
/* 10-08
   COUNTER SELECTION:
   0 0 1  The 1st counter is used
   ...
   1 1 1  The 7th counter is used
   0 0 0  The 8th counter is used
*/

#define Tg8CW_START_BITN 6
#define Tg8CW_START_BITM 0x3
/* 7-6
   START CHOICE:
   0 0  Normal mode: Counting will start at the next ms
   0 1  Chained mode: Couner #N will start when the counter #N-1 fires
   1 0  External mode: Counting will start when an external pulse arrives an the front panel
   1 1  NOT ALLOWED
*/
typedef enum { Tg8CM_NORMAL,	/* Normal counter mode */
	       Tg8CM_CHAINED,	/* Chained counters */
	       Tg8CM_EXTERNAL   /* External start */
} Tg8CounterMode;


#define Tg8CW_PPML_BITN 3
#define Tg8CW_PPML_BITM 0x3
/* 4-3
   PPM LINE STATE CHOICE:
   0 0 Ordinar timing entity
   0 1 The first line of the PPM timing (e.g. PPM action)
   1 1 The rest lines of the PPM timing (e.g. its lines)
*/
typedef enum { Tg8TM_NORMAL,
	       Tg8TM_PPM =1,
	       Tg8TM_LINE=3
} Tg8TimingType;

#define Tg8CW_STATE_BITN 2
#define Tg8CW_STATE_BITM 0x1
/* 2-2
   LINE STATE CHOICE:
   0  Enabled
   1  Disabled
*/

#define Tg8CW_CLOCK_BITN 0
#define Tg8CW_CLOCK_BITM 0x3
/* 1-0
   CLOCK CHOICE:
   0 0  MS machine clock
   0 1  PS Cable Clock (NOT ALLOWED)
   1 0  External clock #1 (connected on the panel) OR 10 Mhz internal clock (dep. on jumper)
   1 1  External clock #2 (connected on the panel)
*/
typedef enum { Tg8CLK_MILLISECOND, /* 1KHz clock train on TG8 cable */
	       Tg8CLK_CABLE,       /* Cable clock */
	       Tg8CLK_X1,          /* External clock 1 OR Internal 10MHz */
	       Tg8CLK_X2           /* External clock input 2 */
} Tg8ClockTrain;

/*********************************/
/* User action description       */
/*********************************/

typedef struct {
   Tg8Event	uEvent;   /* Trigger event */
   Word		uControl; /* Control word (see above for its details) */
   Word         uDelay;   /* Delay value */
} Tg8User;

/*********************************/
/* Declaration of the PPM timing */
/*********************************/

typedef struct {
  /* PLS condition (used when Machine field is not zero) */
  Byte    Machine;    /* PLS condition: machine number (4 msb) & group type (4 lsb)*/
  Byte    GroupNum;   /* --             telegram's groups number [1..Tg8GROUPS] */
  Word    GroupVal;   /* --             group value */
} Tg8Gate;

typedef struct {
  Tg8User Action;     /* Action description(s) */
  Tg8Gate Gate;       /* PLS condition */
} Tg8PpmLine;

typedef struct {
  Tg8Event Trigger;                 /* Trigger event */
  Byte     Machine;                 /* Machine number */
  Byte     GroupNum;                /* Telegram's groups number */
  Byte     GroupType;               /* Group type */
  Byte     Dim;                     /* Dimension */
  Word     LineGv[Tg8PPM_LINES];    /* Group value for each line */
  Word	   LineCw[Tg8PPM_LINES];    /* Control word for each line */
  Word     LineDelay[Tg8PPM_LINES]; /* Delay for each line */
} Tg8PpmUser;

/************************************/
/* Declaration of the Timing Object */
/************************************/

typedef struct {
  int        Id;      /* Object's identifier */
  Tg8PpmUser Timing;  /* Timing definition part */
} Tg8Object;

typedef enum {
  Tg8SEL_STATE= 1,    /* Select line's state (enabled/disabled) */
  Tg8SEL_DELAY= 2     /* Select delay */
} Tg8Selector;

/*******************************/
/* Recording table format      */
/*******************************/

typedef struct {
  Uint   rSc;        /* The Super Cycle number */
  Uint   rOcc,       /* Occurence time from the start of the last S-Cycle */
         rOut;       /* Output time from the start of the last S-Cycle */
  Word   rNumOfTrig; /* How many times an action was triggered */
  Byte   rOvwrCnt,   /* Number of overwritten counting */
         rOvwrAct;   /* Source of the last counting overwitting (e.g. action number) */
} Tg8Recording;

/*******************************/
/* Interrupt buffer format     */
/*******************************/

typedef struct {
  Byte     iRcvErr;    /* Value of the reception error register */
  Byte     iOvwrAct;   /* Which action was overwitten (if not 0x00) */
  Byte     iAct;       /* Which action was triggered and fired */
  Byte     iSem;       /* Semaphore to solve the read/write conflicts */
} Tg8IntAction;

typedef struct {
  Tg8Event iEvent;     /* The trigger event frame */
  Uint     iSc;        /* The Super Cycle number */
  Uint     iOcc,       /* Occurence time from the start of the last S-Cycle */
           iOut;       /* Output time from the start of the last S-Cycle */
  Tg8IntAction iExt;   /* Extra data concerning the interrupted action */
} Tg8Interrupt;

#define Tg8CHANNELS 8 /* Number of channels */
#define Tg8IMM_INTS 8 /* Number of events per ms */

typedef struct {
   Tg8Interrupt CntInter[Tg8CHANNELS]; /* Counter interrupts description list */
   Tg8Interrupt ImmInter[Tg8IMM_INTS]; /* Immediate interrupts description list */
} Tg8InterruptTable;

/*******************************/
/* History table format        */
/*******************************/

#define Tg8HISTORIES 1000 /* The firmware's buffer capacity */

typedef struct {
  Tg8Event hEvent;     /* Received event frame */
  Uint     hSc;        /* The Super Cycle number */
  Uint     hOcc;       /* Occurence time from the start of the last S-Cycle */
  Byte     hRcvErr,    /* Value of the reception error register */
           hHour,      /* The current hour, */
           hMinute,    /* minute, */
           hSecond;    /* second */
} Tg8History;

/*******************************/
/* Clock table format          */
/*******************************/

#define Tg8CLOCKS 16   /* The firmware's buffer capacity */

typedef struct {
  Tg8Event cMsEvent;   /* The last valid ms frame */
  Uint     cNumOfMs;   /* Number of previous ms clock frames received without error */
  Uint     cSc;        /* The Super Cycle number */
  Uint     cOcc;       /* Occurence time from the start of the last S-Cycle */
  Byte     cRcvErr,    /* Value of the reception error register */
           cHour,      /* The current hour, */
           cMinute,    /* minute, */
           cSecond;    /* second */
} Tg8Clock;

/*******************************/
/* Auxiliary table format      */
/*******************************/

typedef struct {
  Byte     aYear,      /* The date */
           aMonth,
           aDay,
           aSpare1;    /* 32-bits alignment is required */
  Byte     aRcvErr,    /* Value of the reception error register */
           aHour,      /* The time */
           aMinute,
           aSecond;
  Word     aMilliSecond,
           aMsDrift;   /* Drift between ms clock and second clock */
} Tg8DateTime;

typedef struct {
  Tg8Event aEvent;     /* The last received event frame */
  Tg8Event aMsEvent;   /* The last received ms frame */
  Uint     aScLen;     /* The last completed S-Cycle length */
  Uint     aScTime;    /* The current value of the S-Cycle time */
  Uint     aSc;        /* The Super Cycle number */
  Uint     aNumOfSc;   /* Number of SC received since the last Reset */
  Word     aTrace;     /* Used for the firmware debugging */
  Word     aIntSrc;    /* There are 16 interrupt sources each
			  corresponds to a bit in this mask */
  Word     aNumOfBus,  /* Number of the MPC bus interrupts */
           aNumOfSpur; /* Number of the spurios interrupts */
  Word     aMbox;      /* The mailbox command code */
  short    aCoco;      /* Completion code */
  Word     aFwStat;    /* The firmware state (booting,runnung,stopped) */
  Word     aSscHeader; /* The SSC header selected (SPS or LHC) */
  Word     aNumOfEv,   /* Number of events received in the current S-Cycle */
           aPrNumOfEv; /* Number of events received in the previous S-Cycle */
  Word     aNumOfAct;  /* Number of defined actions */
  Word     aAlarms;    /* The bitmask of error conditions */
  /* Here is 32-bits alignment */
  Tg8DateTime aDt;     /* Date, time, rcv error */
  Byte     aFwVer[16]; /* The firmware version */
  Word     aSem;       /* Semaphore to solve the read/write conflicts */
  Word     aQueueSize;   /* Number of frames in the AT queue */
  Word     aMaxQueueSize;/* Maximal queue size */
  Word     aMovedFrames; /* Number of unserved frames in the AT queue at the next MS */
  Uint     aMovedScTime; /* The S-Cycle time when frame moving detected */
  Uint     aMovedSc;     /* and the Super Cycle number */
  Word     aProcFrames,  /* Number of the frames processed during the last MS */
           aModStatus;   /* The module's status as visible from the driver site */
  Tg8Event aQueue[8];    /* Incoming frames queue */
  Word     aTelegLHC [Tg8GROUPS]; /* The current LHC telegram */
  Word     aTelegSPS [Tg8GROUPS]; /* The current SPS telegram */
  Word     aTelegCPS [Tg8GROUPS]; /* The current CPS telegram */
  Word     aTelegPSB [Tg8GROUPS]; /* The current PSB telegram */
  Word     aTelegLEA [Tg8GROUPS]; /* The current LEA telegram */
  /* Alignment on 32-bits boundary */
} Tg8Aux;

/**************************************************************************/
/* Firmware status bit masks                                              */
/**************************************************************************/

typedef enum {
  Tg8FS_RUNNING  = 0x1,   /* The firmware program is running normally */
  Tg8FS_ALARMS   = 0x2    /* Fatal errors (alarms) were occured during the
			     execution of event program (Alarms location
			     will contain the error code in this case) */
} Tg8FirmwareStatus;

/**************************************************************************/
/* Alarm codes for the firmware                                           */
/**************************************************************************/

typedef enum {
   Tg8ALARM_OK       = 0,	/* No alarms */
   /* Firmware bits */
   Tg8ALARM_ISR      = 0x1,	/* There where nested MS interrupts (internal usage only!) */
   Tg8ALARM_LOST_IMM = 0x2,     /* The immediate action's interrupt was lost */
   Tg8ALARM_LOST_OUT = 0x4,     /* The counter pulse was lost */
   Tg8ALARM_MANY_PROC= 0x8,     /* The UT processes queue is full */
   Tg8ALARM_QUEUED_PROC = 0x10, /* The UT process was queued */
   Tg8ALARM_DIFF_EVN = 0x20,    /* The number of events for 2 seq. SCs are different */
   Tg8ALARM_DIFF_LEN = 0x40,    /* The SC length has been changed */
   Tg8ALARM_MOVED_PROC=0x80,    /* The UT process is not completed before the next ms */
   Tg8ALARM_MOVED_IMM =0x100,   /* The immediate interrupts where moved */
   Tg8ALARM_ACT_DISBL =0x200,   /* The action was disabled, but till is in the CAM */
   Tg8ALARM_IMMQ_OVF  =0x400,   /* Immediate actions queue overflowed */
   Tg8ALARM_MBX_BUSY  =0x800,   /* The mailbox busy, but there is the new request */
   /* Drivers bits */
   Tg8ALARM_UNCOM    = 0x1000,  /* A counter gives the VME interrupt without the appropriate
				   data in the Interrupt Table. */
   Tg8ALARM_BAD_SWITCH=0x2000,  /* The switch is in bad position */
   Tg8ALARM_INT_LOST  =0x4000   /* Interrupt info was lost due to the driver is too busy */
} Tg8Alarm;

/**************************************************************************/
/* Self test error codes                                                  */
/**************************************************************************/

typedef enum { Tg8ST_OK = 0 /* Self test is OK */
	       /* ... add more later on */
} Tg8SelfTesrError;

/**************************************************************************/
/* Error codes for Mailbox operations                                     */
/**************************************************************************/

typedef enum {
  Tg8ERR_OK = 0,                  /* No errors */
  Tg8ERR_ILLEGAL_OPERATION = -1,  /* Illegal Mbx operation */
  Tg8ERR_ILLEGAL_ARG = -2,        /* Illegal argument supplied */
  Tg8ERR_TIMEOUT     = -3,        /* Mbx timed out */
  Tg8ERR_WRONG_DIM   = -4,	  /* Too big array's dimension */
  Tg8ERR_WRONG_DELAY = -5,        /* Bad counter (==0xFFFF) */
  Tg8ERR_WRONG_CLOCK = -6,        /* Bad clock type */
  Tg8ERR_WRONG_MODE  = -7,        /* Bad mode */
  Tg8ERR_REJECTED    = -8,        /* Operation was rejected by the driver/Timeout */
  Tg8ERR_BAD_OBJECT  = -9,        /* Bad firmware object format */
  Tg8ERR_NO_ACK      = -10,       /* There is no acknowledge from boot program */
  Tg8ERR_NOT_RUN     = -11,       /* The firmware is not running */
  Tg8ERR_NO_FILE     = -12,       /* No such the firmware executable file */
  Tg8ERR_PENDING     = -13,       /* The previous request pending */
  Tg8ERR_BUS_ERR     = -14,       /* VME BUS Error arised */
  Tg8ERR_FIRMWARE    = -15,       /* Tg8 Firmware Error arised */
  Tg8ERR_TABLE_FULL  = -16,       /* The User Table is full */
  Tg8ERR_BAD_VECT    = -17,       /* Bad interrupt vector position */
  Tg8ERR_BAD_SWITCH  = -18,       /* Bad switch position */
  Tg8ERR_BAD_TRIGGER = -19        /* Bad action's trigger (hdr=0xff) */
} Tg8Error;

/**************************************************************************/
/* Status block                                                           */
/**************************************************************************/

typedef struct {
  Tg8DateTime Dt;    /* RcvErr, date, time */
  Uint ScTime;       /* Time in the S-Cycle */
  Uint Epc;          /* Exception PC */
  Word Evo;          /* Exception Vector */
  Word Hw;           /* Hardware status */
  Word Fw;           /* Firmware status */
  Word Cc;           /* Completion code */
  Word Am;           /* Alarms bitmask */
  Word SpareWS;
} Tg8StatusBlock;

/********************************/
/* Module Software status bits  */
/********************************/

typedef enum {
  Tg8DrvrMODULE_ENABLED            = 0x00000001,
  Tg8DrvrMODULE_FIRMWARE_LOADED    = 0x00000004,
  Tg8DrvrMODULE_USER_TABLE_VALID   = 0x00000008,
  Tg8DrvrMODULE_RUNNING            = 0x00000010,
  Tg8DrvrMODULE_FIRMWARE_ERROR     = 0x00000020,
  Tg8DrvrMODULE_HARDWARE_ERROR     = 0x00000040,
  Tg8DrvrMODULE_SWITCH_SET         = 0x00000080,
  Tg8DrvrMODULE_TIMED_OUT          = 0x00000100,
  Tg8DrvrDRIVER_DEBUG_ON           = 0x00000200,
  Tg8DrvrDRIVER_ALARMS             = 0x00000400
} Tg8DrvrSoftStatus;

#endif

/* eof */
