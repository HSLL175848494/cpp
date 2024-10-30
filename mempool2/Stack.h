#ifndef  HSLL_STACK
#define HSLL_STACK

#include<atomic>

namespace HSLL
{
	template<class T>
	class LockFreeStack
	{
		struct Confirm
		{
			T* head;
			unsigned int count;
		};

		std::atomic<Confirm> Head{};

	public:

		void Push(T* Ptr)
		{
			Confirm Old = Head.load(std::memory_order_relaxed);
			Confirm New{ Ptr };

		Recompare:

			Ptr->next = Old.head;
			New.count = Old.count + 1;
			if (Head.compare_exchange_weak(Old, New,
				std::memory_order_release, std::memory_order_relaxed))
				return;
			goto Recompare;
		}

		void Push_List(T* List)
		{
			Confirm New{ List };
			while (List->next)
				List = List->next;
			Confirm Old = Head.load(std::memory_order_relaxed);

		Recompare:

			List->next = Old.head;
			New.count = Old.count + 1;
			if (Head.compare_exchange_weak(Old, New,
				std::memory_order_release, std::memory_order_relaxed))
				return;
			goto Recompare;
		}

		void Push_List(T* List, T* Tail)
		{
			Confirm New{ List };
			Confirm Old = Head.load(std::memory_order_relaxed);

		Recompare:

			Tail->next = Old.head;
			New.count = Old.count + 1;
			if (Head.compare_exchange_weak(Old, New,
				std::memory_order_release, std::memory_order_relaxed))
				return;

			goto Recompare;
		}

		T* Pop()
		{
			Confirm Old = Head.load(std::memory_order_acquire);
			Confirm New;

		Recompare:

			if (!Old.head)
				return nullptr;

			New = { Old.head->next,Old.count + 1 };
			if (Head.compare_exchange_weak(Old, New,
				std::memory_order_acquire, std::memory_order_acquire))
				return Old.head;

			goto Recompare;
		}

		T* PopList(unsigned int Num)
		{
			T* List = nullptr;
			T* Tail = nullptr;
		Reget:

			if (!Num)
				return List;

			Confirm Old = Head.load(std::memory_order_acquire);
			Confirm New;

		Recompare:

			if (!Old.head)
				return List;

			New = { Old.head->next,Old.count + 1 };
			if (Head.compare_exchange_weak(Old, New,
				std::memory_order_acquire, std::memory_order_acquire))
			{
				if (List)
				{
					Tail->next = Old.head;
					Tail = Tail->next;
				}
				else
				{
					List = Old.head;
					Tail = List;
				}
				Tail->next = nullptr;
				Num--;
				goto Reget;
			}

			goto Recompare;
		}

		T* PopAll()
		{
			T* List = nullptr;
			T* Tail = nullptr;
		Reget:

			Confirm Old = Head.load(std::memory_order_acquire);
			Confirm New;

		Recompare:

			if (!Old.head)
				return List;

			New = { Old.head->next,Old.count + 1 };
			if (Head.compare_exchange_weak(Old, New,
				std::memory_order_acquire, std::memory_order_acquire))
			{
				if (List)
				{
					Tail->next = Old.head;
					Tail = Tail->next;
				}
				else
				{
					List = Old.head;
					Tail = List;
				}
				Tail->next = nullptr;
				goto Reget;
			}

			goto Recompare;
		}
	};

	template<class T>
	class SimpleStack
	{
		T* Head{ nullptr };

	public:

		void Push(T* Ptr)
		{
			Ptr->next = Head;
			Head = Ptr;
		}

		void Push_List(T* List)
		{
			T* Tail = List;
			while (Tail->next) {
				Tail = Tail->next;
			}
			Tail->next = Head;
			Head = List;
		}

		void Push_List(T* List, T* Tail)
		{
			Tail->next = Head;
			Head = List;
		}

		T* Pop()
		{
			T* OldNode = Head;
			if (OldNode)
			Head = OldNode->next;
			return OldNode;
		}

		T* PopList(unsigned int Num)
		{
			T* List = nullptr;
			T* Tail = nullptr;

			while (Num-- > 0 && Head)
			{
				T* OldNode = Head;
				Head = OldNode->next;

				if (List) 
				{
					Tail->next = OldNode;
					Tail = Tail->next;
				}
				else 
				{
					List = OldNode;
					Tail = List;
				}
				Tail->next = nullptr;
			}
			return List;
		}

		T* PopAll()
		{
			T* List = Head;
			Head = nullptr;
			return List;
		}
	};
}


#endif // ! HSLL_STACK

