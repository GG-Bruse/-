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
	list._mtx.unlock();//��central_cacheͰ���ͷţ���ʱ�������߳��ͷ��ڴ���������ᵼ������

	//���е��˴�ʱ��û�п���span��ֻ����Page_cache��ȡ
	PageCache::GetInstance()->_pageMutex.lock();
	Span* span = PageCache::GetInstance()->NewSpan(DataHandleRules::NumMovePage(size));
	span->_IsUse = true;
	span->_objSize = size;
	PageCache::GetInstance()->_pageMutex.unlock();

	//����span�Ĵ���ڴ����ʼ��ַ�ʹ���ڴ�Ĵ�С(�ֽ���)
	char* start = (char*)(span->_pageId << PAGE_SHIFT);
	size_t bytes = span->_num << PAGE_SHIFT;
	char* end = start + bytes;

	//������ڴ��г�����������������
	span->_freeList = start;
	start += size;
	void* tail = span->_freeList;
	while (start < end) {
		NextObj(tail) = start;
		tail = NextObj(tail);
		start += size;
	}

	list._mtx.lock();
	list.PushFront(span);//��span�����Ӧ��Ͱ��
	return span;
}



size_t CentralCache::FetchMemoryBlock(void*& start, void*& end, size_t batchNum, size_t size)
{
	size_t index = DataHandleRules::Index(size);
	_spanLists[index]._mtx.lock();
	Span* span = GetOneSpan(_spanLists[index], size);
	assert(span);
	assert(span->_freeList);

	//��span�л�ȡbatchNum���ڴ�飬���������ж��ٻ�ȡ����
	start = span->_freeList;
	end = span->_freeList;
	size_t count = 0, actualNum = 1;
	while (NextObj(end) != nullptr && count < batchNum - 1) {
		end = NextObj(end);
		++count;
		++actualNum;
	}
	span->_freeList = NextObj(end);
	NextObj(end) = nullptr;
	span->_use_count += actualNum;

	_spanLists[index]._mtx.unlock();
	return actualNum;
}



void CentralCache::ReleaseListToSpans(void* start, size_t size)
{
	size_t bucketIndex = DataHandleRules::Index(size);
	_spanLists[bucketIndex]._mtx.lock();
	while (start) 
	{
		void* next = NextObj(start);
		Span* span = PageCache::GetInstance()->MapObjectToSpan(start);
		NextObj(start) = span->_freeList;
		span->_freeList = start;
		span->_use_count--;

		if (span->_use_count == 0) //˵����span�зֵ��ڴ�鶼�Ѿ��黹�ˣ���span���Թ黹��PageCache
		{
			_spanLists[bucketIndex].Erase(span);
			span->_freeList = nullptr;//�������壬��ʹ��ҳ��ת��Ϊ��ַ���ҵ��ڴ�
			span->_next = nullptr;
			span->_prev = nullptr;

			_spanLists[bucketIndex]._mtx.unlock();
			PageCache::GetInstance()->_pageMutex.lock();
			PageCache::GetInstance()->ReleaseSpanToPageCache(span);
			PageCache::GetInstance()->_pageMutex.unlock();
			_spanLists[bucketIndex]._mtx.lock();
		}
		start = next;
	}
	_spanLists[bucketIndex]._mtx.unlock();
}