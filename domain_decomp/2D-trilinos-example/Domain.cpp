#include "Domain.h"
using namespace std;
Domain::Domain(DomainSignature ds, int nx, int ny, double h_x, double h_y)
{
	this->nx  = nx;
	this->ny  = ny;
	this->h_x = h_x / ds.refine_level;
	this->h_y = h_y / ds.refine_level;

#if NDEBUG
	cerr << "I am Domain: " << ds.id << "\n";
	cerr << "h:           " << this->h_x << endl;
	cerr << "I start at:  " << ds.x_start << ", " << ds.y_start << "\n";
	cerr << "Length:     " << ds.x_length << ", " << ds.y_length << "\n";
	cerr << "North: " << ds.nbr(Side::north) << ", " << ds.nbrRight(Side::north) << "\n";
	cerr << "Idx:   " << ds.index(Side::north) << ", " << ds.indexCenter(Side::north) << "\n";
	cerr << "East:  " << ds.nbr(Side::east) << ", " << ds.nbrRight(Side::east) << "\n";
	cerr << "Idx:   " << ds.index(Side::east) << ", " << ds.indexCenter(Side::east) << "\n";
	cerr << "South: " << ds.nbr(Side::south) << ", " << ds.nbrRight(Side::south) << "\n";
	cerr << "Idx:   " << ds.index(Side::south) << ", " << ds.indexCenter(Side::south) << "\n";
	cerr << "West:  " << ds.nbr(Side::west) << ", " << ds.nbrRight(Side::west) << "\n";
	cerr << "Idx:   " << ds.index(Side::west) << ", " << ds.indexCenter(Side::west) << "\n";
	cerr << "\n";
#endif
	f      = valarray<double>(nx * ny);
	f_copy = valarray<double>(nx * ny);
	exact  = valarray<double>(nx * ny);
	tmp    = valarray<double>(nx * ny);
	u      = valarray<double>(nx * ny);
	denom  = valarray<double>(nx * ny);

	boundary_north = valarray<double>(nx);
	boundary_south = valarray<double>(nx);
	boundary_east  = valarray<double>(ny);
	boundary_west  = valarray<double>(ny);

	x_start  = ds.x_start;
	y_start  = ds.y_start;
	x_length = ds.x_length;
	y_length = ds.y_length;

	this->ds = ds;
}

Domain::~Domain()
{
	fftw_destroy_plan(plan1);
	fftw_destroy_plan(plan2);
#if NDEBUG
	cerr << "I am Domain: " << ds.id << "\n";
	cerr << "h:           " << this->h_x << endl;
	cerr << "I start at:  " << ds.x_start << ", " << ds.y_start << "\n";
	cerr << "Length:     " << ds.x_length << ", " << ds.y_length << "\n";
	cerr << "North: " << ds.nbr(Side::north) << ", " << ds.nbrRight(Side::north) << "\n";
	cerr << "Idx:   " << index(Side::north) << ", " << indexCenter(Side::north) << "\n";
	cerr << "Flags: " << hasFineNbr(Side::north) << ", " << hasCoarseNbr(Side::north) << ", "
	     << isCoarseLeft(Side::north) << endl;
	cerr << "East:  " << ds.nbr(Side::east) << ", " << ds.nbrRight(Side::east) << "\n";
	cerr << "Idx:   " << index(Side::east) << ", " << indexCenter(Side::east) << "\n";
	cerr << "Flags: " << hasFineNbr(Side::east) << ", " << hasCoarseNbr(Side::east) << ", "
	     << isCoarseLeft(Side::east) << endl;
	cerr << "South: " << ds.nbr(Side::south) << ", " << ds.nbrRight(Side::south) << "\n";
	cerr << "Idx:   " << index(Side::south) << ", " << indexCenter(Side::south) << "\n";
	cerr << "Flags: " << hasFineNbr(Side::south) << ", " << hasCoarseNbr(Side::south) << ", "
	     << isCoarseLeft(Side::south) << endl;
	cerr << "West:  " << ds.nbr(Side::west) << ", " << ds.nbrRight(Side::west) << "\n";
	cerr << "Idx:   " << index(Side::west) << ", " << indexCenter(Side::west) << "\n";
	cerr << "Flags: " << hasFineNbr(Side::west) << ", " << hasCoarseNbr(Side::west) << ", "
	     << isCoarseLeft(Side::west) << endl;
	cerr << "\n";
#endif
}

void Domain::planDirichlet()
{
	// create plan
	plan1 = fftw_plan_r2r_2d(ny, nx, &f_copy[0], &tmp[0], FFTW_RODFT10, FFTW_RODFT10, FFTW_MEASURE);

	plan2 = fftw_plan_r2r_2d(ny, nx, &tmp[0], &u[0], FFTW_RODFT01, FFTW_RODFT01, FFTW_MEASURE);
	// create denom vector

	for (int yi = 0; yi < nx; yi++) {
		denom[slice(yi * ny, nx, 1)] = -4 / (h_x * h_x) * pow(sin((yi + 1) * M_PI / (2 * nx)), 2);
	}

	valarray<double> ones(ny);
	ones = 1;
	for (int xi = 0; xi < ny; xi++) {
		denom[slice(xi, ny, nx)]
		-= 4 / (h_y * h_y) * pow(sin((xi + 1) * M_PI / (2 * ny)), 2) * ones;
	};

}

void Domain::planNeumann()
{
	neumann                       = true;
	fftw_r2r_kind x_transform     = FFTW_RODFT10;
	fftw_r2r_kind x_transform_inv = FFTW_RODFT01;
	fftw_r2r_kind y_transform     = FFTW_RODFT10;
	fftw_r2r_kind y_transform_inv = FFTW_RODFT01;
	if (!hasNbr(Side::east) && !hasNbr(Side::west)) {
		x_transform     = FFTW_REDFT10;
		x_transform_inv = FFTW_REDFT01;
	} else if (!hasNbr(Side::west)) {
		x_transform     = FFTW_REDFT11;
		x_transform_inv = FFTW_REDFT11;
	} else if (!hasNbr(Side::east)) {
		x_transform     = FFTW_RODFT11;
		x_transform_inv = FFTW_RODFT11;
	}
	if (!hasNbr(Side::north) && !hasNbr(Side::south)) {
		y_transform     = FFTW_REDFT10;
		y_transform_inv = FFTW_REDFT01;
	} else if (!hasNbr(Side::south)) {
		y_transform     = FFTW_REDFT11;
		y_transform_inv = FFTW_REDFT11;
	} else if (!hasNbr(Side::north)) {
		y_transform     = FFTW_RODFT11;
		y_transform_inv = FFTW_RODFT11;
	}
	plan1 = fftw_plan_r2r_2d(ny, nx, &f_copy[0], &tmp[0], y_transform, x_transform, FFTW_MEASURE);

	plan2
	= fftw_plan_r2r_2d(ny, nx, &tmp[0], &u[0], y_transform_inv, x_transform_inv, FFTW_MEASURE);

	// create denom vector
	if (!hasNbr(Side::north) && !hasNbr(Side::south)) {
		for (int yi = 0; yi < nx; yi++) {
			denom[slice(yi * ny, nx, 1)] = -4 / (h_x * h_x) * pow(sin(yi * M_PI / (2 * nx)), 2);
		}
	} else if (!hasNbr(Side::south) || !hasNbr(Side::north)) {
		for (int yi = 0; yi < nx; yi++) {
			denom[slice(yi * ny, nx, 1)]
			= -4 / (h_x * h_x) * pow(sin((yi + 0.5) * M_PI / (2 * nx)), 2);
		}
	} else {
		for (int yi = 0; yi < nx; yi++) {
			denom[slice(yi * ny, nx, 1)]
			= -4 / (h_x * h_x) * pow(sin((yi + 1) * M_PI / (2 * nx)), 2);
		}
	}

	valarray<double> ones(ny);
	ones = 1;

	if (!hasNbr(Side::east) && !hasNbr(Side::west)) {
		for (int xi = 0; xi < ny; xi++) {
			denom[slice(xi, ny, nx)] -= 4 / (h_y * h_y) * pow(sin(xi * M_PI / (2 * ny)), 2) * ones;
		}
	} else if (!hasNbr(Side::west) || !hasNbr(Side::east)) {
		for (int xi = 0; xi < ny; xi++) {
			denom[slice(xi, ny, nx)]
			-= 4 / (h_y * h_y) * pow(sin((xi + 0.5) * M_PI / (2 * ny)), 2) * ones;
		}
	} else {
		for (int xi = 0; xi < ny; xi++) {
			denom[slice(xi, ny, nx)]
			-= 4 / (h_y * h_y) * pow(sin((xi + 1) * M_PI / (2 * ny)), 2) * ones;
		}
	}
}

void Domain::solve()
{
	f_copy = f;
	if (!hasNbr(Side::north) && neumann) {
		f_copy[slice(nx * (ny - 1), nx, 1)] -= 1 / h_y * boundary_north;
	} else {
		f_copy[slice(nx * (ny - 1), nx, 1)] -= 2 / (h_y * h_y) * boundary_north;
	}
	if (!hasNbr(Side::east) && neumann) {
		f_copy[slice((nx - 1), ny, nx)] -= 1 / h_x * boundary_east;
	} else {
		f_copy[slice((nx - 1), ny, nx)] -= 2 / (h_x * h_x) * boundary_east;
	}
	if (!hasNbr(Side::south) && neumann) {
		f_copy[slice(0, nx, 1)] += 1 / h_y * boundary_south;
	} else {
		f_copy[slice(0, nx, 1)] -= 2 / (h_y * h_y) * boundary_south;
	}
	if (!hasNbr(Side::west) && neumann) {
		f_copy[slice(0, ny, nx)] += 1 / h_x * boundary_west;
	} else {
		f_copy[slice(0, ny, nx)] -= 2 / (h_x * h_x) * boundary_west;
	}

	fftw_execute(plan1);

	tmp /= denom;

	if (neumann
	    && !(hasNbr(Side::north) || hasNbr(Side::east) || hasNbr(Side::south)
	         || hasNbr(Side::west))) {
		tmp[0] = 0;
	}

	fftw_execute(plan2);

	u /= 4 * nx * ny;
}

void Domain::putGhostCells(vector_type &ghost)
{
	auto left_ptr  = ghost.getVectorNonConst(0);
	auto right_ptr = ghost.getVectorNonConst(1);
	if (hasNbr(Side::north)) {
		fillDiffVector(Side::north, *right_ptr, true);
	}
	if (hasNbr(Side::east)) {
		fillDiffVector(Side::east, *right_ptr, true);
	}
	if (hasNbr(Side::south)) {
		fillDiffVector(Side::south, *left_ptr, true);
	};
	if (hasNbr(Side::west)) {
		fillDiffVector(Side::west, *left_ptr, true);
	}
}

double Domain::residual(vector_type &ghost)
{
	auto left_ptr  = ghost.getVector(0);
	auto right_ptr = ghost.getVector(1);

	/*
	if (hasNbr(Side::north)) {
	    fillBoundary(Side::north, *left_ptr);
	}
	if (hasNbr(Side::east)) {
	    fillBoundary(Side::east, *left_ptr);
	}
	if (hasNbr(Side::south)) {
	    fillBoundary(Side::south, *right_ptr);
	}
	if (hasNbr(Side::west)) {
	    fillBoundary(Side::west, *right_ptr);
	}
	*/

	valarray<double> f_comp = valarray<double>(nx * ny);
	// integrate in x secton
	double center, north, east, south, west;
	// west
	for (int j = 0; j < ny; j++) {
		west   = boundary_west[j];
		center = u[j * nx];
		east   = u[j * nx + 1];
		if (neumann && !hasNbr(Side::west)) {
			f_comp[j * nx] = (-h_x * west - center + east) / (h_x * h_x);
		} else {
			f_comp[j * nx] = (2 * west - 3 * center + east) / (h_x * h_x);
		}
		/*
		} else if (!hasNbr(Side::west)) {
		} else if (hasFineNbr(Side::west)) {
		    // TODO
		    // f_comp[j * nx] = (1.0 / 3.0 * west - 4.0 / 3.0 * center + east) / (h_x * h_x);
		} else if (hasCoarseNbr(Side::west)) {
		    // TODO
		    f_comp[j * nx] = (2.6118 * west - 3.6105 * center + east) / (h_x * h_x);
		    cerr << west << ", " << center << ", " << east << ", " << f[j*nx]*h_x*h_x << ", "
		<<f_comp[j*nx]*h_x*h_x << endl;
		    f_comp[j * nx] = (west - 2 * center + east) / (h_x * h_x);
		}
		*/
	}
	// middle
	for (int i = 1; i < nx - 1; i++) {
		for (int j = 0; j < ny; j++) {
			east   = u[j * nx + i - 1];
			center = u[j * nx + i];
			west   = u[j * nx + i + 1];

			f_comp[j * nx + i] = (west - 2 * center + east) / (h_x * h_x);
		}
	}
	// east
	for (int j = 0; j < ny; j++) {
		west   = u[j * nx + nx - 2];
		center = u[j * nx + nx - 1];
		east   = boundary_east[j];
		if (neumann && !hasNbr(Side::east)) {
			f_comp[j * nx + nx - 1] = (west - center + h_x * east) / (h_x * h_x);
		} else {
			f_comp[j * nx + nx - 1] = (west - 3 * center + 2 * east) / (h_x * h_x);
        }
        /*
		} else if (!hasNbr(Side::east)) {
		} else if (hasFineNbr(Side::east)) {
			f_comp[j * nx + nx - 1] = (west - 7.0 / 3.0 * center + 4.0 / 3.0 * east) / (h_x * h_x);
		} else if (hasCoarseNbr(Side::east)) {
			// TODO
			// f_comp[j * nx + nx - 1] = (west - 4.0 / 3.0 * center + 1.0 / 3.0 * east) / (h_x *
			// h_x);
			f_comp[j * nx + nx - 1] = (west - 2 * center + east) / (h_x * h_x);
		}
        */
	}
	// south
	for (int i = 0; i < nx; i++) {
		south  = boundary_south[i];
		center = u[i];
		north  = u[nx + i];
		if (neumann && !hasNbr(Side::south)) {
			f_comp[i] += (-h_y * south - center + north) / (h_y * h_y);
		} else {
			f_comp[i] += (2 * south - 3 * center + north) / (h_y * h_y);
        }
        /*
		} else if (!hasNbr(Side::south)) {
			f_comp[i] += (south - 2 * center + north) / (h_y * h_y);
		}
        */
	}
	// middle
	for (int i = 0; i < nx; i++) {
		for (int j = 1; j < ny - 1; j++) {
			south  = u[(j - 1) * nx + i];
			center = u[j * nx + i];
			north  = u[(j + 1) * nx + i];

			f_comp[j * nx + i] += (south - 2 * center + north) / (h_y * h_y);
		}
	}
	// north
	for (int i = 0; i < nx; i++) {
		south  = u[(ny - 2) * nx + i];
		center = u[(ny - 1) * nx + i];
		north  = boundary_north[i];
		if (neumann && !hasNbr(Side::north)) {
			f_comp[(ny - 1) * nx + i] += (south - center + h_y * north) / (h_y * h_y);
		} else {
			f_comp[(ny - 1) * nx + i] += (south - 3 * center + 2 * north) / (h_y * h_y);
        }
        /*
		} else if (!hasNbr(Side::north)) {
			f_comp[(ny - 1) * nx + i] += (south - 2 * center + north) / (h_y * h_y);
		}
        */
	}
	//if (hasFineNbr(Side::east)) {
	//	for (int j = 0; j < ny; j++) {
     //       f_comp[j*nx+nx-1]=f[j*nx+nx-1];
	//	}
	//}
	resid = f - f_comp;
	return sqrt(pow(f - f_comp, 2).sum());
}

void Domain::solveWithInterface(const vector_type &gamma, vector_type &diff)
{
	auto vec_ptr  = gamma.getVector(0);
	auto diff_ptr = diff.getVectorNonConst(0);

	for (Side s = Side::north; s <= Side::west; s++) {
		if (hasNbr(s)) {
			fillBoundary(s, *vec_ptr);
		}
	}

	// solve
	solve();

	for (Side s = Side::north; s <= Side::west; s++) {
		if (hasNbr(s)) {
			fillDiffVector(s, *diff_ptr);
		}
	}
}
double Domain::diffNorm()
{
	error = exact - u;
	return sqrt(pow(exact - u, 2).sum());
}
double Domain::diffNorm(double uavg, double eavg)
{
	error = exact - u - eavg + uavg;
	return sqrt(pow(exact - u - eavg + uavg, 2).sum());
}
double Domain::uSum() { return u.sum(); }
double Domain::exactNorm() { return sqrt((exact * exact).sum()); }
double Domain::fNorm() { return sqrt((f * f).sum()); }
double Domain::exactNorm(double eavg) { return sqrt(pow(exact - eavg, 2).sum()); }
double                          Domain::exactSum() { return exact.sum(); }
valarray<double> Domain::getSide(Side s)
{
	valarray<double> retval(nx);
	switch (s) {
		case Side::north:
			retval = u[slice(nx * (nx - 1), nx, 1)];
			break;
		case Side::east:
			retval = u[slice(nx - 1, nx, nx)];
			break;
		case Side::south:
			retval = u[slice(0, nx, 1)];
			break;
		case Side::west:
			retval = u[slice(0, nx, nx)];
	}
	return retval;
}
valarray<double> Domain::getSideFine(Side s)
{
	valarray<double> retval(nx);
	valarray<double> side = getSide(s);
	// TODO this probably doesn't work
	if (leftToRight(s) != isCoarseLeft(s)) {
		for (int i = 0; i < nx; i++) {
			retval[nx - 1 - i / 2] += side[nx - 1 - i];
		}
	} else {
		for (int i = 0; i < nx; i++) {
			retval[i / 2] += side[i];
		}
	}
	retval /= 2;
	return retval;
}
valarray<double> Domain::getStencil(Side s,Tilt t)
{
		return getSide(s);
}

void Domain::fillBoundary(Side s, const single_vector_type &gamma)
{
	int               curr_i     = index(s) * nx;
	valarray<double> &boundary   = *getBoundaryPtr(s);
	auto              gamma_view = gamma.getLocalView<Kokkos::HostSpace>();

	for (int i = 0; i < nx; i++) {
		boundary[i] = gamma_view(curr_i, 0);
		curr_i++;
	}
}
void Domain::fillDiffVector(Side s, single_vector_type &diff, bool residual)
{
	// weight=false;
	auto diff_view = diff.getLocalView<Kokkos::HostSpace>();
	if (hasFineNbr(s)) {
		valarray<double> side = getSide(s);
		side *= 2.0 / 3.0;
		int curr_i = index(s) * nx;
		for (int i = 0; i < nx; i++) {
			diff_view(curr_i, 0) += side[i];
			curr_i++;
		}
	} else if (hasCoarseNbr(s)) {
		valarray<double> center = getSideFine(s);
		valarray<double> side   = getSide(s);
		if (!residual) {
			center *= 4.0 / 3.0;
			side *= 2.0;
		}
		int curr_i        = index(s) * nx;
		int curr_i_center = indexCenter(s) * nx;
		for (int i = 0; i < nx; i++) {
			diff_view(curr_i_center, 0) += center[i];
			diff_view(curr_i, 0) += side[i];
			curr_i_center++;
			curr_i++;
		}
	} else {
		valarray<double> side   = getSide(s);
		int              curr_i = index(s) * nx;
		for (int i = 0; i < nx; i++) {
			diff_view(curr_i, 0) += side[i];
			curr_i++;
		}
	}
}
