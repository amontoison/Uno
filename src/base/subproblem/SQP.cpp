#include <cmath>
#include <map>
#include "SQP.hpp"
#include "Constraint.hpp"
#include "Utils.hpp"
#include "Logger.hpp"

SQP::SQP(QPSolver& solver, HessianEvaluation& hessian_evaluation) : ActiveSetMethod(solver), hessian_evaluation(hessian_evaluation) {
}

/* compute least-square multipliers */
//if (0 < problem.number_constraints) {
//    first_iterate.compute_constraints_jacobian(problem);
//    first_iterate.multipliers.constraints = Subproblem::compute_least_square_multipliers(problem, first_iterate, multipliers.constraints, 1e4);
//}
Iterate SQP::initialize(Problem& problem, std::vector<double>& x, Multipliers& multipliers, bool use_trust_region) {
    /* call superclass initialize() */
    Iterate first_iterate = ActiveSetMethod::initialize(problem, x, multipliers, use_trust_region);
    
    /* if no trust region is used, the problem should be convexified by changing the inertia of the Hessian */
    this->hessian_evaluation.convexify = !use_trust_region;
    
    return first_iterate;
}

double SQP::compute_predicted_reduction(Iterate& current_iterate, SubproblemSolution& solution, double step_length) {
    // the predicted reduction is quadratic
    if (step_length == 1.) {
        return -solution.objective;
    }
    else {
        double linear_term = dot(solution.x, current_iterate.objective_gradient);
        double quadratic_term = current_iterate.hessian.quadratic_product(solution.x, solution.x) / 2.;
        return -step_length * (linear_term + step_length * quadratic_term);
    }
}

bool SQP::phase_1_required(SubproblemSolution& solution) {
    return (solution.status == INFEASIBLE);
}

/* private methods */

void SQP::evaluate_optimality_iterate(Problem& problem, Iterate& current_iterate) {
    /* compute first- and second-order information */
    current_iterate.compute_objective_gradient(problem);
    current_iterate.compute_constraints_jacobian(problem);
    this->hessian_evaluation.compute(problem, current_iterate, problem.objective_sign, current_iterate.multipliers.constraints);
    return;
}

void SQP::evaluate_feasibility_iterate(Problem& problem, Iterate& current_iterate, SubproblemSolution& phase_II_solution) {
    /* update the multipliers of the general constraints */
    std::vector<double> constraint_multipliers = this->generate_feasibility_multipliers(problem, current_iterate.multipliers.constraints, phase_II_solution.constraint_partition);
    /* compute first- and second-order information */
    current_iterate.compute_constraints_jacobian(problem);
    current_iterate.is_hessian_computed = false;
    double objective_multiplier = 0.;
    this->hessian_evaluation.compute(problem, current_iterate, objective_multiplier, constraint_multipliers);
    return;
}

std::vector<double> SQP::generate_feasibility_multipliers(Problem& problem, std::vector<double>& current_constraint_multipliers, ConstraintPartition& constraint_partition) {
    std::vector<double> constraint_multipliers(problem.number_constraints);
    for (int j = 0; j < problem.number_constraints; j++) {
        if (constraint_partition.constraint_feasibility[j] == INFEASIBLE_LOWER) {
            constraint_multipliers[j] = 1.;
        }
        else if (constraint_partition.constraint_feasibility[j] == INFEASIBLE_UPPER) {
            constraint_multipliers[j] = -1.;
        }
        else {
            constraint_multipliers[j] = current_constraint_multipliers[j];
        }
    }
    return constraint_multipliers;
}

SubproblemSolution SQP::solve_subproblem(std::vector<Range>& variables_bounds, std::vector<Range>& constraints_bounds, Iterate& current_iterate, std::vector<double>& d0) {
    return this->solver.solve_QP(variables_bounds, constraints_bounds, current_iterate.objective_gradient, current_iterate.constraints_jacobian, current_iterate.hessian, d0);
}