#WBL 14 Apr 2026  $Revision: 1.3 $

#Modifications:
#WBL  6 Jun 2025 increase num-callers to 500
#WBL 14 May 2025 for DHAT based on run_massif.bat r1.17 use higher_order_code_209
#WBL 18 Apr 2025 Revert r1.9 (only reference massif --time-unit=ms run)
#                add higher_order_code_20900 for GI@ICSE 2025 slides
#WBL 14 Apr 2025 based on magpie/gem5/cmaes/run_cmaes.bat r1.5/r1.13
#                Sep  2  2024 fit.bat

#run on /tmp/ as it uses multiple files
#use $1 to run simultanously

if($1 == "") then
  echo "missing run parameter"
  exit 1
endif

setenv start `pwd`
setenv work  /tmp/run_dhat_gem5_{$1}/gem5

mkdir -p $work

cd $work/..
if($status) exit $status

if(-e ERROR) then
  echo `pwd` "ERROR exists" > /dev/stderr; exit 99;
endif

if(-e test.srt) then
  echo `ls -l test.srt` `pwd` "exists" > /dev/stderr;
endif


cd $start
if($status) exit $status

echo $0 '$Revision: 1.3 $' "start" $work `date` `pwd` $HOST
valgrind --version

rsync -av \
  ../../examples/code/gem5/gem5.exe \
  ../../examples/code/gem5/hello-custom-binary.py \
  ../../examples/code/gem5/higher_order_code_209 \
  ~/assugi/rnafold/ViennaRNA-2.5.1/src/ViennaRNA/utils/higher_order_code_20900 \
  ../../examples/code/gem5/fit_arg1.awk \
  $work

cd $work
if($status) exit $status

setenv debug ""

setenv PATH /opt/rh/devtoolset-10/root/usr/bin:"$PATH"
setenv PATH /opt/Python/Python-3.10.1/bin:/opt/Python/Python-3.7/bin:"$PATH"
setenv LD_LIBRARY_PATH /opt/Python/Python-3.10.1/lib:"$LD_LIBRARY_PATH"
setenv gem5    gem5.exe
setenv script  hello-custom-binary.py
setenv binary  higher_order_code_209
#setenv binary  higher_order_code_20900

unsetenv MALLOC_MMAP_MAX_
unsetenv MALLOC_PERTURB_
unsetenv MALLOC_TOP_PAD_
unsetenv MALLOC_TRIM_THRESHOLD_
unsetenv MALLOC_MMAP_THRESHOLD_
unsetenv MALLOC_ARENA_TEST_
unsetenv MALLOC_ARENA_MAX_

time \
  $gem5   $debug $script --isa X86 --binary $binary

setenv save $status
echo "gem5 no MALLOC_TRIM_THRESHOLD_ no MALLOC_MMAP_THRESHOLD_ status $status" `date`
if($save) exit $save;

grep instructions test_prog.log

#for simplicity assume massif/dhat has not broken anything

setenv G A
#  --detailed-freq=1 not useful


#https://stackoverflow.com/questions/11242795/how-to-get-the-full-call-stack-from-valgrind (num-callers)
time \
valgrind --tool=dhat --num-callers=500 \
  $gem5   $debug $script --isa X86 --binary $binary
setenv save $status

setenv save $status
echo "dhat $G status $status" `date`
if($save) exit $save;

grep instructions test_prog.log

echo "$0 done" `date`
