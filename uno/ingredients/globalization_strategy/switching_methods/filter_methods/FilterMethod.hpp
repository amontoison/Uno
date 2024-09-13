// Copyright (c) 2018-2024 Charlie Vanaret
// Licensed under the MIT license. See LICENSE file in the project directory for details.

#ifndef UNO_FILTERMETHOD_H
#define UNO_FILTERMETHOD_H

#include "../SwitchingMethod.hpp"
#include "filters/Filter.hpp"

namespace uno {
   // forward declaration
   class Options;

   /*! \class TwoPhaseConstants
    * \brief Constants for filter strategy
    *
    *  Set of constants to control the filter strategy
    */
   struct FilterStrategyParameters {
      double upper_bound;
      double infeasibility_factor;
   };

   class FilterMethod: public SwitchingMethod {
   public:
      explicit FilterMethod(const Options& options);

      void initialize(Statistics& statistics, const Iterate& initial_iterate, const Options& options) override;
      void reset() override;
      void notify_switch_to_feasibility(const ProgressMeasures& current_progress) override;
      void notify_switch_to_optimality(const ProgressMeasures& current_progress) override;

   protected:
      // pointer to allow polymorphism
      const std::unique_ptr<Filter> filter;
      const FilterStrategyParameters parameters; /*!< Set of constants */

      [[nodiscard]] double compute_actual_objective_reduction(double current_objective_measure, double current_infeasibility, double trial_objective_measure);
      void set_statistics(Statistics& statistics) const override;
   };
} // namespace

#endif // UNO_FILTERMETHOD_H
