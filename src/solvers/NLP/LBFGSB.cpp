#include <iostream>
#include <vector>
#include <cmath>
#include <map>
#include <string.h>
#include "LBFGSB.hpp"
#include "Utils.hpp"

// fortran interface to L-BFGS-B
extern "C" {

    void setulb_(int *n, int *m, double *x, double *l, double *u, int *nbd, double *f, double *g,
            double *factr, double *pgtol, double *wa, int *iwa, char *task, int *iprint,
            char *csave, int *lsave, int *isave, double *dsave);
}

LBFGSB::LBFGSB(int limited_memory_size): rho(200.), limited_memory_size(limited_memory_size) {
}

// TODO remove
void LBFGSB::initialize(std::map<int,int> slacked_constraints) {
    this->slacked_constraints_ = slacked_constraints;
    return;
}

LocalSolution LBFGSB::solve(Problem& problem, Iterate& current_iterate,
        double (*compute_objective)(Problem&, std::vector<double>&, std::vector<double>&, std::vector<double>&, double),
        std::vector<double> (*compute_objective_gradient)(Problem&, std::map<int,int>&, std::vector<double>&, std::vector<double>&, std::vector<double>&, double),
        std::vector<double> (*compute_constraints)(Problem& problem, std::map<int,int>& slacked_constraints, std::vector<double>& x),
        std::vector<double>& l, std::vector<double>& u, std::vector<ConstraintType>& variable_status,
        int max_iterations) {

    std::vector<double> x(current_iterate.x);
    /* determine the bound types */
    int n = x.size();
    std::vector<int> nbd(n);
    // set the type of bounds
    for (int i = 0; i < n; i++) {
        if (variable_status[i] == UNBOUNDED) {
            nbd[i] = 0;
        }
        else if (variable_status[i] == BOUNDED_LOWER) {
            nbd[i] = 1;
        }
        else if (variable_status[i] == BOUNDED_UPPER) {
            nbd[i] = 3;
        }
        else { // two bounds
            nbd[i] = 2;
        }
    }
    
    /* memory allocation for L-BFGS-B (limited memory & factors allocation) */
    std::vector<double> wa(this->limited_memory_size*(2*n + 11*this->limited_memory_size + 8) + 5*n);
    std::vector<int> iwa(3*n);
    
    // optimization loop (lbfgsb.f uses reverse communication to get function and gradient values)
    double f; // objective
    std::vector<double> g(n); // gradient of f wrt primal variables
    
    strcpy(this->task_, "START");
    int iterations = 0;
    bool stop = false;
    while (!stop) {
        std::cout << "Task: " << this->task_ << "\n";
        /* call L-BFGS-B */
        setulb_(&n, &this->limited_memory_size, x.data(), l.data(), u.data(), nbd.data(), &f, g.data(), &this->factr_, &this->pgtol_, wa.data(), iwa.data(), this->task_, &this->iprint_, this->csave_, this->lsave_, this->isave_, this->dsave_);

        // evaluate Augmented Lagrangian and its gradient
        if (strncmp(this->task_, "FG", 2) == 0) {
            std::cout << "x: "; print_vector(std::cout, x);
            std::vector<double> constraints = compute_constraints(problem, this->slacked_constraints_, x);
            f = compute_objective(problem, x, constraints, current_iterate.constraint_multipliers, this->rho);
            g = compute_objective_gradient(problem, this->slacked_constraints_, x, constraints, current_iterate.constraint_multipliers, this->rho);
            std::cout << "f is " << f << "\n";
            std::cout << "g is "; print_vector(std::cout, g);
            iterations++;
        }
        std::cout << iterations << " >= " << max_iterations << "\n";
        stop = (iterations >= max_iterations || !(strncmp(this->task_, "FG", 2) == 0 || strncmp(this->task_, "NEW_X", 5) == 0 || strncmp(this->task_, "START", 5) == 0));
        if (stop) {
            std::cout << "STOP!\n";
        }
    }
    
    // no constraint: empty constraint multipliers
    std::vector<double> constraint_multipliers;
    // create local solution from primal and dual variables
    LocalSolution solution(x, g, constraint_multipliers);
    solution.status = OPTIMAL;

    return solution;
}