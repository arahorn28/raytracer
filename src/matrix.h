#ifndef RAYTRACER_MATRIX_H_
#define RAYTRACER_MATRIX_H_

#include "vector.h"

#include <iostream>
#include <type_traits>
// Undef some weird defines in gcc
#ifdef minor
#undef minor
#endif

template <typename T, size_t N>
struct Matrix
{
	static_assert(N > 1, "Minimal matrix size is 2x2");

	T data[N][N];

	Matrix() : data()
	{
		for (size_t i = 0; i < N; ++i)
			data[i][i] = one <T>;
	}

	template <typename ...Args>
	Matrix(Args ... args) : data{ args... }
	{
		static_assert(sizeof...(Args) == (N), "Invalid number of elements passed to constructor\n");
	}

	Matrix <T, N> operator *(const Matrix <T, N> &rhs) const
	{
		Matrix <T, N> res;
		for (size_t x = 0; x < N; ++x)
		{
			for (size_t y = 0; y < N; ++y)
			{
				T sum{};
				for (size_t i = 0; i < N; ++i)
				{
					sum += data[i][y] * rhs.data[x][i];
				}
				res.data[x][y] = sum;
			}
		}
		return res;
	}

	void operator *=(const Matrix <T, N> &rhs)
	{
		T temp[N];
		for (size_t y = 0; y < N; ++y)
		{
			for (size_t x = 0; x < N; ++x)
			{
				temp[x] = data[x][y];
			}
			for (size_t x = 0; x < N; ++x)
			{
				T sum{};
				for (size_t i = 0; i < N; ++i)
				{
					sum += temp[i] * rhs.data[x][i];
				}
				data[x][y] = sum;
			}
		}
	}

	Matrix <T, N> operator +(const Matrix <T, N> &rhs) const
	{
		Matrix <T, N> res;
		for (size_t x = 0; x < N; ++x)
		{
			for (size_t y = 0; y < N; ++y)
			{
				res.data[x][y] = data[x][y] + rhs.data[x][y];
			}
		}
		return res;
	}

	void operator +=(const Matrix <T, N> &rhs)
	{
		for (size_t x = 0; x < N; ++x)
		{
			for (size_t y = 0; y < N; ++y)
			{
				data[x][y] = data[x][y] + rhs.data[x][y];
			}
		}
	}

	Vector <T, N> operator *(const Vector <T, N> &rhs) const
	{
		Vector <T, N> res;
		for (size_t x = 0; x < N; ++x)
		{
			for (size_t y = 0; y < N; ++y)
			{
				res.data[y] += data[x][y] * rhs.data[x];
			}
		}
		return res;
	}

	Vector <T, N - 1> operator *(const Vector <T, N - 1> &rhs) const
	{
		Vector <T, N - 1> res;
		for (size_t y = 0; y < (N - 1); ++y)
		{
			for (size_t x = 0; x < (N - 1); ++x)
			{
				res.data[y] += data[x][y] * rhs.data[x];
			}
			res.data[y] += data[N - 1][y];
		}
		return res;
	}

	void translate(const Vector <T, N - 1> &vec)
	{
		for (size_t y = 0; y < (N - 1); ++y)
		{
			data[N - 1][y] = T{};
			for (size_t x = 0; x < (N - 1); ++x)
			{
				data[N - 1][y] += data[x][y] * vec[x];
			}
			data[N - 1][y] += data[N - 1][y];
		}
	}

	Matrix <T, N> transpose() const
	{
		Matrix <T, N> res;

		for (size_t x = 0; x < N; ++x)
		{
			for (size_t y = 0; y < N; ++y)
			{
				res.data[y][x] = data[x][y];
			}
		}

		return res;
	}

	T determinant() const
	{
		return _determinant(*this);
	}

	Matrix <T, N - 1> minor(size_t i) const
	{
		Matrix <T, N - 1> res;
		for (size_t x = 0, x_dest = 0; x < N; ++x)
		{
			if (x != i)
			{
				std::copy(&data[x][1], &data[x][N], &res.data[x_dest][0]);
				++x_dest;
			}
		}
		return res;
	}

	static Matrix <T, N> fromTranslation(const Vector <T, N - 1> &vec)
	{
		Matrix <T, N> res;

		for (size_t i = 0; i < (N - 1); ++i)
		{
			res.data[N - 1][i] = vec[i];
		}

		return res;
	}

	static Matrix <T, N> fromScaling(const Vector <T, N - 1> &vec)
	{
		Matrix <T, N> res;

		for (size_t i = 0; i < (N - 1); ++i)
		{
			res.data[i][i] = vec[i];
		}

		return res;
	}

	// Enabled only when N > 2
	template <typename ReturnType = Matrix <T, N>>
	static std::enable_if_t <(N > 2), ReturnType>
	fromRotationX(const T &alpha)
	{
		Matrix <T, N> res;

		res.data[1][1] = std::cos(alpha);
		res.data[1][2] = std::sin(alpha);
		res.data[2][1] = -res.data[1][2];
		res.data[2][2] = res.data[1][1];

		return res;
	}

	// Enabled only when N > 2
	template <typename ReturnType = Matrix <T, N>>
	static std::enable_if_t <(N > 2), ReturnType>
	fromRotationY(const T &alpha)
	{
		Matrix <T, N> res;

		res.data[0][0] = std::cos(alpha);
		res.data[0][2] = -std::sin(alpha);
		res.data[2][0] = -res.data[0][2];
		res.data[2][2] = res.data[0][0];

		return res;
	}

	template <typename ReturnType = Matrix <T, N>>
	static std::enable_if_t <(N > 1), ReturnType>
	fromRotationZ(const T &alpha)
	{
		Matrix <T, N> res;

		res.data[0][0] = std::cos(alpha);
		res.data[0][1] = std::sin(alpha);
		res.data[1][0] = -res.data[0][1];
		res.data[1][1] = res.data[0][0];

		return res;
	}

	// Enabled only when N == 4
	template <typename ReturnType = Matrix <T, N>>
	std::enable_if_t <(N == 4), ReturnType>
	invert() const
	{
		Matrix <T, N> res;
		// Copied from some answer on stackoverflow
#define x(i) res.data[i / 4][i % 4]
#define m(i) data[i / 4][i % 4]

		x(0) = m(5)  * m(10) * m(15) -
			m(5)  * m(11) * m(14) -
			m(9)  * m(6)  * m(15) +
			m(9)  * m(7)  * m(14) +
			m(13) * m(6)  * m(11) -
			m(13) * m(7)  * m(10);

		x(4) = -m(4)  * m(10) * m(15) +
			m(4)  * m(11) * m(14) +
			m(8)  * m(6)  * m(15) -
			m(8)  * m(7)  * m(14) -
			m(12) * m(6)  * m(11) +
			m(12) * m(7)  * m(10);

		x(8) = m(4)  * m(9) * m(15) -
			m(4)  * m(11) * m(13) -
			m(8)  * m(5) * m(15) +
			m(8)  * m(7) * m(13) +
			m(12) * m(5) * m(11) -
			m(12) * m(7) * m(9);

		x(12) = -m(4)  * m(9) * m(14) +
			m(4)  * m(10) * m(13) +
			m(8)  * m(5) * m(14) -
			m(8)  * m(6) * m(13) -
			m(12) * m(5) * m(10) +
			m(12) * m(6) * m(9);

		x(1) = -m(1)  * m(10) * m(15) +
			m(1)  * m(11) * m(14) +
			m(9)  * m(2) * m(15) -
			m(9)  * m(3) * m(14) -
			m(13) * m(2) * m(11) +
			m(13) * m(3) * m(10);

		x(5) = m(0)  * m(10) * m(15) -
			m(0)  * m(11) * m(14) -
			m(8)  * m(2) * m(15) +
			m(8)  * m(3) * m(14) +
			m(12) * m(2) * m(11) -
			m(12) * m(3) * m(10);

		x(9) = -m(0)  * m(9) * m(15) +
			m(0)  * m(11) * m(13) +
			m(8)  * m(1) * m(15) -
			m(8)  * m(3) * m(13) -
			m(12) * m(1) * m(11) +
			m(12) * m(3) * m(9);

		x(13) = m(0)  * m(9) * m(14) -
			m(0)  * m(10) * m(13) -
			m(8)  * m(1) * m(14) +
			m(8)  * m(2) * m(13) +
			m(12) * m(1) * m(10) -
			m(12) * m(2) * m(9);

		x(2) = m(1)  * m(6) * m(15) -
			m(1)  * m(7) * m(14) -
			m(5)  * m(2) * m(15) +
			m(5)  * m(3) * m(14) +
			m(13) * m(2) * m(7) -
			m(13) * m(3) * m(6);

		x(6) = -m(0)  * m(6) * m(15) +
			m(0)  * m(7) * m(14) +
			m(4)  * m(2) * m(15) -
			m(4)  * m(3) * m(14) -
			m(12) * m(2) * m(7) +
			m(12) * m(3) * m(6);

		x(10) = m(0)  * m(5) * m(15) -
			m(0)  * m(7) * m(13) -
			m(4)  * m(1) * m(15) +
			m(4)  * m(3) * m(13) +
			m(12) * m(1) * m(7) -
			m(12) * m(3) * m(5);

		x(14) = -m(0)  * m(5) * m(14) +
			m(0)  * m(6) * m(13) +
			m(4)  * m(1) * m(14) -
			m(4)  * m(2) * m(13) -
			m(12) * m(1) * m(6) +
			m(12) * m(2) * m(5);

		x(3) = -m(1) * m(6) * m(11) +
			m(1) * m(7) * m(10) +
			m(5) * m(2) * m(11) -
			m(5) * m(3) * m(10) -
			m(9) * m(2) * m(7) +
			m(9) * m(3) * m(6);

		x(7) = m(0) * m(6) * m(11) -
			m(0) * m(7) * m(10) -
			m(4) * m(2) * m(11) +
			m(4) * m(3) * m(10) +
			m(8) * m(2) * m(7) -
			m(8) * m(3) * m(6);

		x(11) = -m(0) * m(5) * m(11) +
			m(0) * m(7) * m(9) +
			m(4) * m(1) * m(11) -
			m(4) * m(3) * m(9) -
			m(8) * m(1) * m(7) +
			m(8) * m(3) * m(5);

		x(15) = m(0) * m(5) * m(10) -
			m(0) * m(6) * m(9) -
			m(4) * m(1) * m(10) +
			m(4) * m(2) * m(9) +
			m(8) * m(1) * m(6) -
			m(8) * m(2) * m(5);
#undef x
#undef m

		T det = data[0][0] * res.data[0][0] + data[0][1] * res.data[1][0] 
			+ data[0][2] * res.data[2][0] + data[0][3] * res.data[3][0];

		// TODO throw error or add parameter bool* which can be set to false?
		if (det == zero<T>)
			return res;

		det = one<T> / det;

		for (size_t x = 0; x < 4; ++x)
			for (size_t y = 0; y < 4; ++y)
				res.data[x][y] = res.data[x][y] * det;

		return res;
	}
};

// It is not allowed to make partial specialization of template class method,
// so we will use template funtion that is not a class method
template <typename T, size_t N>
T _determinant(const Matrix <T, N> &matrix)
{
	T det{};
	for (size_t x = 0; x < N; ++x)
	{
		det += ((x % 2) ? -matrix.data[x][0] : matrix.data[x][0]) * matrix.minor(x).determinant();
	}
	return det;
}

template <typename T>
T _determinant(const Matrix <T, 2> &m)
{
	return m.data[0][0] * m.data[1][1] - m.data[0][1] * m.data[1][0];
}

template <typename T, size_t N>
std::ostream & operator <<(std::ostream &os, const Matrix <T, N> rhs)
{
	for (size_t y = 0; y < N; ++y)
	{
		if (y == 0)
			os << char(201);
		else if (y == (N - 1))
			os << char(200);
		else
			os << char(186);

		os << ' ';

		for (size_t x = 0; x < N; ++x)
		{
			os << rhs.data[x][y] << " ";
		}

		if (y == 0)
			os << char(187);
		else if (y == (N - 1))
			os << char(188);
		else
			os << char(186);

		os << std::endl;
	}
	return os;
}

using Matrix3f = Matrix <float, 3>;
using Matrix4f = Matrix <float, 4>;

#endif  //RAYTRACER_MATRIX_H_
