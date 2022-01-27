#include <iostream>
#include <assert.h>

using Int32U	=	typename unsigned int;
using Int64U	=	typename unsigned int;

#define DeleteSafe(x)			\
do {							\
	if (x) {					\
		delete x;				\
	}							\
	x = nullptr;				\
} while (0);					

#define DeleteArraySafe(x)		\
do {							\
	if (x) {					\
		delete[] x;				\
	}							\
	x = nullptr;				\
} while (0);					



constexpr Int32U PrimeInt32U_v = 0x087b840FU;			// 1�� �α� �ϰų� - 142,312,463
constexpr Int64U PrimeInt64U_v = 0x0000050B00000002ULL;	// 5�� �α� �ϰų� - 5,544,802,779,138
constexpr Int64U HashXorKey_v = 0x3e4dc77d;				// Xor Ű�� �ϰų� - 1,045,284,733

template <typename T>
struct Hasher
{
	constexpr Int32U operator()(T val) const {
		return ((val ^ HashXorKey_v) % PrimeInt32U_v) * PrimeInt32U_v;
	}
};


template <>
struct Hasher<float>
{
	union Bit
	{
		float val;
		Int32U u;
	};

	constexpr Int32U operator()(float val) const {
		return Hasher<Int32U>()(Bit{ val }.u);
	}
};

template <>
struct Hasher<double>
{
	union Bit
	{
		double val;
		Int64U u;
	};

	constexpr Int32U operator()(double val) const {
		return Hasher<float>()(val);
	}
};


template <typename TKey, typename TValue>
struct LinkedListNode
{
	TKey Key;
	TValue Value;
	LinkedListNode* Next = nullptr;

	~LinkedListNode() {
	}
};



template <typename TKey, typename TValue>
struct Bucket
{
private:
	using TNode = typename LinkedListNode<TKey, TValue>;
public:
	TNode* Tail;
	TNode* Head;
	int Count;

	Bucket() {
		TNode* pDummy = new TNode;

		Head = pDummy;
		Tail = pDummy;
		Count = 0;
	}

	~Bucket() {
		Clear();

		DeleteSafe(Head);
	}

	TNode* FirstElement() {
		return Head->Next;
	}

	TNode* LastElement() {
		return Tail;
	}

	template <typename KeyType, typename ValueType>
	void Add(KeyType&& key, ValueType&& val) {
		TNode* pNewNode = new TNode;
		pNewNode->Key = std::forward<KeyType>(key);
		pNewNode->Value = std::forward<ValueType>(val);

		Tail->Next = pNewNode;
		Tail = Tail->Next;
		Count++;
	}

	TNode* Find(const TKey& key) const {
		TNode* pCur = Head->Next;

		while (pCur != nullptr) {
			if (pCur->Key == key) {
				break;
			}

			pCur = pCur->Next;
		}

		return pCur;
	}

	bool Remove(const TKey& key) {
		TNode* pPrev = Head;
		TNode* pCur = Head->Next;

		while (pCur != nullptr) {

			if (pCur->Key == key) {
				break;
			}

			pPrev = pCur;
			pCur = pCur->Next;
		}

		if (pCur == nullptr) {
			return false;
		}

		if (pCur == Tail)
			Tail = pPrev;
		pPrev->Next = pCur->Next;
		DeleteSafe(pCur);
		Count--;
		return true;
	}

	void Clear() {
		TNode* pDel = nullptr;
		TNode* pCur = FirstElement();

		while (pCur != nullptr) {
			pDel = pCur;
			pCur = pCur->Next;

			DeleteSafe(pDel);
		}

		Tail = Head;
	}

	void PrintInfo() {
		TNode* pCur = FirstElement();
		while (pCur != nullptr) {
			printf("Ű : %d ", pCur->Key);
			pCur = pCur->Next;
		}
		printf("\n");
	}
};


template <typename TKey, typename TValue, typename THasher = Hasher<TKey>>
class HashMap
{
private:
	using TNode				= typename LinkedListNode<TKey, TValue>;
	using TBucket			= typename Bucket<TKey, TValue>;

	static constexpr Int32U	ms_iBucketExpandingFactor = 8;	// ���̺� ũ�⸸ŭ �����Ͱ� ���� Ȯ���ϴµ� ��質 Ȯ���� ��
	static constexpr Int32U	ms_iTableDefaultCapacity = 16;	// ���̺� �ʱ� ũ��
public:
	HashMap(int capacity = ms_iTableDefaultCapacity) {
		m_Table = new TBucket[capacity];
		m_iSize = 0;
		m_iCapacity = capacity;
		m_iMask = capacity - 1;
	}
	~HashMap() {
		DeleteArraySafe(m_Table);
	}
public:


	template <typename KeyType, typename ValueType>
	void Add(KeyType&& key, ValueType&& val) {
		// �̹� �����ϴ� ��� ���� �������ش�.
		TNode* pFind = FindNode(key);

		if (pFind != nullptr) {
			pFind->Value = std::forward<ValueType>(val);
			return;
		}

		if (m_iSize >= m_iCapacity) {
			Resize(m_iCapacity * ms_iBucketExpandingFactor);
		}

		Int32U iHashVal = Hash(key);
		m_Table[iHashVal].Add(
			std::forward<KeyType>(key), 
			std::forward<ValueType>(val)
		);
		m_iSize++;
	}

	bool Remove(const TKey& key) {
		Int32U iHashVal = Hash(key);
		if (m_Table[iHashVal].Remove(key)) {
			m_iSize--;
			return true;
		}

		return false;
	}

	TValue& operator[](const TKey& key) const {
		TNode* pFind = FindNode(key);

		if (pFind == nullptr) {
			throw std::out_of_range("�ش� Ű���� �ش��ϴ� �������� �����ϴ�.");
		}

		return pFind->Value;
	}

	void Resize(const int capacity) {
		TBucket* pNewTable = new TBucket[capacity];
		int iPrevCapacity = m_iCapacity;
		
		m_iCapacity = capacity;
		m_iMask = capacity - 1;

		for (int i = 0; i < iPrevCapacity; i++) {
			if (m_Table[i].Count == 0) {
				continue;
			}

			TNode* pCur = m_Table[i].FirstElement();

			while (pCur != nullptr) {
				Int32U uiHash = Hash(pCur->Key);
				pNewTable[uiHash].Add(
					std::move(pCur->Key),
					std::move(pCur->Value)
				);
				pCur = pCur->Next;
			}
		}

		DeleteArraySafe(m_Table);

		m_Table = pNewTable;
	}

	void PrintInfo() {
		for (int i = 0; i < m_iCapacity; i++) {
			if (m_Table[i].Count <= 0) {
				continue;
			}
			printf("[%d] ������ �� : %d : ", i, m_Table[i].Count);
			m_Table[i].PrintInfo();
		}
		printf("=====\n");
	}

	int GetSize() const {
		return m_iSize;
	}
private:
	Int32U Hash(const TKey& val) const {
		return m_Hasher(val) & m_iMask;
	}

	TNode* FindNode(const TKey& key) const {
		Int32U iHashVal = Hash(key);
		return FindNode(key, iHashVal);
	}

	TNode* FindNode(const TKey& key, Int32U hashval) const {
		TBucket& bucket = m_Table[hashval];
		return bucket.Find(key);
	}


private:
	THasher m_Hasher;
	TBucket* m_Table;
	int m_iSize;
	int m_iCapacity;
	int m_iMask;
};

struct A
{
	int a;
	int b;
};

int main() {
	{
		int maxSize = 20;
		HashMap<int, A> a;

		printf("������ �߰� �׽�Ʈ\n");
		for (int i = 0; i < maxSize; i++) {
			a.Add(i, A{ 5 });
		}
		
		a.PrintInfo();

		printf("������ ���� �׽�Ʈ\n");
		for (int i = 0; i < maxSize; i++) {
			if (a.Remove(i)) {
				printf("%d �����Ϸ� (���� ������ �� : %d)\n", i, a.GetSize());
			} else {
				printf("%d ��������\n", i);
			}
		}

		a.PrintInfo();
		A modifyValue{ 6 };

		printf("������ ���� �׽�Ʈ\n");
		for (int i = 0; i < maxSize; i++) {
			a.Add(i,A{7});
			a[i] = modifyValue;
		}

		for (int i = 0; i < maxSize; i++) {
			if (a[i].a== modifyValue.a) {
				printf("%d ���� ����~!! ����� �� : %d\n", i, a[i].a);
			} else {
				printf("%d ���� ����\n", i);
			}
		}
	}

	_CrtDumpMemoryLeaks();
}

