/***************************************************************************
 *  Thunderegg, a library for solving Poisson's equation on adaptively
 *  refined block-structured Cartesian grids
 *
 *  Copyright (C) 2019  Thunderegg Developers. See AUTHORS.md file at the
 *  top-level directory.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 ***************************************************************************/

#ifndef DOMAINWRAPOP_H
#define DOMAINWRAPOP_H
#include <Operators/Operator.h>
#include <SchurHelper.h>
/**
 * @brief Base class for operators
 */
template <size_t D> class DomainWrapOp : public Operator<D>
{
	private:
	std::shared_ptr<SchurHelper<D>> sh;

	public:
    DomainWrapOp(std::shared_ptr<SchurHelper<D>> sh)
	{
		this->sh = sh;
	}
	/**
	 * @brief Apply Schur matrix
	 *
	 * @param x the input vector.
	 * @param b the output vector.
	 */
	void apply(std::shared_ptr<const Vector<D>> x, std::shared_ptr<Vector<D>> b) const
	{
		sh->apply(x, b);
	}
};
#endif
