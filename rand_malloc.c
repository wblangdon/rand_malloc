/* WB Langdon UCL.AC.UK 17 May 2025 generate sequence of malloc and free */

/*compile gcc -g -o rand_malloc -O3 rand_malloc.c */
/*run: rand_malloc seed times */

/*Traditionally would have piped output into malloc_info.awk
  rand_malloc seed times | gawk -f malloc_info.awk -v fit=1
  but now peak_malloc is last thing output in a successful run.

  To reduce volume of output, at compilation time,
  Malloc_info() display_mallinfo2() or display_mallinfo() and pstatm()
  can be excluded from stats() output.
*/

/*Modifications:
 *WBL 29 May 2025 Display last random number
 *WBL 25 May 2025 speed up by using time[] as back pointer(s). Add peak_malloc
 *WBL 18 May 2025 Add malloc_info and statm
 *WBL 18 May 2025 Add peak use instrumentation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>

#include <gnu/libc-version.h>

//line1419 39456 blocks avg size 512 bytes, avg lifetime 3612.22 instrs
/*
gawk '{N[$6]++;total+=$2;block[$6]+=$2;life[$6]+=$10}END{for(i in N)print i,block[i],block[i]/total,life[i]/N[i];print "total",total}' dhat.8983.dat
16 203864 0.0946823 12980.2
27 203864 0.0946823 260.05
32 545895 0.253535 3764.55
48 119709 0.0555975 233
64 159165 0.0739224 3604.25
65 203726 0.0946182 77
72 159166 0.0739228 5513.67
96 159165 0.0739224 3682.25
168 119709 0.0555975 14756.3
272 119709 0.0555975 15255
512 159165 0.0739224 3529.25
total 2153137
*/

//based on test_prog.c r1.22
/*https://man7.org/linux/man-pages/man3/malloc_info.3.html*/
void Malloc_info(void) {
  /*According to malloc_info.3.html we could use open_memstream
    to send XML to a buffer and then parse it but
    likely to dramatically change heap, so send to stdout*/
  const int e = malloc_info(0,stdout); //XML style output
  if(e) {perror("error malloc_info "); exit(1);}
}
//end test_prog.c

#if __GLIBC__ > 2  || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 33)
/*https://stackoverflow.com/questions/40878169/64-bit-capable-alternative-to-mallinfo
  In glibc 2.33 and later there is a mallinfo2() function that has 64-bit field sizes:*/
//based on Linux man-pages 6.04 2023-03-30 mallinfo(3)
static void display_mallinfo2(void) {
  struct mallinfo2 mi;

  mi = mallinfo2();

  printf("Total non-mmapped bytes (arena):       %zu\n", mi.arena);
  printf("# of free chunks (ordblks):            %zu\n", mi.ordblks);
  printf("# of free fastbin blocks (smblks):     %zu\n", mi.smblks);
  printf("# of mapped regions (hblks):           %zu\n", mi.hblks);
  printf("Bytes in mapped regions (hblkhd):      %zu\n", mi.hblkhd);
  printf("Max. total allocated space (usmblks):  %zu\n", mi.usmblks);
  printf("Free bytes held in fastbins (fsmblks): %zu\n", mi.fsmblks);
  printf("Total allocated space (uordblks):      %zu\n", mi.uordblks);
  printf("Total free space (fordblks):           %zu\n", mi.fordblks);
  printf("Topmost releasable block (keepcost):   %zu\n", mi.keepcost);
}
#define MALLINFO2 1
#endif /*GLIBC*/

#ifndef MALLINFO2
//MALLINFO(3) Linux Programmer's Manual MALLINFO(3)
static void display_mallinfo(void) {
  struct mallinfo mi;

  mi = mallinfo();

  printf("Total non-mmapped bytes (arena):       %d\n", mi.arena);
  printf("# of free chunks (ordblks):            %d\n", mi.ordblks);
  printf("# of free fastbin blocks (smblks):     %d\n", mi.smblks);
  printf("# of mapped regions (hblks):           %d\n", mi.hblks);
  printf("Bytes in mapped regions (hblkhd):      %d\n", mi.hblkhd);
  printf("Max. total allocated space (usmblks):  %d\n", mi.usmblks);
  printf("Free bytes held in fastbins (fsmblks): %d\n", mi.fsmblks);
  printf("Total allocated space (uordblks):      %d\n", mi.uordblks);
  printf("Total free space (fordblks):           %d\n", mi.fordblks);
  printf("Topmost releasable block (keepcost):   %d\n", mi.keepcost);
}
#endif /*no MALLINFO2*/

//based on ~/assugi/z3/z3-master/src/shell/main.cpp r1.6
//from test_prog.c
void pstatm(void) {
  FILE* fp = fopen("/proc/self/statm","r");
  if(!fp) perror("error on /proc/self/statm ");
  int c; while((c = getc(fp))!=EOF && c!='\n') fputc(c,stdout);
  fclose(fp);
}
//end ~/assugi/z3/z3-master/src/shell/main.cpp

#define SIZE 3145728
#define WIDTH 10

int I = 0;
int Time = 0;
int Max = 0; //time of last block to free
int* use[SIZE];
int  time[SIZE*WIDTH];
//accounting info
int Malloc = 0; //bytes on heap
int Max_malloc = 0;
int Size[SIZE];
int peak_malloc = 0;

void stats(){
#if(0)
  printf("Time %7d %6d %6d ",Time,Malloc,Max_malloc);
  Malloc_info();
#ifdef MALLINFO2
  display_mallinfo2();
#else
  display_mallinfo();
#endif /*MALLINFO2*/
  printf("statm ");
  pstatm();
#define display_stats 1
#endif /*display*/

#ifdef MALLINFO2
  struct mallinfo2 mi;
  mi = mallinfo2();
#else
  struct mallinfo mi;
  mi = mallinfo();
#endif /*MALLINFO2*/
  //based on malloc_info.awk r1.6
  const int mmax = mi.arena;  //Total non-mmapped bytes (arena)
  const int mmap = mi.hblkhd; //Bytes in mapped regions (hblkhd)
  const int a = mmax+mmap;
  if(a > peak_malloc) {
#ifdef display_stats
    printf(" peak_malloc %d %d",peak_malloc,a);
#endif
    peak_malloc = a;
  }
#ifdef display_stats
  printf("\n");
#endif
}

int Rand(const int mean){
  const double i5 = (double)rand() + (double)rand() +
                    (double)rand() + (double)rand() + (double)rand();
  const double ans = (i5*mean)/(RAND_MAX*2.5);
  //printf("Rand(%d) %g %g\n",mean,i5,ans);
  return 0.5+ans;
}

int settime(const int index, const int value){
  assert(index>=0 && index < SIZE);
  assert(value>=0);
  for(int i=0;i<WIDTH;i++){
    const int x = index*WIDTH + i;
    if(time[x] == -1) {time[x] = value; return i;} //i only for debug
  }
  fprintf(stderr, "settime(%d,%d) all %d space used\n",index,value,WIDTH);
  exit(1);
}
int gettime(const int index, const int idx2){
  assert(index>=0 && index < SIZE);
  assert(idx2 >=0);
  if(idx2>=WIDTH) return -1;
  const int x = index*WIDTH + idx2;
  return time[x];
}

#ifdef DEBUG
void add(const int size, const int life, const char* text){
#else
void add(const int size, const int life){
#endif /*DEBUG*/
  use[I]  = malloc(size);
  const int t = Time+life;
  const int i = settime(t,I);
  if(t > Max) Max = t;
  Size[I] = size; Malloc += Size[I];
  if(Malloc > Max_malloc) Max_malloc = Malloc;
#ifdef DEBUG
  printf("Time %d %s Max=%d malloc use[%d] time[%d,%d]=%d Rand=%d Malloc=%d Max_malloc %d\n",
	  Time, text,Max,               I,       t,i,  I,   life, Malloc,   Max_malloc);
#endif /*DEBUG*/
  I++;
}

void Add(const int size, const int life){
#ifdef DEBUG
  char buf[80]; sprintf(buf,"Add(%d,%d)",size,life);
  add(size,life,buf);
#else
  add(size,life);
#endif /*DEBUG*/
}

void Add2(const int size, const double life){
#ifdef DEBUG
  char buf[80]; sprintf(buf,"Add2(%d,%g)",size,life);
  add(size,Rand(life),buf);
#else
  add(size,Rand(life));
#endif /*DEBUG*/
}

void Free(void){
  int i;
  for(int x=0;(i=gettime(Time,x))>=0;x++){
    assert(i<SIZE);
    assert(x<=WIDTH);
    assert(i>=0 && i <= Time);
    Malloc -= Size[i];
    free(use[i]);
    use[i]  = NULL;
#ifdef DEBUG
    printf("Time %d free   time[%d,%d]=%d Malloc=%d Max_malloc %d\n",
	   Time,Time,x,i,Malloc,Max_malloc);
#endif /*DEBUG*/
    assert(Malloc >= 0);
  }
}

static inline /*https://stackoverflow.com/questions/19068705/undefined-reference-when-calling-inline-function*/
int F(double* f, const double ff){
  const int ans = (*f < ff);
  *f = *f - ff;
  return ans;
}

int main(int argc, char ** argv) {
  const int seed  = (argc>1)? atoi(argv[1]) : 17082132;
  const int total = (argc>2)? atoi(argv[2]) : 1;
  printf("%s $Revision: 1.34 $ seed=%d %d %dx%d ",
	 argv[0],seed,total,SIZE,WIDTH);
  //based on Linux man-pages 6.04 2023-03-30 gnu_get_libc_version(3)
  printf("glibc %s ", gnu_get_libc_version());
  printf("%s\n",      gnu_get_libc_release());
  if(seed>0) srand(seed);
  memset(use, 0,SIZE*sizeof(int*));
  memset(time,0xff,SIZE*WIDTH*sizeof(int));
  int last = 0;
  int last_rand = -1;
  for(int i=0;i<=Max && i<SIZE;Free(),i++){
    Time = i;
    if((i%100000) == 0 || last != Max_malloc){last = Max_malloc; stats();}
    if(i>=total) continue;
    last_rand = rand();
    double f = last_rand/(double)RAND_MAX;
         if(F(&f,0.0946823)) Add2(16,12980.2); 
    else if(F(&f,0.0946823)) Add (27,260); 
    else if(F(&f,0.253535 )) Add2(32,3764.55); 
    else if(F(&f,0.0555975)) Add (48,233); 
    else if(F(&f,0.0739224)) Add2(64,3604.25); 
    else if(F(&f,0.0946182)) Add (65,77); 
    else if(F(&f,0.0739228)) Add2(72,5513.67); 
    else if(F(&f,0.0739224)) Add2(96,3682.25); 
    else if(F(&f,0.0555975)) Add2(168,14756.3); 
    else if(F(&f,0.0555975)) Add2(272,15255); 
    else if(F(&f,0.0739224)) Add2(512,3529.25); 
    else {printf("OPPS %f\n",f); return 1;}
  }
  stats();
  printf("%d Max_malloc %d bytes (last random number %d)\n",Time,Max_malloc,last_rand);
  //format answer like malloc_info.awk r1.6
  printf("peak_malloc %d bytes\n",peak_malloc);
  return 0;
}
