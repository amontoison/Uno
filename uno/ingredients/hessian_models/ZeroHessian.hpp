// Copyright (c) 2024 Charlie Vanaret
// Licensed under the MIT license. See LICENSE file in the project directory for details.

#ifndef UNO_ZEROHESSIAN_H
#define UNO_ZEROHESSIAN_H

#include "HessianModel.hpp"

namespace uno {
   class ZeroHessian : public HessianModel {
   public:
      ZeroHessian() = default;

      void initialize(const Model& model) override;
      [[nodiscard]] size_t number_nonzeros(const Model& model) const override;
      [[nodiscard]] bool is_positive_definite() const override;
      void evaluate_hessian(Statistics& statistics, const Model& model, const Vector<double>& primal_variables, double objective_multiplier,
         const Vector<double>& constraint_multipliers, SymmetricMatrix<size_t, double>& hessian) override;
      void compute_hessian_vector_product(const Model& model, const Vector<double>& vector, double objective_multiplier,
         const Vector<double>& constraint_multipliers, Vector<double>& result) override;
      [[nodiscard]] std::string get_name() const override;
   };
} // namespace

#endif // UNO_ZEROHESSIAN_H