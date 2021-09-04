#pragma once

#include <array>
#include <vector>
#include <limits>
#include <ranges>
#include <concepts>

using uint = std::size_t;

namespace stdx
{
	template <typename t>
	concept arithmetic_c = std::is_arithmetic_v<t>;

	template <typename t>
	concept arithmeticvector_c = requires(t v)
	{
		v.x; v.y;
		requires arithmetic_c<decltype(v.x)>&& arithmetic_c<decltype(v.y)>;
	};

	template <typename t = float>
	requires arithmetic_c<t> || arithmeticvector_c<t>
	t constexpr tolerance = t(1e-4f);

	template <typename t>
	requires arithmetic_c<t> || arithmeticvector_c<t>
		struct invalid
	{
		constexpr operator t() const { return { std::numeric_limits<t>::max() }; }
	};

	template <arithmeticvector_c t>
	struct arithmeticvector {};

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

	template<typename t> 
	requires arithmetic_c<t>
	constexpr auto pown(t v, uint n)
	{
		t res = 1;
		for (auto i : std::ranges::iota_view{ 0u,  n })
			res *= v;
		return res;
	}

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