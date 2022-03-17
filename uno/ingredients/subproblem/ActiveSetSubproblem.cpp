#include "ActiveSetSubproblem.hpp"

ActiveSetSubproblem::ActiveSetSubproblem(size_t max_number_variables, size_t number_constraints, SecondOrderCorrection soc_strategy,
      bool is_second_order_method, Norm residual_norm): Subproblem(max_number_variables, number_constraints, soc_strategy, is_second_order_method,
      residual_norm),
      initial_point(max_number_variables),
      variable_displacement_bounds(max_number_variables),
      linearized_constraint_bounds(number_constraints) {
}

void ActiveSetSubproblem::set_initial_point(const std::optional<std::vector<double>>& optional_initial_point) {
   // if provided, set the initial point
   if (optional_initial_point.has_value()) {
      const std::vector<double>& point = optional_initial_point.value();
      copy_from(this->initial_point, point);
   }
   else {
      // otherwise, reset the initial point
      initialize_vector(this->initial_point, 0.);
   }
}

void ActiveSetSubproblem::set_variable_displacement_bounds(const Problem& problem, const Iterate& current_iterate) {
   for (size_t i = 0; i < problem.number_variables; i++) {
      const double lb = this->variable_bounds[i].lb - current_iterate.x[i];
      const double ub = this->variable_bounds[i].ub - current_iterate.x[i];
      this->variable_displacement_bounds[i] = {lb, ub};
   }
}

void ActiveSetSubproblem::set_linearized_constraint_bounds(const Problem& problem, const std::vector<double>& current_constraints) {
   for (size_t j = 0; j < problem.number_constraints; j++) {
      const double lb = problem.get_constraint_lower_bound(j) - current_constraints[j];
      const double ub = problem.get_constraint_upper_bound(j) - current_constraints[j];
      this->linearized_constraint_bounds[j] = {lb, ub};
   }
}

void ActiveSetSubproblem::compute_dual_displacements(const Problem& problem, const Iterate& current_iterate, Direction& direction) {
   // compute dual *displacements* (note: active-set methods usually compute the new duals, not the displacements)
   for (size_t j = 0; j < problem.number_constraints; j++) {
      direction.multipliers.constraints[j] -= current_iterate.multipliers.constraints[j];
   }
}