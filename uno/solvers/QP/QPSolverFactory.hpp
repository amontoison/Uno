// Copyright (c) 2018-2023 Charlie Vanaret
// Licensed under the MIT license. See LICENSE file in the project directory for details.

#ifndef UNO_QPSOLVERFACTORY_H
#define UNO_QPSOLVERFACTORY_H

#include <memory>
#include "QPSolver.hpp"

#ifdef HAS_BQPD
#include "BQPDSolver.hpp"
#endif

class QPSolverFactory {
public:
   // create a QP solver
   static std::unique_ptr<QPSolver> create(const std::string& QP_solver_name, size_t number_variables, size_t number_constraints,
         size_t maximum_number_nonzeros, bool quadratic_programming, const Options& options) {
#ifdef HAS_BQPD
      if (QP_solver_name == "BQPD") {
         return std::make_unique<BQPDSolver>(number_variables, number_constraints, maximum_number_nonzeros, quadratic_programming, options);
      }
#endif
      throw std::invalid_argument("QP solver name is unknown");
   }

   // return the list of available QP solvers
   static std::vector<std::string> available_solvers() {
      std::vector<std::string> solvers{};
      #ifdef HAS_BQPD
      solvers.emplace_back("BQPD");
      #endif
      return solvers;
   }
};

#endif // UNO_QPSOLVERFACTORY_H
