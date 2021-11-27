#pragma once

#include "stdx/stdx.h"
#include "stdx/vec.h"

#include <array>
#include <vector>
#include <functional>

// todo : maybe fix this 0 based degree/dimension madness

namespace fluid
{
// d - dimension of field, vd - dimension of vector, l length of field(assuming hypercubic)
//template <uint d, uint vd, uint l>
//struct vecfield : public std::array<stdx::vec<vd>, stdx::pown(l, d + 1)> {};

// todo : this should inherit from a container of fixed size that allocates on heap
template <uint d, uint vd, uint l>
struct vecfield : public std::vector<stdx::vec<vd>>
{
	static constexpr uint _vd = vd;
	vecfield() { this->resize(stdx::pown(l, d + 1)); }
	vecfield(std::vector<stdx::vec<vd>> other) : std::vector<stdx::vec<vd>>(other){}
};

template<uint vd, int l>
using vecfield2 = vecfield<1, vd, l>;

template<int l>
using vecfield21 = vecfield<1, 0, l>;

template<int l>
using vecfield22 = vecfield<1, 1, l>;

template<int l>
using vecfield23 = vecfield<1, 2, l>;

template <uint d, uint vd, uint l>
vecfield<d, vd, l> operator-(vecfield<d, vd, l> const& a) { return { stdx::binaryop(a, stdx::uminus()) }; }

template <uint d, uint vd, uint l>
vecfield<d, vd, l> operator+(vecfield<d, vd, l> const& a, vecfield<d, vd, l> const& b) { return { stdx::binaryop(a, b, std::plus<>()) }; }

template <uint d, uint vd, uint l>
vecfield<d, vd, l> operator-(vecfield<d, vd, l> const& a, vecfield<d, vd, l> const& b) { return { stdx::binaryop(a, b, std::minus<>()) }; }

// n is dimension of simulation(0-based), l is length of box/cube(number of cells)
template<uint n, uint l>
requires (n >= 0)
struct fluidbox
{
	static constexpr uint numcells = stdx::pown(l, n);
	using grididx = stdx::grididx<n>;

	vecfield23<l> d;
	vecfield22<l> v;
	vecfield22<l> v0;
	// diffusion coeffecient, density, velocity, oldvelocity

	void addvelocity(grididx const& cellidx, stdx::vec<n> vel)
	{
		// todo : provide shorthand assignment operators
		v[grididx::to1d<l - 1>(cellidx)] = { v[grididx::to1d<l - 1>(cellidx)] + vel };
	}

	void adddensity(grididx const& cellidx, uint didx, float den)
	{
		// todo : provide shorthand assignment operators
		d[grididx::to1d<l - 1>(cellidx)][didx] = { d[grididx::to1d<l - 1>(cellidx)][didx] + den };
	}
};

// todo : figure out a way to generically handle arbitrary dimensions
// the problem right now is that we have to no way of generating nested loops based on template paramter
// a potential solution is to wirte loop templates that use recursion
// alternatvely a more desirable solution could be to iterate the 1d representation(since data in any dimension is just a 1D array) in single loop
// This might mean padding the vector field with additional cells outside boundary since we require neighbouring cells for solving poisson equations

// todo : provide a generic nd lerp
template<uint vd>
stdx::vec<vd> bilerp(stdx::vec<vd> lt, stdx::vec<vd> rt, stdx::vec<vd> lb, stdx::vec<vd> rb, stdx::vec2 alpha)
{
	return (lt * (1.f - alpha[0]) + rt * alpha[0]) * (1.f - alpha[1]) + (lb * (1.f - alpha[0]) + rb * alpha[0]) * alpha[1];
}

// todo : use callable concept
template<uint vd, uint l>
vecfield2<vd, l> diffuse(vecfield2<vd, l> const& x, vecfield2<vd, l> const& b, float dt, float diff)
{
	using idx = stdx::grididx<1>;
	float const a = diff * dt;
	return jacobi2d(x, b, 4, a, 1 + 4 * a);
}

template<uint vd, uint l>
vecfield2<vd, l> diffusecolor(vecfield2<vd, l> const& x, vecfield2<vd, l> const& b, float dt, float diff, std::vector<stdx::vec3> &col)
{
	using idx = stdx::grididx<1>;
	float const a = diff * dt;
	return jacobi2dcolor(x, b, 4, a, 1 + 4 * a, col);
}

template<uint l>
vecfield21<l> divergence(vecfield22<l> const& f)
{
	using idx = stdx::grididx<1>;
	vecfield21<l> r;
	for (uint j(1); j < l - 1; ++j)
		for (uint i(1); i < l - 1; ++i)
			r[idx::to1d<l - 1>({ i, j })] = -0.5f * stdx::vec1{ f[idx::to1d<l - 1>({ i + 1, j })][0] - f[idx::to1d<l - 1>({ i - 1, j })][0] + f[idx::to1d<l - 1>({ i, j + 1 })][1] - f[idx::to1d<l - 1>({ i, j - 1 })][1] };

	return r;
}

template<uint l>
vecfield22<l> gradient(vecfield21<l> const& f)
{
	using idx = stdx::grididx<1>;
	vecfield22<l> r{};
	for (uint j(1); j < l - 1; ++j)
		for (uint i(1); i < l - 1; ++i)
			r[idx::to1d<l - 1>({ i, j })] = 0.5f * stdx::vec2{ f[idx::to1d<l - 1>({ i + 1, j })] - f[idx::to1d<l - 1>({ i - 1, j })], f[idx::to1d<l - 1>({ i, j + 1 })] - f[idx::to1d<l - 1>({ i, j - 1 })] };
	
	return r;
}

// jacobi iteration to solve poisson equations
template<uint vd, uint l>
vecfield2<vd, l> jacobi2d(vecfield2<vd, l> const& x, vecfield2<vd, l> const& b, uint niters, float alpha, float beta)
{
	using idx = stdx::grididx<1>;
	vecfield2<vd, l> r = x;
	auto const rcbeta = 1.f / beta;
	for (uint k(0); k < niters; ++k)
	{
		for (uint j(1); j < l - 1; ++j)
			for (uint i(1); i < l - 1; ++i)
				r[idx::to1d<l - 1>({ i, j })] = ((r[idx::to1d<l - 1>({ i - 1, j })] + r[idx::to1d<l - 1>({ i + 1, j })] + r[idx::to1d<l - 1>({ i, j - 1 })] +
					r[idx::to1d<l - 1>({ i, j + 1 })]) * alpha + b[idx::to1d<l - 1>({ i, j })]) * rcbeta;
	}
	return r;
}

// jacobi iteration to solve poisson equations
template<uint vd, uint l>
vecfield2<vd, l> jacobi2dcolor(vecfield2<vd, l> const& x, vecfield2<vd, l> const& b, uint niters, float alpha, float beta, std::vector<stdx::vec3> &col)
{
	static constexpr stdx::vec3 const color{ 1.f, 0.f, 0.f };
	using idx = stdx::grididx<1>;
	vecfield2<vd, l> r = x;
	auto const rcbeta = 1.f / beta;
	for (uint k(0); k < niters; ++k)
	{
		for (uint j(1); j < l - 1; ++j)
			for (uint i(1); i < l - 1; ++i)
			{
				auto const idx1d = idx::to1d<l - 1>({ i, j });
				r[idx1d] = ((r[idx::to1d<l - 1>({ i - 1, j })] + r[idx::to1d<l - 1>({ i + 1, j })] + r[idx::to1d<l - 1>({ i, j - 1 })] +
					r[idx::to1d<l - 1>({ i, j + 1 })]) * alpha + b[idx::to1d<l - 1>({ i, j })]) * rcbeta;

				col[idx1d] = color * r[idx1d][0];
			}
	}
	return r;
}

template<uint vd, uint l>
vecfield2<vd, l> advect2d(vecfield2<vd, l> const& a, vecfield22<l> const& v, float dt)
{
	using idx = stdx::grididx<1>;
	vecfield2<vd, l> r{};
	for (uint j(1); j < l - 1; ++j)
		for (uint i(1); i < l - 1; ++i)
		{
			auto const cell = idx::to1d<l - 1>({ i, j });

			// find position at -dt
			stdx::vec2 const xy = (stdx::vec2{ static_cast<float>(i), static_cast<float>(j) } - v[cell] * dt).clamp(0.5f, l - 1.5f);

			// bilinearly interpolate the property across 4 neighbouring cells
			idx const lt2d = { static_cast<int>(xy[0]), static_cast<int>(xy[1]) };
			auto const lt = idx::to1d<l - 1>(lt2d);
			auto const rt = idx::to1d<l - 1>({ lt2d + idx{1, 0} });
			auto const lb = idx::to1d<l - 1>({ lt2d + idx{0, 1} });
			auto const rb = idx::to1d<l - 1>({ lt2d + idx{1, 1} });

			r[cell] = bilerp(a[lt], a[rt], a[lb], a[rb], {xy[0] - lt2d.coords[0], xy[1] - lt2d.coords[1] });
		}

	return r;
}

// todo : this can be done better by using vectors to set the boundary velocities/densities
// todo : this can be done even better by making this work for n-dimensional fluids, though it might be really difficult to do so

// todo : this requries 2 dimension vectors
template<uint vd, uint l>
void sbounds(vecfield2<vd, l>& v, float scale)
{
	using idx = stdx::grididx<1>;
	std::vector<idx> boundaries;
	for (uint i(1); i < l - 1; ++i)
	{
		boundaries.push_back({ 0, i });
		boundaries.push_back({ l - 1, i });
		boundaries.push_back({ i, 0 });
		boundaries.push_back({ i, l - 1 });
	}

	for (auto bcell : boundaries)
	{
		for (uint i(0); i <= vd; ++i)
		{
			if (bcell[0] == 0)
				v[idx::to1d<l - 1>(bcell)][i] = scale * v[idx::to1d<l - 1>(bcell + idx{1, 0})][i];

			if (bcell[0] == l - 1) 
				v[idx::to1d<l - 1>(bcell)][i] = scale * v[idx::to1d<l - 1>(bcell + idx{-1, 0})][i];

			if (bcell[1] == 0)
				v[idx::to1d<l - 1>(bcell)][i] = scale * v[idx::to1d<l - 1>(bcell + idx{0, 1})][i];
			
			if (bcell[1] == l - 1)
				v[idx::to1d<l - 1>(bcell)][i] = scale * v[idx::to1d<l - 1>(bcell + idx{0, -1})][i];
		}
	}

	// assign corner velocities as average of two neighbors
	v[idx::to1d<l - 1>({ 0, 0 })] = (v[idx::to1d<l - 1>({ 1, 0 })] + v[idx::to1d<l - 1>({ 0, 1 })]) / 2.f;
	v[idx::to1d<l - 1>({ 0, l - 1 })] = (v[idx::to1d<l - 1>({ 1, l - 1 })] + v[idx::to1d<l - 1>({ 0, l - 2 })]) / 2.f;
	v[idx::to1d<l - 1>({ l - 1, 0 })] = (v[idx::to1d<l - 1>({ l - 2, 0 })] + v[idx::to1d<l - 1>({ l - 1, 1 })]) / 2.f;
	v[idx::to1d<l - 1>({ l - 1, l - 1 })] = (v[idx::to1d<l - 1>({ l - 2, l - 1 })] + v[idx::to1d<l - 1>({ l - 1, l - 2 })]) / 2.f;
}
}