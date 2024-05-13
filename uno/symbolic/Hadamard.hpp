// Copyright (c) 2018-2024 Charlie Vanaret
// Licensed under the MIT license. See LICENSE file in the project directory for details.

#ifndef UNO_HADAMARD_H
#define UNO_HADAMARD_H

// Hadamard (componentwise) product
template <typename E1, typename E2,
      typename std::enable_if_t<std::is_same_v<typename std::remove_reference_t<E1>::value_type, typename std::remove_reference_t<E2>::value_type>, int> = 0>
class Hadamard {
public:
   using value_type = typename std::remove_reference_t<E1>::value_type;

   Hadamard(E1&& expression1, E2&& expression2): expression1(std::forward<E1>(expression1)), expression2(std::forward<E2>(expression2)) { }

   [[nodiscard]] constexpr size_t size() const { return this->expression1.size(); }
   [[nodiscard]] typename Hadamard::value_type operator[](size_t index) const {
      const double first_term = this->expression1[index];
      if (first_term == 0.) {
         return 0.; // avoid evaluating the second expression
      }
      else {
         return first_term * this->expression2[index];
      }
   }

protected:
   E1 expression1;
   E2 expression2;
};

// free function
template <typename E1, typename E2>
inline Hadamard<E1, E2> hadamard(E1&& expression1, E2&& expression2) {
   return {std::forward<E1>(expression1), std::forward<E2>(expression2)};
}

#endif // UNO_HADAMARD_H