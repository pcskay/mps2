// SPDX-License-Identifier: LGPL-3.0-only

/*
* Author: Rongyang Sun <sun-rongyang@outlook.com>
* Creation Date: 2020-08-19 16:40
* 
* Description: GraceQ/MPS2 project. A fix size vector which supports maintaining
*              elements using a reference (the memory managed by this class) or
*              a pointer (the memory managed by user themselves).
*/

/**
@file duovector.h
@brief A fix size vector supporting elements maintaining by reference or pointer.
*/
#ifndef GQMPS2_ONE_DIM_TN_FRAMEWORK_DUOVECTOR_H
#define GQMPS2_ONE_DIM_TN_FRAMEWORK_DUOVECTOR_H


#include <vector>     // vector


namespace gqmps2 {


/**
A fix size vector supporting elements maintaining by reference or pointer.

@tparam ElemT Type of the elements.
*/
template <typename ElemT>
class DuoVector {
public:
  /**
  Create a DuoVector using its size.

  @param size The size of the DuoVector.
  */
  DuoVector(const size_t size) : raw_data_(size, nullptr) {}

  /**
  Destruct a DuoVector. Release memory it maintained.
  */
  ~DuoVector(void) {
    for (auto &rpelem : raw_data_) {
      if (rpelem != nullptr) {
        delete rpelem;
      }
    }
  }

  // Data access methods.
  /**
  Element getter.

  @param idx The index of the element.
  */
  const ElemT &operator[](const size_t idx) const { return *raw_data_[idx]; }

  /**
  Element setter. If the corresponding memory has not been allocated, allocate
  it first.

  @param idx The index of the element.
  */
  ElemT &operator[](const size_t idx) {
    if (raw_data_[idx] == nullptr) {
      raw_data_[idx] = new ElemT;
    }
    return *raw_data_[idx];
  }

  /**
  Pointer-of-element getter.

  @param idx The index of the element.
  */
  const ElemT *operator()(const size_t idx) const {return raw_data_[idx]; }

  /**
  Pointer-of-element setter.

  @param idx The index of the element.
  */
  ElemT * &operator()(const size_t idx) { return raw_data_[idx]; }

  /**
  Read-only raw data access.
  */
  const std::vector<const ElemT *> cdata(void) const { 
    std::vector<const ElemT *> craw_data;
    for (auto &rpelem : raw_data_) {
      craw_data.push_back(rpelem);
    }
    return craw_data;
  }

  // Memory management methods
  /**
  Allocate memory of the element at given index. If the given place has a
  non-nullptr, release the memory which point to first.

  @param idx The index of the element.
  */
  void alloc(const size_t idx) {
    if (raw_data_[idx] != nullptr) {
      delete raw_data_[idx];
    }
    raw_data_[idx] = new ElemT;
  }

  /**
  Deallocate memory of the element at given index.

  @param idx The index of the element.
  */
  void dealloc(const size_t idx) {
    if (raw_data_[idx] != nullptr) {
      delete raw_data_[idx];
      raw_data_[idx] = nullptr;
    }
  }

  // Property methods.
  /**
  Get the size of the DuoVector .
  */
  size_t size(void) const { return raw_data_.size(); }

private:
  std::vector<ElemT *> raw_data_;
};
} /* gqmps2 */ 
#endif /* ifndef GQMPS2_ONE_DIM_TN_FRAMEWORK_DUOVECTOR_H */
