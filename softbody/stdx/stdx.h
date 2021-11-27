#pragma once

#include "assert.h"

#include <array>
#include <vector>
#include <limits>
#include <ranges>
#include <concepts>

#include "stdxcore.h"

namespace stdx
{
template <typename t = float>
requires arithmetic_c<t> || arithmeticvector_c<t>
t constexpr tolerance = t(1e-4f);

template <typename t>
requires arithmetic_c<t> || arithmeticvector_c<t>
struct invalid { constexpr operator t() const { return { std::numeric_limits<t>::max() }; } };

template <arithmeticvector_c t>
struct arithmeticvector {};

struct uminus { constexpr auto operator() (arithmetic_c auto v) const { return -v; }; };

bool constexpr nearlyequal(arithmetic_c auto const& l, arithmetic_c auto const& r) { return l == r; }

template <typename t>
requires arithmetic_c<t> || arithmeticvector_c<t>
bool constexpr isvalid(t const& val) { return !nearlyequal(std::numeric_limits<t>::max(), val); }

constexpr int ceil(float value)
{
	int intval = static_cast<int>(value);
	if (value == static_cast<float>(intval))
		return intval;
	return intval + (value > 0.f ? 1 : 0);
}

template<typename t>
requires arithmetic_c<t> || arithmeticvector_c<t>
constexpr t lerp(t a, t b, float alpha) { return a * (1 - alpha) + b * alpha; }

template<typename t, uint n>
requires arithmetic_c<t> || arithmeticvector_c<t>
constexpr std::array<t, n> sum(std::array<t, n> const& l, std::array<t, n> const& r)
{
	std::array<t, n> res;
	for (auto i : std::ranges::iota_view{ 0u,  n })
		res[i] = l[i] + r[i];
	return res;
}

constexpr auto pown(arithmetic_c auto v, uint n)
{
	decltype(v) res = 1;
	for (auto i : std::ranges::iota_view{ 0u,  n })
		res *= v;
	return res;
}

template<uint n>
requires (n > 0)
constexpr std::array<uint, n> getdigits(uint d)
{
	std::array<uint, n> ret{};

	uint i = d;
	uint j = n - 1;
	for (; i > 9; i /= 10, --j)
		ret[j] = i % 10;
	ret[j] = i;
	return ret;
}

template<typename t>
void append(std::vector<t>&& source, std::vector<t>& dest)
{
	dest.reserve(dest.size() + source.size());
	std::move(source.begin(), source.end(), std::back_inserter(dest));
}

template<typename t>
void append(std::vector<t> const& source, std::vector<t>& dest)
{
	dest.reserve(dest.size() + source.size());
	for (auto const& e : source) { dest.emplace_back(e); }
}

template<arithmetic_c t, uint n>
constexpr t dot(std::array<t, n> a, std::array<t, n> b)
{
	t r = 0;
	for (uint i(0); i < n; ++i) r += a[i] * b[i];
	return r;
}

// todo : can we make these algorithms return the correct type
template<arithmetic_c t, uint n>
constexpr std::array<t, n> clamp(std::array<t, n> a, t l, t h)
{
	std::array<t, n> r;
	for (uint i(0); i < n; ++i) r[i] = (a[i] < l ? l : (a[i] > h ? h : a[i]));
	return r;
}

// todo : create a concept, std::regular_invocable doesn't work
template<typename t, uint n, typename f_t>
constexpr std::array<t, n> unaryop(std::array<t, n> a, f_t f)
{
	std::array<t, n> r;
	for (uint i(0); i < n; ++i) r[i] = f(a[i]);
	return r;
}

template<typename d_t, typename s_t, uint n>
constexpr auto casted(std::array<s_t, n> a)
{
	std::array<d_t, n> r;
	for (uint i(0); i < n; ++i) r[i] = static_cast<d_t>(a[i]);
	return r;
}

template<typename t, uint n, typename f_t>
constexpr std::array<t, n> binaryop(std::array<t, n> a, std::array<t, n> b, f_t f)
{
	std::array<t, n> r;
	for (uint i(0); i < n; ++i) r[i] = f(a[i], b[i]);
	return r;
}

template<typename t, typename f_t>
constexpr std::vector<t> unaryop(std::vector<t> a, f_t f)
{
	std::vector<t> r;
	r.resize(a.size());
	for (auto e : a) r.emplace_back(f(e));
	return r;
}

template<typename t, typename f_t>
constexpr std::vector<t> binaryop(std::vector<t> a, std::vector<t> b, f_t f)
{
	std::vector<t> r;
	r.resize(a.size());
	for (uint i(0); i < a.size(); ++i) r[i] = f(a[i], b[i]);
	return r;
}

// triangular index
template<uint n>
struct triindex
{
	constexpr triindex(uint _i, uint _j, uint _k) : i(_i), j(_j), k(_k) {}
	constexpr uint to1d() const { return to1d(this->j, this->k); }
	static constexpr uint to1d(uint j, uint k) { return (n - j) * (n - j + 1) / 2 + k; }         // n - j gives us row index

	static triindex to3d(uint idx1d)
	{
		// determine which row the control point belongs to
		// how was this derived?
		float const temp = (sqrt((static_cast<float>(idx1d) + 1.f) * 8.f + 1.f) - 1.f) / 2.0f - tolerance<>;

		// convert to 0 based index
		uint const row = static_cast<uint>(ceil(temp)) - 1u;

		// this will give the index of starting vertex of the row
		// number of vertices till row n(0-based) is n(n+1)/2
		uint const rowStartVertex = (row) * (row + 1) / 2;

		uint const j = n - row;
		uint const k = idx1d - rowStartVertex;
		uint const i = n - j - k;

		return triindex(i, j, k);
	}

	uint i, j, k;
};

// n is dimension of the grid(0 based)
template<uint n>
requires (n >= 0)
struct grididx
{
	constexpr grididx() = default;
	constexpr grididx(std::array<uint, n + 1> const& _coords) : coords(_coords) {}
	explicit constexpr grididx(uint idx) : grididx{ getdigits<n + 1>(idx) } {}
	template<uint_c ... args>
	requires (sizeof...(args) == (n + 1))
	constexpr grididx(args... _coords) : coords{ static_cast<uint>(_coords)... } {}
	constexpr uint& operator[](uint idxidx) { return coords[idxidx]; }
	constexpr uint operator[](uint idxidx) const { return coords[idxidx]; }
	constexpr grididx operator+(grididx const& rhs) const { return { sum(coords, rhs.coords) }; }

	// d is degree(0 based)
	template<uint d>
	static constexpr uint to1d(grididx const& idx)
	{
		uint res = 0;
		// idx = x0 + x1d + x2dd + x3ddd + ....
		for (uint i(n); i >= 1; --i)
			res = (idx.coords[i] + res) * (d + 1);

		res += idx.coords[0];
		return res;
	}

	static constexpr grididx from1d(uint d, uint idx)
	{
		grididx res;
		for (auto i : std::ranges::iota_view{ 0u,  n + 1 })
			res.coords[i] = (idx / pown((d + 1), i)) % (d + 1);
		return res;
	}

	std::array<uint, n + 1> coords = {};
};

template<typename b, typename e>
struct ext
{
	b base;
	e ext;

	constexpr ext(b const& _b, e const& _e) : base(_b), ext(_e) {}

	constexpr operator b& () { return base; }
	constexpr operator b const& () const { return base; }
	constexpr e& ex() { return ext; }
	constexpr e const& ex() const { return ext; }
	constexpr b& get() { return base; }
	constexpr b const& get() const { return base; }
	constexpr b& operator*() { return base; }
	constexpr b const& operator*() const { return base; }
	constexpr b* operator->() { return &base; }
	constexpr b const* operator->() const { return &base; }
};

// these allow iteration over multiple heterogenous containers
template<typename t, indexablecontainer_c... args_t>
struct join;

template<typename t, indexablecontainer_c... args_t>
struct joiniter
{
	using idx_t = uint;
	using join_t = join<t, args_t...>;

	joiniter(join_t* _join, uint _idx) : join(_join), idx(_idx) {}
	t* operator*() { return join->get(idx); }
	bool operator!=(joiniter const& rhs) const { return idx != rhs.idx || join != rhs.join; }
	joiniter& operator++() { idx++; return *this; }

	idx_t idx;
	join_t* join;
};

template<typename t, indexablecontainer_c... args_t>
struct join
{
private:
	using iter_t = joiniter<t, args_t...>;
	friend struct iter_t;
	static constexpr uint mysize = sizeof...(args_t);

public:
	join(args_t&... _args) : data(_args...) {}
	iter_t begin()
	{
		if (calcsizes() == 0) return end();
		return iter_t(this, 0);
	}
	iter_t end() { return iter_t(this, calcsizes()); }

private:
	uint calcsizes()
	{
		uint totalsize = 0;
		for (uint i = 0; i < mysize; ++i) { sizes[i] = getsize(i); totalsize += sizes[i]; }
		return totalsize;
	}

	std::pair<uint, uint> container(uint idx) const
	{
		static constexpr auto invalid = std::make_pair(std::numeric_limits<uint>::max(), std::numeric_limits<uint>::max());
		uint sum = 0;
		std::pair<uint, uint> ret = invalid;
		for (uint i = 0; i < sizes.size(); ++i)
		{
			sum += sizes[i];
			if (idx < sum) return { idx - sum + sizes[i], i };
		}
		return ret;
	}

	t* get(uint idx)
	{
		assert(idx < calcsizes());
		auto const& [idxrel, idxc] = container(idx);
		return getimpl(idxrel, idxc);
	}

	template<uint n = 0>
	t* getimpl(uint idx, uint idxc)
	{
		assert(idxc < mysize);
		if (n == idxc)
			return static_cast<t*>(&(std::get<n>(data)[idx]));
		if constexpr (n + 1 < mysize) return getimpl<n + 1>(idx, idxc);
		return nullptr;
	}

	template<uint n = 0>
	uint getsize(uint idxc) const
	{
		assert(idxc < mysize);
		if (n == idxc) return std::get<n>(data).size();
		if constexpr (n + 1 < mysize) return getsize<n + 1>(idxc);
		return 0;
	}

	std::array<uint, mysize> sizes;
	std::tuple<args_t&...> data;
};

template<typename t, typename... args_t>
auto makejoin(args_t&... _args) { return join<t, args_t...>(_args...); }

}

namespace std
{
	template <typename t>
	class numeric_limits<stdx::arithmeticvector<t>>
	{
		static constexpr t min() noexcept { return t(std::numeric_limits<decltype(t().x)>::min()); }
		static constexpr t max() noexcept { return t(std::numeric_limits<decltype(t().x)>::max()); }
		static constexpr t lowest() noexcept { return t(std::numeric_limits<decltype(t().x)>::lowest()); }
	};
}