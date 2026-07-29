#if !defined( MEM_ARENA_H )
#define MEM_ARENA_H

#include "common.h"

typedef enum MemArenaResult_t
{
   MemArenaResult_Success = 0,

   MemArenaResult_ArenaTooSmall,
   MemArenaResult_SystemMemoryAllocFailed,
   MemArenaResult_OutOfMemory,

   MemArenaResult_Count
}
MemArenaResult_t;

typedef struct MemArenaBlock_t MemArenaBlock_t;
typedef struct MemArenaBlock_t
{
   // the size of the memory to be allocated, does not include the MemArenaBlock_t struct size
   size_t size;

   void* mem;
   MemArenaBlock_t* prev;
   MemArenaBlock_t* next;
}
MemArenaBlock_t;

typedef struct MemArena_t
{
   // the entire size of the arena, including the MemArena_t struct
   size_t size;

   MemArenaBlock_t* firstBlock;
   MemArenaBlock_t* lastBlock;
}
MemArena_t;

typedef struct MemArenaStats_t
{
   size_t totalAllocatedSpace;
   size_t largestAvailableBlock;
   size_t totalUnallocatedSpace;
   size_t totalFragmentedSpace;
   size_t totalUnusableSpace;
}
MemArenaStats_t;

#if defined( __cplusplus )
extern "C" {
#endif

MemArenaResult_t MemArena_Create( MemArena_t** pArena, size_t size );
void MemArena_Destroy( MemArena_t** pArena );
void MemArena_Reset( MemArena_t* arena );
const char* MemoryArena_GetErrorMessage( MemArenaResult_t result );

MemArenaResult_t MemArena_Alloc( MemArena_t* arena, void** user, size_t size );
MemArenaResult_t MemArena_AllocSubArena( MemArena_t* arena, MemArena_t** subArena, size_t size );
void MemArena_Free( MemArena_t* arena, void* mem );

MemArenaStats_t MemArena_GetStats( MemArena_t* arena );

#if defined( __cplusplus )
}
#endif

#endif // MEM_ARENA_H
