/* ------------------------------------------------------------------------
   phase1.c

   University of Arizona
   Computer Science 452
   Fall 2015

   ------------------------------------------------------------------------ */

#include "phase1.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <usloss.h>
#include "kernel.h"

/* ------------------------- Prototypes ----------------------------------- */
int sentinel (char *);
extern int start1 (char *);
void dispatcher(void);
void launch();
static void enableInterrupts();
static void checkDeadlock();
void dump_processes(void);
int inKernelMode(char *procName);
void addToReadyList(procPtr toAdd);


/* -------------------------- Globals ------------------------------------- */

/* Patrick's debugging global variable... */
int debugflag = 1;

/* the process table */
procStruct ProcTable[MAXPROC];

/* Process lists  */
static procPtr ReadyList;

/* current process ID */
procPtr Current;

//number of running processess
int procAmount;

/* the next pid to be assigned */
unsigned int nextPid = SENTINELPID;


/* -------------------------- Functions ----------------------------------- */
/* ------------------------------------------------------------------------
   Name - startup
   Purpose - Initializes process lists and clock interrupt vector.
             Start up sentinel process and the test process.
   Parameters - none, called by USLOSS
   Returns - nothing
   Side Effects - lots, starts the whole thing
   ----------------------------------------------------------------------- */
void startup()
{
    int i;      /* loop index */
    int result; /* value returned by call to fork1() */

    /* initialize the process table */
    if (DEBUG && debugflag)
        USLOSS_Console("startup(): initializing process table, ProcTable[]\n");

    //Initialize specific parts of the ProcTable to be empty
    for(i = 0; i < MAXPROC; i++){
      ProcTable[i].nextProcPtr = NO_CURRENT_PROCESS;
      ProcTable[i].childProcPtr = NO_CURRENT_PROCESS;
      ProcTable[i].name[0] = '\0';
      ProcTable[i].startArg[0] = '\0';
      ProcTable[i].pid = NO_PID;
      ProcTable[i].status = EMPTY;
    }


    /* Initialize the Ready list, etc. */
    if (DEBUG && debugflag)
        USLOSS_Console("startup(): initializing the Ready list\n");
    ReadyList = NULL;

    /* Initialize the clock interrupt handler */

    /* startup a sentinel process */
    if (DEBUG && debugflag)
        USLOSS_Console("startup(): calling fork1() for sentinel\n");
    result = fork1("sentinel", sentinel, NULL, USLOSS_MIN_STACK,
                    SENTINELPRIORITY);
    if (result < 0) {
        if (DEBUG && debugflag) {
            USLOSS_Console("startup(): fork1 of sentinel returned error, ");
            USLOSS_Console("halting...\n");
        }
        USLOSS_Halt(1);
    }
  
    /* start the test tab process */
    if (DEBUG && debugflag)
        USLOSS_Console("startup(): calling fork1() for start1\n");
    result = fork1("start1", start1, NULL, 2 * USLOSS_MIN_STACK, 1);
    if (result < 0) {
        USLOSS_Console("startup(): fork1 for start1 returned an error, ");
        USLOSS_Console("halting...\n");
        USLOSS_Halt(1);
    }
    dispatcher();
    USLOSS_Console("startup(): Should not see this message! ");
    USLOSS_Console("Returned from fork1 call that created start1\n");


    return;
} /* startup */

/* ------------------------------------------------------------------------
   Name - finish
   Purpose - Required by USLOSS
   Parameters - none
   Returns - nothing
   Side Effects - none
   ----------------------------------------------------------------------- */
void finish()
{
    if (DEBUG && debugflag)
        USLOSS_Console("in finish...\n");
} /* finish */

/* ------------------------------------------------------------------------
   Name - fork1
   Purpose - Gets a new process from the process table and initializes
             information of the process.  Updates information in the
             parent process to reflect this child process creation.
   Parameters - the process procedure address, the size of the stack and
                the priority to be assigned to the child process.
   Returns - the process id of the created child or -1 if no child could
             be created or if priority is not between max and min priority.
   Side Effects - ReadyList is changed, ProcTable is changed, Current
                  process information changed
   ------------------------------------------------------------------------ */
int fork1(char *name, int (*procCode)(char *), char *arg,
          int stacksize, int priority)
{
    int procSlot = -1;

    if (DEBUG && debugflag)
        USLOSS_Console("fork1(): creating process %s\n", name);

    /* test if in kernel mode; halt if in user mode 
    if ( (USLOSS_PsrGet()&USLOSS_PSR_CURRENT_MODE)  == 0){
        USLOSS_Console("fork1() realized it's not in kernel mode. Halting... %s\n", name);
        USLOSS_Halt(1);
    }*/

    inKernelMode("fork1");

    /* Return if stack size is too small */
    if (stacksize < USLOSS_MIN_STACK){
      USLOSS_Console("fork1(): Process stack size is too small.  Halting...\n");
      return -2;
    }
    /* Return is the ProcTable is Full */
    if (procAmount >= MAXPROC){
      USLOSS_Console("fork1(): Process Table Full! Returning -1\n");
      return -1;
    }
    /* Return if priority is out of range */
    if (priority < MAXPRIORITY || priority > MINPRIORITY+1){
      USLOSS_Console("fork1(): Priority out of range! Returning -1\n");
      return -1;
    }
    /* Return if name is NULL */
    if (name == NULL){
      USLOSS_Console("fork1(): No name supplied! Returning -1\n");
      return -1;
    }
    /* Return if name is NULL */
    if (procCode == NULL){
      USLOSS_Console("fork1(): No ProcCode supplied! Returning -1\n");
      return -1;
    }

    /* Assign spot in ProcTable */

    //If the first entry is null, then the sentinel still needs to be started
    if (ProcTable[0].pid == NO_PID){
        USLOSS_Console("fork1(): ProcTable is empty, first process going in 0\n");
        procSlot = 0;
    }
    //otherise, assign the next empty slot and pid
    else{
        //ensures we only search the table once
        //a check was already done to see if it's full, so it shouldnt be
        int endValue = nextPid+MAXPROC;
        for (int i = nextPid; i < endValue; ++i)
        {
          //break on the first empty spot in the table
          if (ProcTable[i%MAXPROC].status == EMPTY)
          {
            procSlot = i%MAXPROC-1;
            break;
          }
          else{
            nextPid++;
          }
        }
    }
    //increments PIDs so they never repeat
    int newPid = nextPid;
    USLOSS_Console("fork1(): ProcTable slot %d selected\n", newPid%MAXPROC-1);
    Current = &ProcTable[newPid%MAXPROC-1];
    nextPid++;





    /* fill-in entry in process table */
    if ( strlen(name) >= (MAXNAME - 1) ) {
        USLOSS_Console("fork1(): Process name is too long.  Halting...\n");
        USLOSS_Halt(1);
    }
    //assign name to the process's proctStruct
    strcpy(ProcTable[procSlot].name, name);

    //assign args to process's procStruct
    if ( arg == NULL )
        ProcTable[procSlot].startArg[0] = '\0';
    else if ( strlen(arg) >= (MAXARG - 1) ) {
        USLOSS_Console("fork1(): argument too long.  Halting...\n");
        USLOSS_Halt(1);
    }
    else
        strcpy(ProcTable[procSlot].startArg, arg);

    //assign start function address to procStruct
    ProcTable[procSlot].start_func = procCode;

    //assign pid
    ProcTable[procSlot].pid = newPid;
    //assign priority
    ProcTable[procSlot].priority = priority;
    //assign stacksize
    ProcTable[procSlot].stackSize = stacksize;
    //assign status
    ProcTable[procSlot].status = READY;
    //assign stack, allocate the space
    ProcTable[procSlot].stack = malloc(stacksize);
    
    

    /* Initialize context for this process, but use launch function pointer for
     * the initial value of the process's program counter (PC)
     */
    procAmount++;
    USLOSS_Console("fork1(): Switching contexts to new process\n");
    USLOSS_ContextInit(&(ProcTable[procSlot].state), USLOSS_PsrGet(),
                       ProcTable[procSlot].stack,
                       ProcTable[procSlot].stackSize,
                       launch);
    /* for future phase(s) */
    p1_fork(ProcTable[procSlot].pid);

    /*
    Add to ready list
    */
    USLOSS_Console("fork1(): priority of new proccess: %d\n", ProcTable[procSlot].priority);
    addToReadyList(&ProcTable[procSlot]);

    /* More stuff to do here... */
    dump_processes();
    return newPid;
} /* fork1 */

/* ------------------------------------------------------------------------
   Name - launch
   Purpose - Dummy function to enable interrupts and launch a given process
             upon startup.
   Parameters - none
   Returns - nothing
   Side Effects - enable interrupts
   ------------------------------------------------------------------------ */
void launch()
{
    int result;

    if (DEBUG && debugflag)
        USLOSS_Console("launch(): started\n");

    /* Enable interrupts */
    //enableInterrupts();

    /* Call the function passed to fork1, and capture its return value */
    result = Current->start_func(Current->startArg);

    if (DEBUG && debugflag)
        USLOSS_Console("Process %s returned to launch\n", Current->name);

    quit(result);

} /* launch */


/* ------------------------------------------------------------------------
   Name - join
   Purpose - Wait for a child process (if one has been forked) to quit.  If 
             one has already quit, don't wait.
   Parameters - a pointer to an int where the termination code of the 
                quitting process is to be stored.
   Returns - the process id of the quitting child joined on.
             -1 if the process was zapped in the join
             -2 if the process has no children
   Side Effects - If no child process has quit before join is called, the 
                  parent is removed from the ready list and blocked.
   ------------------------------------------------------------------------ */
int join(int *code)
{
  return -1;
} /* join */


/* ------------------------------------------------------------------------
   Name - quit
   Purpose - Stops the child process and notifies the parent of the death by
             putting child quit info on the parents child completion code
             list.
   Parameters - the code to return to the grieving parent
   Returns - nothing
   Side Effects - changes the parent of pid child completion status list.
   ------------------------------------------------------------------------ */
void quit(int code)
{
    USLOSS_Console("Quit called..\n");
    p1_quit(Current->pid);
    Current->status = QUIT;
    USLOSS_Halt(0);
    dispatcher();

} /* quit */


/* ------------------------------------------------------------------------
   Name - dispatcher
   Purpose - dispatches ready processes.  The process with the highest
             priority (the first on the ready list) is scheduled to
             run.  The old process is swapped out and the new process
             swapped in.
   Parameters - none
   Returns - nothing
   Side Effects - the context of the machine is changed
   ----------------------------------------------------------------------- */
void dispatcher(void)
{
    USLOSS_Console("Dispacher called..\n");
    procPtr nextProcess = Current;
    USLOSS_ContextSwitch(NULL, &Current->state);
    p1_switch(Current->pid, nextProcess->pid);

} /* dispatcher */


/* ------------------------------------------------------------------------
   Name - sentinel
   Purpose - The purpose of the sentinel routine is two-fold.  One
             responsibility is to keep the system going when all other
             processes are blocked.  The other is to detect and report
             simple deadlock states.
   Parameters - none
   Returns - nothing
   Side Effects -  if system is in deadlock, print appropriate error
                   and halt.
   ----------------------------------------------------------------------- */
int sentinel (char *dummy)
{
    if (DEBUG && debugflag)
        USLOSS_Console("sentinel(): called\n");
    /*while (1)
    {
        checkDeadlock();
        USLOSS_WaitInt();
    }*/
} /* sentinel */


/* check to determine if deadlock has occurred... */
static void checkDeadlock()
{
} /* checkDeadlock */


/*
 * Disables the interrupts.
 */
void disableInterrupts()
{
    /* turn the interrupts OFF iff we are in kernel mode */
    if( (USLOSS_PSR_CURRENT_MODE & USLOSS_PsrGet()) == 0 ) {
        //not in kernel mode
        USLOSS_Console("Kernel Error: Not in kernel mode, may not ");
        USLOSS_Console("disable interrupts\n");
        USLOSS_Halt(1);
    } else
        /* We ARE in kernel mode */
        USLOSS_PsrSet( USLOSS_PsrGet() & ~USLOSS_PSR_CURRENT_INT );
} /* disableInterrupts */

void dump_processes(void){
    USLOSS_Console("   NAME   |   PID   |   PRIORITY   |   STATUS   |   STATE   \n");
    USLOSS_Console("------------------------------------------------------------\n");
    for(int i = 0; i < 6; i++){
    USLOSS_Console(" %-9s| %-8d| %-13d| %-10d| %-10d\n", ProcTable[i].name, ProcTable[i].pid,
      ProcTable[i].state, ProcTable[i].priority, ProcTable[i].status);  
    USLOSS_Console("------------------------------------------------------------\n");
    }
}

/*
 *checks the PSR for kernel mode
 *returns true in if its in kernel mode, and false if not
*/
int inKernelMode(char *procName){
    if( (USLOSS_PSR_CURRENT_MODE & USLOSS_PsrGet()) == 0 ) {
      USLOSS_Console("Kernel Error: Not in kernel mode, may not run %s()", procName);
      USLOSS_Halt(1);
      return 0;
    }
    else{
      return 1;
    }
}
/*
 *Adds process to ReadyList in priority queue form
 *Ensures the ReadyList pointer is always pointing to the
 *next process in line to run. 
 *
 *UNTESTED for the most part. It compiles, but no promises.
 */
void addToReadyList(procPtr toAdd){
  //add the sentinel to the ready list if nothing has been added
  if (ReadyList == NULL)
  {
    ReadyList = toAdd;
  }//end of is
  //if the priority of the new process is higher than the first
  //proc in the queue, add it to the head.
  else if (toAdd->priority < ReadyList->priority){
    toAdd->nextProcPtr = ReadyList;
    ReadyList = toAdd;
  }//end of elseif
  //otherwise, scan until it fits in 
  else{
    procPtr prev = NULL;
    for (procPtr cur = ReadyList; cur != NULL; cur = cur->nextProcPtr){
      if (cur->priority > toAdd->priority){
        prev->nextProcPtr = toAdd;
        toAdd->nextProcPtr = cur;
        break;
      }
      prev = cur;
    }
  }//end of else
}
