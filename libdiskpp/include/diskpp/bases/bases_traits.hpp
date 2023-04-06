#pragma once

namespace disk::basis {

template<typename Trial, typename Test>
struct can_take_scalar_product {
    static const bool value = (Trial::tensor_order == Test::tensor_order);
};

template<size_t order>
struct basis_category_tag
{};

using scalar_basis_tag = basis_category_tag<0>;
using vector_basis_tag = basis_category_tag<1>;
using matrix_basis_tag = basis_category_tag<2>;

template<typename Basis>
struct basis_traits
{
    using category = basis_category_tag<Basis::tensor_order>;
};






template<typename T, size_t dimension, size_t order>
struct tensor {
    static_assert(sizeof(T) == -1, "invalid tensor dimension or order");
};

template<typename T, size_t DIM>
struct tensor<T, DIM, 0> {
    static const size_t order = 0;
    static const size_t dimension = DIM;
    using scalar_type = T;
    using value_type = scalar_type;
    using array_type = Eigen::Matrix<scalar_type, Eigen::Dynamic, 1>;
};

template<typename T, size_t DIM>
struct tensor<T, DIM, 1> {
    static const size_t order = 1;
    static const size_t dimension = DIM;
    using scalar_type = T;
    using value_type = Eigen::Matrix<T, dimension, 1>;
    using array_type = Eigen::Matrix<T, Eigen::Dynamic, dimension>;
};

template<typename T>
struct tensor<T, 0, 0> {
    static const size_t order = 0;
    static const size_t dimension = 0;
    using scalar_type = T;
    using value_type = scalar_type;
    using array_type = scalar_type;
};

template<typename T>
struct tensor<T, 0, 1> {
    static const size_t order = 1;
    static const size_t dimension = 0;
    using scalar_type = T;
    using value_type = Eigen::Matrix<T, 1, 1>;
    using array_type = Eigen::Matrix<T, 1, 1>;
};

} // namespace disk::basis