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

#ifndef THUNDEREGG_GMG_INTERLEVELCOMM_H
#define THUNDEREGG_GMG_INTERLEVELCOMM_H

#include <ThunderEgg/Domain.h>
#include <ThunderEgg/RuntimeError.h>
#include <ThunderEgg/ValVector.h>

namespace ThunderEgg
{
namespace GMG
{
/**
 * @brief A communicator between a finer domain and a coarser domain.
 *
 * This class will determine the following things:
 *
 * Which patches in the finer domain have a parent patch in the coarser domain on the same rank?
 * 	- getPatchesWithLocalParent() will return a vector of these patches and the local indexes of
 *    their parent patches.
 *
 * Which patches in the finer domain have a parent patch in the coarser domain on a different rank?
 * 	- getPatchesWithGhostParent() will return a vector of these patches and the local indexes in the
 * 	  ghost vector.
 * 	- getNewGhostVector() will allocate a new vector for these ghost values.
 *
 */
template <int D> class InterLevelComm
{
	private:
	/**
	 * @brief Dimensions of a patch
	 */
	std::array<int, D> ns;
	/**
	 * @brief Number of ghost cells
	 */
	int num_ghost_cells;
	/**
	 * @brief Number of ghost patches
	 */
	int num_ghost_patches;
	/**
	 * @brief The number of components per patch
	 */
	int num_components;
	/**
	 * @brief Number of values in a patch. (including ghost values)
	 */
	int patch_size;
	/**
	 * @brief Patches with a local parent
	 *
	 * First value: local index of coarser patch
	 * Second value: the PatchInfo object
	 */
	std::vector<std::pair<int, std::shared_ptr<const PatchInfo<D>>>> patches_with_local_parent;
	/**
	 * @brief Patches with a ghost parent
	 *
	 * First value: local index in the ghost vector of coarser patch
	 * Second value: the PatchInfo object
	 */
	std::vector<std::pair<int, std::shared_ptr<const PatchInfo<D>>>> patches_with_ghost_parent;
	/**
	 * @brief A vector of pairs where the first value is the rank, and the second vector is the
	 * local_indexes of patches in the order that the other processor is expecting them.
	 */
	std::vector<std::pair<int, std::vector<int>>> rank_and_local_indexes_for_vector;
	/**
	 * @brief A vector of pairs where the first value is the rank, and the second vector is the
	 * local_indexes of ghost patches in the order that the other processor is expecting them.
	 */
	std::vector<std::pair<int, std::vector<int>>> rank_and_local_indexes_for_ghost_vector;
	bool                                          communicating = false;
	bool                                          sending       = false;

	std::shared_ptr<const Vector<D>> current_vector;
	std::shared_ptr<const Vector<D>> current_ghost_vector;

	std::vector<std::vector<double>> recv_buffers;
	std::vector<MPI_Request>         recv_requests;
	std::vector<std::vector<double>> send_buffers;
	std::vector<MPI_Request>         send_requests;

	public:
	/**
	 * @brief Create a new InterLevelComm object.
	 *
	 * @param coarse_domain the coarser DomainCollection.
	 * @param num_coarser_components the number of components for eac cell of the coarser domain
	 * @param fine_domain the finer DomainCollection.
	 */
	InterLevelComm(std::shared_ptr<const Domain<D>> coarser_domain, int num_coarser_components,
	               std::shared_ptr<const Domain<D>> finer_domain)
	: ns(finer_domain->getNs()), num_ghost_cells(finer_domain->getNumGhostCells()),
	  num_components(num_coarser_components)
	{
		int my_patch_size = num_components;
		for (size_t axis = 0; axis < D; axis++) {
			my_patch_size *= ns[axis] + 2 * num_ghost_cells;
		}
		patch_size = my_patch_size;

		// sort into patches with local parents and patches with ghost parents
		std::deque<std::pair<int, std::shared_ptr<const PatchInfo<D>>>> local_parents;
		std::deque<std::shared_ptr<const PatchInfo<D>>>                 ghost_parents;
		std::set<int>                                                   ghost_parents_ids;

		int rank;
		MPI_Comm_rank(MPI_COMM_WORLD, &rank);
		// TODO this has to be changed when domain class is updated
		auto cd_map = coarser_domain->getPatchInfoMap();
		for (auto patch : finer_domain->getPatchInfoVector()) {
			if (patch->parent_rank == rank) {
				local_parents.emplace_back(cd_map.at(patch->parent_id)->local_index, patch);
			} else {
				ghost_parents.push_back(patch);
				ghost_parents_ids.insert(patch->parent_id);
			}
		}
		num_ghost_patches = ghost_parents_ids.size();

		// fill in local vector
		patches_with_local_parent
		= std::vector<std::pair<int, std::shared_ptr<const PatchInfo<D>>>>(local_parents.begin(),
		                                                                   local_parents.end());
		// find local coarse patches that are ghost paches on other ranks
		std::map<int, std::map<int, int>>
		ranks_and_local_patches; // map from rank -> (map of ids -> local
		                         // indexes). The second map is for when
		                         // local indexes are iterated over, they
		                         // are sorted by their cooresponding id
		for (auto pinfo : coarser_domain->getPatchInfoVector()) {
			for (int child_rank : pinfo->child_ranks) {
				if (child_rank != -1 && child_rank != rank)
					ranks_and_local_patches[child_rank][pinfo->id] = pinfo->local_index;
			}
		}

		rank_and_local_indexes_for_vector.reserve(ranks_and_local_patches.size());
		for (auto pair : ranks_and_local_patches) {
			std::vector<int> local_indexes;
			// the map sould have sorted patches by id, the other processors will expect things in
			// this order
			local_indexes.reserve(pair.second.size());
			for (auto id_local_index_pair : pair.second) {
				local_indexes.push_back(id_local_index_pair.second);
			}
			rank_and_local_indexes_for_vector.emplace_back(pair.first, local_indexes);
		}

		// fill in ghost vector
		// first, assign local indexes in ghost vector for ghost patches
		std::map<int, int> id_ghost_vector_local_index_map;
		int                index = 0;
		for (int id : ghost_parents_ids) {
			id_ghost_vector_local_index_map[id] = index;
			index++;
		}

		std::map<int, std::map<int, int>>
		ranks_and_ghost_patches; // map from rank -> (map of ids -> ghost
		                         // indexes). The second map is for when
		                         // local indexes are iterated over, they
		                         // are sorted by their cooresponding id
		patches_with_ghost_parent.reserve(ghost_parents.size());
		for (auto patch : ghost_parents) {
			int ghost_local_index = id_ghost_vector_local_index_map[patch->parent_id];

			ranks_and_ghost_patches[patch->parent_rank][patch->parent_id] = ghost_local_index;

			patches_with_ghost_parent.emplace_back(ghost_local_index, patch);
		}

		rank_and_local_indexes_for_ghost_vector.reserve(ranks_and_local_patches.size());
		for (auto pair : ranks_and_ghost_patches) {
			// the map sould have sorted patches by id, the other processors will expect things in
			// this order
			std::vector<int> local_indexes;
			local_indexes.reserve(pair.second.size());
			for (auto id_local_index_pair : pair.second) {
				local_indexes.push_back(id_local_index_pair.second);
			}
			rank_and_local_indexes_for_ghost_vector.emplace_back(pair.first, local_indexes);
		}
	}
	/**
	 * @brief Destroy the InterLevelComm object
	 */
	~InterLevelComm()
	{
		if (!(send_requests.empty() && recv_requests.empty())) {
			// destructor is being called with unfinished communication
			// finish communication (this will free mpi allocated stuff)
			MPI_Waitall(send_requests.size(), send_requests.data(), MPI_STATUS_IGNORE);
			MPI_Waitall(recv_requests.size(), recv_requests.data(), MPI_STATUS_IGNORE);
		}
	}

	/**
	 * @brief Allocate a new vector for ghost patch values
	 *
	 * @return the newly allocated vector.
	 */
	std::shared_ptr<Vector<D>> getNewGhostVector() const
	{
		return std::make_shared<ValVector<D>>(MPI_COMM_SELF, ns, num_ghost_cells, num_components,
		                                      num_ghost_patches);
	}

	/**
	 * @brief Get the vector of finer patches that have a local parent
	 *
	 * The vector will consist of pair values:
	 * 		- First value: the local index of the parent patch
	 * 		- First value: the finer patch associated with that index
	 *
	 * @return const std::vector<std::pair<int, std::shared_ptr<const PatchInfo<D>>>>& the vector
	 */
	const std::vector<std::pair<int, std::shared_ptr<const PatchInfo<D>>>> &
	getPatchesWithLocalParent() const
	{
		return patches_with_local_parent;
	}

	/**
	 * @brief Get the vector of finer patches that have a ghost parent
	 *
	 * The vector will consist of pair values:
	 * 		- First value: the local index in the ghost vector of the parent patch
	 * 		- First value: the finer patch associated with that index
	 *
	 * @return const std::vector<std::pair<int, std::shared_ptr<const PatchInfo<D>>>>& the vector
	 */
	const std::vector<std::pair<int, std::shared_ptr<const PatchInfo<D>>>> &
	getPatchesWithGhostParent() const
	{
		return patches_with_ghost_parent;
	}

	/**
	 * @brief Start the communication for sending ghost values.
	 *
	 * This will send the values in the ghost vector and will add them to the values in the vector.
	 * This is essentially a reverse scatter.
	 *
	 * This function is seperated into a Start and Finish function, this allows for other
	 * computations to happen while the communication is happening.
	 *
	 * @param vector the vector
	 * @param ghost_vector the associated ghost vector
	 */
	void sendGhostPatchesStart(std::shared_ptr<Vector<D>>       vector,
	                           std::shared_ptr<const Vector<D>> ghost_vector)
	{
		if (communicating) {
			if (sending) {
				throw RuntimeError(
				"InterLevelComm has a sendGhostPatches posted that is unfinished");
			} else {
				throw RuntimeError(
				"InterLevelComm has a getGhostPatches posted that is unfinished");
			}
		}

		// keep track of what vectors we are using
		current_ghost_vector = ghost_vector;
		current_vector       = vector;

		// post receives
		recv_buffers.reserve(rank_and_local_indexes_for_vector.size());
		recv_requests.reserve(rank_and_local_indexes_for_vector.size());
		for (auto rank_indexes_pair : rank_and_local_indexes_for_vector) {
			// allocate buffer
			recv_buffers.emplace_back(patch_size * rank_indexes_pair.second.size());

			// post the receive
			int rank = rank_indexes_pair.first;
			recv_requests.emplace_back();
			MPI_Irecv(recv_buffers.back().data(), recv_buffers.back().size(), MPI_DOUBLE, rank, 0,
			          MPI_COMM_WORLD, &recv_requests.back());
		}
		send_buffers.reserve(rank_and_local_indexes_for_ghost_vector.size());
		send_requests.reserve(rank_and_local_indexes_for_ghost_vector.size());
		// post sends
		for (auto rank_indexes_pair : rank_and_local_indexes_for_ghost_vector) {
			// allocate buffer
			send_buffers.emplace_back(patch_size * rank_indexes_pair.second.size());

			// fill buffer with values
			int buffer_idx = 0;
			for (int local_index : rank_indexes_pair.second) {
				auto local_datas = ghost_vector->getLocalDatas(local_index);
				for (const auto &local_data : local_datas) {
					nested_loop<D>(local_data.getGhostStart(), local_data.getGhostEnd(),
					               [&](const std::array<int, D> &coord) {
						               send_buffers.back()[buffer_idx] = local_data[coord];
						               buffer_idx++;
					               });
				}
			}

			// post the send
			int rank = rank_indexes_pair.first;
			send_requests.emplace_back();
			MPI_Isend(send_buffers.back().data(), send_buffers.back().size(), MPI_DOUBLE, rank, 0,
			          MPI_COMM_WORLD, &send_requests.back());
		}

		// set state
		communicating = true;
		sending       = true;
	}
	/**
	 * @brief Finish the communication for sending ghost values.
	 *
	 * This will send the values in the ghost vector and will add them to the values in the
	 * vector. This is essentially a reverse scatter.
	 *
	 * This function is seperated into a Start and Finish function, this allows for other
	 * computations to happen while the communication is happening.
	 *
	 * @param vector the vector
	 * @param ghost_vector the associated ghost vector
	 */
	void sendGhostPatchesFinish(std::shared_ptr<Vector<D>>       vector,
	                            std::shared_ptr<const Vector<D>> ghost_vector)
	{
		if (!communicating) {
			throw RuntimeError(
			"InterLevelComm cannot finish sendGhostPatches since communication was not started");
		} else if (!sending) {
			throw RuntimeError(
			"InterLevelComm sendGhostPatchesFinish is being called after getGhostPatchesStart was called");
		}
		if (vector != current_vector) {
			throw RuntimeError(
			"InterLevelComm sendGhostPatchesFinish is being called with a different vector than when sendGhostPatchesStart was called");
		}
		if (ghost_vector != current_ghost_vector) {
			throw RuntimeError(
			"InterLevelComm senGhostPatchesFinish is being called with a different ghost vector than when sendGhostPatchesStart was called");
		}

		// finish recvs
		for (size_t i = 0; i < rank_and_local_indexes_for_vector.size(); i++) {
			int finished_idx;
			MPI_Waitany(recv_requests.size(), recv_requests.data(), &finished_idx,
			            MPI_STATUS_IGNORE);

			// get local indexes for the buffer that was received
			std::vector<int> &local_indexes
			= rank_and_local_indexes_for_vector.at(finished_idx).second;

			// add the values in the buffer to the vector
			std::vector<double> &buffer     = recv_buffers.at(finished_idx);
			int                  buffer_idx = 0;
			for (int local_index : local_indexes) {
				auto local_datas = vector->getLocalDatas(local_index);
				for (auto &local_data : local_datas) {
					nested_loop<D>(local_data.getGhostStart(), local_data.getGhostEnd(),
					               [&](const std::array<int, D> &coord) {
						               local_data[coord] += buffer[buffer_idx];
						               buffer_idx++;
					               });
				}
			}
		}

		// wait for sends for finish
		MPI_Waitall(send_requests.size(), send_requests.data(), MPI_STATUS_IGNORE);

		// clear buffers
		recv_requests.clear();
		recv_buffers.clear();
		send_requests.clear();
		send_buffers.clear();

		// set state
		communicating        = false;
		current_ghost_vector = nullptr;
		current_vector       = nullptr;
	}
	/**
	 * @brief Start the communication for getting ghost values.
	 *
	 * This will send the values in the vector to the ghost vector, and will overwrite the
	 * values in the ghost vector. This is essentially a forward scatter.
	 *
	 * This function is seperated into a Start and Finish function, this allows for other
	 * computations to happen while the communication is happening.
	 *
	 * @param vector the vector
	 * @param ghost_vector the associated ghost vector
	 */
	void getGhostPatchesStart(std::shared_ptr<const Vector<D>> vector,
	                          std::shared_ptr<Vector<D>>       ghost_vector)
	{
		if (communicating) {
			if (sending) {
				throw RuntimeError(
				"InterLevelComm has a sendGhostPatches posted that is unfinished");
			} else {
				throw RuntimeError(
				"InterLevelComm has a getGhostPatches posted that is unfinished");
			}
		}

		// keep track of what vectors we are using
		current_ghost_vector = ghost_vector;
		current_vector       = vector;

		// post receives
		recv_buffers.reserve(rank_and_local_indexes_for_ghost_vector.size());
		recv_requests.reserve(rank_and_local_indexes_for_ghost_vector.size());
		for (auto rank_indexes_pair : rank_and_local_indexes_for_ghost_vector) {
			// allocate buffer
			recv_buffers.emplace_back(patch_size * rank_indexes_pair.second.size());

			// post the recieve
			int rank = rank_indexes_pair.first;
			recv_requests.emplace_back();
			MPI_Irecv(recv_buffers.back().data(), recv_buffers.back().size(), MPI_DOUBLE, rank, 0,
			          MPI_COMM_WORLD, &recv_requests.back());
		}
		send_buffers.reserve(rank_and_local_indexes_for_vector.size());
		send_requests.reserve(rank_and_local_indexes_for_vector.size());
		// post sends
		for (auto rank_indexes_pair : rank_and_local_indexes_for_vector) {
			// allocate buffer
			send_buffers.emplace_back(patch_size * rank_indexes_pair.second.size());

			// fill buffer with values
			int buffer_idx = 0;
			for (int local_index : rank_indexes_pair.second) {
				auto local_datas = vector->getLocalDatas(local_index);
				for (const auto &local_data : local_datas) {
					nested_loop<D>(local_data.getGhostStart(), local_data.getGhostEnd(),
					               [&](const std::array<int, D> &coord) {
						               send_buffers.back()[buffer_idx] = local_data[coord];
						               buffer_idx++;
					               });
				}
			}

			// post the send
			int rank = rank_indexes_pair.first;
			send_requests.emplace_back();
			MPI_Isend(send_buffers.back().data(), send_buffers.back().size(), MPI_DOUBLE, rank, 0,
			          MPI_COMM_WORLD, &send_requests.back());
		}

		// set state
		communicating = true;
		sending       = false;
	}
	/**
	 * @brief Finish the communication for getting ghost values.
	 *
	 * This will send the values in the vector to the ghost vector, and will overwrite the
	 * values in the ghost vector. This is essentially a forward scatter.
	 *
	 * This function is seperated into a Start and Finish function, this allows for other
	 * computations to happen while the communication is happening.
	 *
	 * @param vector the vector
	 * @param ghost_vector the associated ghost vector
	 */
	void getGhostPatchesFinish(std::shared_ptr<const Vector<D>> vector,
	                           std::shared_ptr<Vector<D>>       ghost_vector)
	{
		if (!communicating) {
			throw RuntimeError(
			"InterLevelComm cannot finish sendGhostPatches since communication was not started");
		} else if (sending) {
			throw RuntimeError(
			"InterLevelComm getGhostPatchesFinish is being called after sendGhostPatchesStart was called");
		}
		if (vector != current_vector) {
			throw RuntimeError(
			"InterLevelComm getGhostPatchesFinish is being called with a different vector than when getGhostPatchesStart was called");
		}
		if (ghost_vector != current_ghost_vector) {
			throw RuntimeError(
			"InterLevelComm getGhostPatchesFinish is being called with a different ghost vector than when getGhostPatchesStart was called");
		}

		// finish recvs
		for (size_t i = 0; i < rank_and_local_indexes_for_ghost_vector.size(); i++) {
			int finished_idx;
			MPI_Waitany(recv_requests.size(), recv_requests.data(), &finished_idx,
			            MPI_STATUS_IGNORE);

			// get local indexes for the buffer that was recieved
			std::vector<int> &local_indexes
			= rank_and_local_indexes_for_ghost_vector.at(finished_idx).second;

			// add the values in the buffer to the vector
			std::vector<double> &buffer     = recv_buffers.at(finished_idx);
			int                  buffer_idx = 0;
			for (int local_index : local_indexes) {
				auto local_datas = ghost_vector->getLocalDatas(local_index);
				for (auto &local_data : local_datas) {
					nested_loop<D>(local_data.getGhostStart(), local_data.getGhostEnd(),
					               [&](const std::array<int, D> &coord) {
						               local_data[coord] = buffer[buffer_idx];
						               buffer_idx++;
					               });
				}
			}
		}

		// wait for sends for finish
		MPI_Waitall(send_requests.size(), send_requests.data(), MPI_STATUS_IGNORE);

		// clear buffers
		recv_requests.clear();
		recv_buffers.clear();
		send_requests.clear();
		send_buffers.clear();

		// set state
		communicating        = false;
		current_ghost_vector = nullptr;
		current_vector       = nullptr;
	}
};
extern template class InterLevelComm<2>;
extern template class InterLevelComm<3>;
} // namespace GMG
} // namespace ThunderEgg
#endif
