#include "PageCache.h"

PageCache PageCache::_sInst;
PageCache* PageCache::GetInstance() { return &_sInst; }

Span* PageCache::NewSpan(size_t k)//��ȡһ��kҳ��span
{
	assert(k > 0 && k < NPAGES);

	//����k��Ͱ���Ƿ��п��õ�span
	if (!_spanLists[k].IsEmpty()) return _spanLists[k].PopFront();

	//��������Ͱ���Ƿ��п��õ�span,����������з�
	for (int i = k + 1; i < NPAGES; ++i) {
		if (!_spanLists[i].IsEmpty())
		{
			Span* nSpan = _spanLists[i].PopFront();
			Span* kSpan = new Span;

			kSpan->_pageId = nSpan->_pageId;
			nSpan->_pageId += k;
			kSpan->_num = k;
			nSpan->_num -= k;

			_spanLists[nSpan->_num].PushFront(nSpan);

			//�洢nSpan����βҳ����nSpan����ӳ���ϵ������PageCache����Central_Cache��spanʱ���кϲ�ҳ
			_idSpanMap[nSpan->_pageId] = nSpan;
			_idSpanMap[nSpan->_pageId + nSpan->_num - 1] = nSpan;
			//����id��span��ӳ�䣬����central cache����С���ڴ�ʱ�����Ҷ�Ӧ��span
			for (PAGE_ID i = 0; i < kSpan->_num; ++i) {
				_idSpanMap[kSpan->_pageId + i] = kSpan;
			}

			return kSpan;
		}
	}

	//���д˴���˵��PageCache��û�п��õ�span,�����������128ҳ��span
	Span* newSpan = new Span;
	void* address = SystemAlloc(NPAGES - 1);
	newSpan->_pageId = (PAGE_ID)address >> PAGE_SHIFT;
	newSpan->_num = NPAGES - 1;
	_spanLists[newSpan->_num].PushFront(newSpan);
	return NewSpan(k);//ͨ���ݹ齫��span��ΪСspan
}



//��ȡ��С�ڴ�鵽span��ӳ��
Span* PageCache::MapObjectToSpan(void* obj)
{
	PAGE_ID id = ((PAGE_ID)obj >> PAGE_SHIFT);
	std::unordered_map<PAGE_ID,Span*>::iterator ret = _idSpanMap.find(id);
	if (ret != _idSpanMap.end()) {
		return ret->second;
	}
	else {
		assert(false);
		return nullptr;
	}
}



void PageCache::ReleaseSpanToPageCache(Span* span)
{
	while (1)//��ǰ�ϲ�
	{
		PAGE_ID prevId = span->_pageId - 1;
		std::unordered_map<PAGE_ID, Span*>::iterator ret = _idSpanMap.find(prevId);
		if (ret == _idSpanMap.end()) break;

		//ǰ������ҳ��span����ʹ����
		Span* prevSpan = ret->second;
		if (prevSpan->_IsUse == true) break;

		if (prevSpan->_num + span->_num > NPAGES - 1) break;//���ϲ������128ҳ���޷�����

		span->_pageId = prevSpan->_pageId;
		span->_num += prevSpan->_num;
		_spanLists[prevSpan->_num].Erase(prevSpan);
		delete prevSpan;
	}
	while (1)//���ϲ�
	{
		PAGE_ID nextId = span->_pageId + span->_num;
		std::unordered_map<PAGE_ID, Span*>::iterator ret = _idSpanMap.find(nextId);
		if (ret == _idSpanMap.end()) break;

		//��������ҳ��span����ʹ����
		Span* nextSpan = ret->second;
		if (nextSpan->_IsUse == true) break;

		if (nextSpan->_num + span->_num > NPAGES - 1) break;//���ϲ������128ҳ���޷�����

		span->_num += nextSpan->_num;
		_spanLists[nextSpan->_num].Erase(nextSpan);
		delete nextSpan;
	}

	_spanLists[span->_num].PushFront(span);
	_idSpanMap[span->_pageId] = span;
	_idSpanMap[span->_pageId + span->_num - 1] = span;
	span->_IsUse = false;
}