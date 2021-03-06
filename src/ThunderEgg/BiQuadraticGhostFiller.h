/***************************************************************************
 *  ThunderEgg, a library for solving Poisson's equation on adaptively
 *  refined block-structured Cartesian grids
 *
 *  Copyright (C) 2019  ThunderEgg Developers. See AUTHORS.md file at the
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

#ifndef THUNDEREGG_BIQUADRATICGHOSTFILLER_H
#define THUNDEREGG_BIQUADRATICGHOSTFILLER_H
#include <ThunderEgg/GMG/Level.h>
#include <ThunderEgg/MPIGhostFiller.h>
namespace ThunderEgg
{
/**
 * @brief Exchanges ghost cells on patches, handles refinement boundaries with a biquadratic
 * interpolation scheme
 */
class BiQuadraticGhostFiller : public MPIGhostFiller<2>
{
	public:
	/**
	 * @brief Construct a new BiQuadraticGhostFiller object
	 *
	 * @param domain_in the domain that is being fill for
	 */
	BiQuadraticGhostFiller(std::shared_ptr<const Domain<2>> domain_in)
	: MPIGhostFiller<2>(domain_in, 1)
	{
	}

	void fillGhostCellsForNbrPatch(std::shared_ptr<const PatchInfo<2>> pinfo,
	                               const std::vector<LocalData<2>> &   local_datas,
	                               const std::vector<LocalData<2>> &nbr_datas, const Side<2> side,
	                               const NbrType nbr_type, const Orthant<2> orthant) const override;

	void fillGhostCellsForLocalPatch(std::shared_ptr<const PatchInfo<2>> pinfo,
	                                 const std::vector<LocalData<2>> &local_datas) const override;
};
} // namespace ThunderEgg
#endif
