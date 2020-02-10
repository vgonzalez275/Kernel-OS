// tools.c

#include "k-include.h"
#include "k-type.h"
#include "k-data.h"
#include "tools.h"

// clear DRAM data block, zero-fill it
void Bzero(char *p, int bytes) {
   while(bytes--) {
      *p++='\0';
   }
}

int QisEmpty(q_t *p) { // return 1 if empty, else 0
  if((p->tail)==0) {
	return 1;
  }
  else 	{
	return 0;
  } 
}

int QisFull(q_t *p) { // return 1 if full, else 0
  if((p->tail)==Q_SIZE) {
	return 1;
  }
  else {
	return 0;
  }
}

// dequeue, 1st # in queue; if queue empty, return -1
// move rest to front by a notch, set empty spaces -1
int DeQ(q_t *p) { // return -1 if q[] is empty
   int i, value;

   if(QisEmpty(p)) {
      return NONE;
   }
   value = p->q[0];
   for(i=0;i<Q_SIZE;i++) {
	if(i < p->tail-1) {
		p->q[i]=p->q[i+1];
	}
	else {
		p->q[i]=NONE;
	}
   }
   p->tail--;
   return value;

}

// if not full, enqueue # to tail slot in queue
void EnQ(int to_add, q_t *p) {
   if(QisFull(p)==1) {
      cons_printf("Panic: queue is full, cannot EnQ! to_add= %c | tail= \n", to_add);
      return;
   }
   p->q[p->tail]=to_add;
   p->tail++;
   //p->q[p->tail]=NONE;
 
}

void MemCpy(char *dst, char *src, int size) {
   int i;
   for(i=0; i<size; i++) {
	*dst=*src;
	dst++;
	src++;
   }
}

int StrCmp(char *str1, char *str2) {
   while(*str1==*str2) {
  	if(*str1=='\0' && *str2=='\0') {
		return TRUE;
	}
        str1++;
	str2++;
   }
   if(*str1== '\0' && *str2=='\0') {
	//cons_printf("You typed in fork!");
	return TRUE;
   }
   else {
	//cons_printf("You didn't type fork.");
	return FALSE;
   }
}

void Itoa(char *str, int x) {
    if(x >= 100000 || x < 0) {
        return;
    }
    if(x<10){
        str[0] = '0' + x % 10;
	str[1] = '\0';
    }
    else if(x<100){
	    str[0] = '0' + x / 10 % 10;
	    str[1] = '0' + x % 10;
	    str[2] = '\0';
    }	    
    else if(x<1000){
	    str[0] = '0' + x / 100 % 10;
	    str[1] = '0' + x / 10 % 10;
	    str[2] = '0' + x % 10;
	    str[3] = '\0';
    }else{
            str[0] = '0' + x / 1000 % 10;
            str[1] = '0' + x / 100 % 10;
            str[2] = '0' + x / 10 % 10;
            str[3] = '0' + x % 10;
	    str[4] = '\0';
    }
}


