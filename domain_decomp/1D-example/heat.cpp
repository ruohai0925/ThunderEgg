#include <cmath>
#include <iostream>
#include <limits>
#include <valarray>
#include <vector>
#define PI M_PI

using namespace std;
#include "Domain.h"
#include "TriDiagSolver.h"

double uxx_init(double x) { return -PI * PI * sin(PI * x); }
double exact_solution(double x) { return sin(PI * x); }
double error(vector<Domain *> &dmns)
{
	double l2norm     = 0;
	double exact_norm = 0;
	for (Domain *d_ptr : dmns) {
		int    m       = d_ptr->size();
		double d_begin = d_ptr->domainBegin();
		double d_end   = d_ptr->domainEnd();
		for (int i = 0; i < m; i++) {
			double x     = d_begin + (i + 0.5) / m * (d_end - d_begin);
			double exact = exact_solution(x);
			double diff  = exact - d_ptr->u_curr[i];
			l2norm += diff * diff;
			exact_norm += exact * exact;
		}
	}
	return sqrt(l2norm) / sqrt(exact_norm);
}

/**
 * @brief solve over each of the domains
 *
 * @param tds the solver to use
 * @param dmns the domains to over
 * @param gammas the gamma values that are used
 *
 * @return 
 */
valarray<double> solveOnAllDomains(TriDiagSolver &tds, vector<Domain *> &dmns,
                                   valarray<double> &gammas)
{
	// solve over the domains
	for (Domain *d_ptr : dmns) {
		tds.solve(*d_ptr);
	}

	// get the difference between the gamma value and computed solution at the interface
	valarray<double> z(gammas.size());
	for (size_t i = 0; i < gammas.size(); i++) {
		Domain *left_dmn_ptr  = dmns[i];
		Domain *right_dmn_ptr = dmns[i + 1];

		double left_val  = left_dmn_ptr->u_curr[left_dmn_ptr->u_curr.size() - 1];
		double right_val = right_dmn_ptr->u_curr[0];

		z[i] = left_val + right_val + 2 * gammas[i];
	}
	return z;
}

void printSolution(vector<Domain *> &dmns)
{
	for (Domain *d_ptr : dmns) {
		for (double x : d_ptr->u_curr) {
			cout << x << "\t";
		}
	}
	cout << '\n';
}

int main(int argc, char *argv[])
{
	// set cout to print full precision
	// cout.precision(numeric_limits<double>::max_digits10);
	cout.precision(9);

	// create a solver with 0 for the boundary conditions
	TriDiagSolver    tds(0.0, 0.0);
	int              m           = stoi(argv[1]);
	int              num_domains = stoi(argv[2]);
	vector<Domain *> dmns(num_domains);

	// create the domains
	for (int i = 0; i < num_domains; i++) {
		double x_start = (0.0 + i) / num_domains;
		double x_end   = (1.0 + i) / num_domains;
		dmns[i]        = new Domain(x_start, x_end, m / num_domains, uxx_init);
	}

	// create an array to store the gamma values for each of the interfaces
	valarray<double> gammas(num_domains - 1);

	// set the gamma pointers
	if (num_domains > 1) {
		const int last_i         = num_domains - 1;
		dmns[0]->right_gamma_ptr = &gammas[0];
		for (int i = 1; i < last_i; i++) {
			dmns[i]->left_gamma_ptr  = &gammas[i - 1];
			dmns[i]->right_gamma_ptr = &gammas[i];
		}
		dmns[last_i]->left_gamma_ptr = &gammas[last_i - 1];
	}

	/*
	 * solve with gammas set to 0
	 */
	gammas             = 0;
	valarray<double> b = solveOnAllDomains(tds, dmns, gammas);

	// print out solution
	printSolution(dmns);
	cout << '\n';

	cout << "b value(s):\n";
	for (double x : b) {
		cout << x << ' ';
	}
	cout << "\n\n";

	/*
	 * solve with gammas set to 1
	 */
	gammas             = 1;
	valarray<double> a = solveOnAllDomains(tds, dmns, gammas) - b;

	// print out solution
	printSolution(dmns);
	cout << '\n';

	cout << "a value(s):\n";
	for (double x : a) {
		cout << x << ' ';
	}
	cout << "\n\n";

	/*
	 * calculate the gamma values, and solve with that
	 */
	gammas = -b / a;
	solveOnAllDomains(tds, dmns, gammas);

	cout << "calculated gamma value(s):\n";
	for (double x : gammas) {
		cout << x << ' ';
	}
	cout << "\n\n";

	// print out solution
	printSolution(dmns);
	cout << '\n';

	cerr << '\n';
	cerr.precision(3);
	cerr << "error: " << scientific << error(dmns) << "\n";

	// delete the domains
	for (Domain *d_ptr : dmns) {
		delete d_ptr;
	}
}
