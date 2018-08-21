// RUN: %atjitc -O2 -DBLOCKING_DRIVER %s -o %t.blocking
// RUN: %atjitc -O2 %s -o %t.default
// RUN: %time %t.blocking.time %t.blocking 7
// RUN: %time %t.default.time %t.default 7

// FIXME: we can't run this comparison reliably because
// the time here includes compile jobs still in the queue.
// plus I don't know if this is a good test cause it's so fast.

// %compareTimes %t.default.time %t.blocking.time



// NOTE: this test ensures that recompile requests are faster
// if blocking mode is not turned on.
// This test also makes sure blocking mode is off by default.


/* The Computer Language Benchmarks Game
   https://salsa.debian.org/benchmarksgame-team/benchmarksgame/

   contributed by Branimir Maksimovic
*/
#include <cstdlib>
#include <cstdio>
#include <algorithm>

#include <tuner/driver.h>

typedef unsigned char int_t;

void rotate(int_t* p, int n)
{
   int_t tmp = p[0];
   for(int i = 0; i < n; ++i)p[i]=p[i+1];
   p[n] = tmp;
}

bool next_permutation(int_t* beg, int n, int_t* c)
{
   int i = 1;
   while(i<n)
   {
      rotate(beg,i);
      if(c[i]>=i)c[i++]=0;
      else break;
   }
   if(i>=n)return false;
   ++c[i];
   return true;
}


struct Result{
   int checksum;
   int maxflips;
};

Result fannkuch(int n)
{
   Result tmp = {0};
   int i=0,permcount=0;
   int_t perm[16],tperm[16],cnt[16]={0};

   std::generate(perm,perm+n,[&i](){ return ++i; });

   do
   {
      std::copy(perm,perm+n,tperm);
      int flips = 0;
      while(tperm[0] != 1)
      {
         std::reverse(tperm,tperm+tperm[0]);
         ++flips;
      }
      tmp.checksum += (permcount%2 == 0)?flips:-flips;
      tmp.maxflips = std::max(tmp.maxflips,flips);
   }while(++permcount,next_permutation(perm,n,cnt));

   return tmp;
}

int main(int argc, char** argv)
{
   int n = 7;
   if(argc > 1)n = atoi(argv[1]);
   if(n < 3 || n > 16)
   {
      printf("n should be between [3 and 16]\n");
      return 0;
   }

   using namespace easy::options;
   using namespace std::placeholders;
   tuner::ATDriver AT;
   Result r;

   const int ITERS = 50;
   for (int i = 0; i < ITERS; i++) {

     auto const& fannkuch_tuned =
        AT.reoptimize(fannkuch, n, tuner_kind(tuner::AT_Bayes)
#ifdef BLOCKING_DRIVER
        , blocking(true)
#endif
      );
     r = fannkuch_tuned();

     // r = fannkuch(n);
   }

   printf("%d\nPfannkuchen(%d) = %d\n",r.checksum,n,r.maxflips);
}
