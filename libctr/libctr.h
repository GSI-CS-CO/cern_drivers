/**
 * Julian Lewis Wed 28th March 2012 BE/CO/HT
 * julian.lewis@cern.ch
 *
 * CTR timing library definitions.
 * This library is to be used exclusivlely by the new open CBCM timing library.
 * It is up to the implementers of the open CBCM timing library to provide any
 * abstraction layers and to hide this completely from user code.
 */

#include <sys/time.h>
#include <ctrdrvr.h>

/**
 * As this library runs exclusivley on Linux I use standard kernel coding
 * style and error reporting where possible. It is available both as a shared
 * object and as a static link. It exports the ctrdrvr public definitions
 * which follow the old OSF-Motif coding style.
 * In all cases if the return value is -1, then errno contains the error number.
 * The errno variable is per thread and so this mechanism is thread safe.
 * A return of zero or a posative value means success.
 * Error numbers are defined in errno.h and there are standard Linux
 * facilities for treating them. See err(3), error(3), perror(3), strerror(3)
 */

#define CTR_ERROR (-1)

/**
 * @brief Get a handle to be used in subsequent library calls
 * @return The handle to be used in subsequent calls or -1
 *
 * The ctr_open call returns a pointer to an opeaque structure
 * defined within the library internal implementation. Clients
 * never see what is behind the void pointer.
 *
 * The returned handle is -1 on error otherwise its a valid handle.
 * On error use the standard Linux error functions for details.
 *
 * Each time ctr_open is called a new handle is allocated, due to the
 * current ctr driver implementation there can never be more than
 * 16 open handles at any one time (this limitation should be removed).
 *
 *  void *my_handle;
 *  my_handle = ctr_open();
 *  if ((int) my_handle == CTR_ERROR)
 *          perror("ctr_open error");
 *
 */

void *ctr_open();

/**
 * @brief Close a handle and free up resources
 * @param A handle that was allocated in open
 * @return Zero means success else -1 is returned on error, see errno
 *
 * This routine disconnects from all interrupts, frees up memory and
 * closes the ctr driver. It should be called once for each ctr_open.
 */

int ctr_close(void *handle);

/**
 * @brief Get the number of installed CTR modules
 * @param A handle that was allocated in open
 * @return The installed module count or -1 on error
 */

int ctr_get_module_count(void *handle);

/**
 * @brief Set the current working module number
 * @param A handle that was allocated in open
 * @param modnum module number 1..n  (n = module_count)
 * @return Zero means success else -1 is returned on error, see errno
 *
 * A client owns the handle he opened and should use it exclusivley
 * never giving it to another thread. In this case it is thread safe
 * to call set_module for your handle. All subsequent calls will work
 * using the set module number.
 */

int ctr_set_module(void *handle, int modnum);

/**
 * @brief Get the current working module number
 * @param A handle that was allocated in open
 * @return module number 1..n or -1 on error
 */

int ctr_get_module(void *handle);

/**
 * @brief Connect to a ctr interrupt
 * @param A handle that was allocated in open
 * @param ctr_class see CtrDrvrConnectionClass, the class of timing to connect
 * @param equip is class specific: hardware mask, ctim number or ltim number
 * @return Zero means success else -1 is returned on error, see errno
 *
 * In the case of connecting to a ctim event you create the ctim first and 
 * pass this id in parameter here
 *
 *  Connect to the PPS hardware event on module 2
 *
 *  CtrDrvrConnectionClass ctr_class = CtrDrvrConnectionClassHARD;
 *  CtrDrvrInterruptMask hmask       = CtrDrvrInterruptMaskPPS;
 *  int modnum                       = 2;
 *
 *  if (ctr_set_module(handle,modnum) < 0) ...
 *  if (ctr_connect(handle,ctr_class,(int) hmask) < 0) ...
 */

int ctr_connect(void *handle, CtrDrvrConnectionClass ctr_class, int equip);

/**
 * @brief Connect to a ctr interrupt with a given payload
 * @param A handle that was allocated in open
 * @param ctim you want to connect to.
 * @param payload that must match the CTIM event (equality)
 * @return Zero means success else -1 is returned on error, see errno
 *
 * In the case of connecting to a ctim event you create the ctim first and 
 * pass this id in parameter here
 *
 *  Connect to the millisecond CTIM at C100 on module 1
 *
 *  int ctim    = 911; /* (0x0100FFFF) Millisecond C-Event with wildcard */
 *  int payload = 100; /* C-time to be woken up at i.e. C100 */
 *  int modnum  = 1;   /* Module 1 */
 *
 *  if (ctr_set_module(handle,modnum) < 0) ...
 *  if (ctr_connect_payload(handle,ctim,payload) < 0) ...
 */

int ctr_connect_payload(void *handle, int ctim, int payload);

/**
 * @brief Disconnect from an interrupt on current module
 * @param A handle that was allocated in open
 * @param ctr_class the calss of timing to disconnect
 * @param mask the class specific, hardware mask, ctim or ltim number
 * @return Zero means success else -1 is returned on error, see errno
 *
 * The client code must remember what it is connected to in order to disconnect.
 */

int ctr_disconnect(void *handle, CtrDrvrConnectionClass ctr_class, int mask);

/**
 * @brief Wait for an interrupt
 * @param A handle that was allocated in open
 * @param Pointer to an interrupt structure
 * @return Zero means success else -1 is returned on error, see errno
 */

struct ctr_time_s {
	struct timeval time;    /** Standard Linux time value */
	int ctrain;             /** Corresponding ctrain value */
	int machine;            /** Machine of ctrain */
};

struct ctr_interrupt_s {
	CtrDrvrConnectionClass  ctr_class; /** CTR interrupt class */
	int equip;                         /** LTIM id, hardware mask, CTIM id */
	int payload;                       /** 16-Bit payload of the event if CTIM */
	int modnum;                        /** Module number of interrupting device */
	ctr_time_s end;                    /** Time of end of action */
	ctr_time_s trigger;                /** Trigger time of action */
	ctr_time_s start;                  /** Counter start time */
};

int ctr_wait(void *handle, struct ctr_interrupt_s *ctr_interrupt);

/**
 * Go to a lemo connector 1..8 and invite signals from this list
 */

typedef enum {
	CTR_LEMO_OUT_1 = 0x001, /* Output:  Counter 1        Lemo state */
	CTR_LEMO_OUT_2 = 0x002, /* Output:  Counter 2        Lemo state */
	CTR_LEMO_OUT_3 = 0x004, /* Output:  Counter 3        Lemo state */
	CTR_LEMO_OUT_4 = 0x008, /* Output:  Counter 4        Lemo state */
	CTR_LEMO_OUT_5 = 0x010, /* Output:  Counter 5        Lemo state */
	CTR_LEMO_OUT_6 = 0x020, /* Output:  Counter 6        Lemo state */
	CTR_LEMO_OUT_7 = 0x040, /* Output:  Counter 7        Lemo state */
	CTR_LEMO_OUT_8 = 0x080, /* Output:  Counter 8        Lemo state */
	CTR_LEMO_XST_1 = 0x100, /* Input:   External Start 1 Lemo state */
	CTR_LEMO_XST_2 = 0x200, /* Input:   External Start 2 Lemo state */
	CTR_LEMO_XCL_1 = 0x400, /* Input:   External Clock 1 Lemo state */
	CTR_LEMO_XCL_2 = 0x800  /* Input:   External Clock 2 Lemo state */
 } ctr_lemo_mask_t;

typedef struct {
	int enable;                         /* Enable = 1, Disable = 0 */
	CtrDrvrCounterStart start;          /* The counters start. */
	CtrDrvrCounterMode mode;            /* The counters operating mode. */
	CtrDrvrCounterClock clock;          /* Clock specification. */
	int pulse_width;                    /* Number of 40MHz ticks, 0 = as fast as possible. */
	int delay;                          /* 32 bit delay to load into counter. */
	ctr_lemo_mask_t lemo_mask;          /* Output lemo connectors mask, invited signals */
	CtrDrvrPolarity polarity;           /* Polarity of output */
	int ctim;                           /* CTIM triggering event of this action */
	int payload;                        /* 16-Bit Payload */
	CtrDrvrTriggerCondition cmp_method; /* Payload compare method */
	int counter;                        /* Counter number 1..8 (0 is a special hardware counter) */
} ctr_ccv_s;

typedef enum {
	CTR_CCV_ENABLE      = 0x001,
	CTR_CCV_START       = 0x002,
	CTR_CCV_CLOCK       = 0x004,
	CTR_CCV_PULSE_WIDTH = 0x008,
	CTR_CCV_DELAY       = 0x010,
	CTR_CCV_OUTPUT_MASK = 0x020,
	CTR_CCV_POLARITY    = 0x040,
	CTR_CCV_CTIM        = 0x080,
	CTR_CCV_PAYLOAD     = 0x100,
	CTR_CCV_CMP_METHOD  = 0x200,
	CTR_CCV_COUNTER     = 0x400
} ctr_ccv_fields_t;

/**
 * @brief Set a CCV
 * @param A handle that was allocated in open
 * @param ltim number to be set
 * @param index into ptim action array 0..size-1
 * @param ctr_ccv are the values to be set
 * @param ctr_ccv_fields to be set from ctr_ccv
 * @return Zero means success else -1 is returned on error, see errno
 */

int ctr_set_ccv(void *handle, int ptim, int index, struct ctr_ccv_s *ctr_ccv, ctr_ccv_fields_t ctr_ccv_fields);

/**
 * @brief get an ltim action setting
 * @param A handle that was allocated in open
 * @param ltim number to get
 * @param index into ltim action array 0..size-1
 * @param ctr_ccv points to where the values will be stored
 * @return Zero means success else -1 is returned on error, see errno
 */

int ctr_get_ccv(void *handle, int ltim, int index, struct ctr_ccv_s *ctr_ccv);

/**
 * @brief Create an empty LTIM object on the current module
 * @param A handle that was allocated in open
 * @param ltim number to create
 * @param size of ltim action array (PLS lines)
 * @return Zero means success else -1 is returned on error, see errno
 */

int ctr_create_ltim(void *handle, int ltim, int size);

/**
 * @brief get a telegram
 * @param index into the array of telegrams 0..7
 * @param telegram point to a short array of at least size 32
 * @return Zero means success else -1 is returned on error, see errno
 */

int ctr_get_telegram(void *handle, int index, short *telegram);

/**
 * @brief Get time
 * @param A handle that was allocated in open
 * @param ctr_time point to where time will be stored
 * @return Zero means success else -1 is returned on error, see errno
 */

int ctr_get_time(void *handle, ctr_time_s *ctr_time);

/**
 * @brief Get cable ID
 * @param A handle that was allocated in open
 * @param cid points to where id will be stored
 * @return Zero means success else -1 is returned on error, see errno
 */

int ctr_get_cid(void *handle, int *cid);

/**
 * @brief Get firmware version
 * @param A handle that was allocated in open
 * @param version points to where version will be stored
 * @return Zero means success else -1 is returned on error, see errno
 */

int ctr_get_fw_version(void *handle, int *version);

/**
 * @brief Get driver version
 * @param A handle that was allocated in open
 * @param version points to where version will be stored
 * @return Zero means success else -1 is returned on error, see errno
 */

int ctr_get_dvr_version(void *handle, int *version);

/**
 * @brief Associate a CTIM number to a Frame
 * @param A handle that was allocated in open
 * @param ctim event Id to create
 * @param mask event frame, like 0x2438FFFF (if there is a payload, set FFFF at the end)
 * @return Zero means success else -1 is returned on error, see errno
 */

int ctr_create_ctim(void *handle, int ctim, int mask);

/**
 * @brief Destroy a CTIM
 * @param A handle that was allocated in open
 * @param ctim event Id to destroy
 * @return Zero means success else -1 is returned on error, see errno
 */

int ctr_destroy_ctim(void *handle, int ctim);

/**
 * @brief Get the size of your queue for a given handle
 * @param A handle that was allocated in open
 * @return Queue size or -1 on error
 */

int ctr_queue_size(void *handle);

/**
 * @brief Turn your queue on or off
 * @param A handle that was allocated in open
 * @param flag 1=>queuing is off, 0=>queuing is on
 * @return Zero means success else -1 is returned on error, see errno
 */

int ctr_set_queue_flag(void *handle, int flag);

/**
 * @brief Get the current queue flag setting
 * @param A handle that was allocated in open
 * @return The queue flag 0..1 (QOFF..QON)  else -1 on error
 */

int ctr_get_queue_flag(void *handle);

int ctr_SetTime(int lun, ctr_Time timeToSet);
int ctr_SetCableId(int lun, unsigned long ulId);
int ctr_SetP2(int lun, unsigned long ulEnable, ctr_P2Bits outputBits);
int ctr_GetP2(int lun, unsigned long* pulEnable, ctr_P2Bits* pOutputBits);
int ctr_GetModuleEnable(int lun, unsigned long* pulEnable);
int ctr_SetModuleEnable(int lun, unsigned long ulEnable);
int ctr_SetInputDelay(int lun, unsigned long ulDelayTick);
int ctr_GetInputDelay(int lun, unsigned long* pulDelayTick);
int ctr_Memtest(int lun, unsigned long* pulMemtest);
int ctr_SetPllLocking(int lun, unsigned long lockflag);
int ctr_GetPllLocking(int lun, unsigned long* pulLockflag);
int ctr_GetClientsHandle(unsigned long* pulPidList, void** pHandleList, unsigned long* pulListSize, unsigned long ulMaxSize);//(all clients processes connected to the driver and the associated handle)
int ctr_GetConnected(void *handle, unsigned long* pulIdList, ctr_Class* pclassList, unsigned long* pulModuleList, unsigned long* pulListSize, unsigned long ulMaxSize);//(get all events connected to a handle: class and equipment id)
int ctr_GetModuleStats(int lun, ctr_ModuleStats* resStats);
int ctr_SetDebug(unsigned long level);
int ctr_GetStatus(int lun, ctr_Status* pStatus);
int ctr_Simulate(ctr_Class iclss, unsigned long equip, unsigned long module, unsigned long grnum, unsigned long grval);
int ctr_RemoteControl(unsigned long remflg, unsigned long module, unsigned long cntr, ctr_Remote rcmd, ctr_CcvMask ccvm, ctr_Ccv *ccv);
int ctr_GetRemote(int lun, unsigned long cntr, unsigned long *remflg, ctr_CcvMask *ccvm, ctr_Ccv *ccv);
int ctr_GetIoStatus(ctr_Lemo *input);
