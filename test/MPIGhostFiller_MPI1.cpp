#include "MPIGhostFiller_MOCKS.h"
#include "catch.hpp"
#include "utils/DomainReader.h"
#include <ThunderEgg/DomainTools.h>
#include <ThunderEgg/MPIGhostFiller.h>
#include <ThunderEgg/ValVector.h>
#include <list>
using namespace std;
using namespace ThunderEgg;

constexpr auto single_mesh_file  = "mesh_inputs/2d_uniform_2x2_mpi1.json";
constexpr auto refined_mesh_file = "mesh_inputs/2d_uniform_2x2_refined_nw_mpi1.json";
constexpr auto cross_mesh_file   = "mesh_inputs/2d_uniform_8x8_refined_cross_mpi1.json";

TEST_CASE("No calls for 1 patch domain", "[MPIGhostFiller]")
{
	auto                  num_components = GENERATE(1, 2, 3);
	auto                  nx             = GENERATE(2, 5);
	auto                  ny             = GENERATE(2, 5);
	int                   num_ghost      = 1;
	DomainReader<2>       domain_reader(single_mesh_file, {nx, ny}, num_ghost);
	shared_ptr<Domain<2>> d_coarse = domain_reader.getCoarserDomain();

	auto vec = ValVector<2>::GetNewVector(d_coarse, num_components);

	CallMockMPIGhostFiller<2> mgf(d_coarse, num_components, 1);

	mgf.fillGhost(vec);

	CHECK(mgf.called == true);
	mgf.checkCalls();
}
TEST_CASE("Calls for various domains 1-side cases", "[MPIGhostFiller]")
{
	auto num_components = GENERATE(1, 2, 3);
	auto mesh_file
	= GENERATE(as<std::string>{}, single_mesh_file, refined_mesh_file, cross_mesh_file);
	INFO("MESH: " << mesh_file);
	auto                  nx        = GENERATE(2, 5);
	auto                  ny        = GENERATE(2, 5);
	int                   num_ghost = 1;
	DomainReader<2>       domain_reader(mesh_file, {nx, ny}, num_ghost);
	shared_ptr<Domain<2>> d_fine = domain_reader.getFinerDomain();

	auto vec = ValVector<2>::GetNewVector(d_fine, num_components);

	CallMockMPIGhostFiller<2> mgf(d_fine, num_components, 1);

	mgf.fillGhost(vec);

	CHECK(mgf.called == true);

	mgf.checkCalls();
}
TEST_CASE("Exchange for various domains 1-side cases", "[MPIGhostFiller]")
{
	auto num_components = GENERATE(1, 2, 3);
	auto mesh_file
	= GENERATE(as<std::string>{}, single_mesh_file, refined_mesh_file, cross_mesh_file);
	INFO("MESH: " << mesh_file);
	auto                  nx        = GENERATE(2, 5);
	auto                  ny        = GENERATE(2, 5);
	int                   num_ghost = 1;
	DomainReader<2>       domain_reader(mesh_file, {nx, ny}, num_ghost);
	shared_ptr<Domain<2>> d_fine = domain_reader.getFinerDomain();

	auto vec = ValVector<2>::GetNewVector(d_fine, num_components);
	for (auto pinfo : d_fine->getPatchInfoVector()) {
		for (int c = 0; c < num_components; c++) {
			auto data = vec->getLocalData(c, pinfo->local_index);
			nested_loop<2>(data.getStart(), data.getEnd(),
			               [&](const std::array<int, 2> &coord) { data[coord] = pinfo->id; });
		}
	}

	ExchangeMockMPIGhostFiller<2> mgf(d_fine, 1);

	mgf.fillGhost(vec);

	mgf.checkVector(vec);
}
TEST_CASE("Two Exchanges for various domains 1-side cases", "[MPIGhostFiller]")
{
	auto num_components = GENERATE(1, 2, 3);
	auto mesh_file
	= GENERATE(as<std::string>{}, single_mesh_file, refined_mesh_file, cross_mesh_file);
	INFO("MESH: " << mesh_file);
	auto                  nx        = GENERATE(2, 5);
	auto                  ny        = GENERATE(2, 5);
	int                   num_ghost = 1;
	DomainReader<2>       domain_reader(mesh_file, {nx, ny}, num_ghost);
	shared_ptr<Domain<2>> d_fine = domain_reader.getFinerDomain();

	auto vec = ValVector<2>::GetNewVector(d_fine, num_components);
	for (auto pinfo : d_fine->getPatchInfoVector()) {
		for (int c = 0; c < num_components; c++) {
			auto data = vec->getLocalData(c, pinfo->local_index);
			nested_loop<2>(data.getStart(), data.getEnd(),
			               [&](const std::array<int, 2> &coord) { data[coord] = pinfo->id; });
		}
	}

	ExchangeMockMPIGhostFiller<2> mgf(d_fine, 1);

	mgf.fillGhost(vec);
	mgf.fillGhost(vec);

	mgf.checkVector(vec);
}