########################################################################
# Compiler and external dependences
########################################################################
CC        = mpicc
F77       = mpif77
CXX       = mpicxx
F90       = mpif90
# HYPRE_DIR = /cm/shared/apps/hypre/gcc/64/2.10.1/
LAPACK_DIR = /Users/calhoun/lapack-3.5.0
HYPRE_DIR = /Users/calhoun/software/hypre/src/hypre


########################################################################
# Compiling and linking options
########################################################################
COPTS     = -g -Wall
CINCLUDES = -I$(HYPRE_DIR)/include -I$(LAPACK_DIR)/include
CDEFS     = -DHAVE_CONFIG_H -DHYPRE_TIMING
CFLAGS    = $(COPTS) $(CINCLUDES) $(CDEFS)
FOPTS     = -g
FINCLUDES = $(CINCLUDES)
FFLAGS    = $(FOPTS) $(FINCLUDES)
CXXOPTS   = $(COPTS) -Wno-deprecated
CXXINCLUDES = $(CINCLUDES) -I..
CXXDEFS   = $(CDEFS)
IFLAGS_BXX = -I../babel-runtime/sidl
CXXFLAGS  = $(CXXOPTS) $(CXXINCLUDES) $(CXXDEFS) $(IFLAGS_BXX)
IF90FLAGS = -I../babel/bHYPREClient-F90
F90FLAGS = $(FFLAGS) $(IF90FLAGS)


LINKOPTS  = $(COPTS)
LIBS      = -L$(HYPRE_DIR)/lib -lHYPRE -lm -L$(LAPACK_DIR)/lib -llapack -lblas
LFLAGS    = $(LINKOPTS) $(LIBS) -lstdc++
LFLAGS_B =\
 -L${HYPRE_DIR}/lib\
 -L${LAPACK_DIR}/lib\
 -lbHYPREClient-C\
 -lbHYPREClient-CX\
 -lbHYPREClient-F\
 -lbHYPRE\
 -lsidl -ldl -lxml2
LFLAGS77 = $(LFLAGS)
LFLAGS90 =

########################################################################
# Rules for compiling the source files
########################################################################
.SUFFIXES: .c .f .cxx .f90

.c.o:
	$(CC) $(CFLAGS) -c $<
.f.o:
	$(F77) $(FFLAGS) -c $<
.cxx.o:
	$(CXX) $(CXXFLAGS) -c $<

# This Fortran 90 build code is adapted from the sample in the Babel Users' Guide,
# which explains the separate preprocessing steps as a workaround to the 31-
# character name limit in Fortran 90.
.f90.o:
	$(CC) -E -traditional -P -o $*.tmp -x c $(IF90FLAGS) $<
	sed -e 's/^#pragma.*$$//' < $*.tmp > $*-pp.f90
	$(F90) -c -o $@ $(F90FLAGS) $*-pp.f90
	rm -f $*.tmp
# ... we can't delete the $*-pp.f90 files because they are the source code which
# debuggers need to look at

########################################################################
# List of all programs to be compiled
########################################################################
ALLPROGS = ex1 ex2 ex3 ex4 ex5 ex5f ex6 ex7 ex8 ex9 ex10 ex11 ex12 ex12f \
           ex13 ex14 ex15 ex16
64BITPROGS = ex5big ex15big
BABELPROGS = ex5b ex5b77 ex5bxx ex6b ex6b77
# ... ex5bp and ex5b90 are not in BABELPROGS because they require a
# software environment which many people haven't set up.
FORTRANPROGS = ex5f ex12f

all: $(ALLPROGS)

64bit: $(64BITPROGS)

babel: $(BABELPROGS)

fortran: $(FORTRANPROGS)

########################################################################
# Example 1
########################################################################
ex1: ex1.o
	$(CC) -o $@ $^ $(LFLAGS)

########################################################################
# Example 2
########################################################################
ex2: ex2.o
	$(CC) -o $@ $^ $(LFLAGS)

########################################################################
# Example 3
########################################################################
ex3: ex3.o
	$(CC) -o $@ $^ $(LFLAGS)

########################################################################
# Example 4
########################################################################
ex4: ex4.o
	$(CC) -o $@ $^ $(LFLAGS)

########################################################################
# Example 5
########################################################################
ex5: ex5.o
	$(CC) -o $@ $^ $(LFLAGS)

########################################################################
# Example 5 with 64-bit integers
########################################################################
ex5big: ex5big.o
	$(CC) -o $@ $^ $(LFLAGS)

########################################################################
# Example 5 Fortran 77
########################################################################
ex5f: ex5f.o
	$(F77) -o $@ $^ $(LFLAGS77)

########################################################################
# Example 5 Babel C
########################################################################
ex5b: ex5b.o
	$(CC) -o $@ $^ $(LFLAGS_B) $(LFLAGS)

########################################################################
# Example 5 Babel Fortran 77
########################################################################
ex5b77: ex5b77.o
	$(F77) -o $@ $^ $(LFLAGS_B) $(LFLAGS)

########################################################################
# Example 5 Babel Fortran 90
########################################################################
ex5b90: ex5b90.o
	$(F90) -o $@ $^ $(LFLAGS_B) $(LFLAGS) $(LFLAGS90)

########################################################################
# Example 5 Babel C++
########################################################################
ex5bxx: ex5bxx.o
	$(CXX) -o $@ $^ $(LFLAGS_B) $(LFLAGS)

########################################################################
# Example 6
########################################################################
ex6: ex6.o
	$(CC) -o $@ $^ $(LFLAGS)

########################################################################
# Example 6 Babel C
########################################################################
ex6b: ex6b.o
	$(CC) -o $@ $^ $(LFLAGS_B) $(LFLAGS)

########################################################################
# Example 6 Babel Fortran 77
########################################################################
ex6b77: ex6b77.o
	$(F77) -o $@ $^ $(LFLAGS_B) $(LFLAGS)

########################################################################
# Example 7
########################################################################
ex7: ex7.o
	$(CC) -o $@ $^ $(LFLAGS)

########################################################################
# Example 8
########################################################################
ex8: ex8.o
	$(CC) -o $@ $^ $(LFLAGS)

########################################################################
# Example 9
########################################################################
ex9: ex9.o
	$(CC) -o $@ $^ $(LFLAGS)

########################################################################
# Example 10
########################################################################
ex10: ex10.o
	$(CXX) -o $@ $^ $(LFLAGS)

########################################################################
# Example 11
########################################################################
ex11: ex11.o
	$(CC) -o $@ $^ $(LFLAGS)

########################################################################
# Example 12
########################################################################
ex12: ex12.o
	$(CC) -o $@ $^ $(LFLAGS)

########################################################################
# Example 12 Fortran 77
########################################################################
ex12f: ex12f.o
	$(F77) -o $@ $^ $(LFLAGS77)

########################################################################
# Example 13
########################################################################
ex13: ex13.o
	$(CC) -o $@ $^ $(LFLAGS)

########################################################################
# Example 14
########################################################################
ex14: ex14.o
	$(CC) -o $@ $^ $(LFLAGS)

########################################################################
# Example 15
########################################################################
ex15: ex15.o
	$(CC) -o $@ $^ $(LFLAGS)

########################################################################
# Example 15 with 64-bit integers
########################################################################
ex15big: ex15big.o
	$(CC) -o $@ $^ $(LFLAGS)

########################################################################
# Example 16
########################################################################
ex16: ex16.o
	$(CC) -o $@ $^ $(LFLAGS)

########################################################################
# Exact Solution
########################################################################
Exact_Solution: Exact_Solution.o
	$(CC) -o $@ $^ $(LFLAGS)

try2: try2.o
	$(CC) -o $@ $^ $(LFLAGS)

try2_varxy: try2_varxy.o
	$(CC) -o $@ $^ $(LFLAGS)

ParCSR.: ParCSR.o
	$(CC) -o $@ $^ $(LFLAGS)

ExactIJ: ExactIJ.o
	$(CC) -o $@ $^ $(LFLAGS)

ExactIJNeumann: ExactIJNeumann.o
	$(CC) -o $@ $^ $(LFLAGS)

UnstructNeumann: UnstructNeumann.o
	$(CC) -o $@ $^ $(LFLAGS)

twopartsoneproc: twopartsoneproc.o
	$(CC) -o $@ $^ $(LFLAGS)

multipart: multipart.o
	$(CC) -o $@ $^ $(LFLAGS)

americagreat: americagreat.o
	$(CC) -o $@ $^ $(LFLAGS)

two_parts_ss: two_parts_ss.o
	$(CC) -o $@ $^ $(LFLAGS)


########################################################################
# Clean up
########################################################################
clean:
	rm -f $(ALLPROGS:=.o)
	rm -f $(64BITPROGS:=.o)
	rm -f $(BABELPROGS:=.o)
	rm -f $(FORTRANPROGS:=.o)
	cd vis; make clean
distclean: clean
	rm -f $(ALLPROGS) $(ALLPROGS:=*~)
	rm -f $(64BITPROGS) $(64BITPROGS:=*~)
	rm -f $(BABELPROGS) $(BABELPROGS:=*~)
	rm -f $(FORTRANLPROGS) $(FORTRANPROGS:=*~)
	rm -rf $(BABELPROGS:=*-pp.f90)
