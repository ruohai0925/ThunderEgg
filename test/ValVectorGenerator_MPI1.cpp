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

#include "catch.hpp"
#include "utils/DomainReader.h"
#include <ThunderEgg/ValVectorGenerator.h>
using namespace std;
using namespace ThunderEgg;
#define MESHES                                                                                     \
	"mesh_inputs/2d_uniform_2x2_mpi1.json", "mesh_inputs/2d_uniform_8x8_refined_cross_mpi1.json"
TEST_CASE("ValVectorGenerator getNewVector", "[ValVectorGenerator]")
{
	int  num_components = GENERATE(1, 2, 3);
	auto mesh_file      = GENERATE(as<std::string>{}, MESHES);
	INFO("MESH FILE " << mesh_file);
	int nx = GENERATE(1, 4, 5);
	int ny = GENERATE(1, 4, 5);
	INFO("nx:       " << nx);
	INFO("ny:       " << ny);
	int                   num_ghost = 0;
	DomainReader<2>       domain_reader(mesh_file, {nx, ny}, num_ghost);
	shared_ptr<Domain<2>> d_fine = domain_reader.getFinerDomain();

	ValVectorGenerator<2> vg(d_fine, num_components);
	auto                  val_vector = vg.getNewVector();

	CHECK(val_vector->getNumLocalCells() == d_fine->getNumLocalCells());
	CHECK(val_vector->getNumComponents() == num_components);
	CHECK(val_vector->getNumLocalPatches() == d_fine->getNumLocalPatches());
	CHECK(val_vector->getMPIComm() == MPI_COMM_WORLD);
	CHECK(val_vector->getLocalData(0, 0).getLengths()[0] == nx);
	CHECK(val_vector->getLocalData(0, 0).getLengths()[1] == ny);
}