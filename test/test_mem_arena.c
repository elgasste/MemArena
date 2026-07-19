#include "unity.h"
#include "mem_arena.h"

void setUp( void )
{
}

void tearDown( void )
{
}

void test_MemArena_Create_ArenaTooSmall_ReturnsArenaTooSmall( void )
{
   MemArena_t* arena;
   MemArenaResult_t result;

   result = MemArena_Create( &arena, sizeof( MemArena_t ) + sizeof( MemArenaBlock_t ) );

   TEST_ASSERT_EQUAL( MemArenaResult_ArenaTooSmall, result );
}

void test_MemArena_Create_ArenaMinSize_ReturnsSuccess( void )
{
   MemArena_t* arena;
   MemArenaResult_t result;

   result = MemArena_Create( &arena, sizeof( MemArena_t ) + sizeof( MemArenaBlock_t ) + 1 );

   TEST_ASSERT_EQUAL( MemArenaResult_Success, result );

   MemArena_Destroy( &arena );
}

void test_MemArena_Create_SetsCorrectParameters( void )
{
   MemArena_t* arena;
   MemArenaResult_t result;

   result = MemArena_Create( &arena, 1000 );
   
   TEST_ASSERT_EQUAL( MemArenaResult_Success, result );
   TEST_ASSERT_EQUAL( arena->size, 1000 );
   TEST_ASSERT_EQUAL( arena->firstBlock, 0 );
   TEST_ASSERT_EQUAL( arena->lastBlock, 0 );

   MemArena_Destroy( &arena );
}

void test_MemArena_Destroy_CleansUpArena( void )
{
   MemArena_t* arena;
   MemArenaResult_t result;

   arena = 0;

   result = MemArena_Create( &arena, 1000 );
   TEST_ASSERT_NOT_NULL( arena );

   MemArena_Destroy( &arena );

   TEST_ASSERT_NULL( arena );
}

void test_MemArena_Reset_ResetsBlockPointers( void )
{
   MemArena_t* arena;
   u8 *mem1, *mem2;

   MemArena_Create( &arena, 1000 );
   MemArena_Alloc( arena, &mem1, 10 );
   MemArena_Alloc( arena, &mem2, 10 );

   TEST_ASSERT_EQUAL( mem1, arena->firstBlock->mem );
   TEST_ASSERT_EQUAL( mem2, arena->lastBlock->mem );

   MemArena_Reset( arena );

   TEST_ASSERT_NULL( arena->firstBlock );
   TEST_ASSERT_NULL( arena->lastBlock );

   MemArena_Destroy( &arena );
}

void test_MemArena_Free_MemoryNotFound_ReturnsMemNotFound( void )
{
   MemArena_t* arena;
   MemArenaResult_t result;
   u8* mem;

   MemArena_Create( &arena, 1000 );
   MemArena_Alloc( arena, &mem, 10 );

   TEST_ASSERT_EQUAL( mem, arena->firstBlock->mem );

   result = MemArena_Free( arena, mem + 1 );

   TEST_ASSERT_EQUAL( MemArenaResult_MemNotFound, result );

   MemArena_Destroy( &arena );
}

void test_MemArena_Free_MemoryFound_MarksAsDispose( void )
{
   MemArena_t* arena;
   MemArenaResult_t result;
   u8* mem;

   MemArena_Create( &arena, 1000 );
   MemArena_Alloc( arena, &mem, 10 );

   TEST_ASSERT_EQUAL( mem, arena->firstBlock->mem );

   result = MemArena_Free( arena, mem );

   TEST_ASSERT_EQUAL( MemArenaResult_Success, result );
   TEST_ASSERT_EQUAL( True, arena->firstBlock->dispose );

   MemArena_Destroy( &arena );
}

void test_MemArena_Alloc_NoBlocksAllocatedWithSpaceAvailable_AllocatesBlock( void )
{
   MemArena_t* arena;
   MemArenaResult_t result;
   u8* mem;

   MemArena_Create( &arena, 1000 );
   
   TEST_ASSERT_NULL( arena->firstBlock );

   result = MemArena_Alloc( arena, &mem, 20 );

   TEST_ASSERT_EQUAL( MemArenaResult_Success, result );
   TEST_ASSERT_NOT_NULL( mem );

   MemArena_Destroy( &arena );
}

void test_MemArena_Alloc_NoBlocksAllocatedWithNoSpaceAvailable_DoesNotAllocateBlock( void )
{
   MemArena_t* arena;
   MemArenaResult_t result;
   u8* mem;

   MemArena_Create( &arena, 1000 );
   TEST_ASSERT_NULL( arena->firstBlock );

   mem = 0;
   result = MemArena_Alloc( arena, &mem, 1000 );

   TEST_ASSERT_EQUAL( MemArenaResult_OutOfMemory, result );
   TEST_ASSERT_NULL( mem );

   MemArena_Destroy( &arena );
}

void test_MemArena_Alloc_OneImmediateBlockPresentWithSpaceAvailable_AllocatesBlock( void )
{
   MemArena_t* arena;
   MemArenaResult_t result;
   u8* mem;

   MemArena_Create( &arena, 1000 );
   MemArena_Alloc( arena, &mem, 10 );
   TEST_ASSERT_EQUAL( (u8*)arena + sizeof( MemArena_t ), (u8*)( arena->firstBlock ) );
   TEST_ASSERT_EQUAL( arena->firstBlock->mem, mem );
   TEST_ASSERT_EQUAL( arena->lastBlock->mem, mem );

   result = MemArena_Alloc( arena, &mem, 10 );

   TEST_ASSERT_EQUAL( MemArenaResult_Success, result );
   TEST_ASSERT_NOT_EQUAL( arena->firstBlock->mem, mem );
   TEST_ASSERT_EQUAL( arena->lastBlock->mem, mem );
   TEST_ASSERT_NULL( arena->firstBlock->prev );
   TEST_ASSERT_EQUAL( arena->firstBlock->next, arena->lastBlock );
   TEST_ASSERT_EQUAL( arena->lastBlock->prev, arena->firstBlock );
   TEST_ASSERT_NULL( arena->lastBlock->next );
}

void test_MemArena_Alloc_OneImmediateBlockPresentWithNoSpaceAvailable_DoesNotAllocateBlock( void )
{
   MemArena_t* arena;
   MemArenaResult_t result;
   u8* mem;

   MemArena_Create( &arena, sizeof( MemArena_t ) + ( sizeof( MemArenaBlock_t ) * 2 ) + 19 );
   MemArena_Alloc( arena, &mem, 10 );
   TEST_ASSERT_EQUAL( (u8*)arena + sizeof( MemArena_t ), (u8*)( arena->firstBlock ) );
   TEST_ASSERT_EQUAL( arena->firstBlock->mem, mem );
   TEST_ASSERT_EQUAL( arena->lastBlock->mem, mem );

   result = MemArena_Alloc( arena, &mem, 10 );

   TEST_ASSERT_EQUAL( MemArenaResult_OutOfMemory, result );
}

// MUFFINS: we've tested with one block, I think these should be more structured though,
// like let's figure out a solid series of tests that hits EVERY case.

int main( void )
{
   UNITY_BEGIN();
   
   RUN_TEST( test_MemArena_Create_ArenaTooSmall_ReturnsArenaTooSmall );
   RUN_TEST( test_MemArena_Create_ArenaMinSize_ReturnsSuccess );
   RUN_TEST( test_MemArena_Create_SetsCorrectParameters );

   RUN_TEST( test_MemArena_Destroy_CleansUpArena );

   RUN_TEST( test_MemArena_Reset_ResetsBlockPointers );

   RUN_TEST( test_MemArena_Free_MemoryNotFound_ReturnsMemNotFound );
   RUN_TEST( test_MemArena_Free_MemoryFound_MarksAsDispose );

   RUN_TEST( test_MemArena_Alloc_NoBlocksAllocatedWithSpaceAvailable_AllocatesBlock );
   RUN_TEST( test_MemArena_Alloc_NoBlocksAllocatedWithNoSpaceAvailable_DoesNotAllocateBlock );
   RUN_TEST( test_MemArena_Alloc_OneImmediateBlockPresentWithSpaceAvailable_AllocatesBlock );
   RUN_TEST( test_MemArena_Alloc_OneImmediateBlockPresentWithNoSpaceAvailable_DoesNotAllocateBlock );
   
   return UNITY_END();
}
