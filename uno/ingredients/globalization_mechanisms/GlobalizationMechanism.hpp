// Copyright (c) 2018-2024 Charlie Vanaret
// Licensed under the MIT license. See LICENSE file in the project directory for details.

#ifndef UNO_GLOBALIZATIONMECHANISM_H
#define UNO_GLOBALIZATIONMECHANISM_H

#include <memory>
#include "ingredients/constraint_relaxation_strategies/ConstraintRelaxationStrategy.hpp"
#include "optimization/Direction.hpp"

namespace uno {
   // forward declarations
   class Iterate;
   class Model;
   class Options;
   class Statistics;
   class UserCallbacks;
   struct WarmstartInformation;

   class GlobalizationMechanism {
   public:
      GlobalizationMechanism(size_t number_constraints, size_t number_bounds_constraints, const Options& options);
      virtual ~GlobalizationMechanism() = default;

      virtual void initialize(Statistics& statistics, const Model& model, Iterate& initial_iterate, const Options& options) = 0;
      virtual void compute_next_iterate(Statistics& statistics, const Model& model, Iterate& current_iterate, Iterate& trial_iterate,
            WarmstartInformation& warmstart_information, UserCallbacks& user_callbacks) = 0;

      [[nodiscard]] virtual std::string get_strategy_combination() const = 0;
      [[nodiscard]] size_t get_hessian_evaluation_count() const;
      [[nodiscard]] size_t get_number_subproblems_solved() const;

   protected:
      // reference to allow polymorphism
      std::unique_ptr<ConstraintRelaxationStrategy> constraint_relaxation_strategy; /*!< Constraint relaxation strategy */
      Direction direction{};

      static void assemble_trial_iterate(const Model& model, Iterate& current_iterate, Iterate& trial_iterate, const Direction& direction,
            double primal_step_length, double dual_step_length);
   };
} // namespace

#endif // UNO_GLOBALIZATIONMECHANISM_H
