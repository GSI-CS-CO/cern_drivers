/* ================================================ */
/*                                                  */
/* Tim Client / Server                              */
/* Send events to a client via a stream             */
/*                                                  */
/* Julian Lewis AB/CO/HT CERN 28th Feb 2006         */
/* EMAIL: Julian.Lewis@CERN.ch                      */
/*        Place the word "nospam" in the subject    */
/*                                                  */
/* ================================================ */

#ifndef TIM_SERVER
#define TIM_SERVER

/* =========================================== */
/* Now the usual program definitions and types */

#define BACK_LOG 8
#define OK 1
#define FAIL 0
#define MAX_ERRORS 20
#define PACKET_STRING_SIZE 512
#define MAX_EVENTS 32
#define IP_NAME_SIZE 32
#define FILE_NAME_SIZE 80
#define DEFAULT_PORT 1234
#define LOOPBACK "127.0.0.1"

#define SinSIZE (sizeof(struct sockaddr_in))

typedef enum {
   PORT,
   SERVER,
   CTIM,
   PTIM,
   DEBUG,
   REMOTE,
   OPTIONS
 } Options;

typedef struct {
   unsigned long Eqp;
   char Name[32];
 } PtmNames;


typedef struct {
   unsigned long Equipment;      /* Event doing the interrupt */
   TimLibClass   Class;          /* Class of equipment */
   unsigned long SequenceNumber; /* Packet sequence number  */
   TimLibTime    Acquisition;    /* Acquisition time */
   TimLibTime    StartCycle;     /* Start time of SPS cycle */
   unsigned long Payload;        /* Cycle USER Id */
   char          CycleName[32];  /* Name of cycle */
 } AcqPacket;

#endif

