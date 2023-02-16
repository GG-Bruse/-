#pragma once
#include "common.h"

class ThreadCache
{
public:
	void* Allocate(size_t);
	void* Deallocate(void* , size_t);
	void* FetchFromCentralCache(size_t index, size_t size);//�����Ļ�������ȡ�ڴ�
private:
	FreeList _freelists[NFREELIST];
};