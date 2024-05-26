#pragma once

#include <glm/glm.hpp>

namespace wc 
{
	template<typename T, size_t Size>
	class List 
	{
		T m_Data[Size];
	public:
		size_t counter = 0;

		T& operator[](const size_t& index) { return m_Data[index]; }
		const T& operator[](const size_t& index) const { return m_Data[index]; }

		T* Data() { return &m_Data[0]; }
		const T* Data() const { return &m_Data[0]; }

		size_t PushBack(const T& type) 
		{ 
			Data[counter++] = type;
			return counter - 1;
		}

		size_t Size() const { return counter; }
		constexpr size_t Capacity() const { return Size; }
		constexpr size_t ByteSize() const { return Size * sizeof(T); }
	};

	template<typename T, size_t Size>
	class PointerList 
	{
	public:
		size_t counter = 0;
		T* Data = nullptr;

		T& operator[](const size_t& index) { return Data[index]; }
		const T& operator[](const size_t& index) const { return Data[index]; }

		size_t PushBack(const T& type) 
		{
			Data[counter++] = type;
			return counter - 1;
		}

		size_t PushBack() {	return counter++; }

		size_t Size() const { return counter; }
		constexpr size_t Capacity() const { return Size; }
		constexpr size_t ByteSize() const { return Size * sizeof(T); }
	};
}