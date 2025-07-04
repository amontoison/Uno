// Copyright (c) 2018-2024 Charlie Vanaret
// Licensed under the MIT license. See LICENSE file in the project directory for details.

#ifndef UNO_CONSTRAINTRELAXATIONSTRATEGY_H
#define UNO_CONSTRAINTRELAXATIONSTRATEGY_H

#include <cstddef>
#include <functional>
#include "ingredients/globalization_strategies/ProgressMeasures.hpp"
#include "linear_algebra/Norm.hpp"
#include "optimization/IterateStatus.hpp"

namespace uno {
   // forward declarations
   class Direction;
   class GlobalizationStrategy;
   class InequalityHandlingMethod;
   class Iterate;
   class Model;
   class Multipliers;
   class OptimizationProblem;
   class Options;
   class Statistics;
   class UserCallbacks;
   template <typename ElementType>
   class Vector;
   class WarmstartInformation;

   class ConstraintRelaxationStrategy {
   public:
      explicit ConstraintRelaxationStrategy(const Options& options);
      virtual ~ConstraintRelaxationStrategy();

      virtual void initialize(Statistics& statistics, const Model& model, Iterate& initial_iterate, Direction& direction,
         const Options& options) = 0;

      // direction computation
      virtual void compute_feasible_direction(Statistics& statistics, GlobalizationStrategy& globalization_strategy,
         const Model& model, Iterate& current_iterate, Direction& direction, double trust_region_radius,
         WarmstartInformation& warmstart_information) = 0;
      /*
      void compute_feasible_direction(Statistics& statistics, GlobalizationStrategy& globalization_strategy, const Model& model,
         Iterate& current_iterate, Direction& direction, const Vector<double>& initial_point, double trust_region_radius,
         WarmstartInformation& warmstart_information);
      */
      [[nodiscard]] virtual bool solving_feasibility_problem() const = 0;
      virtual void switch_to_feasibility_problem(Statistics& statistics, GlobalizationStrategy& globalization_strategy,
         const Model& model, Iterate& current_iterate, WarmstartInformation& warmstart_information) = 0;

      // trial iterate acceptance
      [[nodiscard]] virtual bool is_iterate_acceptable(Statistics& statistics, GlobalizationStrategy& globalization_strategy,
         const Model& model, Iterate& current_iterate, Iterate& trial_iterate, const Direction& direction, double step_length,
         WarmstartInformation& warmstart_information, UserCallbacks& user_callbacks) = 0;
      [[nodiscard]] IterateStatus check_termination(const Model& model, Iterate& iterate);

      // primal-dual residuals
      virtual void compute_primal_dual_residuals(const Model& model, Iterate& iterate) = 0;
      virtual void set_dual_residuals_statistics(Statistics& statistics, const Iterate& iterate) const = 0;

      [[nodiscard]] virtual std::string get_name() const = 0;
      [[nodiscard]] virtual size_t get_hessian_evaluation_count() const = 0;
      [[nodiscard]] virtual size_t get_number_subproblems_solved() const = 0;

   protected:
      const Norm progress_norm;
      const Norm residual_norm;
      const double residual_scaling_threshold;
      const double tight_tolerance; /*!< Tight tolerance of the termination criteria */
      const double loose_tolerance; /*!< Loose tolerance of the termination criteria */
      size_t loose_tolerance_consecutive_iterations{0};
      const size_t loose_tolerance_consecutive_iteration_threshold;
      const double unbounded_objective_threshold;
      // first_order_predicted_reduction is true when the predicted reduction can be taken as first-order (e.g. in line-search methods)
      const bool first_order_predicted_reduction;

      void set_objective_measure(const Model& model, Iterate& iterate) const;
      void set_infeasibility_measure(const Model& model, Iterate& iterate) const;
      [[nodiscard]] double compute_predicted_infeasibility_reduction(const Model& model, const Iterate& current_iterate,
         const Vector<double>& primal_direction, double step_length) const;
      [[nodiscard]] std::function<double(double)> compute_predicted_objective_reduction(InequalityHandlingMethod& inequality_handling_method,
         const Iterate& current_iterate, const Vector<double>& primal_direction, double step_length) const;
      void compute_progress_measures(InequalityHandlingMethod& inequality_handling_method, const OptimizationProblem& problem,
         GlobalizationStrategy& globalization_strategy, Iterate& current_iterate, Iterate& trial_iterate) const;
      [[nodiscard]] ProgressMeasures compute_predicted_reductions(InequalityHandlingMethod& inequality_handling_method,
         const OptimizationProblem& problem, const Iterate& current_iterate, const Direction& direction, double step_length) const;
      [[nodiscard]] bool is_iterate_acceptable(Statistics& statistics, GlobalizationStrategy& globalization_strategy,
         const OptimizationProblem& problem, InequalityHandlingMethod& inequality_handling_method, Iterate& current_iterate,
         Iterate& trial_iterate, Multipliers& trial_multipliers, const Direction& direction, double step_length,
         UserCallbacks& user_callbacks) const;
      virtual void evaluate_progress_measures(InequalityHandlingMethod& inequality_handling_method,
         const OptimizationProblem& problem, Iterate& iterate) const = 0;

      void compute_primal_dual_residuals(const Model& model, const OptimizationProblem& optimality_problem,
         const OptimizationProblem& feasibility_problem, Iterate& iterate) const;

      [[nodiscard]] double compute_stationarity_scaling(const Model& model, const Multipliers& multipliers) const;
      [[nodiscard]] double compute_complementarity_scaling(const Model& model, const Multipliers& multipliers) const;

      [[nodiscard]] IterateStatus check_first_order_convergence(const Model& model, Iterate& current_iterate, double tolerance) const;

      void set_statistics(Statistics& statistics, const Model& model, const Iterate& iterate) const;
      void set_primal_statistics(Statistics& statistics, const Model& model, const Iterate& iterate) const;
   };
} // namespace

#endif //UNO_CONSTRAINTRELAXATIONSTRATEGY_H