#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "pstat.h"



struct {
    struct spinlock lock;
    struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

struct pstat* pstat;

struct proc *MLFQ[4][NPROC];
int index[4] = {-1,-1,-1,-1};
int nice;

int cnt;
int cntn;
    void
pinit(void)
{
    initlock(&ptable.lock, "ptable");
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
    static struct proc*
allocproc(void)
{
    struct proc *p;
    char *sp;

    acquire(&ptable.lock);

    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
        if(p->state == UNUSED)
            goto found;

    release(&ptable.lock);
    return 0;

found:
    p->state = EMBRYO;
    p->pid = nextpid++;
    p->niceness=0;
    p->timeslice=0;
    int bl,i;
     for(i=0;i<=index[0];i++){
        if(MLFQ[0][i]->pid == p->pid && p->pid != 0 ){
            bl=1;
            break;
        }
        bl=0;

    }
    if(bl){
    }else{
        if(p->pid != 0)
            MLFQ[0][++index[0]]=p;
    }
 
      release(&ptable.lock);

    // Allocate kernel stack.
    if((p->kstack = kalloc()) == 0){
        p->state = UNUSED;
        return 0;
    }
    sp = p->kstack + KSTACKSIZE;

    // Leave room for trap frame.
    sp -= sizeof *p->tf;
    p->tf = (struct trapframe*)sp;

    // Set up new context to start executing at forkret,
    // which returns to trapret.
    sp -= 4;
    *(uint*)sp = (uint)trapret;

    sp -= sizeof *p->context;
    p->context = (struct context*)sp;
    memset(p->context, 0, sizeof *p->context);
    p->context->eip = (uint)forkret;

    return p;
}

//PAGEBREAK: 32
// Set up first user process.
    void
userinit(void)
{
    struct proc *p;
    extern char _binary_initcode_start[], _binary_initcode_size[];

    p = allocproc();

    initproc = p;
   // cprintf("init process , p->name : %s ,p-> pid : %d \n",p->name,p->pid);
    initproc->niceness = 0;
    initproc->curticks = 0;

    if((p->pgdir = setupkvm()) == 0)
        panic("userinit: out of memory?");
    inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
    p->sz = PGSIZE;
    memset(p->tf, 0, sizeof(*p->tf));
    p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
    p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
    p->tf->es = p->tf->ds;
    p->tf->ss = p->tf->ds;
    p->tf->eflags = FL_IF;
    p->tf->esp = PGSIZE;
    p->tf->eip = 0;  // beginning of initcode.S

    safestrcpy(p->name, "initcode", sizeof(p->name));
    p->cwd = namei("/");

    // this assignment to p->state lets other cores
    // run this process. the acquire forces the above
    // writes to be visible, and the lock is also needed
    // because the assignment might not be atomic.
    acquire(&ptable.lock);

    p->state = RUNNABLE;
   
    release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
    int
growproc(int n)
{
    uint sz;

    sz = proc->sz;
    if(n > 0){
        if((sz = allocuvm(proc->pgdir, sz, sz + n)) == 0)
            return -1;
    } else if(n < 0){
        if((sz = deallocuvm(proc->pgdir, sz, sz + n)) == 0)
            return -1;
    }
    proc->sz = sz;
    switchuvm(proc);
    return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
    int
fork(void)
{
    int i, pid;
    struct proc *np;

//    int nice = cpu->proc->niceness;
    int nice=0;
    // Allocate process.

    if((np = allocproc()) == 0){
        return -1;
    }

    np->niceness = nice;                    //nice 값도 parent에서 가져와야한다.
    
    
    // Copy process state from p.
    if((np->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0){
        kfree(np->kstack);
        np->kstack = 0;
        np->state = UNUSED;
        return -1;
    }
    


    np->sz = proc->sz;
    np->parent = proc;
    *np->tf = *proc->tf;

    // Clear %eax so that fork returns 0 in the child.
    np->tf->eax = 0;

    for(i = 0; i < NOFILE; i++)
        if(proc->ofile[i])
            np->ofile[i] = filedup(proc->ofile[i]);
    np->cwd = idup(proc->cwd);

    safestrcpy(np->name, proc->name, sizeof(proc->name));

    pid = np->pid;

    acquire(&ptable.lock);

    np->state = RUNNABLE;

    //추가
    //  index[np->niceness]++;
    // MLFQ[np->niceness][index[np->niceness]] = np;
    int bl;
    for(i=0;i<=index[np->niceness];i++){
        if(MLFQ[np->niceness][i]->pid == np->pid){
            bl=1;
            break;
        }
        bl=0;

    }
    if(bl){
    }else{
        if(np->pid!=0){
        index[np->niceness]++;
        MLFQ[np->niceness][index[np->niceness]]=np;
        }
    }

//    proc->state=RUNNABLE;  //만들기만하고 현재 프로세스는 부모 프로세스인가봄 
   // sched();
    if(proc->niceness > np->niceness){     //제일 중요한 줄이였다 리얼로 
        proc->state = RUNNABLE;
        sched();
    }


    release(&ptable.lock);

        return pid;     //parent: return pid of child , child:0
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
    void
exit(void)
{
    struct proc *p;
    int fd;

    if(proc == initproc)
        panic("init exiting");

    // Close all open files.
    for(fd = 0; fd < NOFILE; fd++){
        if(proc->ofile[fd]){
            fileclose(proc->ofile[fd]);
            proc->ofile[fd] = 0;
        }
    }

    begin_op();
    iput(proc->cwd);
    end_op();
    proc->cwd = 0;

    acquire(&ptable.lock);

    // Parent might be sleeping in wait().
    wakeup1(proc->parent);

    // Pass abandoned children to init.
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if(p->parent == proc){
            p->parent = initproc;
            if(p->state == ZOMBIE)
                wakeup1(initproc);
        }
    }
//    cprintf("proc name: %s , proc->pid: %d , proc-> nice : %d, proc->state : %d exit();\n",proc->name, proc->pid,proc->niceness,proc->state);
//           int j,idx=0 ;
//        for(j=0;j<=index[proc->niceness];j++){
//                if(MLFQ[proc->niceness][j]->pid == proc->pid){
//                     idx=j;
//                     break;
//                }
//        }
        int j;
        for(j=cnt;j<index[proc->niceness];j++)
              MLFQ[proc->niceness][j]=MLFQ[proc->niceness][j+1];
        index[proc->niceness]--;

    // Jump into the scheduler, never to return. 
    proc->state = ZOMBIE;
    sched();
    panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
    int
wait(void)
{
    struct proc *p;
    int havekids, pid;

    acquire(&ptable.lock);
    for(;;){
        // Scan through table looking for exited children.
        havekids = 0;
        for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
            if(p->parent != proc)
                continue;
            havekids = 1;
            if(p->state == ZOMBIE){
                // Found one.
                pid = p->pid;
                kfree(p->kstack);
                p->kstack = 0;
                freevm(p->pgdir);
                p->pid = 0;
                p->parent = 0;
                p->name[0] = 0;
                p->killed = 0;
                p->state = UNUSED;
                p->curticks=0;
                release(&ptable.lock);
                return pid;
            }
        }

        // No point waiting if we don't have any children.
        if(!havekids || proc->killed){
            release(&ptable.lock);
            return -1;
        }

        // Wait for children to exit.  (See wakeup1 call in proc_exit.)
        sleep(proc, &ptable.lock);  //DOC: wait-sleep
    }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
    void
scheduler(void)
{
    //struct proc *p;
    for(;;){
        // Enable interrupts on this processor.
        sti();
        acquire(&ptable.lock);
          
        
        int i;
        for(i=0;i<4;i++){
            if(index[i] != -1){
                int j ;
                for(j=0;j<=index[i];j++){
                    if(MLFQ[i][j]->state != RUNNABLE)
                        continue;
                        
                        cnt=j;
                        cntn = i;
                        

                        proc=MLFQ[cntn][cnt];
                        switchuvm(MLFQ[cntn][cnt]);
                        MLFQ[cntn][cnt]->state = RUNNING;
                        swtch(&cpu->scheduler,MLFQ[cntn][cnt]->context);
                        switchkvm();
                        
                        
                        //MLFQ[cntn][cnt]->state = RUNNABLE;
                // Process is done running for now.
                // It should have changed its p->state before coming back.
                        proc = 0;    //이거 없애니까 쓸데없는거 다 없어졌다.
                      //  break;    
                }
           }
           // break;
        }

           release(&ptable.lock);
    }

}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
    void
sched(void)
{
    int intena;

    if(!holding(&ptable.lock))
        panic("sched ptable.lock");
    if(cpu->ncli != 1)
        panic("sched locks");
    if(proc->state == RUNNING)
        panic("sched running");
    if(readeflags()&FL_IF)
        panic("sched interruptible");
    intena = cpu->intena;
    swtch(&proc->context, cpu->scheduler);
    cpu->intena = intena;
}

// Give up the CPU for one scheduling round.
    void
yield(void)
{
    acquire(&ptable.lock);  //DOC: yieldlock
    proc->state = RUNNABLE;

    proc->curticks=0;
        if(proc->niceness < 3 ){
            proc->niceness++;    

            //이전 priority에서 삭제 

           int j ;
            for(j=cnt;j<index[proc->niceness-1];j++){
                MLFQ[proc->niceness-1][j]=MLFQ[proc->niceness-1][j+1];
            }
            index[proc->niceness-1]--;

            //다음 priority 에 추가 
            MLFQ[proc->niceness][++index[proc->niceness]] = proc;
        }else if(proc->niceness == 3){
            proc->niceness = 3;
            index[3]++;
            MLFQ[3][index[3]]=proc;
            
            int j; 
            for(j=cnt;j<index[3];j++){
                MLFQ[3][j]=MLFQ[3][j+1];
            }
            index[3]--;
         }
    //============= 추가 =============//
    sched();
    release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
    void
forkret(void)
{
    static int first = 1;
    // Still holding ptable.lock from scheduler.
    release(&ptable.lock);

    if (first) {
        // Some initialization functions must be run in the context
        // of a regular process (e.g., they call sleep), and thus cannot
        // be run from main().
        first = 0;
        iinit(ROOTDEV);
        initlog(ROOTDEV);
    }

    // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
    void
sleep(void *chan, struct spinlock *lk)
{
    if(proc == 0)
        panic("sleep");

    if(lk == 0)
        panic("sleep without lk");

    // Must acquire ptable.lock in order to
    // change p->state and then call sched.
    // Once we hold ptable.lock, we can be
    // guaranteed that we won't miss any wakeup
    // (wakeup runs with ptable.lock locked),
    // so it's okay to release lk.
    if(lk != &ptable.lock){  //DOC: sleeplock0
        acquire(&ptable.lock);  //DOC: sleeplock1
        release(lk);
    }

    // Go to sleep.
    proc->chan = chan;
    proc->state = SLEEPING;
    sched();
  //  cprintf("process sleep :%s\n",proc->name);
    // Tidy up.
    proc->chan = 0;

    // Reacquire original lock.
    if(lk != &ptable.lock){  //DOC: sleeplock2
        release(&ptable.lock);
        acquire(lk);
    }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
    static void
wakeup1(void *chan)
{
    struct proc *p;
    //int i;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
        if(p->state == SLEEPING && p->chan == chan){
            p->state = RUNNABLE;
//            p->curticks = 0; 
//            int j ;
//            for(j=cnt;j<index[p->niceness];j++){
//                MLFQ[p->niceness][j] = MLFQ[p->niceness][j+1];
//            }
//            MLFQ[p->niceness][index[p->niceness]]=p; 
             
        }
}

// Wake up all processes sleeping on chan.
    void
wakeup(void *chan)
{
    acquire(&ptable.lock);
    wakeup1(chan);
    release(&ptable.lock);
    //cprintf("wakeup :: process name : %s , process id : %d, process nice : %d \n",proc->name,proc->pid,proc->niceness);
       
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
    int
kill(int pid)
{
    struct proc *p;

    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if(p->pid == pid){
            p->killed = 1;
            // Wake process from sleep if necessary.
            if(p->state == SLEEPING){
                p->state = RUNNABLE;
                    
          //      index[p->niceness]++;
          //      MLFQ[p->niceness][index[p->niceness]]=p;
            }
            release(&ptable.lock);
            return 0;
        }
    }
    release(&ptable.lock);
    return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
    void
procdump(void)
{
    static char *states[] = {
        [UNUSED]    "unused",
        [EMBRYO]    "embryo",
        [SLEEPING]  "sleep ",
        [RUNNABLE]  "runble",
        [RUNNING]   "run   ",
        [ZOMBIE]    "zombie"
    };
    int i;
    struct proc *p;
    char *state;
    uint pc[10];

    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if(p->state == UNUSED)
            continue;
        if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
            state = states[p->state];
        else
            state = "???";
        cprintf("%d %s %s", p->pid, state, p->name);
        if(p->state == SLEEPING){
            getcallerpcs((uint*)p->context->ebp+2, pc);
            for(i=0; i<10 && pc[i] != 0; i++)
                cprintf(" %p", pc[i]);
        }
        cprintf("\n");
    }
}

int 
getnice(int pid){

    struct proc *p;
    acquire(&ptable.lock);
    for(p=ptable.proc;p<&ptable.proc[NPROC];p++){
        if(p->pid == pid){
            release(&ptable.lock);
            return p->niceness;
        }
    }
    release(&ptable.lock);
    return -1;
}

int
setnice(int pid,int value){

    struct proc *p;
    acquire(&ptable.lock);
    if(value < 0 || value > 40){
        release(&ptable.lock);
        return -1;

    }
    //
    //
    int i,j,k;
    for(p=ptable.proc;p<&ptable.proc[NPROC];p++){
        if(p->pid == pid){

            p->niceness = value;

            break;
        }
    }

    for(i=0;i<4;i++){
        if(index[i] != -1){
            for(j=0;j<=index[i];j++){    
                if(MLFQ[i][j]->pid == p->pid){
                    if(i == value){
                        release(&ptable.lock);
                        return -1;
                    }else{
                        MLFQ[i][j]->niceness = value;
                        //curticks값도 조정을 해야할거같은데?         //나중에 다시 확인               
                        p->curticks=0;
                        MLFQ[i][j]->curticks=0;
                        proc->curticks=0;
 
                        //queue에 추가 하고
                        index[value]++;
                        MLFQ[value][index[value]]=p;
                        //  cprintf("pid:%d name:%s nice:%d\n",p->pid,p->name,p->niceness);
                        
                        //원래 queue에서 삭제 
                        for(k=j;k<index[i];k++)
                            MLFQ[i][k]=MLFQ[i][k+1];
                        index[i]--;
                        proc->state= RUNNABLE;
                        sched();
                        release(&ptable.lock);
                        //                  ps(0);
                        return 0;
                    }
                }
            }
        }
    }
    release(&ptable.lock);
    return -1;


}

void
ps(int pid){


    int i,j;
    acquire(&ptable.lock);
    cprintf("=========================================================================================================\n");
    cprintf("state 0: UNUSED ,state 1 : EMBRYO, state 2: SLEEPING, state 3:RUNNABLE, state 4:RUNNING state 5:zombie\n");
    for(i=0;i<4;i++){
        cprintf("the number of process in queue(%d):%d\n",i,index[i]+1);
        for(j=0;j<=index[i];j++){
            cprintf("queue num(nice): %d, pid: %d , name:%s , state:%d, tick :%d, total: %d  \n",i,MLFQ[i][j]->pid,MLFQ[i][j]->name,MLFQ[i][j]->state,MLFQ[i][j]->curticks,MLFQ[i][j]->timeslice);
        }
    }
    //    release(&ptable.lock);

    cprintf("=========================================================================================================\n");
    
/*    struct proc *p;
    if(pid == 0){
        for(p = ptable.proc ; p<&ptable.proc[NPROC];p++){
            if(p->state == SLEEPING)
                cprintf("%d %d SLEEPING %s tick:%d\n",p->pid, p->niceness, p->name,p->curticks);
            else if(p->state == RUNNING)
                cprintf("%d %d RUNNING %s tick:%d\n",p->pid, p->niceness, p->name,p->curticks);
            else if(p->state == RUNNABLE)
                cprintf("%d %d RUNNABLE %s tick:%d\n",p->pid, p->niceness, p->name,p->curticks);
            //        else if(p->state == UNUSED)
            //            cprintf("%d %d UNUSED %s\n",p->pid, p->niceness, p->name);
            else if(p->state == ZOMBIE)
                cprintf("%d %d ZOMBIE %s tick:%d\n",p->pid, p->niceness, p->name,p->curticks);
            else if(p->state == EMBRYO)
                cprintf("%d %d EMBRYO %s tick:%d\n",p->pid, p->niceness, p->name,p->curticks);

        }
    }else{
        for(p = ptable.proc ; p<&ptable.proc[NPROC];p++){
            if(p->pid == pid){
                if(p->state == SLEEPING)
                    cprintf("%d %d SLEEPING %s tick:%d\n",p->pid, p->niceness, p->name,p->curticks);
                else if(p->state == RUNNING)
                    cprintf("%d %d RUNNING %s tick:%d\n",p->pid, p->niceness, p->name,p->curticks);
                else if(p->state == RUNNABLE)
                    cprintf("%d %d RUNNABLE %s tick:%d\n",p->pid, p->niceness, p->name,p->curticks);
                //           else if(p->state == UNUSED)
                //               cprintf("%d %d UNUSED %s\n",p->pid, p->niceness, p->name);
                else if(p->state == ZOMBIE)
                    cprintf("%d %d ZOMBIE %s tick:%d\n",p->pid, p->niceness, p->name,p->curticks);
                else if(p->state == EMBRYO)
                    cprintf("%d %d EMBRYO %s tick:%d\n",p->pid, p->niceness, p->name,p->curticks);
            }
        }
    }*/
    release(&ptable.lock);

}

int
getpinfo(struct pstat* pst){

      int i,j;
      int count=0;
      //int idx=0;
      for(i=0;i<4;i++){
          if(index[i]!=-1){
            for(j=0;j<=index[i];j++){
                if(MLFQ[i][j]->state !=UNUSED){
                    pst->inuse[count] = 1;
                    pst->nice[count] = i;
                    pst->pid[count] = MLFQ[i][j]->pid;
                    pst->ticks[count] = MLFQ[i][j]->timeslice;
                    count++;

                }
            }
          }
      }

      //test
//      for(i=0;i<count;i++){
//            cprintf("inused: %d nice: %d pid: %d tick:%d\n",pst->inuse[i],pst->nice[i],pst->pid[i],pst->ticks[i]);
//      }
      if(count==0)
          return -1;
      return 0;
}
