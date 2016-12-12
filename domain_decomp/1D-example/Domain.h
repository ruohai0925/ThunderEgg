#ifndef DOMAIN_H_SEEN
#define DOMAIN_H_SEEN
#include <valarray>
#include <string>
/**
 * @brief This class is an object that represents a domain.
 */
class Domain
{
	private:
	/**
	 * @brief the beginning of this domain
	 */
	double domain_begin;

	/**
	 * @brief the end of this domain
	 */
	double domain_end;

	/**
	 * @brief the spacing between the points
	 */
	double h;

	/**
	 * @brief the number of points on this grid
	 */
	int m;

	/**
	 * @brief point to the Domain that is left of this one
	 */
	Domain *left_nbr_ptr = nullptr;

	/**
	 * @brief point to the Domain that is right of this one
	 */
	Domain *right_nbr_ptr = nullptr;

	public:
	/**
	 * @brief the values that we are integrating over
	 */
	std::valarray<double> u_xx;

	/**
	 * @brief the solution on this domain
	 */
	std::valarray<double> u_curr;

	/**
	 * @brief the solution on this domain, from the previous iteration
	 */
	std::valarray<double> u_prev;

	/**
	 * @brief Create a new Domain object, and initialize the values
	 *
	 * @param d_begin the beginning of the domain
	 * @param d_end the end of the domain
	 * @param m the number of points on the domain
	 * @param f is the function that is used to initialize the domain over x,
	 * the function must have the form double f(double x)
	 */
	Domain(double d_begin, double d_end, int m, double f(double x));

	/**
	 * @return the spacing between grid points
	 */
	double spaceDelta();

	/**
	 * @return begining of the domain
	 */
	double domainBegin();

	/**
	 * @return end of the domain
	 */
	double domainEnd();

	/**
	 * @return the number of grid points
	 */
	int size();

	/**
	 * @return whether or not this grid has a neighbor to it's left
	 */
	bool hasLeftNbr();

	/**
	 * @return whether or not this grid has a neighbor to it's right
	 */
	bool hasRightNbr();

	/**
	 * @brief set the neighbor to this grid's left
	 *
	 * @param grid the grid that is to the left of this one
	 */
	void setLeftNbr(Domain &grid);

	/**
	 * @brief set the neighbor to this grid's right
	 *
	 * @param grid the grid that is to the right of this one
	 */
	void setRightNbr(Domain &grid);

	/**
	 * @brief swap the pointer of to the u_curr vector and the u_prev vector
	 */
	void swapCurrPrev();

	/**
	 * @brief Return the domain that is to the left of this one
	 *
	 * @return a reference to the Domain
	 */
	Domain &leftNbr();

	/**
	 * @brief Return the domain that is to the right of this one
	 *
	 * @return a reference to the domain
	 */
	Domain &rightNbr();
};

#endif
