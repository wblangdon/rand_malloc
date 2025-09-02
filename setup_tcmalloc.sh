10787  git clone git@github.com:gperftools/gperftools.git
10792  cd gperftools
./autogen.sh
./configure --enable-minimal
ls /usr/local/lib | grep tcmalloc\n
gcc -o rand_malloc rand_malloc.c -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free -L/usr/local/lib -ltcmalloc_minimal
