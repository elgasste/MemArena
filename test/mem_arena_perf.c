#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "mem_arena.h"

typedef struct PerfSlot_t
{
   void* mem;
   size_t size;
}
PerfSlot_t;

internal size_t RandInRange( size_t min, size_t max )
{
   if ( max <= min )
   {
      return min;
   }

   return min + ( (size_t)rand() % ( max - min + 1 ) );
}

internal double ElapsedSeconds( clock_t start, clock_t end )
{
   return (double)( end - start ) / (double)CLOCKS_PER_SEC;
}

int main( int argc, char** argv )
{
   size_t arenaSize;
   size_t operations;
   size_t maxLiveAllocs;
   size_t minAllocSize;
   size_t maxAllocSize;
   unsigned int seed;

   MemArena_t* arena;
   PerfSlot_t* slots;
   size_t* liveSlots;
   size_t* freeSlots;
   size_t liveCount;
   size_t freeCount;

   size_t op;
   size_t allocAttempts;
   size_t allocSuccess;
   size_t freeOps;
   size_t bytesAllocated;
   size_t peakLiveBytes;
   size_t liveBytes;

   clock_t startTime;
   clock_t endTime;
   double seconds;
   double opsPerSec;
   MemArenaResult_t createResult;
   MemArenaStats_t stats;

   arenaSize = 16 * 1024 * 1024; // 16 MB
   operations = 250000;
   maxLiveAllocs = 4096;
   minAllocSize = 8;
   maxAllocSize = 2048;
   seed = (unsigned int)time( 0 );

   if ( argc > 1 ) arenaSize = (size_t)strtoull( argv[1], 0, 10 );
   if ( argc > 2 ) operations = (size_t)strtoull( argv[2], 0, 10 );
   if ( argc > 3 ) maxLiveAllocs = (size_t)strtoull( argv[3], 0, 10 );
   if ( argc > 4 ) minAllocSize = (size_t)strtoull( argv[4], 0, 10 );
   if ( argc > 5 ) maxAllocSize = (size_t)strtoull( argv[5], 0, 10 );
   if ( argc > 6 ) seed = (unsigned int)strtoul( argv[6], 0, 10 );

   if ( maxLiveAllocs == 0 || minAllocSize == 0 || maxAllocSize < minAllocSize )
   {
      printf( "Invalid arguments.\n" );
      printf( "Usage: mem_arena_perf [arenaBytes] [operations] [maxLiveAllocs] [minAlloc] [maxAlloc] [seed]\n" );
      return 1;
   }

   slots = (PerfSlot_t*)calloc( maxLiveAllocs, sizeof( PerfSlot_t ) );
   if ( !slots )
   {
      printf( "Failed to allocate slot table.\n" );
      return 1;
   }

   liveSlots = (size_t*)calloc( maxLiveAllocs, sizeof( size_t ) );
   if ( !liveSlots )
   {
      printf( "Failed to allocate live slot table.\n" );
      free( slots );
      return 1;
   }

   freeSlots = (size_t*)calloc( maxLiveAllocs, sizeof( size_t ) );
   if ( !freeSlots )
   {
      printf( "Failed to allocate free slot table.\n" );
      free( liveSlots );
      free( slots );
      return 1;
   }

   for ( size_t i = 0; i < maxLiveAllocs; ++i )
   {
      freeSlots[i] = i;
   }

   createResult = MemArena_Create( &arena, arenaSize );
   if ( createResult != MemArenaResult_Success )
   {
      printf( "MemArena_Create failed: %s\n", MemoryArena_GetErrorMessage( createResult ) );
      free( freeSlots );
      free( liveSlots );
      free( slots );
      return 1;
   }

   srand( seed );

   liveCount = 0;
   freeCount = maxLiveAllocs;
   allocAttempts = 0;
   allocSuccess = 0;
   freeOps = 0;
   bytesAllocated = 0;
   peakLiveBytes = 0;
   liveBytes = 0;

   startTime = clock();

   for ( op = 0; op < operations; ++op )
   {
      b32 shouldAlloc;

      if ( liveCount == 0 )
      {
         shouldAlloc = True;
      }
      else if ( liveCount >= maxLiveAllocs )
      {
         shouldAlloc = False;
      }
      else
      {
         shouldAlloc = ( rand() & 1 ) ? True : False;
      }

      if ( shouldAlloc )
      {
         size_t freeIndex;
         size_t slotIndex;
         size_t allocSize;
         void* mem;
         MemArenaResult_t allocResult;

         freeIndex = RandInRange( 0, freeCount - 1 );
         slotIndex = freeSlots[freeIndex];
         freeSlots[freeIndex] = freeSlots[freeCount - 1];
         --freeCount;

         allocSize = RandInRange( minAllocSize, maxAllocSize );
         mem = 0;
         ++allocAttempts;
         allocResult = MemArena_Alloc( arena, &mem, allocSize );

         if ( allocResult == MemArenaResult_Success )
         {
            slots[slotIndex].mem = mem;
            slots[slotIndex].size = allocSize;
            liveSlots[liveCount] = slotIndex;
            ++allocSuccess;
            ++liveCount;
            bytesAllocated += allocSize;
            liveBytes += allocSize;
            if ( liveBytes > peakLiveBytes )
            {
               peakLiveBytes = liveBytes;
            }
         }
         else
         {
            freeSlots[freeCount] = slotIndex;
            ++freeCount;
         }
      }
      else
      {
         size_t liveIndex;
         size_t slotIndex;

         liveIndex = RandInRange( 0, liveCount - 1 );
         slotIndex = liveSlots[liveIndex];
         liveSlots[liveIndex] = liveSlots[liveCount - 1];
         --liveCount;

         MemArena_Free( arena, slots[slotIndex].mem );
         liveBytes -= slots[slotIndex].size;
         slots[slotIndex].mem = 0;
         slots[slotIndex].size = 0;
         freeSlots[freeCount] = slotIndex;
         ++freeCount;
         ++freeOps;
      }
   }

   endTime = clock();
   stats = MemArena_GetStats( arena );

   /* Cleanup any remaining live allocations to leave arena consistent at exit. */
   for ( op = 0; op < maxLiveAllocs; ++op )
   {
      if ( slots[op].mem )
      {
         MemArena_Free( arena, slots[op].mem );
         slots[op].mem = 0;
         slots[op].size = 0;
      }
   }

   seconds = ElapsedSeconds( startTime, endTime );
   opsPerSec = ( seconds > 0.0 ) ? ( (double)operations / seconds ) : 0.0;

   printf( "MemArena random allocation benchmark\n" );
   printf( "-----------------------------------\n" );
   printf( "arena bytes      : %zu\n", arenaSize );
   printf( "operations       : %zu\n", operations );
   printf( "max live allocs  : %zu\n", maxLiveAllocs );
   printf( "alloc size range : %zu..%zu\n", minAllocSize, maxAllocSize );
   printf( "seed             : %u\n", seed );
   printf( "\n" );
   printf( "alloc attempts   : %zu\n", allocAttempts );
   printf( "alloc success    : %zu\n", allocSuccess );
   printf( "alloc failures   : %zu\n", allocAttempts - allocSuccess );
   printf( "free ops         : %zu\n", freeOps );
   printf( "bytes allocated  : %zu\n", bytesAllocated );
   printf( "peak live bytes  : %zu\n", peakLiveBytes );
   printf( "elapsed seconds  : %.6f\n", seconds );
   printf( "ops/sec          : %.2f\n", opsPerSec );
   printf( "\n" );
   printf( "MemArena stats before cleanup\n" );
   printf( "-----------------------------------\n" );
   printf( "largest remaining block : %zu\n", stats.largestAvailableBlock );
   printf( "total allocated space   : %zu\n", stats.totalAllocatedSpace );
   printf( "total unallocated space : %zu\n", stats.totalUnallocatedSpace );
   printf( "total fragmented space  : %zu\n", stats.totalFragmentedSpace );
   printf( "total unusable space    : %zu\n", stats.totalUnusableSpace );

   MemArena_Destroy( &arena );
   free( freeSlots );
   free( liveSlots );
   free( slots );

   return 0;
}
