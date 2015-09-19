/* Patrick's DEBUG printing constant... */
#define DEBUG 0


typedef struct procStruct procStruct;

typedef struct procStruct * procPtr;



struct procStruct {
   procPtr         nextProcPtr;
   procPtr         childProcPtr;
   procPtr         nextSiblingPtr;
   char            name[MAXNAME];     /* process's name */
   char            startArg[MAXARG];  /* args passed to process */
   USLOSS_Context  state;             /* current context for process */
   short           pid;               /* process id */
   int             priority;
   int (* start_func) (char *);   /* function where process begins -- launch */
   char           *stack;
   unsigned int    stackSize;
   int             status;   
   int             childStatus;     /* READY, BLOCKED, QUIT, etc. */
   int             parentPid;  
   int             numChildren;
   int             isZapped;
   int             pidOfZapper;
   int 			       sliceStartTime;
   int			       runTime;
   procPtr         nextZapper;
};

/* 
Indicates the status of the USLOSS processor

accessed via USLOSS_PsrGet and USLOSS_PsrSet
*/
struct psrBits {
    unsigned int curMode:1; //1 if the processor is in kernal mode, 0 otherwise
    unsigned int curIntEnable:1; //1 if interrupts are enabled, 0 otherwise
    unsigned int prevMode:1; //curMode goes here when iterrupt occurs
    unsigned int prevIntEnable:1; //curIntEnabled goes here when interupt occurs
    unsigned int unused:28;
};

union psr_values {
   struct psrBits bits;
   unsigned int integerPart;
};

/* Some useful constants.  Add more as needed... */
#define NO_CURRENT_PROCESS NULL
#define NO_PID -1
#define EMPTY 0
#define MAXTIME 80000
#define MINPRIORITY 5
#define MAXPRIORITY 1
#define SENTINELPID 1
#define SENTINELPRIORITY (MINPRIORITY + 1)

//ProcStruct Status constants
#define READY 1
#define QUIT 2
#define JOINBLOCKED 3
#define ZAPBLOCKED 4
#define ZOMBIE 5
#define RUNNING 6
#define RELEASE_BLOCKED 13


