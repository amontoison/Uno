// Copyright (c) 2018-2024 Charlie Vanaret
// Licensed under the MIT license. See LICENSE file in the project directory for details.

#ifndef UNO_AMPLMODEL_H
#define UNO_AMPLMODEL_H

#include <vector>
#include "optimization/Model.hpp"
#include "linear_algebra/RectangularMatrix.hpp"
#include "tools/CollectionAdapter.hpp"

// include AMPL Solver Library (ASL)
extern "C" {
#include "asl_pfgh.h"
#include "getstub.h"
}

/*! \class AMPLModel
 * \brief AMPL model
 *
 *  Description of an AMPL model
 */
class AMPLModel: public Model {
public:
   explicit AMPLModel(const std::string& file_name);
   ~AMPLModel() override;

   [[nodiscard]] double evaluate_objective(const std::vector<double>& x) const override;
   void evaluate_objective_gradient(const std::vector<double>& x, SparseVector<double>& gradient) const override;
   void evaluate_constraints(const std::vector<double>& x, std::vector<double>& constraints) const override;
   void evaluate_constraint_gradient(const std::vector<double>& x, size_t constraint_index, SparseVector<double>& gradient) const override;
   void evaluate_constraint_jacobian(const std::vector<double>& x, RectangularMatrix<double>& constraint_jacobian) const override;
   void evaluate_lagrangian_hessian(const std::vector<double>& x, double objective_multiplier, const std::vector<double>& multipliers,
         SymmetricMatrix<double>& hessian) const override;

   [[nodiscard]] double variable_lower_bound(size_t variable_index) const override;
   [[nodiscard]] double variable_upper_bound(size_t variable_index) const override;
   [[nodiscard]] BoundType get_variable_bound_type(size_t variable_index) const override;
   [[nodiscard]] const Collection<size_t>& get_lower_bounded_variables() const override;
   [[nodiscard]] const Collection<size_t>& get_upper_bounded_variables() const override;
   [[nodiscard]] const Collection<size_t>& get_slacks() const override;
   [[nodiscard]] const Collection<size_t>& get_single_lower_bounded_variables() const override;
   [[nodiscard]] const Collection<size_t>& get_single_upper_bounded_variables() const override;

   [[nodiscard]] double constraint_lower_bound(size_t constraint_index) const override;
   [[nodiscard]] double constraint_upper_bound(size_t constraint_index) const override;
   [[nodiscard]] FunctionType get_constraint_type(size_t constraint_index) const override;
   [[nodiscard]] BoundType get_constraint_bound_type(size_t constraint_index) const override;
   [[nodiscard]] const Collection<size_t>& get_equality_constraints() const override;
   [[nodiscard]] const Collection<size_t>& get_inequality_constraints() const override;
   [[nodiscard]] const std::vector<size_t>& get_linear_constraints() const override;

   void initial_primal_point(std::vector<double>& x) const override;
   void initial_dual_point(std::vector<double>& multipliers) const override;
   void postprocess_solution(Iterate& iterate, TerminationStatus termination_status) const override;

   [[nodiscard]] size_t number_objective_gradient_nonzeros() const override;
   [[nodiscard]] size_t number_jacobian_nonzeros() const override;
   [[nodiscard]] size_t number_hessian_nonzeros() const override;

private:
   // private constructor to pass the dimensions to the Model base constructor
   AMPLModel(const std::string& file_name, ASL* asl);

   // mutable: can be modified by const methods (internal state not seen by user)
   mutable ASL* asl; /*!< Instance of the AMPL Solver Library class */
   mutable std::vector<double> asl_gradient{};
   mutable std::vector<double> asl_hessian{};
   size_t number_asl_hessian_nonzeros{0}; /*!< Number of nonzero elements in the Hessian */

   std::vector<Interval> variable_bounds;
   std::vector<Interval> constraint_bounds;
   std::vector<BoundType> variable_status; /*!< Status of the variables (EQUALITY, BOUNDED_LOWER, BOUNDED_UPPER, BOUNDED_BOTH_SIDES) */
   std::vector<FunctionType> constraint_type; /*!< Types of the constraints (LINEAR, QUADRATIC, NONLINEAR) */
   std::vector<BoundType> constraint_status; /*!< Status of the constraints (EQUAL_BOUNDS, BOUNDED_LOWER, BOUNDED_UPPER, BOUNDED_BOTH_SIDES,
 * UNBOUNDED) */
   std::vector<size_t> linear_constraints;

   // lists of variables and constraints + corresponding collection objects
   std::vector<size_t> equality_constraints{};
   CollectionAdapter<std::vector<size_t>&> equality_constraints_collection;
   std::vector<size_t> inequality_constraints{};
   CollectionAdapter<std::vector<size_t>&> inequality_constraints_collection;
   Range<FORWARD> slacks;
   std::vector<size_t> lower_bounded_variables;
   CollectionAdapter<std::vector<size_t>&> lower_bounded_variables_collection;
   std::vector<size_t> upper_bounded_variables;
   CollectionAdapter<std::vector<size_t>&> upper_bounded_variables_collection;
   std::vector<size_t> single_lower_bounded_variables{}; // indices of the single lower-bounded variables
   CollectionAdapter<std::vector<size_t>&> single_lower_bounded_variables_collection;
   std::vector<size_t> single_upper_bounded_variables{}; // indices of the single upper-bounded variables
   CollectionAdapter<std::vector<size_t>&> single_upper_bounded_variables_collection;

   void generate_variables();
   void generate_constraints();

   void set_number_hessian_nonzeros();
   [[nodiscard]] size_t compute_hessian_number_nonzeros(double objective_multiplier, const std::vector<double>& multipliers) const;
   static void determine_bounds_types(std::vector<Interval>& variables_bounds, std::vector<BoundType>& status);
};

#endif // UNO_AMPLMODEL_H
