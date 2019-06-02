#include "param.h"
#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct shm_page {
    uint id;
    char *frame;
    int refcnt;
  } shm_pages[64];
} shm_table;

void shminit() {
  int i;
  initlock(&(shm_table.lock), "SHM lock");
  acquire(&(shm_table.lock));
  for (i = 0; i< 64; i++) {
    shm_table.shm_pages[i].id =0;
    shm_table.shm_pages[i].frame =0;
    shm_table.shm_pages[i].refcnt =0;
  }
  release(&(shm_table.lock));
}

int shm_open(int id, char **pointer) {
//you write this
  int i;
  //Case 1. Checking to see if the id of the physical page is in the table already.
  acquire(&(shm_table.lock)); 
  for (i = 0; i < 64; ++i) {
    if(shm_table.shm_pages[i].id == id) {      
      mappages(myproc()->pgdir,(void*) PGROUNDUP(myproc()->sz), PGSIZE, V2P(shm_table.shm_pages[i].frame), PTE_W|PTE_U);//create PTE
      shm_table.shm_pages[i].refcnt = +1;
      *pointer = (char *)PGROUNDUP(myproc()->sz);
      myproc()->sz += PGSIZE;
      release(&(shm_table.lock));//release here because we're done.
      return 0;
    }
  }
  
  //Case 2. If this is reached then shared memory segment doesn't exist.
  for (i = 0; i< 64; i++) { //looking for the first empty entry in table.
    if(shm_table.shm_pages[i].refcnt == 0) {  
      shm_table.shm_pages[i].id = id;  
      shm_table.shm_pages[i].frame = kalloc();//allocating page frame
      shm_table.shm_pages[i].refcnt = 1;

      //now do the same as case 1
      memset(shm_table.shm_pages[i].frame, 0, PGSIZE);//don't know why this is needed. suggested on piazza
      mappages(myproc()->pgdir,(void*) PGROUNDUP(myproc()->sz), PGSIZE, V2P(shm_table.shm_pages[i].frame), PTE_W|PTE_U);//create PTE
      *pointer = (char *)PGROUNDUP(myproc()->sz);
      myproc()->sz += PGSIZE;
      release(&(shm_table.lock));//release here because we're done.
      return 0;
    }
  }
 
  
  release(&(shm_table.lock));//Leave here in case table is full
  
  return 0; //added to remove compiler warning -- you should decide what to return
}


int shm_close(int id) {
//you write this too!
  int i;
 
  acquire(&(shm_table.lock)); 
  for (i = 0; i < 64; ++i) {
    if(shm_table.shm_pages[i].id == id) {
      shm_table.shm_pages[i].refcnt--;
      if(shm_table.shm_pages[i].refcnt > 0) 
        break;
      else {
        shm_table.shm_pages[i].id = 0;
        shm_table.shm_pages[i].frame = 0;
        shm_table.shm_pages[i].refcnt = 0;
      }
      break;
    } 
  }
  
  //reaches here when id doesn't exist as well.
  
  release(&(shm_table.lock));
  return 0; //added to remove compiler warning -- you should decide what to return
}
