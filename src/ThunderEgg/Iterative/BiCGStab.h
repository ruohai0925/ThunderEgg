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

#ifndef THUNDEREGG_ITERATIVE_BICGSTAB_H
#define THUNDEREGG_ITERATIVE_BICGSTAB_H
#include <ThunderEgg/BreakdownError.h>
#include <ThunderEgg/DivergenceError.h>
#include <ThunderEgg/Iterative/Solver.h>
#include <ThunderEgg/Operator.h>
#include <ThunderEgg/Timer.h>
#include <ThunderEgg/VectorGenerator.h>

namespace ThunderEgg
{
namespace Iterative
{
/**
 * @brief ThunderEgg implementation of BiCGStab iterative solver.
 *
 * @tparam D the number of Cartesian dimensions
 */
template <int D> class BiCGStab : public Solver<D>
{
	private:
	/**
	 * @brief The maximum number of iterations
	 */
	int max_iterations = 1000;
	/**
	 * @brief The maximum number of iterations
	 */
	double tolerance = 1e-12;
	/**
	 * @brief The timer
	 */
	std::shared_ptr<Timer> timer = nullptr;

	void applyWithPreconditioner(std::shared_ptr<VectorGenerator<D>> vg,
	                             std::shared_ptr<const Operator<D>>  M_l,
	                             std::shared_ptr<const Operator<D>>  A,
	                             std::shared_ptr<const Operator<D>>  M_r,
	                             std::shared_ptr<const Vector<D>>    x,
	                             std::shared_ptr<Vector<D>>          b) const
	{
		if (M_l == nullptr && M_r == nullptr) {
			A->apply(x, b);
		} else if (M_l == nullptr && M_r != nullptr) {
			std::shared_ptr<Vector<D>> tmp = vg->getNewVector();
			M_r->apply(x, tmp);
			A->apply(tmp, b);
		}
	}

	public:
	/**
	 * @brief Set the maximum number of iterations
	 *
	 * Default is 1000
	 *
	 * @param max_iterations_in the maximum number of iterations
	 */
	void setMaxIterations(int max_iterations_in)
	{
		max_iterations = max_iterations_in;
	};
	/**
	 * @brief Get the maximum number of iterations
	 *
	 * Default is 1000
	 *
	 * @return int the maximum number of iterations
	 */
	int getMaxIterations() const
	{
		return max_iterations;
	}
	/**
	 * @brief Set the stopping tolerance
	 *
	 * Default is 1e-12
	 *
	 * @param tolerance_in the stopping tolerance
	 */
	void setTolerance(double tolerance_in)
	{
		tolerance = tolerance_in;
	};
	/**
	 * @brief Get the stopping tolerance
	 *
	 * Default is 1e-12
	 *
	 * @return double the stopping tolerance
	 */
	double getTolerance() const
	{
		return tolerance;
	}
	/**
	 * @brief Set the Timer object
	 *
	 * @param timer_in the Timer
	 */
	void setTimer(std::shared_ptr<Timer> timer_in)
	{
		timer = timer_in;
	}

	/**
	 * @brief Get the Timer object
	 *
	 * @return std::shared_ptr<Timer> the Timer
	 */
	std::shared_ptr<Timer> getTimer() const
	{
		return timer;
	}

	public:
	int solve(std::shared_ptr<VectorGenerator<D>> vg, std::shared_ptr<const Operator<D>> A,
	          std::shared_ptr<Vector<D>> x, std::shared_ptr<const Vector<D>> b,
	          std::shared_ptr<const Operator<D>> Mr = nullptr, bool output = false,
	          std::ostream &os = std::cout) const override
	{
		std::shared_ptr<Vector<D>> resid = vg->getNewVector();

		A->apply(x, resid);
		resid->scaleThenAdd(-1, b);

		std::shared_ptr<Vector<D>> initial_guess = vg->getNewVector();
		initial_guess->copy(x);
		x->set(0);

		double                     r0_norm = b->twoNorm();
		std::shared_ptr<Vector<D>> rhat    = vg->getNewVector();
		rhat->copy(resid);
		std::shared_ptr<Vector<D>> p = vg->getNewVector();
		p->copy(resid);
		std::shared_ptr<Vector<D>> ap = vg->getNewVector();
		std::shared_ptr<Vector<D>> as = vg->getNewVector();

		std::shared_ptr<Vector<D>> s   = vg->getNewVector();
		double                     rho = rhat->dot(resid);

		int num_its = 0;
		if (r0_norm == 0) {
			return num_its;
		}
		double residual = resid->twoNorm() / r0_norm;
		if (output) {
			char buf[100];
			sprintf(buf, "%5d %16.8e\n", num_its, residual);
			os << std::string(buf);
		}
		while (residual > tolerance && num_its < max_iterations) {
			if (timer) {
				timer->start("Iteration");
			}

			if (rho == 0) {
				throw BreakdownError("BiCGStab broke down, rho was 0 on iteration "
				                     + std::to_string(num_its));
			}

			applyWithPreconditioner(vg, nullptr, A, Mr, p, ap);
			double alpha = rho / rhat->dot(ap);
			s->copy(resid);
			s->addScaled(-alpha, ap);
			if (s->twoNorm() / r0_norm <= tolerance) {
				x->addScaled(alpha, p);
				if (timer) {
					timer->stop("Iteration");
				}
				break;
			}
			applyWithPreconditioner(vg, nullptr, A, Mr, s, as);
			double omega = as->dot(s) / as->dot(as);
			x->addScaled(alpha, p, omega, s);
			resid->addScaled(-alpha, ap);
			resid->addScaled(-omega, as);

			double rho_new = resid->dot(rhat);
			double beta    = rho_new * alpha / (rho * omega);
			p->addScaled(-omega, ap);
			p->scaleThenAdd(beta, resid);

			num_its++;
			rho      = rho_new;
			residual = resid->twoNorm() / r0_norm;

			if (residual > 1e6) {
				throw DivergenceError("BiCGStab reached divergence criteria on iteration "
				                      + std::to_string(num_its) + " with residual two norm "
				                      + std::to_string(residual));
			}
			if (output) {
				char buf[100];
				sprintf(buf, "%5d %16.8e\n", num_its, residual);
				os << std::string(buf);
			}
			if (timer) {
				timer->stop("Iteration");
			}
		}
		if (Mr != nullptr) {
			Mr->apply(x, resid);
			x->copy(resid);
		}
		x->add(initial_guess);
		return num_its;
	}
};
} // namespace Iterative
} // namespace ThunderEgg
#endif