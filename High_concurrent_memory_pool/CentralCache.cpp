#include "CentralCache.h"

CentralCache CentralCache::_sInst;
CentralCache* CentralCache::GetInstance() { return &_sInst; }

Span* CentralCache::GetOneSpan(SpanList& list, size_t size)
{
	//�鿴��ǰCentralCache�е�spanlist���Ƿ��л�����δ�����span
	Span* it = list.Begin();
	while (it != list.End()) {
		if (it->_freeList != nullptr) {
			return it;
		}
		else {
			it = it->_next;
		}
	}
	//list._mtx.unlock();//��central_cacheͰ���ͷţ���ʱ�������߳��ͷ��ڴ���������ᵼ������

	//���е��˴�ʱ��û�п���span��ֻ����Page_cache��ȡ
	PageCache::GetInstance()->_pageMutex.lock();
	Span* span = PageCache::GetInstance()->NewSpan(DataHandleRules::NumMovePage(size));
	PageCache::GetInstance()->_pageMutex.unlock();
	//����span�Ĵ���ڴ����ʼ��ַ�ʹ���ڴ�Ĵ�С(�ֽ���)
	char* start = (char*)(span->_pageid << PAGE_SHIFT);
	size_t bytes = span->_num << PAGE_SHIFT;
	char* end = start + bytes;
	//������ڴ��г�����������������
	span->_freeList = start;
	start += size;
	void* tail = span->_freeList;
	while (start <= end) {
		NextObj(tail) = start;
		tail = start;
		start += size;
	}

	//list._mtx.lock();
	list.PushFront(span);//��span�����Ӧ��Ͱ��
	return span;
}

size_t CentralCache::FetchMemoryBlock(void*& start, void*& end, size_t batchNum, size_t size)
{
	size_t index = DataHandleRules::Index(size);
	_spanLists[index]._mtx.lock();
	Span* span = GetOneSpan(_spanLists[index], batchNum);
	assert(span);
	assert(span->_freeList);

	start = span->_freeList;
	end = start;
	size_t count = 0, actualNum = 1;
	while (count <= batchNum - 1 && NextObj(end) != nullptr) {
		end = NextObj(end);
		++count;
		++actualNum;
	}
	span->_freeList = NextObj(end);
	NextObj(end) = nullptr;

	_spanLists[index]._mtx.unlock();
	return actualNum;
}
