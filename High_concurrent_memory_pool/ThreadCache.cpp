#include "ThreadCache.h"
#include "CentralCache.h"

void* ThreadCache::Allocate(size_t size) 
{
	assert(size <= MAX_BYTES);
	size_t alignSize = DataHandleRules::AlignUp(size);
	size_t bucketIndex = DataHandleRules::Index(size);
	if (!_freeLists[bucketIndex].IsEmpty()){
		return _freeLists[bucketIndex].Pop();
	}
	else {
		return FetchFromCentralCache(bucketIndex, alignSize);
	}
}
void ThreadCache::Deallocate(void* ptr, size_t size)
{
	assert(ptr != nullptr);
	assert(size <= MAX_BYTES);
	size_t bucketIndex = DataHandleRules::Index(size);
	_freeLists[bucketIndex].Push(ptr);
}

void* ThreadCache::FetchFromCentralCache(size_t index, size_t size)
{
	//����ʼ���������㷨
	//������һ��ʼһ������central_cache��Ҫ̫�࣬����ʹ�ò���
	size_t batchNum = std::min(_freeLists[index].MaxSize(), DataHandleRules::MoveSize(size));
	if (batchNum == _freeLists[index].MaxSize()) {
		_freeLists[index].MaxSize() += 2;//��������Ҫsize��С���ڴ棬��ôbatchNum�ͻ᲻������ֱ������
	}

	void* start = nullptr, * end = nullptr;
	size_t actualNum = CentralCache::GetInstance()->FetchMemoryBlock(start, end, batchNum, size);
	assert(actualNum > 1);//���ٷ���һ��

	if (actualNum == 1) {
		assert(start == end);
		return start;
	}
	else{
		_freeLists[index].PushRange(NextObj(start), end);//��������ڴ�ͷ��thread_cache����������
		return start;//����һ���ڴ�鷵�ظ�����ʹ��
	}
}

