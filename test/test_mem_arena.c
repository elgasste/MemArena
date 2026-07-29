#include "unity.h"
#include "mem_arena.h"

#define MEMARENA_TEST_HELPER_DECLARE_ARENA() \
   MemArena_t* arena; \
   MemArenaResult_t result

#define MEMARENA_TEST_HELPER_CREATE_ARENA( s ) \
   result = MemArena_Create( &arena, ( s ) ); \
   TEST_ASSERT_EQUAL( MemArenaResult_Success, result )

#define MEMARENA_TEST_HELPER_ALLOC( m, s ) \
   m = 0; \
   result = MemArena_Alloc( arena, &( m ), ( s ) ); \
   TEST_ASSERT_EQUAL( MemArenaResult_Success, result )

// MUFFINS: we don't care about the blockUser in these cases, no need to pass them in
internal MemArena_t* MemArenaTestHelper_CreateArenaWithBlockAtOffset( size_t arenaSize,
                                                                      size_t blockOffset,
                                                                      size_t blockSize )
{
   MEMARENA_TEST_HELPER_DECLARE_ARENA();
   MemArenaBlock_t* block;

   MEMARENA_TEST_HELPER_CREATE_ARENA( arenaSize );

   block = (MemArenaBlock_t*)( (u8*)arena + blockOffset );
   arena->firstBlock = block;
   arena->lastBlock = block;
   block->prev = 0;
   block->next = 0;
   block->size = blockSize;
   block->dispose = False;
   block->mem = (u8*)block + sizeof( MemArenaBlock_t );

   return arena;
}

internal MemArena_t* MemArenaTestHelper_CreateArenaWithTwoBlocksAtOffsets( size_t arenaSize,
                                                                           size_t blockOffset1,
                                                                           size_t blockSize1,
                                                                           size_t blockOffset2,
                                                                           size_t blockSize2 )
{
   MemArena_t* arena;
   MemArenaBlock_t *block1, *block2;

   arena = MemArenaTestHelper_CreateArenaWithBlockAtOffset( arenaSize, blockOffset1, blockSize1 );

   block1 = arena->firstBlock;
   block2 = (MemArenaBlock_t*)( (u8*)arena + blockOffset2 );
   block1->next = block2;
   block2->prev = block1;
   arena->lastBlock = block2;
   block2->next = 0;
   block2->size = blockSize2;
   block2->dispose = False;
   block2->mem = (u8*)block2 + sizeof( MemArenaBlock_t );

   return arena;
}

void setUp( void ) {}
void tearDown( void ) {}

internal void test_MemArena_Create_ArenaTooSmall_ReturnsArenaTooSmall( void )
{
   MEMARENA_TEST_HELPER_DECLARE_ARENA();

   result = MemArena_Create( &arena, sizeof( MemArena_t ) + sizeof( MemArenaBlock_t ) );
   TEST_ASSERT_EQUAL( MemArenaResult_ArenaTooSmall, result );
}

internal void test_MemArena_Create_ArenaMinSize_ReturnsSuccess( void )
{
   MEMARENA_TEST_HELPER_DECLARE_ARENA();

   result = MemArena_Create( &arena, sizeof( MemArena_t ) + sizeof( MemArenaBlock_t ) + 1 );
   TEST_ASSERT_EQUAL( MemArenaResult_Success, result );

   MemArena_Destroy( &arena );
}

internal void test_MemArena_Create_SetsCorrectParameters( void )
{
   MEMARENA_TEST_HELPER_DECLARE_ARENA();

   MEMARENA_TEST_HELPER_CREATE_ARENA( 1000 );
   TEST_ASSERT_EQUAL( arena->size, 1000 );
   TEST_ASSERT_EQUAL( arena->firstBlock, 0 );
   TEST_ASSERT_EQUAL( arena->lastBlock, 0 );

   MemArena_Destroy( &arena );
}

internal void test_MemArena_Destroy_CleansUpArena( void )
{
   MEMARENA_TEST_HELPER_DECLARE_ARENA();

   arena = 0;
   MEMARENA_TEST_HELPER_CREATE_ARENA( 1000 );
   TEST_ASSERT_NOT_NULL( arena );

   MemArena_Destroy( &arena );
   TEST_ASSERT_NULL( arena );
}

internal void test_MemArena_Reset_ResetsBlockPointers( void )
{
   MEMARENA_TEST_HELPER_DECLARE_ARENA();
   u8 *mem1, *mem2;

   MEMARENA_TEST_HELPER_CREATE_ARENA( 1000 );
   MEMARENA_TEST_HELPER_ALLOC( mem1, 10 );

   MEMARENA_TEST_HELPER_ALLOC( mem2, 10 );
   TEST_ASSERT_EQUAL( mem1, arena->firstBlock->mem );
   TEST_ASSERT_EQUAL( mem2, arena->lastBlock->mem );

   MemArena_Reset( arena );
   TEST_ASSERT_NULL( arena->firstBlock );
   TEST_ASSERT_NULL( arena->lastBlock );

   MemArena_Destroy( &arena );
}

internal void test_MemArena_Free_MemoryNotFound_ReturnsMemNotFound( void )
{
   MEMARENA_TEST_HELPER_DECLARE_ARENA();
   u8* mem;

   MEMARENA_TEST_HELPER_CREATE_ARENA( 1000 );

   MEMARENA_TEST_HELPER_ALLOC( mem, 10 );
   TEST_ASSERT_EQUAL( mem, arena->firstBlock->mem );

   result = MemArena_Free( arena, mem + 1 );
   TEST_ASSERT_EQUAL( MemArenaResult_MemNotFound, result );

   MemArena_Destroy( &arena );
}

internal void test_MemArena_Free_MemoryFound_MarksAsDispose( void )
{
   MEMARENA_TEST_HELPER_DECLARE_ARENA();
   u8* mem;

   MEMARENA_TEST_HELPER_CREATE_ARENA( 1000 );

   MEMARENA_TEST_HELPER_ALLOC( mem, 10 );
   TEST_ASSERT_EQUAL( mem, arena->firstBlock->mem );

   result = MemArena_Free( arena, mem );
   TEST_ASSERT_EQUAL( MemArenaResult_Success, result );
   TEST_ASSERT_EQUAL( True, arena->firstBlock->dispose );

   MemArena_Destroy( &arena );
}

internal void test_MemArena_Alloc_NoBlocksAllocatedWithSpaceAvailable_AllocatesBlock( void )
{
   MEMARENA_TEST_HELPER_DECLARE_ARENA();
   u8* mem;

   MEMARENA_TEST_HELPER_CREATE_ARENA( 1000 );
   TEST_ASSERT_NULL( arena->firstBlock );

   MEMARENA_TEST_HELPER_ALLOC( mem, 20 );
   TEST_ASSERT_NOT_NULL( mem );

   MemArena_Destroy( &arena );
}

internal void test_MemArena_Alloc_NoBlocksAllocatedWithNoSpaceAvailable_DoesNotAllocateBlock( void )
{
   MEMARENA_TEST_HELPER_DECLARE_ARENA();
   u8* mem;

   MEMARENA_TEST_HELPER_CREATE_ARENA( 1000 );
   TEST_ASSERT_NULL( arena->firstBlock );

   mem = 0;
   result = MemArena_Alloc( arena, &mem, 1000 );
   TEST_ASSERT_EQUAL( MemArenaResult_OutOfMemory, result );
   TEST_ASSERT_NULL( mem );

   MemArena_Destroy( &arena );
}

internal void test_MemArena_Alloc_OneImmediateBlockPresentWithSpaceAvailable_AllocatesBlock( void )
{
   MEMARENA_TEST_HELPER_DECLARE_ARENA();
   u8* mem;

   MEMARENA_TEST_HELPER_CREATE_ARENA( 1000 );

   MEMARENA_TEST_HELPER_ALLOC( mem, 10 );
   TEST_ASSERT_EQUAL( (u8*)arena + sizeof( MemArena_t ), (u8*)( arena->firstBlock ) );
   TEST_ASSERT_EQUAL( arena->firstBlock->mem, mem );
   TEST_ASSERT_EQUAL( arena->lastBlock->mem, mem );

   MEMARENA_TEST_HELPER_ALLOC( mem, 10 );
   TEST_ASSERT_NOT_EQUAL( arena->firstBlock->mem, mem );
   TEST_ASSERT_EQUAL( arena->lastBlock->mem, mem );
   TEST_ASSERT_NULL( arena->firstBlock->prev );
   TEST_ASSERT_EQUAL( arena->firstBlock->next, arena->lastBlock );
   TEST_ASSERT_EQUAL( arena->lastBlock->prev, arena->firstBlock );
   TEST_ASSERT_NULL( arena->lastBlock->next );

   MemArena_Destroy( &arena );
}

internal void test_MemArena_Alloc_OneImmediateBlockPresentWithNoSpaceAvailable_DoesNotAllocateBlock( void )
{
   MEMARENA_TEST_HELPER_DECLARE_ARENA();
   u8* mem;

   MEMARENA_TEST_HELPER_CREATE_ARENA( sizeof( MemArena_t ) + ( sizeof( MemArenaBlock_t ) * 2 ) + 19 );

   MEMARENA_TEST_HELPER_ALLOC( mem, 10 );
   TEST_ASSERT_EQUAL( (u8*)arena + sizeof( MemArena_t ), (u8*)( arena->firstBlock ) );
   TEST_ASSERT_EQUAL( arena->firstBlock->mem, mem );
   TEST_ASSERT_EQUAL( arena->firstBlock, arena->lastBlock );
   TEST_ASSERT_NULL( arena->firstBlock->prev );
   TEST_ASSERT_NULL( arena->lastBlock->next );

   result = MemArena_Alloc( arena, &mem, 10 );
   TEST_ASSERT_EQUAL( MemArenaResult_OutOfMemory, result );

   MemArena_Destroy( &arena );
}

internal void test_MemArena_Alloc_OneOffsetBlockPresentWithPrecedingSpaceAvailable_InsertsBlock( void )
{
   MEMARENA_TEST_HELPER_DECLARE_ARENA();
   size_t arenaSize, blockSize, blockOffset;
   u8* mem;

   blockSize = 100;
   arenaSize = sizeof( MemArena_t ) + ( ( sizeof( MemArenaBlock_t ) + blockSize ) * 3 ) - 1;
   blockOffset = sizeof( MemArena_t ) + sizeof( MemArenaBlock_t ) + blockSize;
   arena = MemArenaTestHelper_CreateArenaWithBlockAtOffset( arenaSize, blockOffset, blockSize );
   
   MEMARENA_TEST_HELPER_ALLOC( mem, blockSize );
   TEST_ASSERT_EQUAL( (u8*)arena + sizeof( MemArena_t ) + sizeof( MemArenaBlock_t ), mem );
   TEST_ASSERT_EQUAL( arena->firstBlock->mem, mem );
   TEST_ASSERT_EQUAL( arena->firstBlock->next, arena->lastBlock );
   TEST_ASSERT_NULL( arena->firstBlock->prev );
   TEST_ASSERT_EQUAL( arena->lastBlock->prev, arena->firstBlock );
   TEST_ASSERT_NULL( arena->lastBlock->next );
   TEST_ASSERT_EQUAL( arena->firstBlock->mem, mem );

   MemArena_Destroy( &arena );
}

internal void test_MemArena_Alloc_OneOffsetBlockPresentWithSpaceAvailableAfter_AppendsBlock( void )
{
   MEMARENA_TEST_HELPER_DECLARE_ARENA();
   size_t arenaSize, blockSize, blockOffset;
   u8* mem;

   blockSize = 100;
   arenaSize = sizeof( MemArena_t ) + ( ( sizeof( MemArenaBlock_t ) + blockSize ) * 3 ) - 1;
   blockOffset = sizeof( MemArena_t ) + sizeof( MemArenaBlock_t ) + blockSize - 1;
   arena = MemArenaTestHelper_CreateArenaWithBlockAtOffset( arenaSize, blockOffset, blockSize );

   MEMARENA_TEST_HELPER_ALLOC( mem, blockSize );
   TEST_ASSERT_EQUAL( (u8*)arena + blockOffset + ( sizeof( MemArenaBlock_t ) + blockSize ) + sizeof( MemArenaBlock_t ), mem);
   TEST_ASSERT_EQUAL( arena->lastBlock->mem, mem );
   TEST_ASSERT_EQUAL( arena->firstBlock->next, arena->lastBlock );
   TEST_ASSERT_NULL( arena->firstBlock->prev );
   TEST_ASSERT_EQUAL( arena->lastBlock->prev, arena->firstBlock );
   TEST_ASSERT_NULL( arena->lastBlock->next );
   TEST_ASSERT_EQUAL( arena->lastBlock->mem, mem );

   MemArena_Destroy( &arena );
}

internal void test_MemArena_Alloc_OneOffsetBlockPresentWithNoSpaceAvailableOnEitherSide_DoesNotAllocateBlock( void )
{
   MEMARENA_TEST_HELPER_DECLARE_ARENA();
   size_t arenaSize, blockSize, blockOffset;
   u8* mem;

   blockSize = 100;
   arenaSize = sizeof( MemArena_t ) + ( ( sizeof( MemArenaBlock_t ) + blockSize ) * 3 ) - 2;
   blockOffset = sizeof( MemArena_t ) + sizeof( MemArenaBlock_t ) + blockSize - 1;
   arena = MemArenaTestHelper_CreateArenaWithBlockAtOffset( arenaSize, blockOffset, blockSize );

   mem = 0;
   result = MemArena_Alloc( arena, &mem, blockSize );
   TEST_ASSERT_EQUAL( MemArenaResult_OutOfMemory, result );
   TEST_ASSERT_EQUAL( arena->firstBlock, arena->lastBlock );
   TEST_ASSERT_NULL( arena->firstBlock->next );
   TEST_ASSERT_NULL( arena->firstBlock->prev );
   TEST_ASSERT_NULL( mem );

   MemArena_Destroy( &arena );
}

internal void test_MemArena_Alloc_OneDisposedOffsetBlockPresentWithNoSpaceAvailableOnEitherSide_DisposesAndAllocatesBlock( void )
{
   MEMARENA_TEST_HELPER_DECLARE_ARENA();
   size_t arenaSize, blockSize, blockOffset;
   u8* mem;

   blockSize = 100;
   arenaSize = sizeof( MemArena_t ) + ( ( sizeof( MemArenaBlock_t ) + blockSize ) * 3 ) - 2;
   blockOffset = sizeof( MemArena_t ) + sizeof( MemArenaBlock_t ) + blockSize - 1;
   arena = MemArenaTestHelper_CreateArenaWithBlockAtOffset( arenaSize, blockOffset, blockSize );
   arena->firstBlock->dispose = True;

   MEMARENA_TEST_HELPER_ALLOC( mem, blockSize );
   TEST_ASSERT_EQUAL( (u8*)arena + sizeof( MemArena_t ) + sizeof( MemArenaBlock_t ), mem );
   TEST_ASSERT_EQUAL( arena->firstBlock->mem, mem );
   TEST_ASSERT_EQUAL( arena->firstBlock, arena->lastBlock );
   TEST_ASSERT_NULL( arena->firstBlock->prev );
   TEST_ASSERT_NULL( arena->firstBlock->next );

   MemArena_Destroy( &arena );
}

internal void test_MemArena_Alloc_TwoBlocksPresentWithSpaceBetween_InsertsBlock( void )
{
   MEMARENA_TEST_HELPER_DECLARE_ARENA();
   size_t arenaSize, blockSize, blockOffset1, blockOffset2;
   u8* mem;

   blockSize = 100;
   arenaSize = sizeof( MemArena_t ) + ( ( sizeof( MemArenaBlock_t ) + blockSize ) * 3 );
   blockOffset1 = sizeof( MemArena_t );
   blockOffset2 = blockOffset1 + ( ( sizeof( MemArenaBlock_t ) + blockSize ) * 2 );
   arena = MemArenaTestHelper_CreateArenaWithTwoBlocksAtOffsets( arenaSize, blockOffset1, blockSize, blockOffset2, blockSize );

   MEMARENA_TEST_HELPER_ALLOC( mem, blockSize );
   TEST_ASSERT_EQUAL( (u8*)arena + sizeof( MemArena_t ) + ( sizeof( MemArenaBlock_t ) * 2 ) + blockSize, mem );
   TEST_ASSERT_EQUAL( arena->firstBlock->next->mem, mem );
   TEST_ASSERT_NULL( arena->firstBlock->prev );
   TEST_ASSERT_NULL( arena->lastBlock->next );
   TEST_ASSERT_EQUAL( arena->lastBlock->prev, arena->firstBlock->next );
   TEST_ASSERT_EQUAL( arena->firstBlock->next->prev, arena->firstBlock );
   TEST_ASSERT_EQUAL( arena->lastBlock->prev->next, arena->lastBlock );

   MemArena_Destroy( &arena );
}

internal void test_MemArena_Alloc_TwoBlocksPresentWithNoSpaceBetweenOrAfter_DoesNotAllocateBlock( void )
{
   MEMARENA_TEST_HELPER_DECLARE_ARENA();
   size_t arenaSize, blockSize, blockOffset1, blockOffset2;
   u8* mem;

   blockSize = 100;
   arenaSize = sizeof( MemArena_t ) + ( ( sizeof( MemArenaBlock_t ) + blockSize ) * 3 );
   blockOffset1 = sizeof( MemArena_t ) + 1;
   blockOffset2 = sizeof( MemArena_t ) + ( ( sizeof( MemArenaBlock_t ) + blockSize ) * 2 );
   arena = MemArenaTestHelper_CreateArenaWithTwoBlocksAtOffsets( arenaSize, blockOffset1, blockSize, blockOffset2, blockSize );

   mem = 0;
   result = MemArena_Alloc( arena, &mem, blockSize );
   TEST_ASSERT_EQUAL( MemArenaResult_OutOfMemory, result );
   TEST_ASSERT_EQUAL( arena->firstBlock->next, arena->lastBlock );
   TEST_ASSERT_EQUAL( arena->lastBlock->prev, arena->firstBlock );
   TEST_ASSERT_NULL( arena->lastBlock->next );
   TEST_ASSERT_NULL( arena->firstBlock->prev );
   TEST_ASSERT_NULL( mem );

   MemArena_Destroy( &arena );
}

internal void test_MemArena_Alloc_TwoBlocksPresentWithNoSpaceBetweenButSpaceAfter_AppendsBlock( void )
{
   MEMARENA_TEST_HELPER_DECLARE_ARENA();
   size_t arenaSize, blockSize, blockOffset1, blockOffset2;
   u8* mem;

   blockSize = 100;
   arenaSize = sizeof( MemArena_t ) + ( ( sizeof( MemArenaBlock_t ) + blockSize ) * 3 );
   blockOffset1 = sizeof( MemArena_t );
   blockOffset2 = blockOffset1 + sizeof( MemArenaBlock_t ) + blockSize;
   arena = MemArenaTestHelper_CreateArenaWithTwoBlocksAtOffsets( arenaSize, blockOffset1, blockSize, blockOffset2, blockSize );

   MEMARENA_TEST_HELPER_ALLOC( mem, blockSize );
   TEST_ASSERT_EQUAL( (u8*)arena + sizeof( MemArena_t ) + ( ( sizeof( MemArenaBlock_t ) + blockSize ) * 2 ) + sizeof( MemArenaBlock_t ), mem );
   TEST_ASSERT_EQUAL( arena->lastBlock->mem, mem );
   TEST_ASSERT_NULL( arena->firstBlock->prev );
   TEST_ASSERT_NULL( arena->lastBlock->next );
   TEST_ASSERT_EQUAL( arena->lastBlock->prev, arena->firstBlock->next );
   TEST_ASSERT_EQUAL( arena->firstBlock->next->prev, arena->firstBlock );
   TEST_ASSERT_EQUAL( arena->lastBlock->prev->next, arena->lastBlock );

   MemArena_Destroy( &arena );
}

internal void test_MemArena_Alloc_TwoBlocksPresentWithSpaceAvailableAfterDisposingFirstBlock_DisposesAndAllocatesBlock( void )
{
   MEMARENA_TEST_HELPER_DECLARE_ARENA();
   size_t arenaSize, blockSize, blockOffset1, blockOffset2;
   u8* mem;

   blockSize = 100;
   arenaSize = sizeof( MemArena_t ) + ( ( sizeof( MemArenaBlock_t ) + blockSize ) * 3 );
   blockOffset1 = sizeof( MemArena_t ) + 1;
   blockOffset2 = sizeof( MemArena_t ) + ( ( sizeof( MemArenaBlock_t ) + blockSize ) * 2 );
   arena = MemArenaTestHelper_CreateArenaWithTwoBlocksAtOffsets( arenaSize, blockOffset1, blockSize, blockOffset2, blockSize );
   arena->firstBlock->dispose = True;

   MEMARENA_TEST_HELPER_ALLOC( mem, blockSize );
   TEST_ASSERT_EQUAL( (u8*)arena + sizeof( MemArena_t ) + sizeof( MemArenaBlock_t ), mem );
   TEST_ASSERT_EQUAL( arena->firstBlock->mem, mem );
   TEST_ASSERT_NULL( arena->firstBlock->prev );
   TEST_ASSERT_NULL( arena->lastBlock->next );
   TEST_ASSERT_EQUAL( arena->lastBlock->prev, arena->firstBlock );
   TEST_ASSERT_EQUAL( arena->firstBlock->next, arena->lastBlock );

   MemArena_Destroy( &arena );
}

internal void test_MemArena_Alloc_TwoBlocksPresentWithSpaceAvailableAfterDisposingSecondBlock_DisposesAndAllocatesBlock( void )
{
   MEMARENA_TEST_HELPER_DECLARE_ARENA();
   size_t arenaSize, blockSize, blockOffset1, blockOffset2;
   u8* mem;

   blockSize = 100;
   arenaSize = sizeof( MemArena_t ) + ( ( sizeof( MemArenaBlock_t ) + blockSize ) * 3 );
   blockOffset1 = sizeof( MemArena_t ) + 1;
   blockOffset2 = sizeof( MemArena_t ) + ( ( sizeof( MemArenaBlock_t ) + blockSize ) * 2 );
   arena = MemArenaTestHelper_CreateArenaWithTwoBlocksAtOffsets( arenaSize, blockOffset1, blockSize, blockOffset2, blockSize );
   arena->lastBlock->dispose = True;

   MEMARENA_TEST_HELPER_ALLOC( mem, blockSize );
   TEST_ASSERT_EQUAL( (u8*)arena + blockOffset1 + ( sizeof( MemArenaBlock_t ) * 2 ) + blockSize, mem );
   TEST_ASSERT_EQUAL( arena->lastBlock->mem, mem );
   TEST_ASSERT_NULL( arena->firstBlock->prev );
   TEST_ASSERT_NULL( arena->lastBlock->next );
   TEST_ASSERT_EQUAL( arena->lastBlock->prev, arena->firstBlock );
   TEST_ASSERT_EQUAL( arena->firstBlock->next, arena->lastBlock );

   MemArena_Destroy( &arena );
}

internal void test_MemArena_Alloc_TwoBlocksPresentWithSpaceAvailableAfterDisposingBothBlocks_DisposesAndAllocatesBlock( void )
{
   MEMARENA_TEST_HELPER_DECLARE_ARENA();
   size_t arenaSize, blockSize, blockOffset1, blockOffset2;
   u8* mem;

   blockSize = 100;
   arenaSize = sizeof( MemArena_t ) + ( ( sizeof( MemArenaBlock_t ) + blockSize ) * 3 );
   blockOffset1 = sizeof( MemArena_t );
   blockOffset2 = blockOffset1 + ( sizeof( MemArenaBlock_t ) + blockSize );
   arena = MemArenaTestHelper_CreateArenaWithTwoBlocksAtOffsets( arenaSize, blockOffset1, blockSize, blockOffset2, blockSize );
   arena->firstBlock->dispose = True;
   arena->lastBlock->dispose = True;

   MEMARENA_TEST_HELPER_ALLOC( mem, blockSize * 2 );
   TEST_ASSERT_EQUAL( (u8*)arena + sizeof( MemArena_t ) + sizeof( MemArenaBlock_t ), mem);
   TEST_ASSERT_EQUAL( arena->firstBlock->mem, mem );
   TEST_ASSERT_EQUAL( arena->firstBlock, arena->lastBlock );
   TEST_ASSERT_NULL( arena->firstBlock->prev );
   TEST_ASSERT_NULL( arena->firstBlock->next );

   MemArena_Destroy( &arena );
}

internal void test_MemArena_AllocSubArena_AllocatesAndInitializesMemArenaAndFreesCorrectly( void )
{
   MemArena_t *arena, *subArena;
   MemArenaResult_t result;
   size_t arenaSize, subArenaSize;

   arenaSize = 1000;
   MEMARENA_TEST_HELPER_CREATE_ARENA( arenaSize );

   subArenaSize = 500;
   subArena = 0;
   result = MemArena_AllocSubArena( arena, &subArena, subArenaSize );
   TEST_ASSERT_EQUAL( MemArenaResult_Success, result );
   TEST_ASSERT_EQUAL( (u8*)arena + sizeof( MemArena_t ) + sizeof( MemArenaBlock_t ), subArena );
   TEST_ASSERT_EQUAL( arena->firstBlock->mem, subArena );
   TEST_ASSERT_EQUAL( subArenaSize, subArena->size );
   TEST_ASSERT_NULL( subArena->firstBlock );
   TEST_ASSERT_NULL( subArena->lastBlock );
   TEST_ASSERT_EQUAL( False, arena->firstBlock->dispose );

   result = MemArena_Free( arena, subArena );
   TEST_ASSERT_EQUAL( MemArenaResult_Success, result );
   TEST_ASSERT_EQUAL( True, arena->firstBlock->dispose );

   MemArena_Destroy( &arena );
}

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
   RUN_TEST( test_MemArena_Alloc_OneOffsetBlockPresentWithPrecedingSpaceAvailable_InsertsBlock );
   RUN_TEST( test_MemArena_Alloc_OneOffsetBlockPresentWithSpaceAvailableAfter_AppendsBlock );
   RUN_TEST( test_MemArena_Alloc_OneOffsetBlockPresentWithNoSpaceAvailableOnEitherSide_DoesNotAllocateBlock );
   RUN_TEST( test_MemArena_Alloc_OneDisposedOffsetBlockPresentWithNoSpaceAvailableOnEitherSide_DisposesAndAllocatesBlock );
   RUN_TEST( test_MemArena_Alloc_TwoBlocksPresentWithSpaceBetween_InsertsBlock );
   RUN_TEST( test_MemArena_Alloc_TwoBlocksPresentWithNoSpaceBetweenOrAfter_DoesNotAllocateBlock );
   RUN_TEST( test_MemArena_Alloc_TwoBlocksPresentWithNoSpaceBetweenButSpaceAfter_AppendsBlock );
   RUN_TEST( test_MemArena_Alloc_TwoBlocksPresentWithSpaceAvailableAfterDisposingFirstBlock_DisposesAndAllocatesBlock );
   RUN_TEST( test_MemArena_Alloc_TwoBlocksPresentWithSpaceAvailableAfterDisposingSecondBlock_DisposesAndAllocatesBlock );
   RUN_TEST( test_MemArena_Alloc_TwoBlocksPresentWithSpaceAvailableAfterDisposingBothBlocks_DisposesAndAllocatesBlock );

   RUN_TEST( test_MemArena_AllocSubArena_AllocatesAndInitializesMemArenaAndFreesCorrectly );
   
   return UNITY_END();
}
