#include <stdlib.h>

#include "mem_arena.h"

internal b32 MemArena_AllocTryAppend( MemArena_t* arena, u8** mem, u64 size );
internal b32 MemArena_AllocTryInsert( MemArena_t* arena, u8** mem, u64 size );

MemArenaResult_t MemArena_Create( MemArena_t** pArena, u64 size )
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
   ( *pArena )->firstBlock = 0;
   ( *pArena )->lastBlock = 0;

   return MemArenaResult_Success;
}

void MemArena_Destroy( MemArena_t** arena )
{
   if ( arena )
   {
      free( ( *arena ) );
      *arena = 0;
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
      case MemArenaResult_MemNotFound: return "memory was not found in arena";

      default: return "undefined error";
   }
}

MemArenaResult_t MemArena_Alloc( MemArena_t* arena, u8** mem, u64 size )
{
   if ( MemArena_AllocTryAppend( arena, mem, size ) ||
        MemArena_AllocTryInsert( arena, mem, size ) )
   {
      return MemArenaResult_Success;
   }

   return MemArenaResult_OutOfMemory;
}

MemArenaResult_t MemArena_Free( MemArena_t* arena, u8* mem )
{
   MemArenaBlock_t* block;

   block = arena->firstBlock;
   while ( block != 0 )
   {
      if ( block->mem == mem )
      {
         block->dispose = True;
         return MemArenaResult_Success;
      }

      block = block->next;
   }

   return MemArenaResult_MemNotFound;
}

internal b32 MemArena_AllocTryAppend( MemArena_t* arena, u8** mem, u64 size )
{
   u64 freeSize;
   MemArenaBlock_t* newBlock;

   freeSize = arena->lastBlock
      ? ( ( (u8*)arena + arena->size ) - ( (u8*)( arena->lastBlock ) ) ) - sizeof( MemArenaBlock_t ) - arena->lastBlock->size
      : ( ( (u8*)arena + arena->size ) - (u8*)arena ) - sizeof( MemArena_t );

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
   newBlock->dispose = False;
   newBlock->mem = (u8*)newBlock + sizeof( MemArenaBlock_t );
   ( *mem ) = newBlock->mem;

   return True;
}

internal b32 MemArena_AllocTryInsert( MemArena_t* arena, u8** mem, u64 size )
{
   MemArenaBlock_t *stopBlock, *prevBlock, *newBlock;
   u8 *insertionPoint;
   u64 freeSize;

   insertionPoint = (u8*)arena + sizeof( MemArena_t );
   stopBlock = arena->firstBlock;
   prevBlock = 0;

   while ( 1 )
   {
      freeSize = stopBlock
         ? (u8*)stopBlock - insertionPoint
         : ( (u8*)arena + arena->size ) - insertionPoint;

      if ( freeSize >= ( size + sizeof( MemArenaBlock_t ) ) )
      {
         newBlock = (MemArenaBlock_t*)insertionPoint;
         newBlock->size = size;
         newBlock->mem = (u8*)newBlock + sizeof( MemArenaBlock_t );
         ( *mem ) = newBlock->mem;
         newBlock->prev = prevBlock ? prevBlock : 0;
         newBlock->next = stopBlock ? stopBlock : 0;

         if ( prevBlock )
            prevBlock->next = newBlock;
         if ( stopBlock )
            stopBlock->prev = newBlock;

         return True;
      }
      
      if ( stopBlock == 0 )
      {
         break;
      }

      if ( stopBlock->dispose )
      {
         if ( stopBlock->prev )
            stopBlock->prev->next = stopBlock->next;
         if ( stopBlock->next )
            stopBlock->next->prev = stopBlock->prev;
         if ( arena->firstBlock == stopBlock )
            arena->firstBlock = stopBlock->next;
         if ( arena->lastBlock == stopBlock )
            arena->lastBlock = stopBlock->prev;
      }
      else
      {
         insertionPoint = (u8*)stopBlock + sizeof( MemArenaBlock_t ) + stopBlock->size;
         prevBlock = stopBlock;
      }

      stopBlock = stopBlock->next;
   }

   return False;
}
