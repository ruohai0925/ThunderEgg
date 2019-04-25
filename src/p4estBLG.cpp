#include "p4estBLG.h"
#include <p4est_extended.h>
#include <p4est_iterate.h>
using namespace std;
void get_num_levels(p4est_iter_volume_info_t *info, void *user_data)
{
	int &max_level  = *(int *) user_data;
	int  quad_level = info->quad->level;
	if (quad_level > max_level) { max_level = quad_level; }
}
void set_ids(p4est_iter_volume_info_t *info, void *user_data)
{
	int *data = (int *) &info->quad->p.user_data;
	data[0]   = *(int *) user_data + info->quadid;
}
p4estBLG::p4estBLG(p4est_t *p4est, int n)
{
	p4est_t *my_p4est = p4est_copy(p4est, false);

	int max_level = 0;
	p4est_iterate_ext(my_p4est, nullptr, &max_level, get_num_levels, nullptr, nullptr, 0);
	int global_max_level = 0;
	MPI_Allreduce(&max_level, &global_max_level, 1, MPI_INT, MPI_MAX, my_p4est->mpicomm);

	levels.resize(global_max_level + 1);

	int global_id_start;
	MPI_Scan(&my_p4est->local_num_quadrants, &global_id_start, 1, MPI_INT, MPI_SUM,
	         my_p4est->mpicomm);
	global_id_start -= my_p4est->local_num_quadrants;

	// set global ids
	p4est_iterate_ext(my_p4est, nullptr, &global_id_start, set_ids, nullptr, nullptr, 0);

	extractLevel(my_p4est, global_max_level, n);
}
void set_ranks(p4est_iter_volume_info_t *info, void *user_data)
{
	int *data = (int *) &info->quad->p.user_data;
	data[1]   = info->p4est->mpirank;
}
struct create_domains_ctx {
	void **              ghost_data;
	p4estBLG::DomainMap *dmap;
};
void create_domains(p4est_iter_volume_info_t *info, void *user_data)
{
	create_domains_ctx & ctx  = *(create_domains_ctx *) user_data;
	p4estBLG::DomainMap &dmap = *ctx.dmap;

	// create domain object
	int                    global_id = info->quad->p.user_int;
	shared_ptr<Domain<2>> &ptr       = dmap[global_id];
	if (ptr == nullptr) { ptr.reset(new Domain<2>); }
	Domain<2> &d = *ptr;

	d.id       = global_id;
	d.id_local = info->quadid;

	// get length and coord (this is assuming trees are unit length)
	double length = (double) P4EST_QUADRANT_LEN(info->quad->level) / P4EST_ROOT_LEN;
	d.lengths.fill(length);
	double x    = (double) info->quad->x / P4EST_ROOT_LEN;
	double y    = (double) info->quad->y / P4EST_ROOT_LEN;
	d.starts[0] = x;
	d.starts[1] = y;

	// set refinement level
	d.refine_level = info->quad->level;
}
void link_domains(p4est_iter_face_info_t *info, void *user_data)
{
	create_domains_ctx & ctx  = *(create_domains_ctx *) user_data;
	p4estBLG::DomainMap &dmap = *ctx.dmap;
	if (info->sides.elem_count == 2) {
		p4est_iter_face_side_t side_info1 = ((p4est_iter_face_side_t *) info->sides.array)[0];
		p4est_iter_face_side_t side_info2 = ((p4est_iter_face_side_t *) info->sides.array)[1];

		auto link_side_to_side
		= [&](p4est_iter_face_side_t side_info1, p4est_iter_face_side_t side_info2) {
			  Side<2> side = side_info1.face;
			  if (side_info1.is_hanging) {
				  // coarse nbr
				  int nbr_id, nbr_rank;
				  if (side_info2.is.full.is_ghost) {
					  int *data = (int *) &ctx.ghost_data[side_info2.is.full.quadid];
					  nbr_id    = data[0];
					  nbr_rank  = data[1];
				  } else {
					  nbr_id   = side_info2.is.full.quad->p.user_int;
					  nbr_rank = info->p4est->mpirank;
				  }
				  for (int i = 0; i < 2; i++) {
					  if (!side_info1.is.hanging.is_ghost[i]) {
						  int        id         = side_info1.is.hanging.quad[i]->p.user_int;
						  Domain<2> &d          = *dmap[id];
						  d.getNbrInfoPtr(side) = new CoarseNbrInfo<2>(nbr_id, i);
						  d.getCoarseNbrInfo(side).updateRank(nbr_rank);
					  }
				  }
			  } else if (side_info2.is_hanging) {
				  // fine nbr
				  if (!side_info1.is.full.is_ghost) {
					  int id = side_info1.is.full.quad->p.user_int;

					  std::array<int, 2> nbr_ids;
					  int                ranks[2];
					  for (int i = 0; i < 2; i++) {
						  if (side_info2.is.hanging.is_ghost[i]) {
							  int *data  = (int *) &ctx.ghost_data[side_info2.is.hanging.quadid[i]];
							  nbr_ids[i] = data[0];
							  ranks[i]   = data[1];
						  } else {
							  nbr_ids[i] = side_info2.is.hanging.quad[i]->p.user_int;
							  ranks[i]   = info->p4est->mpirank;
						  }
					  }
					  Domain<2> &d          = *dmap[id];
					  d.getNbrInfoPtr(side) = new FineNbrInfo<2>(nbr_ids);
					  d.getFineNbrInfo(side).updateRank(ranks[0], 0);
					  d.getFineNbrInfo(side).updateRank(ranks[1], 1);
				  }
			  } else {
				  // normal nbr
				  if (!side_info1.is.full.is_ghost) {
					  int id = side_info1.is.full.quad->p.user_int;
					  int nbr_id, nbr_rank;
					  if (side_info2.is.full.is_ghost) {
						  int *data = (int *) &ctx.ghost_data[side_info2.is.full.quadid];
						  nbr_id    = data[0];
						  nbr_rank  = data[1];
					  } else {
						  nbr_id   = side_info2.is.full.quad->p.user_int;
						  nbr_rank = info->p4est->mpirank;
					  }
					  Domain<2> &d          = *dmap[id];
					  d.getNbrInfoPtr(side) = new NormalNbrInfo<2>(nbr_id);
					  d.getNormalNbrInfo(side).updateRank(nbr_rank);
				  }
			  }
		  };

		// 1 to 2
		link_side_to_side(side_info1, side_info2);
		// 2 to 1
		link_side_to_side(side_info2, side_info1);
	}
}

struct coarsen_domains_ctx {
	int                  level;
	p4estBLG::DomainMap *dmap;
};
int coarsen(p4est_t *p4est, p4est_topidx_t which_tree, p4est_quadrant_t *quads[])
{
	coarsen_domains_ctx &ctx = *(coarsen_domains_ctx *) p4est->user_pointer;
	return quads[0]->level > ctx.level;
}

void coarsen_replace(p4est_t *p4est, p4est_topidx_t which_tree, int num_outgoing,
                     p4est_quadrant_t *outgoing[], int num_incoming, p4est_quadrant_t *incoming[])
{
	coarsen_domains_ctx &ctx  = *(coarsen_domains_ctx *) p4est->user_pointer;
	p4estBLG::DomainMap &dmap = *ctx.dmap;
	incoming[0]->p.user_int            = outgoing[0]->p.user_int;
	for (int i = 0; i < 4; i++) {
		Domain<2> &d    = *dmap[outgoing[i]->p.user_int];
		d.oct_on_parent = i;
		d.parent_id     = incoming[0]->p.user_int;
	}
}
void p4estBLG::extractLevel(p4est_t *p4est, int level, int n)
{
	// set ranks
	p4est_iterate_ext(p4est, nullptr, nullptr, set_ranks, nullptr, nullptr, 0);

	// get ids of ghosts
	p4est_ghost_t *ghost = p4est_ghost_new(p4est, P4EST_CONNECT_FACE);
	void *         ghost_data[ghost->ghosts.elem_count];
	p4est_ghost_exchange_data(p4est, ghost, ghost_data);

	create_domains_ctx ctx = {ghost_data, &levels[level]};
	p4est_iterate_ext(p4est, ghost, &ctx, create_domains, link_domains, nullptr, 0);

	for (auto p : levels[level]) {
		p.second->n = n;
		p.second->setPtrs(this->levels[level]);
	}
	if (level > 0) {
		coarsen_domains_ctx ctx = {level - 1, &levels[level]};
		p4est->user_pointer     = &ctx;
		p4est_coarsen_ext(p4est, false, true, coarsen, nullptr, coarsen_replace);
	for (auto p : levels[level]) {
        Domain<2> &d = *p.second;
        if(!d.hasCoarseParent()){
            d.parent_id=d.id;
        }
    }
        p4est_partition_ext(p4est,true,nullptr);
        extractLevel(p4est,level-1,n);
	}
}