#include "hcmalloc.h"

// ntimes һ��������ͷ��ڴ�Ĵ���
// rounds �ִ�
void BenchmarkMalloc(size_t ntimes, size_t nworks, size_t rounds)
{
	std::vector<std::thread> vthread(nworks);
	std::atomic<size_t> malloc_costtime = 0;
	std::atomic<size_t> free_costtime = 0;

	for (size_t k = 0; k < nworks; ++k)
	{
		vthread[k] = std::thread([&, k]() {
			std::vector<void*> v;
			v.reserve(ntimes);

			for (size_t j = 0; j < rounds; ++j)
			{
				size_t begin1 = clock();
				for (size_t i = 0; i < ntimes; i++)
				{
					//v.push_back(malloc(16));
					v.push_back(malloc((16 + i) % 8192 + 1));
				}
				size_t end1 = clock();

				size_t begin2 = clock();
				for (size_t i = 0; i < ntimes; i++)
				{
					free(v[i]);
				}
				size_t end2 = clock();
				v.clear();

				malloc_costtime += (end1 - begin1);
				free_costtime += (end2 - begin2);
			}
		});
	}

	for (auto& t : vthread) {
		t.join();
	}

	cout << nworks << "���̲߳���ִ��" << rounds << "�ִΣ�ÿ�ִ�malloc"
		<< ntimes << "��: ����:" << malloc_costtime << "ms" << endl;
	cout << nworks << "���̲߳���ִ��" << rounds << "�ִΣ�ÿ�ִ�free"
		<< ntimes << "��: ����:" << free_costtime << "ms" << endl;
	cout << nworks << "���̲߳��� malloc && free" << nworks * rounds * ntimes << "�ִΣ��ܼƻ���"
		<< malloc_costtime + free_costtime << "ms" << endl;
}


// ���ִ������ͷŴ��� �߳��� �ִ�
void BenchmarkConcurrentMalloc(size_t ntimes, size_t nworks, size_t rounds)
{
	std::vector<std::thread> vthread(nworks);
	std::atomic<size_t> malloc_costtime = 0;
	std::atomic<size_t> free_costtime = 0;

	for (size_t k = 0; k < nworks; ++k)
	{
		vthread[k] = std::thread([&](std::function<void()> callback) {
			std::vector<void*> v;
			v.reserve(ntimes);

			for (size_t j = 0; j < rounds; ++j)
			{
				size_t begin1 = clock();
				for (size_t i = 0; i < ntimes; i++)
				{
					//v.push_back(hcmalloc(16));
					v.push_back(hcmalloc((16 + i) % 8192 + 1));
				}
				size_t end1 = clock();

				size_t begin2 = clock();
				for (size_t i = 0; i < ntimes; i++)
				{
					hcfree(v[i]);
				}
				size_t end2 = clock();
				v.clear();
				malloc_costtime += (end1 - begin1);
				free_costtime += (end2 - begin2);		
				callback();
			}
		}, ThreadCacheClean);
	}

	for (auto& t : vthread)
	{
		t.join();
	}

	cout << nworks << "���̲߳���ִ��" << rounds << "�ִΣ�ÿ�ִ�hcmalloc"
		<< ntimes << "��: ����:" << malloc_costtime << "ms" << endl;
	cout << nworks << "���̲߳���ִ��" << rounds << "�ִΣ�ÿ�ִ�hcfree"
		<< ntimes << "��: ����:" << free_costtime << "ms" << endl;
	cout << nworks << "���̲߳��� hcmalloc && hcfree" << nworks * rounds * ntimes << "�ִΣ��ܼƻ���" 
		<< malloc_costtime + free_costtime << "ms" << endl;
}





int main()
{
	size_t n = 10000;
	cout << "==========================================================" << endl;
	BenchmarkConcurrentMalloc(n, 4, 10);
	cout << endl << endl;

	BenchmarkMalloc(n, 4, 10);
	cout << "==========================================================" << endl;

	return 0;
}