#include <stdlib.h>

#include "mem_arena.h"

internal b32 MemArena_AllocTryAppend( MemArena_t* arena, void** user, size_t size );
internal b32 MemArena_AllocTryInsert( MemArena_t* arena, void** user, size_t size );

MemArenaResult_t MemArena_Create( MemArena_t** pArena, size_t size )
{
   // there should be enough space for at least one 1-byte block
   if ( size < ( sizeof( MemArena_t ) + sizeof( MemArenaBlock_t ) + 1 ) )
   {
      return MemArenaResult_ArenaTooSmall;
   }

   *pArena = malloc( size );
   if ( !( *pArena ) )
   {
      return MemArenaResult_SystemMemoryAllocFailed;
   }

   ( *pArena )->size = size;
   MemArena_Reset( *pArena );

   return MemArenaResult_Success;
}

void MemArena_Destroy( MemArena_t** pArena )
{
   if ( pArena )
   {
      free( ( *pArena ) );
      *pArena = 0;
   }
}

void MemArena_Reset( MemArena_t* arena )
{
   arena->firstBlock = 0;
   arena->lastBlock = 0;
}

const char* MemoryArena_GetErrorMessage( MemArenaResult_t result )
{
   switch ( result )
   {
      case MemArenaResult_Success: return "success";

      case MemArenaResult_ArenaTooSmall: return "requested arena size is too small";
      case MemArenaResult_SystemMemoryAllocFailed: return "system memory allocation failed";
      case MemArenaResult_OutOfMemory: return "arena is out of memory";

      default: return "undefined error";
   }
}

MemArenaResult_t MemArena_Alloc( MemArena_t* arena, void** user, size_t size )
{
   if ( MemArena_AllocTryAppend( arena, user, size ) ||
        MemArena_AllocTryInsert( arena, user, size ) )
   {
      return MemArenaResult_Success;
   }

   return MemArenaResult_OutOfMemory;
}

MemArenaResult_t MemArena_AllocSubArena( MemArena_t* arena, MemArena_t** subArena, size_t size )
{
   MemArenaResult_t result;

   result = MemArena_Alloc( arena, subArena, size );
   if ( result == MemArenaResult_Success )
   {
      ( *subArena )->size = size;
      MemArena_Reset( *subArena );
   }

   return result;
}

void MemArena_Free( MemArena_t* arena, void* mem )
{
   MemArenaBlock_t* block;

   block = (MemArenaBlock_t*)( (u8*)mem - sizeof( MemArenaBlock_t ) );

   if ( block->prev )
      block->prev->next = block->next;
   if ( block->next )
      block->next->prev = block->prev;
   if ( arena->firstBlock == block )
      arena->firstBlock = block->next;
   if ( arena->lastBlock == block )
      arena->lastBlock = block->prev;
}

MemArenaStats_t MemArena_GetStats( MemArena_t* arena )
{
   MemArenaBlock_t* nextBlock;
   u8 *insertionPoint;
   size_t freeSize, smallestBlockSize, availableSize;
   MemArenaStats_t stats;

   stats.largestAvailableBlock = 0;
   stats.totalAllocatedSpace = 0;
   stats.totalUnallocatedSpace = 0;
   stats.totalFragmentedSpace = 0;
   stats.totalUnusableSpace = 0;

   smallestBlockSize = sizeof( MemArenaBlock_t ) + 1;
   insertionPoint = (u8*)arena + sizeof( MemArena_t );
   nextBlock = arena->firstBlock;

   while ( 1 )
   {
      freeSize = nextBlock
         ? (u8*)nextBlock - insertionPoint
         : ( (u8*)arena + arena->size ) - insertionPoint;
      availableSize = ( freeSize < smallestBlockSize )
         ? 0
         : freeSize - sizeof( MemArenaBlock_t );

      stats.totalUnallocatedSpace += availableSize;

      if ( freeSize < smallestBlockSize )
      {
         stats.totalUnusableSpace += freeSize;
      }
      else if ( nextBlock )
      {
         stats.totalFragmentedSpace += availableSize;
      }

      if ( availableSize > stats.largestAvailableBlock )
      {
         stats.largestAvailableBlock = availableSize;
      }

      if ( nextBlock )
      {
         stats.totalAllocatedSpace += nextBlock->size;
      }
      else
      {
         break;
      }

      insertionPoint = (u8*)nextBlock + sizeof( MemArenaBlock_t ) + nextBlock->size;
      nextBlock = nextBlock->next;
   }

   return stats;
}

internal b32 MemArena_AllocTryAppend( MemArena_t* arena, void** user, size_t size )
{
   size_t freeSize;
   MemArenaBlock_t* newBlock;

   freeSize = arena->lastBlock
      ? ( (u8*)arena + arena->size ) - ( u8* )( arena->lastBlock ) - sizeof( MemArenaBlock_t ) - arena->lastBlock->size
      : arena->size - sizeof( MemArena_t );

   if ( freeSize < ( size + sizeof( MemArenaBlock_t ) ) )
   {
      return False;
   }

   if ( arena->lastBlock )
   {
      newBlock = (MemArenaBlock_t*)( (u8*)( arena->lastBlock ) + sizeof( MemArenaBlock_t ) + arena->lastBlock->size );
      arena->lastBlock->next = newBlock;
      newBlock->prev = arena->lastBlock;
      arena->lastBlock = newBlock;
   }
   else
   {
      newBlock = (MemArenaBlock_t*)( (u8*)arena + sizeof( MemArena_t ) );
      arena->firstBlock = newBlock;
      arena->lastBlock = newBlock;
      newBlock->prev = 0;
   }

   newBlock->next = 0;
   newBlock->size = size;
   newBlock->mem = (u8*)newBlock + sizeof( MemArenaBlock_t );
   ( *user ) = newBlock->mem;

   return True;
}

internal b32 MemArena_AllocTryInsert( MemArena_t* arena, void** user, size_t size )
{
   MemArenaBlock_t *nextBlock, *prevBlock, *newBlock;
   u8 *insertionPoint;
   size_t freeSize;

   insertionPoint = (u8*)arena + sizeof( MemArena_t );
   nextBlock = arena->firstBlock;
   prevBlock = 0;

   while ( 1 )
   {
      freeSize = nextBlock
         ? (u8*)nextBlock - insertionPoint
         : ( (u8*)arena + arena->size ) - insertionPoint;

      if ( freeSize >= ( size + sizeof( MemArenaBlock_t ) ) )
      {
         newBlock = (MemArenaBlock_t*)insertionPoint;
         newBlock->size = size;
         newBlock->mem = (u8*)newBlock + sizeof( MemArenaBlock_t );
         ( *user ) = newBlock->mem;
         newBlock->prev = prevBlock;
         newBlock->next = nextBlock;

         if ( prevBlock )
         {
            prevBlock->next = newBlock;
            if ( arena->lastBlock == prevBlock )
               arena->lastBlock = newBlock;
         }
         else
            arena->firstBlock = newBlock;

         if ( nextBlock )
         {
            nextBlock->prev = newBlock;
            if ( arena->firstBlock == nextBlock )
               arena->firstBlock = newBlock;
         }
         else
            arena->lastBlock = newBlock;

         return True;
      }
      
      if ( !nextBlock )
      {
         break;
      }
      
      insertionPoint = (u8*)nextBlock + sizeof( MemArenaBlock_t ) + nextBlock->size;
      prevBlock = nextBlock;
      nextBlock = nextBlock->next;
   }

   return False;
}
