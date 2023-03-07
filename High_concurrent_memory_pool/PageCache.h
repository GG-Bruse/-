#pragma once
#include "Common.h"

//��������ģʽ
class PageCache
{
public:
	static PageCache* GetInstance();
	Span* NewSpan(size_t k);

private:
	PageCache() {}
	PageCache(const PageCache&) = delete;
	static PageCache _sInst;
	SpanList _spanLists[NPAGES];
public:
	std::mutex _pageMutex;//����page_cache����
};