#include "StackAllocator.h"

mfmError mfmInternalStackAllocate(void* allocator, void** memory, mfmU64 size)
{
	return mfmStackAllocate((mfmStackAllocator*)allocator, memory, size);
}

mfmError mfmInternalStackDeallocate(void* allocator, void* memory)
{
	return mfmStackSetHead(allocator, memory);
}

mfmError mfmCreateStackAllocator(mfmStackAllocator ** stackAllocator, mfmU64 size)
{
	// Check if the arguments are valid
	if (stackAllocator == NULL || size == 0)
		return MFM_ERROR_INVALID_ARGUMENTS;
	
	// Allocate memory for the stack allocator
	mfmU8* memory = (mfmU8*)malloc(sizeof(mfmStackAllocator) + size);
	if (memory == NULL)
		return MFM_ERROR_ALLOCATION_FAILED;

	// Get data pointers
	*stackAllocator = (mfmStackAllocator*)(memory + 0);
	(*stackAllocator)->onMemory = MFM_FALSE;
	(*stackAllocator)->stackSize = size;
	(*stackAllocator)->stackBegin = memory + sizeof(mfmStackAllocator);
	(*stackAllocator)->stackHead = memory + sizeof(mfmStackAllocator);

	// Set functions
	(*stackAllocator)->base.allocate = &mfmInternalStackAllocate;
	(*stackAllocator)->base.deallocate = &mfmInternalStackDeallocate;

	// Set destructor function
	(*stackAllocator)->base.object.destructorFunc = &mfmDestroyStackAllocator;

	// Successfully created a stack allocator
	return MFM_ERROR_OKAY;
}

mfmError mfmCreateStackAllocatorOnMemory(mfmStackAllocator ** stackAllocator, mfmU64 size, void * memory, mfmU64 memorySize)
{
	// Check if the arguments are valid
	if (stackAllocator == NULL || size == 0 || memory == NULL || memorySize < sizeof(mfmStackAllocator) + size)
		return MFM_ERROR_INVALID_ARGUMENTS;

	// Get data pointers
	*stackAllocator = (mfmStackAllocator*)((mfmU8*)memory + 0);
	(*stackAllocator)->onMemory = MFM_TRUE;
	(*stackAllocator)->stackSize = size;
	(*stackAllocator)->stackBegin = (mfmU8*)memory + sizeof(mfmStackAllocator);
	(*stackAllocator)->stackHead = (mfmU8*)memory + sizeof(mfmStackAllocator);

	// Set functions
	(*stackAllocator)->base.allocate = &mfmInternalStackAllocate;
	(*stackAllocator)->base.deallocate = &mfmInternalStackDeallocate;

	// Set destructor function
	(*stackAllocator)->base.object.destructorFunc = &mfmDestroyStackAllocator;

	// Successfully created a stack allocator
	return MFM_ERROR_OKAY;
}

void mfmDestroyStackAllocator(void * stackAllocator)
{
	if (((mfmStackAllocator*)stackAllocator)->onMemory == MFM_FALSE)
		free(stackAllocator);
}

mfmError mfmStackAllocate(mfmStackAllocator * allocator, void ** memory, mfmU64 size)
{
	// Check if allocation fits on the stack
	if (allocator->stackHead + size > allocator->stackBegin + allocator->stackSize)
		return MFM_ERROR_ALLOCATOR_OVERFLOW;

	// Get stack head and move it
	*memory = allocator->stackHead;
	allocator->stackHead += size;
	return MFM_ERROR_OKAY;
}

mfmError mfmStackSetHead(mfmStackAllocator * allocator, void * head)
{
	// Check if new head is out of bounds
	if (head < allocator->stackBegin || head > allocator->stackBegin + allocator->stackSize)
		return MFM_ERROR_OUT_OF_BOUNDS;

	// Set stack head
	allocator->stackHead = head;
	return MFM_ERROR_OKAY;
}
