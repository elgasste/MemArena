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
      case MemArenaResult_MemNotFound: return "memory was not found in arena";

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

// TODO: test this
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

MemArenaResult_t MemArena_Free( MemArena_t* arena, void* mem )
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

// TODO: test this
MemArenaResult_t MemArena_FreeSubArena( MemArena_t* arena, MemArena_t* subArena )
{
   return MemArena_Free( arena, subArena );
}

internal b32 MemArena_AllocTryAppend( MemArena_t* arena, void** user, size_t size )
{
   size_t freeSize;
   MemArenaBlock_t* newBlock;
   u8* arenaEnd;

   arenaEnd = (u8*)arena + arena->size;
   freeSize = arena->lastBlock
      ? ( arenaEnd - ( (u8*)( arena->lastBlock ) ) ) - sizeof( MemArenaBlock_t ) - arena->lastBlock->size
      : ( arenaEnd - (u8*)arena ) - sizeof( MemArena_t );

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
   ( *user ) = newBlock->mem;

   return True;
}

internal b32 MemArena_AllocTryInsert( MemArena_t* arena, void** user, size_t size )
{
   MemArenaBlock_t *stopBlock, *prevBlock, *newBlock;
   u8 *insertionPoint;
   size_t freeSize;

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
         ( *user ) = newBlock->mem;
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
