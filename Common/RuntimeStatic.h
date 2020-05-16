#pragma once
#include <functional>
class RuntimeStaticBase
{
protected:
	static bool constructed;
	static RuntimeStaticBase* first;
	RuntimeStaticBase* next = nullptr;
	std::function<void(void*)> constructor;
	void(*destructor)(void*) = nullptr;
	void* dataPtr;
public:
	static void ConstructAll();
	static void DestructAll();
};
template <typename T>
class RuntimeStatic : public RuntimeStaticBase
{
private:
	struct CoverT
	{
		T value;
	};
public:
	alignas(T) char c[sizeof(T)];
	template <typename ... Args>
	RuntimeStatic(Args&&... args)
	{
		next = first;
		first = this;
		constructor = [=](void* p)->void
		{
			new (p)T(std::forward<Args>(std::remove_const_t<Args>(args))...);
		};
		destructor = [](void* p)->void
		{
			CoverT* ptr = (CoverT*)p;
			ptr->~CoverT();
		};
		dataPtr = c;
	}
};

