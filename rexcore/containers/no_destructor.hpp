#pragma once

namespace RexCore
{
	template<typename T>
	class NoDestructor
	{
	public:
		template<typename ...Args>
		NoDestructor(Args&& ...args)
		{
			new (m_instance) T(std::forward<Args>(args)...);
		}

		T& operator*() { return *reinterpret_cast<T*>(m_instance); }
		const T& operator*() const { return *reinterpret_cast<const T*>(m_instance); }

		T* operator->() { return reinterpret_cast<T*>(m_instance); }
		const T* operator->() const { return reinterpret_cast<const T*>(m_instance); }

	private:
		alignas(T) Byte m_instance[sizeof(T)];
	};
}