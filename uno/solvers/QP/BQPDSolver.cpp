#include <cassert>
#include <algorithm>
#include "BQPDSolver.hpp"
#include "tools/Logger.hpp"
#include "linear_algebra/Vector.hpp"

#define BIG 1e30

extern "C" {
// fortran common block used in bqpd/bqpd.f
extern struct {
   int kk, ll, kkk, lll, mxws, mxlws;
} wsc_;

// fortran common for inertia correction in wdotd
extern struct {
   double alpha;
} kktalphac_;

extern void
bqpd_(const int* n, const int* m, int* k, int* kmax, double* a, int* la, double* x, double* bl, double* bu, double* f, double* fmin, double* g,
      double* r, double* w, double* e, int* ls, double* alp, int* lp, int* mlp, int* peq, double* ws, int* lws, const int* mode, int* ifail,
      int* info, int* iprint, int* nout);
}

// preallocate a bunch of stuff
BQPDSolver::BQPDSolver(size_t max_number_variables, size_t number_constraints, size_t maximum_number_nonzeros, bool quadratic_programming):
      QPSolver(), maximum_number_nonzeros(maximum_number_nonzeros),
      lb(max_number_variables + number_constraints),
      ub(max_number_variables + number_constraints), jacobian(max_number_variables * (number_constraints + 1)),
      jacobian_sparsity(max_number_variables * (number_constraints + 1) + number_constraints + 3),
      kmax(quadratic_programming ? 500 : 0), alp(mlp), lp(mlp), ls(max_number_variables + number_constraints),
      w(max_number_variables + number_constraints), gradient_solution(max_number_variables), residuals(max_number_variables + number_constraints),
      e(max_number_variables + number_constraints),
      size_hessian_sparsity(quadratic_programming ? maximum_number_nonzeros + max_number_variables + 3 : 0),
      size_hessian_workspace(maximum_number_nonzeros + kmax * (kmax + 9) / 2 + 2 * max_number_variables + number_constraints + mxwk0),
      size_hessian_sparsity_workspace(size_hessian_sparsity + kmax + mxiwk0),
      hessian_values(size_hessian_workspace), hessian_sparsity(size_hessian_sparsity_workspace) {
   // active set
   for (size_t i = 0; i < max_number_variables + number_constraints; i++) {
      this->ls[i] = static_cast<int>(i) + this->fortran_shift;
   }
}

Direction BQPDSolver::solve_QP(size_t number_variables, size_t number_constraints, const std::vector<Range>& variables_bounds,
      const std::vector<Range>& constraint_bounds, const SparseVector<double>& linear_objective,
      const std::vector<SparseVector<double>>& constraint_jacobian, const SymmetricMatrix& hessian, const std::vector<double>& initial_point) {
   this->save_hessian_to_local_format(hessian);
   return this->solve_subproblem(number_variables, number_constraints, variables_bounds, constraint_bounds, linear_objective, constraint_jacobian,
         initial_point);
}

Direction BQPDSolver::solve_LP(size_t number_variables, size_t number_constraints, const std::vector<Range>& variables_bounds,
      const std::vector<Range>& constraint_bounds, const SparseVector<double>& linear_objective,
      const std::vector<SparseVector<double>>& constraint_jacobian, const std::vector<double>& initial_point) {
   return this->solve_subproblem(number_variables, number_constraints, variables_bounds, constraint_bounds, linear_objective, constraint_jacobian,
         initial_point);
}

Direction BQPDSolver::solve_subproblem(size_t number_variables, size_t number_constraints, const std::vector<Range>& variables_bounds,
      const std::vector<Range>& constraint_bounds, const SparseVector<double>& linear_objective,
      const std::vector<SparseVector<double>>& constraint_jacobian, const std::vector<double>& initial_point) {
   // initialize wsc_ common block (Hessian & workspace for bqpd)
   // setting the common block here ensures that several instances of BQPD can run simultaneously
   wsc_.kk = static_cast<int>(this->maximum_number_nonzeros);
   wsc_.ll = static_cast<int>(this->size_hessian_sparsity);
   wsc_.mxws = static_cast<int>(this->size_hessian_workspace);
   wsc_.mxlws = static_cast<int>(this->size_hessian_sparsity_workspace);
   kktalphac_.alpha = 0; // inertia control

   DEBUG << "objective gradient: ";
   DEBUG << linear_objective;
   for (size_t j = 0; j < number_constraints; j++) {
      DEBUG << "gradient c" << j << ": " << constraint_jacobian[j];
   }
   for (size_t i = 0; i < number_variables; i++) {
      DEBUG << "Δx" << i << " in [" << variables_bounds[i].lb << ", " << variables_bounds[i].ub << "]\n";
   }
   for (size_t j = 0; j < number_constraints; j++) {
      DEBUG << "linearized c" << j << " in [" << constraint_bounds[j].lb << ", " << constraint_bounds[j].ub << "]\n";
   }

   // Jacobian
   this->save_gradients_to_local_format(number_constraints, linear_objective, constraint_jacobian);

   // bounds
   for (size_t i = 0; i < number_variables; i++) {
      this->lb[i] = (variables_bounds[i].lb == -std::numeric_limits<double>::infinity()) ? -BIG : variables_bounds[i].lb;
      this->ub[i] = (variables_bounds[i].ub == std::numeric_limits<double>::infinity()) ? BIG : variables_bounds[i].ub;
   }
   for (size_t j = 0; j < number_constraints; j++) {
      this->lb[number_variables + j] = (constraint_bounds[j].lb == -std::numeric_limits<double>::infinity()) ? -BIG : constraint_bounds[j].lb;
      this->ub[number_variables + j] = (constraint_bounds[j].ub == std::numeric_limits<double>::infinity()) ? BIG : constraint_bounds[j].ub;
   }

   Direction direction(number_variables, number_constraints);
   copy_from(direction.x, initial_point);
   const int n = static_cast<int>(number_variables);
   const int m = static_cast<int>(number_constraints);
   const int current_mode = static_cast<int>(this->mode);

   // solve the LP/QP
   bqpd_(&n, &m, &this->k, &this->kmax, this->jacobian.data(), this->jacobian_sparsity.data(),
         direction.x.data(), this->lb.data(), this->ub.data(), &direction.objective, &this->fmin, this->gradient_solution.data(),
         this->residuals.data(), this->w.data(), this->e.data(), this->ls.data(), this->alp.data(), this->lp.data(),
         &this->mlp, &this->peq_solution, this->hessian_values.data(), this->hessian_sparsity.data(), &current_mode, &this->ifail,
         this->info.data(), &this->iprint, &this->nout);
   direction.status = BQPDSolver::status_to_int(this->ifail);

   // project solution into bounds
   for (size_t i = 0; i < number_variables; i++) {
      direction.x[i] = std::min(std::max(direction.x[i], variables_bounds[i].lb), variables_bounds[i].ub);
   }
   this->analyze_constraints(number_variables, number_constraints, direction);
   return direction;
}

// save Hessian (in arbitrary format) to a "weak" CSC format: compressed columns but row indices are not sorted, nor unique
void BQPDSolver::save_hessian_to_local_format(const SymmetricMatrix& hessian) {
   const size_t header_size = 1;
   // pointers withing the single array
   int* row_indices = &this->hessian_sparsity[header_size];
   int* column_starts = &this->hessian_sparsity[header_size + hessian.number_nonzeros];
   // header
   this->hessian_sparsity[0] = static_cast<int>(hessian.number_nonzeros + 1);
   // count the elements in each column
   for (size_t j = 0; j < hessian.dimension + 1; j++) {
      column_starts[j] = 0;
   }
   hessian.for_each([&](size_t /*i*/, size_t j, double /*entry*/) {
      column_starts[j + 1]++;
   });
   // carry over the column starts
   for (size_t j = 1; j < hessian.dimension + 1; j++) {
      column_starts[j] += column_starts[j-1];
      column_starts[j-1] += this->fortran_shift;
   }
   column_starts[hessian.dimension] += this->fortran_shift;
   // copy the entries
   std::vector<int> current_indices(hessian.dimension);
   hessian.for_each([&](size_t i, size_t j, double entry) {
      const size_t index = static_cast<size_t>(column_starts[j] + current_indices[j] - this->fortran_shift);
      assert(index <= column_starts[j+1] && "BQPD: error in converting the Hessian matrix to the local format");
      this->hessian_values[index] = entry;
      row_indices[index] = static_cast<int>(i) + this->fortran_shift;
      current_indices[j]++;
   });
   DEBUG << "Hessian: " << hessian;
}

void BQPDSolver::save_gradients_to_local_format(size_t number_constraints, const SparseVector<double>& linear_objective,
      const std::vector<SparseVector<double>>& constraint_jacobian) {
   size_t current_index = 0;
   linear_objective.for_each([&](size_t i, double derivative) {
      this->jacobian[current_index] = derivative;
      this->jacobian_sparsity[current_index + 1] = static_cast<int>(i) + this->fortran_shift;
      current_index++;
   });
   for (size_t j = 0; j < number_constraints; j++) {
      constraint_jacobian[j].for_each([&](size_t i, double derivative) {
         this->jacobian[current_index] = derivative;
         this->jacobian_sparsity[current_index + 1] = static_cast<int>(i) + this->fortran_shift;
         current_index++;
      });
   }
   current_index++;
   this->jacobian_sparsity[0] = static_cast<int>(current_index);
   // header
   size_t size = 1;
   this->jacobian_sparsity[current_index] = static_cast<int>(size);
   current_index++;
   size += linear_objective.size();
   this->jacobian_sparsity[current_index] = static_cast<int>(size);
   current_index++;
   for (size_t j = 0; j < number_constraints; j++) {
      size += constraint_jacobian[j].size();
      this->jacobian_sparsity[current_index] = static_cast<int>(size);
      current_index++;
   }
}

void BQPDSolver::analyze_constraints(size_t number_variables, size_t number_constraints, Direction& direction) {
   ConstraintPartition constraint_partition(number_constraints);

   // active constraints
   for (size_t j = 0; j < number_variables - this->k; j++) {
      const size_t index = static_cast<size_t>(std::abs(this->ls[j]) - this->fortran_shift);

      if (index < number_variables) {
         // bound constraint
         if (0 <= this->ls[j]) { // lower bound active
            direction.multipliers.lower_bounds[index] = this->residuals[index];
            direction.active_set.bounds.at_lower_bound.push_back(index);
         }
         else { // upper bound active */
            direction.multipliers.upper_bounds[index] = -this->residuals[index];
            direction.active_set.bounds.at_upper_bound.push_back(index);
         }
      }
      else {
         // general constraint
         size_t constraint_index = index - number_variables;
         constraint_partition.feasible.push_back(constraint_index);
         if (0 <= this->ls[j]) { // lower bound active
            direction.multipliers.constraints[constraint_index] = this->residuals[index];
            direction.active_set.constraints.at_lower_bound.push_back(constraint_index);
         }
         else { // upper bound active
            direction.multipliers.constraints[constraint_index] = -this->residuals[index];
            direction.active_set.constraints.at_upper_bound.push_back(constraint_index);
         }
      }
   }

   // inactive constraints
   for (size_t j = number_variables - this->k; j < number_variables + number_constraints; j++) {
      size_t index = std::abs(this->ls[j]) - this->fortran_shift;

      if (number_variables <= index) { // general constraints
         size_t constraint_index = index - number_variables;
         if (this->residuals[index] < 0.) { // infeasible constraint
            constraint_partition.infeasible.push_back(constraint_index);
            if (this->ls[j] < 0) { // upper bound violated
               constraint_partition.upper_bound_infeasible.push_back(constraint_index);
            }
            else { // lower bound violated
               constraint_partition.lower_bound_infeasible.push_back(constraint_index);
            }
         }
         else { // feasible constraint
            constraint_partition.feasible.push_back(constraint_index);
         }
      }
   }
   direction.constraint_partition = constraint_partition;
}

Status BQPDSolver::status_to_int(int ifail) {
   assert(0 <= ifail && ifail <= 9 && "BQPDSolver.status_to_int: ifail does not belong to [0, 9]");
   return static_cast<Status> (ifail);
}