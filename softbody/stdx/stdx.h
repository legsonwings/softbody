#pragma once

#ifdef _WIN32
#include "assert.h"
#endif 

#include <cmath>
#include <array>
#include <vector>
#include <limits>
#include <ranges>
#include <concepts>
#include <algorithm>

#include "stdxcore.h"

namespace stdx
{
template <typename t = float>
requires arithmetic_c<t>
t constexpr tolerance = t(1e-4f);

template <typename t>
requires arithmetic_c<t>
struct invalid { constexpr operator t() const { return { std::numeric_limits<t>::max() }; } };

struct uminus { constexpr auto operator() (arithmeticpure_c auto v) const { return -v; }; };

bool constexpr nearlyequal(arithmeticpure_c auto const& l, arithmeticpure_c auto const& r) { return l == r; }

template <typename t>
requires arithmetic_c<t>
bool constexpr isvalid(t const& val) { return !nearlyequal(std::numeric_limits<t>::max(), val); }

template<typename t>
using containervalue_t = std::decay_t<t>::value_type;

template <uint alignment, typename t>
constexpr bool isaligned(t value) { return ((uint)value & (alignment - 1)) == 0; }

constexpr bool ispowtwo(uint value) { return (value & (value - 1)) == 0; }

constexpr uint nextpowoftwomultiple(uint value, uint multipleof) 
{
	assert(((multipleof & (multipleof - 1)) == 0));
	auto const mask = multipleof - 1;
	return (value + mask) & ~mask;
}

constexpr int ceil(float value)
{
	int intval = static_cast<int>(value);
	if (value == static_cast<float>(intval))
		return intval;
	return intval + (value > 0.f ? 1 : 0);
}

template<typename t, uint n>
requires arithmetic_c<t>
constexpr std::array<t, n> sum(std::array<t, n> const& l, std::array<t, n> const& r)
{
	std::array<t, n> res;
	for (auto i : std::ranges::iota_view{ 0u,  n })
		res[i] = l[i] + r[i];
	return res;
}

constexpr auto pown(arithmeticpure_c auto v, uint n)
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

template<typename d_t, typename s_t, uint n>
constexpr auto castas(std::array<s_t, n> a)
{
	std::array<d_t, n> r;
	for (uint i(0); i < n; ++i) r[i] = static_cast<d_t>(a[i]);
	return r;
}

template<indexablecontainer_c l_t, indexablecontainer_c r_t>
requires stdx::arithmeticpure_c<containervalue_t<l_t>> && stdx::arithmeticpure_c<containervalue_t<r_t>>
constexpr auto dot(l_t&& a, r_t&& b)
{
	using v_t = containervalue_t<l_t>;
	v_t r = v_t(0);
	for (uint i(0); i < std::min(a.size(), b.size()); ++i) r += a[i] * b[i];
	return r;
}

template<indexablecontainer_c t>
requires stdx::arithmeticpure_c<containervalue_t<t>>
constexpr auto clamp(t&& a, containervalue_t<t> l, containervalue_t<t> h)
{
	std::decay_t<t> r;
	ensuresize(r, a.size());
	for (uint i(0); i < a.size(); ++i) r[i] = (a[i] < l ? l : (a[i] > h ? h : a[i]));
	return r;
}

template<indexablecontainer_c t>
requires stdx::arithmeticpure_c<containervalue_t<t>>
constexpr void clamp(t& a, containervalue_t<t> l, containervalue_t<t> h)
{
	for (uint i(0); i < a.size(); ++i) a[i] = (a[i] < l ? l : (a[i] > h ? h : a[i]));
}

template<indexablecontainer_c t, typename f_t>
requires std::invocable<f_t, containervalue_t<t>>
constexpr auto unaryop(t&& a, f_t f)
{
	std::decay_t<t> r;
	ensuresize(r, a.size());
	for (uint i(0); i < a.size(); ++i) r[i] = f(a[i]);
	return r;
}

template<indexablecontainer_c l_t, indexablecontainer_c r_t, typename f_t>
requires std::invocable<f_t, containervalue_t<l_t>, containervalue_t<r_t>>
constexpr auto binaryop(l_t&& a, r_t&& b, f_t f)
{
	std::decay_t<l_t> r;
	ensuresize(r, std::min(a.size(), b.size()));
	for (uint i(0); i < std::min(a.size(), b.size()); ++i) r[i] = f(a[i], b[i]);
	return r;
}

template<indexablecontainer_c t>
void ensuresize(t &c, uint size) {}

template<typename t>
void ensuresize(std::vector<t>& v, uint size) { v.resize(size); }

// type, lerp degree, current dimension being lerped
template<typename t, uint n, uint d = 0>
requires (n >= 0 && arithmetic_c<t>)
struct nlerp
{
	static constexpr t lerp(std::array<t, stdx::pown(2, n - d + 1)> const& data, std::array<float, n + 1> alpha)
	{
		std::array<t, stdx::pown(2, n - d)> r;
		for (uint i{ 0 }; i < r.size(); ++i)
			r[i] = data[i * 2] * (1.f - alpha[d]) + data[i * 2 + 1] * alpha[d];
		return nlerp<t, n, d + 1>::lerp(r, alpha);
	}
};

template<typename t, uint n, uint d>
requires (n >= 0 && d == n + 1 && arithmetic_c<t>)
struct nlerp<t, n, d>
{
	static constexpr t lerp(std::array<t, 1> const& data, std::array<float, n + 1> alpha) { return data[0]; }
};

template <arithmetic_c t, uint n, uint d = 0>
constexpr auto lerp(std::array<t, stdx::pown(2, n - d + 1)> const& data, std::array<float, n + 1> alpha) { return nlerp<t, n, d>::lerp(data, alpha); }

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
		// this was derived empirically
		float const temp = (std::sqrt((static_cast<float>(idx1d) + 1.f) * 8.f + 1.f) - 1.f) / 2.0f - tolerance<>;

		// convert to 0 based index
		uint const row = static_cast<uint>(ceil(temp)) - 1u;

		// this will give the index of starting vertex of the row
		// number of vertices till row n(0-based) is n(n+1)/2
		uint const rowstartvertex = (row) * (row + 1) / 2;

		uint const j = n - row;
		uint const k = idx1d - rowstartvertex;
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
		static constexpr auto invalidcont = std::make_pair(invalid<uint>(), invalid<uint>());
		uint sum = 0;
		std::pair<uint, uint> ret = invalidcont;
		for (uint i = 0; i < sizes.size(); ++i)
		{
			sum += sizes[i];
			if (idx < sum) return { idx - sum + sizes[i], i };
		}
		return ret;
	}

	t* get(uint idx)
	{
#ifdef _WIN32
		assert(idx < calcsizes());
#endif
		auto const& [idxrel, idxc] = container(idx);
		return getimpl(idxrel, idxc);
	}

	template<uint n = 0>
	t* getimpl(uint idx, uint idxc)
	{
#ifdef _WIN32
		assert(idxc < mysize);
#endif
		if (n == idxc)
			return static_cast<t*>(&(std::get<n>(data)[idx]));
		if constexpr (n + 1 < mysize) return getimpl<n + 1>(idx, idxc);
		return nullptr;
	}

	template<uint n = 0>
	uint getsize(uint idxc) const
	{
#ifdef _WIN32
		assert(idxc < mysize);
#endif
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