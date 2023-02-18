#pragma once
#include <iostream>
#include <thread>
#include <cassert>
using std::cout;
using std::endl;

static const size_t MAX_BYTES = 256 * 1024;//����threadcache���������ֽ���
static const size_t NFREELIST = 208;//Ͱ��


class FreeList//�����������ڹ����зֹ���С���ڴ�
{
private:
	static void*& NextObj(void* obj) { return *(void**)obj; }

public:
	void Push(void* obj)
	{
		assert(obj != nullptr);
		NextObj(obj) = _freelist;
		_freelist = obj;
	}
	void* Pop()
	{
		assert(_freelist != nullptr);
		void* obj = _freelist;
		_freelist = NextObj(obj);
		return obj;
	}
	bool IsEmpty() { return _freelist == nullptr; }

private:
	void* _freelist = nullptr;
};



//���롢ӳ�����
class AlignMappingRules
{
	// ������������10%���ҵ�����Ƭ�˷�
	// [1,128]					8byte����	    freelist[0,16)
	// [128+1,1024]				16byte����	    freelist[16,72)
	// [1024+1,8*1024]			128byte����	    freelist[72,128)
	// [8*1024+1,64*1024]		1024byte����     freelist[128,184)
	// [64*1024+1,256*1024]		8*1024byte����   freelist[184,208)

public://���϶������
	/*static inline size_t _AlignUp(size_t size, size_t alignNum)
	{
		size_t alignRet = 0;
		if (size % alignNum != 0) {
			alignRet = ((size / alignNum) + 1) * alignNum;
		}
		else {
			alignRet = size;
		}
		return alignRet;
	}*/
	static inline size_t _AlignUp(size_t bytes, size_t alignNum) { return ((bytes + alignNum - 1) & ~(alignNum - 1)); }

	static inline size_t AlignUp(size_t size)
	{
		if (size < 128) return _AlignUp(size, 8);
		else if (size < 1024) return _AlignUp(size, 16);
		else if (size < 8 * 1024) return _AlignUp(size, 128);
		else if (size < 64 * 1024) return _AlignUp(size, 1024);
		else if (size < 256 * 1024) return _AlignUp(size, 8 * 1024);
		else exit(-1);
	}


public://ӳ����� : ����ӳ�������һ����������Ͱ
	
	/*size_t _Index(size_t bytes, size_t alignNum)
	{
		if (bytes % alignNum == 0) {
			return bytes / alignNum - 1;
		}
		else {
			return bytes / alignNum;
		}
	}*/
	//...

	static inline size_t _Index(size_t bytes, size_t align_shift) { return ((bytes + (1 << align_shift) - 1) >> align_shift) - 1; }

	static inline size_t Index(size_t bytes)
	{
		assert(bytes <= MAX_BYTES);

		static int group_array[4] = { 16, 56, 56, 56 };
		if (bytes <= 128) {
			return _Index(bytes, 3);//8 ���� 2��3�η�
		}
		else if (bytes <= 1024) {
			return _Index(bytes - 128, 4) + group_array[0];
		}
		else if (bytes <= 8 * 1024) {
			return _Index(bytes - 1024, 7) + group_array[1] + group_array[0];
		}
		else if (bytes <= 64 * 1024) {
			return _Index(bytes - 8 * 1024, 10) + group_array[2] + group_array[1] + group_array[0];
		}
		else if (bytes <= 256 * 1024) {
			return _Index(bytes - 64 * 1024, 13) + group_array[3] + group_array[2] + group_array[1] + group_array[0];
		}
		else {
			assert(false);
		}

		return -1;
	}
};