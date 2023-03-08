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

			kSpan->_pageid = nSpan->_pageid;
			nSpan->_pageid += k;
			kSpan->_num = k;
			nSpan->_num -= k;

			_spanLists[nSpan->_num].PushFront(nSpan);

			//�洢nSpan����λҳ����nSpan����ӳ���ϵ������PageCache����Central_Cache��spanʱ���кϲ�ҳ
			_idSpanMap[nSpan->_pageid] = nSpan;
			_idSpanMap[nSpan->_pageid + nSpan->_num - 1] = nSpan;
			//����id��span��ӳ�䣬����central cache����С���ڴ�ʱ�����Ҷ�Ӧ��span
			for (PAGE_ID i = 0; i < kSpan->_num; ++i) {
				_idSpanMap[kSpan->_pageid + i] = kSpan;
			}

			return kSpan;
		}
	}

	//���д˴���˵��PageCache��û�п��õ�span,�����������128ҳ��span
	Span* newSpan = new Span;
	void* address = SystemAlloc(NPAGES - 1);
	newSpan->_pageid = (PAGE_ID)address >> PAGE_SHIFT;
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

}