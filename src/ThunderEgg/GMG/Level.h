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

#ifndef THUNDEREGG_GMG_LEVEL_H
#define THUNDEREGG_GMG_LEVEL_H
#include <ThunderEgg/Domain.h>
#include <ThunderEgg/GMG/Interpolator.h>
#include <ThunderEgg/GMG/Restrictor.h>
#include <ThunderEgg/GMG/Smoother.h>
#include <ThunderEgg/Operator.h>
#include <ThunderEgg/VectorGenerator.h>
#include <memory>
namespace ThunderEgg
{
namespace GMG
{
/**
 * @brief Represents a level in geometric multi-grid.
 */
template <int D> class Level
{
	private:
	/**
	 * @brief The domain for this level
	 */
	std::shared_ptr<const Domain<D>> domain;
	/**
	 * @brief the VectorGenerator for this level.
	 */
	std::shared_ptr<VectorGenerator<D>> vg;
	/**
	 * @brief The operator (matrix) for this level.
	 */
	std::shared_ptr<const Operator<D>> op;
	/**
	 * @brief The restrictor from this level to the coarser level.
	 */
	std::shared_ptr<const Restrictor<D>> restrictor;
	/**
	 * @brief The interpolator from this level to the finer level.
	 */
	std::shared_ptr<const Interpolator<D>> interpolator;
	/**
	 * @brief The smoother for this level.
	 */
	std::shared_ptr<const Smoother<D>> smoother;
	/**
	 * @brief Pointer to coarser level
	 */
	std::shared_ptr<Level> coarser;
	/**
	 * @brief Pointer to finer level
	 */
	std::weak_ptr<Level> finer;

	public:
	/**
	 * @brief Create a Level object.
	 *
	 * @param dc pointer to the DomainCollection for this level
	 */
	Level(std::shared_ptr<const Domain<D>> domain, std::shared_ptr<VectorGenerator<D>> vg)
	{
		this->domain = domain;
		this->vg     = vg;
	}
	/**
	 * @brief Get the Domain object
	 */
	std::shared_ptr<const Domain<D>> getDomain() const
	{
		return domain;
	}
	/**
	 * @brief Set the restriction operator for restricting from this level to the coarser level.
	 *
	 * @param restrictor the restriction operator.
	 */
	void setRestrictor(std::shared_ptr<const Restrictor<D>> restrictor)
	{
		this->restrictor = restrictor;
	}
	/**
	 * @brief Get the restriction operator for this level.
	 *
	 * @return Pointer to the restrictor
	 */
	std::shared_ptr<const Restrictor<D>> getRestrictor() const
	{
		return restrictor;
	}
	/**
	 * @brief Set the interpolation operator for interpolating from this level to the finer level.
	 *
	 * @param interpolator the interpolation operator.
	 */
	void setInterpolator(std::shared_ptr<const Interpolator<D>> interpolator)
	{
		this->interpolator = interpolator;
	}
	/**
	 * @brief Get the interpolation operator for this level.
	 *
	 * @return Pointer to the interpolator.
	 */
	std::shared_ptr<const Interpolator<D>> getInterpolator() const
	{
		return interpolator;
	}
	/**
	 * @brief Set the operator (matrix) for this level.
	 *
	 * @param op the operator
	 */
	void setOperator(std::shared_ptr<const Operator<D>> op)
	{
		this->op = op;
	}
	/**
	 * @brief Get the operator for this level.
	 *
	 * @return Pointer to the operator.
	 */
	std::shared_ptr<const Operator<D>> getOperator() const
	{
		return op;
	}
	/**
	 * @brief Set the smoother for this level.
	 *
	 * @param smoother the smoother
	 */
	void setSmoother(std::shared_ptr<const Smoother<D>> smoother)
	{
		this->smoother = smoother;
	}
	/**
	 * @brief Get smoother operator for this level.
	 *
	 * @return Pointer to the smoother operator.
	 */
	std::shared_ptr<const Smoother<D>> getSmoother() const
	{
		return smoother;
	}
	/**
	 * @brief Get DomainCollection for this level.
	 *
	 * @return DomainCollection for this level.
	 */
	const std::shared_ptr<VectorGenerator<D>> &getVectorGenerator() const
	{
		return vg;
	}
	/**
	 * @brief Set pointer to the coarser level.
	 *
	 * @param coarser the pointer to the coarser level.
	 */
	void setCoarser(std::shared_ptr<Level> coarser)
	{
		this->coarser = coarser;
	}
	/**
	 * @brief get reference to the coarser level.
	 *
	 * @return reference to the coarser level.
	 */
	std::shared_ptr<const Level> getCoarser() const
	{
		return coarser;
	}
	/**
	 * @brief Set the pointer to the finer level.
	 *
	 * @param finer pointer to the finer level.
	 */
	void setFiner(std::shared_ptr<Level> finer)
	{
		this->finer = finer;
	}
	/**
	 * @brief Get the finer level
	 */
	std::shared_ptr<Level> getFiner()
	{
		return finer;
	}
	/**
	 * @brief Get the finer level
	 */
	std::shared_ptr<const Level> getFiner() const
	{
		return std::shared_ptr<const Level>(finer);
	}
	/**
	 * @brief Check if this level is the finest level.
	 *
	 * @return whether or not this level is the finest level.
	 */
	bool finest() const
	{
		return finer.expired();
	}
	/**
	 * @brief Check if this level is the coarsest level.
	 *
	 * @return whether or not this level is the coarsest level.
	 */
	bool coarsest() const
	{
		return coarser == nullptr;
	}
};
} // namespace GMG
} // namespace ThunderEgg
#endif