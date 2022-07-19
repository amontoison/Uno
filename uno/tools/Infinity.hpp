// Copyright (c) 2022 Charlie Vanaret
// Licensed under the MIT license. See LICENSE file in the project root for details.

#ifndef UNO_INFINITY_H
#define UNO_INFINITY_H

const double INF = std::numeric_limits<double>::infinity();

inline bool is_finite(double value) {
   return std::abs(value) < INF;
}

#endif // UNO_INFINITY_H