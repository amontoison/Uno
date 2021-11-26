#ifndef LINESEARCH_H
#define LINESEARCH_H

#include "GlobalizationMechanism.hpp"
#include "RegularizationStrategy.hpp"

/*! \class LineSearch
 * \brief Line-search
 *
 *  Line-search strategy
 */
class BacktrackingLineSearch : public GlobalizationMechanism {
public:
   explicit BacktrackingLineSearch(ConstraintRelaxationStrategy& constraint_relaxation_strategy, const Options& options);

   void initialize(Statistics& statistics, const Problem& problem, Iterate& first_iterate) override;
   std::tuple<Iterate, double> compute_acceptable_iterate(Statistics& statistics, const Problem& problem, Iterate& current_iterate) override;

private:
   std::unique_ptr<RegularizationStrategy> regularization_strategy;
   double step_length{1.};
   /* ratio of step length update in ]0, 1[ */
   const double backtracking_ratio;
   const double min_step_length;

   bool termination();
   void print_iteration();
   void add_statistics(Statistics& statistics, const Direction& direction);
   static void check_unboundedness(const Direction& direction);
   void decrease_step_length();
};

#endif // LINESEARCH_H
