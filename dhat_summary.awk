#WBL 17 May 2025 gawk to extract summary from dhat output $Revision: 1.40 $

#Modications:
#WBL 8 Jun 2025 Revert r1.36 then process (leafs) siblings/grandchilds in order
#  use parts of r1.37
#WBL 7 Jun 2025 r1.37 process dhat.out.8983 directly
#WBL 6 Jun 2025 Remove null entries from order
#WBL 5 Jun 2025 replace if/else with order struct
#WBL 5 Jun 2025 output 1 block leaf nodes in order of input
#WBL 3 Jun 2025 check total blocks and force bytes to match root node
#  add scaletime
#WBL 2 Jun 2025 include post processing that was done by dhat.gnu r1.1
#  and for rand_malloc.c
#  Bugfix treat (1 children) nodes as leafs, where needed include insignificant
#WBL 2 Jun 2025 Decided to treat " insignificant]" as already folded
#  into prior data. Also subsequently deal with "var" and
#  not to average similar blocks

#Usage:
#gawk -f dhat_summary.awk dhat.out.8983.txt > order.c


BEGINFILE{
  if(scaletime=="") scaletime = 500.0;
  if(seed=="") seed = 164919;
  srand(seed);
  if(norder=="") norder = 3502077;
  print "//dhat_summary.awk $Revision: 1.40 $ ",seed,scaletime,FILENAME,ENVIRON["HOST"],strftime();

  print "#define norder",norder;
  print "const struct{"
  print "int    type;  //Add=1, Add2=2 Addr=3";
  print "double size;"
  print "double life;"
  print "} order[norder] = {"; //nb C starts at 0
  header=1;
}
(index($0,"AP significance threshold:")){header=1}
($2=="AP"){header=0}
(header){print "//"$0;next}

(s=index($0," AP ")){
  ;
  Print();
  u = substr($0,s+4);
  split(u,t);
  id = sprintf("line%04d %-16s",FNR,t[1]);
  on = index($0," children") < s;
  if(on) parent = Parent(t[1]);
  #print "line"FNR,id,on,$0;
}

(index($0,"Total:")==5 && $3=="bytes" && $8=="blocks" && $16=="lifetime"){
  rootb = f($2);
  rootn = f($7);
  rootl = f($17);
  ordersize = rootn*1.5;
}
(on && s = index($0,"Total:")){total = substr($0,s)}
(on &&     index($0,"Accesses:")){fixed[id] = 1}
(on &&     index($0," insignificant]")){
  error("insignificant no longer supported",99);
}
(index($1,"aps")){error("wrong file",99)}

#Based on rand_malloc r1.34
#todo include var
END{
  ;
  if(status) exit status;
  Print();
  DO();
  print "//I="I,"ordermax",ordermax,"max malloc block size",maxsize,"bytes maxblks",maxblks,"done",done,"max life time",maxlife;
  printf("};/*end order*/\n");
  print "//total",totaln,"blocks",totalb,"bytes (avg)";
  missingb = rootb-totalb;

  if(totaln != rootn) error("totaln does not match rootn="rootn" mismatch "rootn-totaln,20);
  if(totalb < 0.999*rootb || totalb > 1.001*rootb)
    error("totalb does not match rootb="rootb" mismatch "rootb-totalb,21);
}

function Parent(text,  n,t){
  ;
  s = match(text,/^([0-9\.]+)\.[0-9]+\/[0-9]+$/,t);
  if(s!=1) error("bad Parent("text") s="s,50);
  #print "Parent("text") s="s,"returns \""t[1]"\"";
  return t[1];
}

function clear(){
  id = total = "";
}
function nearint(x){
  if((!x) || 0+x<1) error("bad nearint("x")",25);
  #if(!x) error("bad nearint("x")",25);
  #if(x==260.05) return 1;
  return (int(x+0.06)==x) || (int(x-0.06)==x);
}
function nearint_debug(x,  a,aa,b,bb){
  if((!x) || 0+x<1) error("bad nearint("x")",25);
  #if(!x) error("bad nearint("x")",25);
  #if(x==260.05) return 1;
  a = x+0.05;
  b = x-0.05;
  aa = int(x+0.05)==x;
  bb = int(x-0.05)==x;
  ans = aa || bb;
  printf("nearint(%f) a=%f,aa=%f,b=%f,bb=%f,ans=%s\n",x,a,aa,b,bb,ans);
  return ans;
}

function code(add,size,life){
  ;
  if(add="Add ") return "1,"size","life;
  if(add="Add2") return "2,"size","life;
  if(add="Addr") return "3,"size","life;
  error("bad add code("add","size","life")",32);
}

function out(i,id,N){
  ;
  if(!(i in type)) error("out("i","id","N") missing type",60);
  if(!(i in size)) error("out("i","id","N") missing size",61);
  if(!(i in life)) error("out("i","id","N") missing life",62);
  if(!(i in blks)) error("out("i","id","N") missing blks",63);
  printf("{%-22s}",code(type[i],size[i],life[i]));
  printf((1)? "," : " ");
  printf(" //%s %d of %d",id,N,blks[i]);
  printf("\n");
  totaln++;
  totalb += size[i];
}
function DO(  cont,i,id){
  ;#in sibling order until done
  print "//DO old="old" "parent" ntodo="ntodo,FNR;
  do{
    ;
    cont = 0;
    for(i=1;i<=ntodo;i++){
      ;
      if(!(i in togo)) error("DO old="old" ntodo="ntodo" missing togo["i"]",64);
      if(!(i in toid)) error("DO old="old" ntodo="ntodo" missing toid["i"]",65);
      if(togo[i]>0){
        ;
	id = toid[i];
	out(i,id,1+blks[i]-togo[i]);
	togo[i]--;
	cont++;
      }
    }
  } while(cont);
  #print "end DO old="old" ntodo="ntodo,FNR;
  for(i in togo) delete togo[i]; 
  for(i in toid) delete toid[i];
  for(i in type) delete type[i];
  for(i in size) delete size[i];
  for(i in life) delete life[i];
  for(i in blks) delete blks[i];
  ntodo = 0;
}
function new_parent(parent,old,  o,to,p,tp,m,i){
  ;
  o = split(old,   to,".");
  p = split(parent,tp,".");
  m = (o<p)? o : p;
  #print "m="m,parent,old;
  for(i=1;i<m;i++) if(to[i]!=tp[i]) return 1;
  return 0;
}
function save(id,blocks,add,siz,lif,  l,n){
  ;
  #print "save("id","blocks","add","siz","lif")";
  if(!old) old=parent;
  if(new_parent(parent,old)) {DO();old = parent;}
  l = (lif < 1e6)? sprintf("%s",lif) : sprintf("%d",lif);
  n = f(blocks);
  I++;
  if(siz  > maxsize) maxsize = siz;
  if( l+0 > maxlife) maxlife = l+0;
  if(   n > maxblks) maxblks = n;
  ntodo++;i=ntodo;
  togo[i] = n;
  toid[i] = id;
  type[i] = add;
  size[i] = siz;
  life[i] = lif;
  blks[i] = n;
  out(i,id,1);
  togo[i]--;
  #exit status=99;
}
function Var(id,total,  n,t){
  ;
  n = split(total,t);
  if(n<22) error("Var("id","total") n="n, 5);
  save(id,t[7],"Addr",f(t[13]),f(t[17])/scaletime);
}

function Normalise(id,t2,t6,t10,  l,add){ #helper for Fixed()
  ;
  if(nearint(t10)) {
    add = "Add ";
    l   = int(0.5+t10);
  } else {
    add = "Add2";
    l   = t10;
  }
  save(id,t2,add,t6,int(0.5+l/scaletime));
}

#Based on dhat.gnu r1.1
function f(x,  n,t){
  n=split(x,t,",");
  if(n>4) error("f("x") n="n,30);
  return  (n>3)? t[1]*1e9 + t[2]*1e6 + t[3]*1e3 + t[3] :
         ((n>2)? t[1]*1e6 + t[2]*1e3 + t[3] :
	 ((n>1)? t[1]*1e3 + t[2] : t[1] + 0));
}
function Fixed(id,total, n,t){ #helper for Print()
  ;
  n = split(total,t);
  if(n<22) error("Fixed("id","total") n="n, 2);
  #print "Fixed("id"..",f(t[7]),t[8],t[11],t[12],t[13],t[14],t[15],t[16],f(t[17]),t[18];
  Normalise(id,f(t[7]),f(t[13]),f(t[17]));
}
function Print(  n,t) {
  if(total=="") return;
# print id,(id && (id in fixed))? "Fixed |" : "var   |",total;
  n = split(total,t);
  if(n<22) error("Print()" id","total") n="n, 22);

  if(t[7]==1)                  Fixed(id,total);
  else if(id && (id in fixed)) Fixed(id,total);
  else                           Var(id,total);
  clear();
}
function error(text,id) {
  print "error="id,text,"line"FNR,FILENAME > "/dev/stderr";
  exit status=id;
}
