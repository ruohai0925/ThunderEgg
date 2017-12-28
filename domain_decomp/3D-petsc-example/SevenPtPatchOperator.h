#ifndef SEVENPTPATCHOPERATOR_H
#define SEVENPTPATCHOPERATOR_H
#include "PatchOperator.h"
inline int index(const int &n, const int &xi, const int &yi, const int &zi)
{
	return xi + yi * n + zi * n * n;
}
class SevenPtPatchOperator : public PatchOperator
{
	public:
	void apply(Domain &d, const Vec u, const Vec gamma, Vec f)
	{
		int     n     = d.n;
		double  h_x   = d.x_length / n;
		double  h_y   = d.y_length / n;
		int     start = n * n * d.id_local;
		double *u_view, *f_view, *gamma_view;
		VecGetArray(u, &u_view);
		VecGetArray(f, &f_view);
		double *f_ptr = f_view + start;
		double *u_ptr = u_view + start;
		VecGetArray(gamma, &gamma_view);
		// TODO fix interfaces
		const double *boundary_north = nullptr;
		if (d.hasNbr(Side::north)) {
			boundary_north = &gamma_view[d.n * d.index(Side::north)];
		}
		const double *boundary_east = nullptr;
		if (d.hasNbr(Side::east)) {
			boundary_east = &gamma_view[d.n * d.index(Side::east)];
		}
		const double *boundary_south = nullptr;
		if (d.hasNbr(Side::south)) {
			boundary_south = &gamma_view[d.n * d.index(Side::south)];
		}
		const double *boundary_west = nullptr;
		if (d.hasNbr(Side::west)) {
			boundary_west = &gamma_view[d.n * d.index(Side::west)];
		}

		double center, north, east, south, west, bottom, top;

		// derive in x direction
		// west
		for (int zi = 0; zi < n; zi++) {
			for (int yi = 0; yi < n; yi++) {
				west = 0;
				if (boundary_west != nullptr) {
					// west = boundary_west[j];
				}
				center = u_ptr[index(n, 0, yi, zi)];
				east   = u_ptr[index(n, 1, yi, zi)];
				if (d.isNeumann(Side::west) && !d.hasNbr(Side::west)) {
					f_ptr[index(n, 0, yi, zi)] = (-h_x * west - center + east) / (h_x * h_x);
				} else {
					f_ptr[index(n, 0, yi, zi)] = (2 * west - 3 * center + east) / (h_x * h_x);
				}
			}
		}
		// middle
		for (int zi = 0; zi < n; zi++) {
			for (int yi = 0; yi < n; yi++) {
				for (int xi = 1; xi < n - 1; xi++) {
					west   = u_ptr[index(n, xi - 1, yi, zi)];
					center = u_ptr[index(n, xi, yi, zi)];
					east   = u_ptr[index(n, xi + 1, yi, zi)];

					f_ptr[index(n, xi, yi, zi)] = (west - 2 * center + east) / (h_x * h_x);
				}
			}
		}
		// east
		for (int zi = 0; zi < n; zi++) {
			for (int yi = 0; yi < n; yi++) {
				west   = u_ptr[index(n, n - 2, yi, zi)];
				center = u_ptr[index(n, n - 1, yi, zi)];
				east   = 0;
				if (boundary_east != nullptr) {
					// east = boundary_east[j];
				}
				if (d.isNeumann(Side::east) && !d.hasNbr(Side::east)) {
					f_ptr[index(n, n - 1, yi, zi)] = (west - center + h_x * east) / (h_x * h_x);
				} else {
					f_ptr[index(n, n - 1, yi, zi)] = (west - 3 * center + 2 * east) / (h_x * h_x);
				}
			}
		}

		// derive in y direction
		// south
		for (int zi = 0; zi < n; zi++) {
			for (int xi = 0; xi < n; xi++) {
				south = 0;
				if (boundary_south != nullptr) {
					// south = boundary_south[i];
				}
				center = u_ptr[index(n, xi, 0, zi)];
				north  = u_ptr[index(n, xi, 1, zi)];
				if (d.isNeumann(Side::south) && !d.hasNbr(Side::south)) {
					f_ptr[index(n, xi, 0, zi)] += (-h_y * south - center + north) / (h_y * h_y);
				} else {
					f_ptr[index(n, xi, 0, zi)] += (2 * south - 3 * center + north) / (h_y * h_y);
				}
			}
		}
		// middle
		for (int zi = 0; zi < n; zi++) {
			for (int yi = 1; yi < n - 1; yi++) {
				for (int xi = 0; xi < n; xi++) {
					south  = u_ptr[index(n, xi, yi - 1, zi)];
					center = u_ptr[index(n, xi, yi, zi)];
					north  = u_ptr[index(n, xi, yi + 1, zi)];

					f_ptr[index(n, xi, yi, zi)] += (south - 2 * center + north) / (h_y * h_y);
				}
			}
		}
		// north
		for (int zi = 0; zi < n; zi++) {
			for (int xi = 0; xi < n; xi++) {
				south  = u_ptr[index(n, xi, n - 2, zi)];
				center = u_ptr[index(n, xi, n - 1, zi)];
				north  = 0;
				if (boundary_north != nullptr) {
					// north = boundary_north[i];
				}
				if (d.isNeumann(Side::north) && !d.hasNbr(Side::north)) {
					f_ptr[index(n, xi, n - 1, zi)] += (south - center + h_y * north) / (h_y * h_y);
				} else {
					f_ptr[index(n, xi, n - 1, zi)]
					+= (south - 3 * center + 2 * north) / (h_y * h_y);
				}
			}
		}

		// derive in z direction
		// bottom
		for (int yi = 0; yi < n; yi++) {
			for (int xi = 0; xi < n; xi++) {
				bottom = 0;
				if (boundary_south != nullptr) {
					// south = boundary_south[i];
				}
				center = u_ptr[index(n, xi, yi, 0)];
				top    = u_ptr[index(n, xi, yi, 1)];
				if (d.isNeumann(Side::bottom) && !d.hasNbr(Side::bottom)) {
					f_ptr[index(n, xi, yi, 0)] += (-h_y * bottom - center + top) / (h_y * h_y);
				} else {
					f_ptr[index(n, xi, yi, 0)] += (2 * bottom - 3 * center + top) / (h_y * h_y);
				}
			}
		}
		// middle
		for (int zi = 1; zi < n - 1; zi++) {
			for (int yi = 0; yi < n; yi++) {
				for (int xi = 0; xi < n; xi++) {
					bottom = u_ptr[index(n, xi, yi, zi - 1)];
					center = u_ptr[index(n, xi, yi, zi)];
					top    = u_ptr[index(n, xi, yi, zi + 1)];

					f_ptr[index(n, xi, yi, zi)] += (bottom - 2 * center + top) / (h_y * h_y);
				}
			}
		}
		// top
		for (int yi = 0; yi < n; yi++) {
			for (int xi = 0; xi < n; xi++) {
				bottom = u_ptr[index(n, xi, yi, n - 2)];
				center = u_ptr[index(n, xi, yi, n - 1)];
				top    = 0;
				if (boundary_north != nullptr) {
					// north = boundary_north[i];
				}
				if (d.isNeumann(Side::top) && !d.hasNbr(Side::top)) {
					f_ptr[index(n, xi, yi, n - 1)] += (bottom - center + h_y * top) / (h_y * h_y);
				} else {
					f_ptr[index(n, xi, yi, n - 1)] += (bottom - 3 * center + 2 * top) / (h_y * h_y);
				}
			}
		}
		VecRestoreArray(gamma, &gamma_view);
		VecRestoreArray(u, &u_view);
		VecRestoreArray(f, &f_view);
	}
};
#endif