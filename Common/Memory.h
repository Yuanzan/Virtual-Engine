#pragma once
#include <cstdlib>
inline constexpr void* aligned_malloc(size_t size, size_t alignment)
{
	if (alignment & (alignment - 1))
	{
		return nullptr;
	}
	else
	{
		void* praw = malloc(sizeof(void*) + size + alignment);
		if (praw)
		{
			void* pbuf = reinterpret_cast<void*>(reinterpret_cast<size_t>(praw) + sizeof(void*));
			void* palignedbuf = reinterpret_cast<void*>((reinterpret_cast<size_t>(pbuf) | (alignment - 1)) + 1);
			(static_cast<void**>(palignedbuf))[-1] = praw;

			return palignedbuf;
		}
		else
		{
			return nullptr;
		}
	}
}

inline void aligned_free(void* palignedmem)
{
	free(reinterpret_cast<void*>((static_cast<void**>(palignedmem))[-1]));
}

template <typename AllocFunc>
inline constexpr void aligned_free(void* palignedmem, const AllocFunc& func)
{
	func(reinterpret_cast<void*>((static_cast<void**>(palignedmem))[-1]));
}
//allocFunc:: void* operator()(size_t)
template <typename AllocFunc>
inline constexpr void* aligned_malloc(size_t size, size_t alignment, const AllocFunc& allocFunc)
{
	if (alignment & (alignment - 1))
	{
		return nullptr;
	}
	else
	{
		void* praw = allocFunc(sizeof(void*) + size + alignment);
		if (praw)
		{
			void* pbuf = reinterpret_cast<void*>(reinterpret_cast<size_t>(praw) + sizeof(void*));
			void* palignedbuf = reinterpret_cast<void*>((reinterpret_cast<size_t>(pbuf) | (alignment - 1)) + 1);
			(static_cast<void**>(palignedbuf))[-1] = praw;

			return palignedbuf;
		}
		else
		{
			return nullptr;
		}
	}
}