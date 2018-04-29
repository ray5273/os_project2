#include "types.h"
#include "stat.h"
#include "user.h"


int test1()
{
  int pid = getpid();
  setnice(pid, 2);
  int nice = getnice(pid);
  nice = getnice(pid);
  if(nice == 2) {
      printf(1, "TEST1: CORRECT\n");
      return 1;
  }
  else{
      printf(1, "TEST1: wrong... invalid nice value\n");
      return 0;
  }
}

int test2()
{
  int pid = getpid();
  int nice = getnice(pid);
  printf(1,"value :%d\n ",nice);
  if(nice != 0) {  
      printf(1, "TEST2: wrong... invalid default nice value\n");
      return 0;
  }

  setnice(pid, -10);  // invalid nice value check
  setnice(pid, 41);  // invalid nice value check
  nice = getnice(pid);
  if(nice != 0) {
      printf(1, "TEST2: wrong... invalid nice value range\n");
      return 0;
  }
  printf(1, "TEST2: CORRECT\n");
  return 1;
}

int test3()
{
  int ppid = getpid();
  setnice(ppid, 1);

  int pid = fork();
  if(pid==0) {  // child
      int cpid = getpid();
      setnice(cpid, 7);
     while(1);
      return 0;
  }
  else { // parent
      sleep(10);
      int pnice = getnice(ppid);
      int cnice = getnice(pid);
      if(pnice==1 && cnice == 7){
          printf(1, "TEST3: CORRECT\n");
          kill(pid);
          return 1;
      }
      else{
          printf(1, "TEST3: wrong... invalid parent and child nice values\n");
          kill(pid);
          return 0;
      }
  }
}
  
int test4()
{
  int pid_arr[10];

  for(int i=0;i<10;i++){
      int pid = fork();
      if(pid==0){
          while(1);
          return 0;
      }
      else{
          pid_arr[i] = pid;
      }
  }

  for(int i=0;i<10;i++){
      setnice(pid_arr[i], i);
  }

  for(int i=0;i<10;i++){
      if(i != getnice(pid_arr[i])){
          printf(1, "TEST4: wrong... invalid child nice values\n");
          for(int j=0;j<10;j++){
              kill(pid_arr[j]);
          }
          return 0;
      }
  }
  for(int j=0;j<10;j++){
      kill(pid_arr[j]);
  }
  ps(0);
  printf(1, "TEST4: CORRECT\n");
  return 1;
}

int test5(){

    int pid;
    int x=0;
    pid = fork();
    if(pid==0){
        while(1){
          x=x+1; 
//          printf(1,"1111\n");
//          ps(0);
      //    sleep(100); 
        };
        return 0;
    }
    return 1;

}

int test6(){
    int pid;
    int x=0;
    pid = fork();
    if(pid==0){
            while(1){
                x=x+2;
                printf(1,"2222\n");
                ps(0);
       //         sleep(100);
            };
           return 0;
    }
    return 1;

}
int
main(int argc, char **argv)
{
 // test2();
 // test1();
 // test4();
 //   test5();
 /*int k,n,id;
 double x = 0,z,d;

 if(argc<2)
     n=1;
 else
     n= atoi(argv[1]);
 if(n<0||n>20)
     n=2;

 if(argc<3)
     d=1.0;
 else
    d=atoi(argv[2]);
 x=0;
 id=0;
 for(k=0;k<n;k++){
        id = fork();
        if(id<0){
            printf(1,"%d failed in fork!\n",getpid());

        }else if(id>0){
            printf(1,"Parent %d creating child %d \n",getpid(),id);
            wait();
        }else {
            printf(1,"Child %d created \n",getpid());
            for(z=0;z<8000000.0;z+=d)
                x=x+3.14*89.64;
            break;
                 


        }

 }
*/if(atoi(argv[1]) == 1)
     test5();
  else if(atoi(argv[1])==2)
     test6();
  exit();
}
