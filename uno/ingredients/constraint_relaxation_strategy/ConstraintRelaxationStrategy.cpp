// Copyright (c) 2018-2024 Charlie Vanaret
// Licensed under the MIT license. See LICENSE file in the project directory for details.

#include "ConstraintRelaxationStrategy.hpp"
#include "linear_algebra/view.hpp"

ConstraintRelaxationStrategy::ConstraintRelaxationStrategy(const Model& model, const Options& options):
      model(model),
      progress_norm(norm_from_string(options.get_string("progress_norm"))),
      residual_norm(norm_from_string(options.get_string("residual_norm"))),
      residual_scaling_threshold(options.get_double("residual_scaling_threshold")) {
}

void ConstraintRelaxationStrategy::compute_primal_dual_residuals(const RelaxedProblem& feasibility_problem, Iterate& iterate) {
   iterate.evaluate_objective_gradient(this->model);
   iterate.evaluate_constraints(this->model);
   iterate.evaluate_constraint_jacobian(this->model);

   // stationarity error
   ConstraintRelaxationStrategy::evaluate_lagrangian_gradient(this->model.number_variables, iterate, iterate.multipliers, iterate.multipliers.objective);
   iterate.residuals.optimality_stationarity = this->stationarity_error(iterate);
   iterate.residuals.feasibility_stationarity = feasibility_problem.stationarity_error(iterate, this->residual_norm);

   // constraint violation of the original problem
   iterate.residuals.infeasibility = this->model.constraint_violation(iterate.evaluations.constraints, this->residual_norm);

   // complementarity error
   iterate.residuals.optimality_complementarity = this->complementarity_error(iterate.primals, iterate.evaluations.constraints, iterate.multipliers);
   iterate.residuals.feasibility_complementarity = feasibility_problem.complementarity_error(iterate.primals, iterate.evaluations.constraints,
         iterate.multipliers, this->residual_norm);

   // scaling factors
   iterate.residuals.stationarity_scaling = this->compute_stationarity_scaling(iterate);
   iterate.residuals.complementarity_scaling = this->compute_complementarity_scaling(iterate);
}

// Lagrangian gradient split in two parts: objective contribution and constraints' contribution
void ConstraintRelaxationStrategy::evaluate_lagrangian_gradient(size_t number_variables, Iterate& iterate, const Multipliers& multipliers,
      double objective_multiplier) {
   initialize_vector(iterate.lagrangian_gradient.objective_contribution, 0.);
   initialize_vector(iterate.lagrangian_gradient.constraints_contribution, 0.);

   // objective gradient
   iterate.evaluations.objective_gradient.for_each([&](size_t variable_index, double derivative) {
      iterate.lagrangian_gradient.objective_contribution[variable_index] += objective_multiplier * derivative;
   });

   // constraints
   for (size_t constraint_index: Range(iterate.number_constraints)) {
      if (multipliers.constraints[constraint_index] != 0.) {
         iterate.evaluations.constraint_jacobian[constraint_index].for_each([&](size_t variable_index, double derivative) {
            iterate.lagrangian_gradient.constraints_contribution[variable_index] -= multipliers.constraints[constraint_index] * derivative;
         });
      }
   }

   // bound constraints
   for (size_t variable_index: Range(number_variables)) {
      iterate.lagrangian_gradient.constraints_contribution[variable_index] -= multipliers.lower_bounds[variable_index] + multipliers.upper_bounds[variable_index];
   }
}

double ConstraintRelaxationStrategy::stationarity_error(const Iterate& iterate) const {
   // norm of the Lagrangian gradient
   return norm(this->residual_norm, iterate.lagrangian_gradient);
}

double ConstraintRelaxationStrategy::compute_stationarity_scaling(const Iterate& iterate) const {
   const size_t total_size = this->model.get_lower_bounded_variables().size() + this->model.get_upper_bounded_variables().size() + this->model.number_constraints;
   if (total_size == 0) {
      return 1.;
   }
   else {
      const double scaling_factor = this->residual_scaling_threshold * static_cast<double>(total_size);
      const double multiplier_norm = norm_1(
            view(iterate.multipliers.constraints, this->model.number_constraints),
            view(iterate.multipliers.lower_bounds, this->model.number_variables),
            view(iterate.multipliers.upper_bounds, this->model.number_variables)
      );
      return std::max(1., multiplier_norm / scaling_factor);
   }
}

double ConstraintRelaxationStrategy::compute_complementarity_scaling(const Iterate& iterate) const {
   const size_t total_size = this->model.get_lower_bounded_variables().size() + this->model.get_upper_bounded_variables().size();
   if (total_size == 0) {
      return 1.;
   }
   else {
      const double scaling_factor = this->residual_scaling_threshold * static_cast<double>(total_size);
      const double bound_multiplier_norm = norm_1(
            view(iterate.multipliers.lower_bounds, this->model.number_variables),
            view(iterate.multipliers.upper_bounds, this->model.number_variables)
      );
      return std::max(1., bound_multiplier_norm / scaling_factor);
   }
}
