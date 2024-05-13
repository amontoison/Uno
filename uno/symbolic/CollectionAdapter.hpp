// Copyright (c) 2018-2024 Charlie Vanaret
// Licensed under the MIT license. See LICENSE file in the project directory for details.

#ifndef UNO_COLLECTIONADAPTER_H
#define UNO_COLLECTIONADAPTER_H

#include "Collection.hpp"

// Adapter to adapt standard Arrays (e.g. std::vector) to the Collection interface
template <typename Array>
class CollectionAdapter: public Collection<typename std::remove_reference_t<Array>::value_type> {
public:
   // using iterator = typename std::remove_reference_t<Array>::iterator;

   explicit CollectionAdapter(Array&& array);

   void for_each(const std::function<void(size_t /*index*/, typename CollectionAdapter::value_type /*element*/)>& f) const override;
   [[nodiscard]] size_t size() const override;

   // [[nodiscard]] iterator begin() const { return this->array.begin(); }
   // [[nodiscard]] iterator end() const  { return this->array.end(); }

protected:
   Array array;
};

template <typename Array>
CollectionAdapter<Array>::CollectionAdapter(Array&& array):
   Collection<typename std::remove_reference_t<Array>::value_type>(), array(std::forward<Array>(array)) {
}

template <typename Array>
void CollectionAdapter<Array>::for_each(const std::function<void(size_t /*index*/, typename CollectionAdapter::value_type /*element*/)>& f) const {
   size_t index = 0;
   for (auto element: this->array) {
      f(index, element);
      index++;
   }
}

template <typename Array>
size_t CollectionAdapter<Array>::size() const {
   return this->array.size();
}

// free function
template <typename Array>
CollectionAdapter<Array> adapt(Array&& array) {
   return CollectionAdapter(std::forward<Array>(array));
}

#endif // UNO_COLLECTIONADAPTER_H
