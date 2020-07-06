#ifndef RAYTRACER_VECTOR_H_
#define RAYTRACER_VECTOR_H_

#include <numeric>
#include <cmath>
#include <iostream>
#include <tuple>

template <typename T>
constexpr T two = T(2);
template <typename T>
constexpr T one = T(1);
template <typename T>
constexpr T zero = T(0);

template <typename T, size_t N>
struct Vector
{
	static bool back;

	T data[N];

	Vector() : data()
	{
		
	}

	// Expand vector to one with bigger size using val
	explicit Vector(const Vector <T, N - 1> &v, const T &val)
	{
		std::copy(v.data, v.data + N - 1, data);
		data[N - 1] = val;
	}

	//explicit
	Vector(const Vector <T, N + 1> &v)
	{
		std::copy(v.data, v.data + N, data);
	}

	template <typename ...Args>
	Vector(Args ... args) : data{args...}
	{
		static_assert(sizeof...(Args) == N, "Invalid number of elements passed to constructor\n");
	}

	Vector operator +(const Vector <T, N> &rhs) const
	{
		Vector <T, N> res;
		for (size_t i = 0; i < N; ++i)
		{
			res.data[i] = data[i] + rhs.data[i];
		}
		return res;
	}

	void operator +=(const Vector <T, N> &rhs)
	{
		for (size_t i = 0; i < N; ++i)
		{
			data[i] += rhs.data[i];
		}
	}

	Vector operator -(const Vector <T, N> &rhs) const
	{
		Vector <T, N> res;
		for (size_t i = 0; i < N; ++i)
		{
			res[i] = data[i] - rhs.data[i];
		}
		return res;
	}

	void operator -=(const Vector <T, N> &rhs)
	{
		for (size_t i = 0; i < N; ++i)
		{
			data[i] -= rhs.data[i];
		}
	}

	T operator *(const Vector <T, N> &rhs) const
	{
		return dot(rhs);
	}

	Vector <T, N> operator *(const T &rhs) const
	{
		Vector <T, N> res(*this);
		res *= rhs;
		return res;
	}

	void operator *=(const T &rhs)
	{
		for (size_t i = 0; i < N; ++i)
		{
			data[i] *= rhs;
		}
	}

	Vector <T, N> operator /(const T &rhs) const
	{
		Vector <T, N> res;
		for (size_t i = 0; i < N; ++i)
		{
			res.data[i] = data[i] / rhs;
		}
		return res;
	}

	void operator /=(const T &rhs)
	{
		for (size_t i = 0; i < N; ++i)
		{
			data[i] /= rhs;
		}
	}

	Vector <T, N> operator /(const Vector <T, N> &rhs)
	{
		Vector <T, N> res;
		for (size_t i = 0; i < N; ++i)
		{
			res[i] = data[i] / rhs[i];
		}
		return res;
	}

	Vector <T, N> operator -() const
	{
		Vector res = (*this);
		for (size_t i = 0; i < N; ++i)
		{
			res[i] = -res[i];
		}
		return res;
	}

	bool operator == (const Vector <T, N> &rhs) const
	{
		for (size_t i = 0; i < N; ++i)
		{
			if (data[i] != rhs[i])
				return false;
		}
		return true;
	}

	bool operator != (const Vector <T, N> &rhs) const
	{
		return !((*this) == rhs);
	}

	const T & operator [](size_t ind) const
	{
		return data[ind];
	}

	T & operator [](size_t ind)
	{
		return data[ind];
	}

	void normalize()
	{		
		(*this) /= this->length();
	}

	T length() const
	{
		return std::sqrt(sqrLength());
	}

	// Square length
	// Can be used to avoid unnecessary root computation
	T sqrLength() const
	{
		T sum{};
		for (size_t i = 0; i < N; ++i)
		{
			sum += data[i] * data[i];
		}
		return sum;
	}

	T dot(const Vector <T, N> &rhs) const
	{
		T sum{};
		for (size_t i = 0; i < N; ++i)
		{
			sum += data[i] * rhs.data[i];
		}
		return sum;
	}

	// Only available for Vector with size = 3
	template <typename ReturnType = Vector <T, 3>>
	std::enable_if_t <N == 3, ReturnType>
	cross(const Vector <T, 3> &v) const
	{
		return Vector <T, 3> {
			data[1] * v[2] - data[2] * v[1],
			data[2] * v[0] - data[0] * v[2],
			data[0] * v[1] - data[1] * v[0]
		};
	}

	Vector <T, N> reflect(const Vector <T, N> &surfaceNormal) const
	{
		return (*this) - surfaceNormal * (two <T> * ((*this) * surfaceNormal));
	}

	// <refracted vector, was really refracted (not reflected), normal negated>
	std::tuple <Vector <T, N>, bool, bool> refract(Vector <T, N> normal, T iof1, T iof2) const
	{
		bool negated = false;
		T dot = normal * (*this);
		if (dot > zero<T>)
		{
			negated = true;
			dot = -dot;
			normal = -normal;
			T t = iof2;
			iof2 = iof1;
			iof1 = t;
		}

		T k = iof1 / iof2;
		T d = one<T> - k * k * (one<T> - (dot * dot));

		if (d < zero<T>)
			return { reflect(normal), false, negated };

		return { (*this) * k + normal * (k * (-dot) - std::sqrt(d)), true, negated };
	}
};

template <typename T, size_t N>
std::ostream & operator <<(std::ostream &os, const Vector <T, N> &rhs)
{
	os << '[';
	for (size_t i = 0; i < N; ++i)
	{
		os << rhs.data[i];
		if (i != (N - 1u))
			os << ", ";
	}
	os << ']';
	return os;
}

using Vector2f = Vector <float, 2>;
using Vector3f = Vector <float, 3>;
using Vector4f = Vector <float, 4>;

#endif  // RAYTRACER_VECTOR_H_
