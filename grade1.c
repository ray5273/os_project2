#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char **argv){
        int i=0;//, sum=0;
        int child;//, parent = getpid();
        struct pstat* pst=NULL;
        //start
        printf(1, "Start\n");
        
        if((child = fork()) == 0) { /* child executes later because new process is added to the end of the queue. */
                printf(1, "2\n"); // child's current nice value is 0, while parent's is 1, 2, or 3.
                ps(getpid());
                if(fork() == 0) { // creates grand child process
                        printf(1, "4\n");  
                        ps(getpid());
                        yield();
                } else { // child continues as its priority is higher than the parent. 
                        printf(1, "3\n");
                        
                        ps(getpid());
                        if(fork() == 0){
                                printf(1, "6\n");
                                ps(getpid());
                        } else{
                                printf(1, "5\n"); 
                                
                                ps(getpid());
                                wait();
                        }
                        wait();
                }
        }
        else {  /* parent executes first. */
                printf(1, "1\n");
                for(i=0;i<1000;i++) getpid(); // parent process' nice level should be 3.
   //             printf(1,"parent pid : %d nicevalue: %d \n",getpid(),getnice(getpid()));
                ps(getpid());
                wait();
                printf(1, "7\n");
                getpinfo(pst);
        }
        //end
        exit();
}

