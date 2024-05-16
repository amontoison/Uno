// Copyright (c) 2018-2024 Charlie Vanaret
// Licensed under the MIT license. See LICENSE file in the project directory for details.

#include "FletcherFilterMethod.hpp"
#include "../ProgressMeasures.hpp"
#include "optimization/Iterate.hpp"
#include "tools/Logger.hpp"
#include "tools/Statistics.hpp"

FletcherFilterMethod::FletcherFilterMethod(const Options& options): FilterMethod(options) {
}

/* check acceptability of step(s) (filter & sufficient reduction)
 * filter methods enforce an *unconstrained* sufficient decrease condition
 * precondition: feasible step
 * */
bool FletcherFilterMethod::is_iterate_acceptable(Statistics& statistics, const ProgressMeasures& current_progress,
      const ProgressMeasures& trial_progress, const ProgressMeasures& predicted_reduction, double objective_multiplier) {
   const bool solving_feasibility_problem = (objective_multiplier == 0.);
   std::string scenario;
   bool accept = false;
   // solving the feasibility problem = working on infeasibility only (no filter acceptability test)
   if (solving_feasibility_problem) {
      if (this->armijo_sufficient_decrease(predicted_reduction.infeasibility, current_progress.infeasibility - trial_progress.infeasibility)) {
         DEBUG << "Trial iterate (h-type) was accepted by satisfying the Armijo condition\n";
         accept = true;
      }
      else {
         DEBUG << "Trial iterate (h-type) was rejected by violating the Armijo condition\n";
      }
      scenario = "h-type Armijo";
      Iterate::number_eval_objective--;
   }
   else {
      // in filter methods, we construct an unconstrained measure by ignoring infeasibility and scaling the objective measure by 1
      const double current_merit = FilterMethod::unconstrained_merit_function(current_progress);
      const double trial_merit = FilterMethod::unconstrained_merit_function(trial_progress);
      const double merit_predicted_reduction = FilterMethod::unconstrained_merit_function(predicted_reduction);
      DEBUG << "Current: (infeas., objective+auxiliary) = (" << current_progress.infeasibility << ", " << current_merit << ")\n";
      DEBUG << "Trial:   (infeas., objective+auxiliary) = (" << trial_progress.infeasibility << ", " << trial_merit << ")\n";
      DEBUG << "Unconstrained predicted reduction: " << merit_predicted_reduction << '\n';
      DEBUG << "Current filter:\n" << *this->filter << '\n';

      if (this->filter->acceptable(trial_progress.infeasibility, trial_merit)) {
         if (this->filter->acceptable_wrt_current_iterate(current_progress.infeasibility, current_merit, trial_progress.infeasibility, trial_merit)) {
            // switching condition: check whether the unconstrained predicted reduction is sufficiently positive
            if (this->switching_condition(merit_predicted_reduction, current_progress.infeasibility, this->parameters.delta)) {
               // unconstrained Armijo sufficient decrease condition: predicted reduction should be positive (f-type)
               const double objective_actual_reduction = this->compute_actual_objective_reduction(current_merit, current_progress.infeasibility,
                     trial_merit);
               DEBUG << "Actual reduction: " << objective_actual_reduction << '\n';
               if (this->armijo_sufficient_decrease(merit_predicted_reduction, objective_actual_reduction)) {
                  DEBUG << "Trial iterate (f-type) was accepted by satisfying the Armijo condition\n";
                  accept = true;
               }
               else { // switching condition holds, but not Armijo condition
                  DEBUG << "Trial iterate (f-type) was rejected by violating the Armijo condition\n";
               }
               scenario = "f-type Armijo";
            }
               // switching condition violated: predicted reduction is not promising (h-type)
            else {
               DEBUG << "Trial iterate (h-type) was accepted by violating the switching condition\n";
               accept = true;
               this->filter->add(current_progress.infeasibility, current_merit);
               DEBUG << "Current iterate was added to the filter\n";
               scenario = "h-type";
            }
         }
         else {
            scenario = "current point";
         }
      }
      else {
         scenario = "filter";
      }
   }
   statistics.set("status", std::string(accept ? "accepted" : "rejected") + " (" + scenario + ")");
   DEBUG << '\n';
   return accept;
}

bool FletcherFilterMethod::is_infeasibility_sufficiently_reduced(const ProgressMeasures& /*current_progress*/, const ProgressMeasures& trial_progress) const {
   // if the trial infeasibility improves upon the best known infeasibility
   return this->filter->infeasibility_sufficient_reduction(this->filter->get_smallest_infeasibility(), trial_progress.infeasibility);
}