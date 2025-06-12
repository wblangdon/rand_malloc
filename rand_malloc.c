/* WB Langdon UCL.AC.UK 17 May 2025 generate sequence of malloc and free */

/*PATH /opt/rh/devtoolset-10/root/usr/bin:"$PATH" */
/*compile gcc -g -o rand_malloc -O3 rand_malloc.c */
/*run: rand_malloc seed times */

/*Traditionally would have piped output into malloc_info.awk
  rand_malloc seed times | gawk -f malloc_info.awk -v fit=1
  but now peak_malloc is last thing output in a successful run.

  To reduce volume of output, at compilation time,
  Malloc_info() display_mallinfo2() or display_mallinfo() and pstatm()
  can be excluded from stats() output.

  ok to ignore gcc -fvar-tracking-assignments warning
*/

/*Modifications:
 *WBL  8 Jun 2025 bugfix add R() (note dhat_summary.awk r1.40 all type 1)
 *WBL  5 Jun 2025 Replace if/else with order
 *WBL  5 Jun 2025 dhat_summary.awk r1.31 tried deterministic order for DHAT's node with 1 block
 *WBL  3 Jun 2025 increase SIZE, thrsh for speedup
 *WBL  2 Jun 2025 Addr, settime bounds check
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

#define SIZE 10000000
#define WIDTH 20

int I = 0;
int Time = 0;
int Max = 0; //time of last block to free
int* use[SIZE];
int  time[SIZE*WIDTH];

#include "order.c"

//accounting info
int Malloc = 0; //bytes on heap
int Max_malloc = 0;
long long int Talloc = 0;
int Nalloc = 0;
int Nfree = 0;
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

int Rand(const double mean){
  const double i5 = (double)rand() + (double)rand() +
                    (double)rand() + (double)rand() + (double)rand();
  const double a = (i5*mean)/(RAND_MAX*2.5);
  const int  ans = (mean>=1.0 && a < 1.0)? 1 : (int)(0.5+a);
  //printf("Rand(%g) %g %d\n",mean,i5/(RAND_MAX*5.0),ans);
  assert(a>0.0);
  return ans;
}

int last_settime = 0;
int maxwidth     = -1;
int settime(const int index, const int value){
  assert(index>=0);
  assert(value>=0);
  if(index >= SIZE) {
    last_settime = index;
    return 0; //HACK ignore stuff in far future...
  }
  for(int i=0;i<WIDTH;i++){
    const int x = index*WIDTH + i;
    if(time[x] == -1) {time[x] = value;
#ifdef DEBUG
      printf("settime(%d,%d) time[%d]=%d\n",index,value,x,time[x]);
#endif /*DEBUG*/
      if(i>maxwidth) maxwidth=i;
      return i; //i only for debug
    }
  }
  fprintf(stderr, "settime(%d,%d) all %d space used\n",index,value,WIDTH);
  exit(1);
}
int gettime(const int index, const int idx2){
  assert(index>=0);
  assert(idx2 >=0);
  if(index >= SIZE) return -1; //nothing stored past end of time array
  if(idx2 >= WIDTH) return -1; //idx2 == WIDTH means end of data in time[index,idx2]
  const int x = index*WIDTH + idx2;
#ifdef DEBUG
  if(index<15) printf("gettime(%d,%d) time[%d]=%d\n",index,idx2,x,time[x]);
#endif /*DEBUG*/
  return time[x];
}

#ifdef DEBUG
void add(const int size, const int life, const char* text){
#else
void add(const int size, const int life){
#endif /*DEBUG*/
  assert(size >  0);
  assert(life >= 0);
  assert(I >= 0 && I < SIZE);
  use[I]  = malloc(size);
  Talloc += size;
  Nalloc++;
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

void Addr(const double size, const double life){
#ifdef DEBUG
  char buf[80]; sprintf(buf,"Addr(%g,%g)",size,life);
  add(Rand(size),Rand(life),buf);
#else
  add(Rand(size),Rand(life));
#endif /*DEBUG*/
}

int lasttime = -1; //sanity check only
void Free(void){
#ifdef DEBUG
  if(lasttime+1 != Time || Time<15) {
    printf("Free() Time %d lasttime %d\n",Time,lasttime); fflush(stdout);}
#endif /*DEBUG*/
  assert(lasttime+1 == Time); lasttime = Time;
  int i;
  for(int x=0;(i=gettime(Time,x))>=0;x++){
#ifdef DEBUG
    printf("free Time %d time[%d,%d]=%d Size[%d]=%d Malloc=%d Max_malloc %d\n",
	   Time,Time,x,i,i,Size[i],Malloc,Max_malloc); fflush(stdout);
#endif /*DEBUG*/
    assert(i>= 0 && i<SIZE);
    assert(x<=WIDTH);
    assert(i>=0 && i <= Time);
    assert(use[i]);
    Malloc -= Size[i];
    free(use[i]);
    Nfree++;
    use[i]  = NULL;
    assert(Malloc >= 0);
  }
}

static inline /*https://stackoverflow.com/questions/19068705/undefined-reference-when-calling-inline-function*/
int F(double* f, const double ff){
  const int ans = (*f < ff);
  *f = *f - ff;
  return ans;
}

int R(const double x) {return 0.5+x;}
int main(int argc, char ** argv) {
  const int seed  = (argc>1)? atoi(argv[1]) : 17082132;
  const int total = (argc>2)? atoi(argv[2]) : 1;
  const int thrsh = (argc>3)? atoi(argv[3]) : 0; //speed up by not calling stats before thrsh
  printf("%s $Revision: 1.36 $ seed=%d %d %d %dx%d %d ",
	 argv[0],seed,total,thrsh,SIZE,WIDTH,norder);
  //based on Linux man-pages 6.04 2023-03-30 gnu_get_libc_version(3)
  printf("glibc %s ", gnu_get_libc_version());
  printf("%s\n",      gnu_get_libc_release());
  if(seed>0) srand(seed);
  memset(use, 0,SIZE*sizeof(int*));
  memset(time,0xff,SIZE*WIDTH*sizeof(int));
  //#include "probabilities.c"
  //#include "x.c"
  int last = 0;
  int i=0;
  for(;(i<norder || Time<=Max) && i<SIZE;i++){
    //printf("i %d, Time %d, Max %d, total %d, norder %d, SIZE %d\n",i,Time,Max,total,norder,SIZE);
    if((i%100000) == 0 || last != Max_malloc) {
      last = Max_malloc; if(Max_malloc>=thrsh) stats();
    }
    if(i < total && i < norder) {
    //printf("order[%d] %d,%f,%f\n",
    //	     i,order[i].type,order[i].size,order[i].life);
    switch(order[i].type){
    case 1:{ Add (R(order[i].size),R(order[i].life)); break;}
    case 2:{ Add2(R(order[i].size),  order[i].life);  break;}
    case 3:{ Addr(  order[i].size,   order[i].life);  break;}
    default:
      fprintf(stderr, "order[%d] bad type %d\n",i,order[i].type);
      exit(1);
    }}
    Free(); Time++;
  }
  //printf("end loop ");
  //printf("i %d, Time %d, Max %d, total %d, norder %d, SIZE %d\n",i,Time,Max,total,norder,SIZE);
  stats();
  if(last_settime < 0 || last_settime >= SIZE) {
    printf("settime index %d out of bounds %d (not all heap will be freed)\n",
	   last_settime,SIZE);
  }
  const int last_rand = rand();
  printf("%d steps (maxwidth %d, last random number %d)\n",Time+1,maxwidth+1,last_rand);
  printf("Last free at %d, malloc event limit %d\n",Max,norder);
  printf("Total allocated %lld, %d mallocs %d frees, Max_malloc %d bytes, %d still on heap\n",
	 Talloc,Nalloc,Nfree,Max_malloc,Malloc);
  //format answer like malloc_info.awk r1.6
  printf("peak_malloc %d bytes\n",peak_malloc);
  return 0;
}
