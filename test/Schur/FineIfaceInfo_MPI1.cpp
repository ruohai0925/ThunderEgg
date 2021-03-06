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
#include <ThunderEgg/Schur/FineIfaceInfo.h>
#include <algorithm>
using namespace std;
using namespace ThunderEgg;

template <typename Container, typename Value> bool contains(Container &deque, Value a)
{
	return find(deque.begin(), deque.end(), a) != deque.end();
}
TEST_CASE("Schur::FineIfaceInfo constructor", "[Schur::FineIfaceInfo]")
{
	for (Side<2> s : Side<2>::getValues()) {
		int           id                  = 1;
		array<int, 2> nbr_ids             = {2, 3};
		auto          pinfo               = make_shared<PatchInfo<2>>();
		pinfo->rank                       = 0;
		pinfo->id                         = id;
		pinfo->nbr_info[s.getIndex()]     = make_shared<FineNbrInfo<2>>(nbr_ids);
		pinfo->getFineNbrInfo(s).ranks[0] = 1;
		pinfo->getFineNbrInfo(s).ranks[1] = 2;
		Schur::FineIfaceInfo<2> iface_info(pinfo, s);
		INFO("Side: " << s);
		CHECK(iface_info.rank == 0);
		CHECK(iface_info.fine_ranks[0] == 1);
		CHECK(iface_info.fine_ranks[1] == 2);
		// check that the id is encoded as expected
		CHECK(iface_info.id / (int) Side<2>::num_sides == id);
		CHECK(iface_info.id % Side<2>::num_sides == s.getIndex());
		// check that iface belongs to nbr
		CHECK(iface_info.fine_ids[0] / (int) Side<2>::num_sides == nbr_ids[0]);
		CHECK(iface_info.fine_ids[0] % Side<2>::num_sides == s.opposite().getIndex());
		CHECK(iface_info.fine_ids[1] / (int) Side<2>::num_sides == nbr_ids[1]);
		CHECK(iface_info.fine_ids[1] % Side<2>::num_sides == s.opposite().getIndex());
		// local and global index should be set to -1
		CHECK(iface_info.patch_local_index == -1);
		CHECK(iface_info.row_local_index == -1);
		CHECK(iface_info.col_local_index == -1);
		CHECK(iface_info.fine_col_local_indexes[0] == -1);
		CHECK(iface_info.fine_col_local_indexes[1] == -1);
		CHECK(iface_info.global_index == -1);
		CHECK(iface_info.fine_global_indexes[0] == -1);
		CHECK(iface_info.fine_global_indexes[1] == -1);
		CHECK(iface_info.nbr_info == pinfo->nbr_info[s.getIndex()]);
	}
}