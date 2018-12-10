/*
 *       /\         DISK++, a template library for DIscontinuous SKeletal
 *      /__\        methods.
 *     /_\/_\
 *    /\    /\      Matteo Cicuttin (C) 2016, 2017, 2018
 *   /__\  /__\     matteo.cicuttin@enpc.fr
 *  /_\/_\/_\/_\    École Nationale des Ponts et Chaussées - CERMICS
 *
 * This file is copyright of the following authors:
 * Matteo Cicuttin (C) 2016, 2017, 2018         matteo.cicuttin@enpc.fr
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * If you use this code or parts of it for scientific publications, you
 * are required to cite it as following:
 *
 * Implementation of Discontinuous Skeletal methods on arbitrary-dimensional,
 * polytopal meshes using generic programming.
 * M. Cicuttin, D. A. Di Pietro, A. Ern.
 * Journal of Computational and Applied Mathematics.
 * DOI: 10.1016/j.cam.2017.09.017
 */


#include <iostream>
#include <iomanip>
#include <regex>

#include <unistd.h>

#include <xmmintrin.h>
//#define EIGEN_USE_MKL_ALL
#include "bases/bases.hpp"
#include "quadratures/quadratures.hpp"
#include "methods/hho"

#include "core/loaders/loader.hpp"

#include "common.hpp"


template<typename Mesh>
struct test_functor
{
    /* Expect k+1 convergence */
    typename Mesh::coordinate_type
    operator()(const Mesh& msh, size_t degree) const
    {
        typedef Mesh mesh_type;
        typedef typename mesh_type::cell        cell_type;
        typedef typename mesh_type::face        face_type;
        typedef typename mesh_type::coordinate_type scalar_type;
        typedef typename mesh_type::point_type  point_type;


        auto f = make_scalar_testing_data(msh);

        typename disk::hho_degree_info hdi(degree);

        scalar_type error = 0.0;
        for (auto& cl : msh)
        {
            Matrix<scalar_type, Dynamic, 1> proj = disk::project_function(msh, cl, hdi, f, 2);
            auto gr = disk::make_scalar_hho_laplacian(msh, cl, hdi);

            size_t rec_size = disk::scalar_basis_size(hdi.reconstruction_degree(), Mesh::dimension);

            Matrix<scalar_type, Dynamic, 1> reconstr = Matrix<scalar_type, Dynamic, 1>::Zero(rec_size);
            reconstr.tail(rec_size-1) = gr.first * proj;
            reconstr(0) = proj(0);

            auto cb = disk::make_scalar_monomial_basis(msh, cl, hdi.reconstruction_degree());
            Matrix<scalar_type, Dynamic, Dynamic> mass = disk::make_mass_matrix(msh, cl, cb);
            Matrix<scalar_type, Dynamic, 1> rhs = disk::make_rhs(msh, cl, cb, f);
            Matrix<scalar_type, Dynamic, 1> exp_reconstr = mass.llt().solve(rhs);

            Matrix<scalar_type, Dynamic, 1> diff = reconstr - exp_reconstr;

            Matrix<scalar_type, Dynamic, Dynamic> stiffness = disk::make_stiffness_matrix(msh, cl, cb);

            error += diff.dot(stiffness*diff);
        }

        return std::sqrt(error);
    }
};

template<typename Mesh>
test_functor<Mesh>
get_test_functor(const std::vector<Mesh>& meshes)
{
    return test_functor<Mesh>();
}

void test_triangles_generic(void)
{
    std::cout << "*** TESTING TRIANGLES ON GENERIC MESH ***" << std::endl;
    using T = double;

    auto meshes = get_triangle_generic_meshes<T>();
    auto tf = get_test_functor(meshes);
    do_testing(meshes, tf);
}

void test_triangles_netgen(void)
{
    std::cout << "*** TESTING TRIANGLES ON NETGEN MESH ***" << std::endl;
    using T = double;

    auto meshes = get_triangle_netgen_meshes<T>();
    auto tf = get_test_functor(meshes);
    do_testing(meshes, tf);
}

void test_quads(void)
{
    std::cout << "*** TESTING QUADS ON GENERIC MESH ***" << std::endl;
    using T = double;

    auto meshes = get_quad_generic_meshes<T>();
    auto tf = get_test_functor(meshes);
    do_testing(meshes, tf);
}

void test_tetrahedra_netgen(void)
{
    std::cout << "*** TESTING TETRAHEDRONS ON NETGEN MESH ***" << std::endl;
    using T = double;

    auto meshes = get_tetrahedra_netgen_meshes<T>();
    auto tf = get_test_functor(meshes);
    do_testing(meshes, tf);
}

void test_cartesian_diskpp(void)
{
    std::cout << "*** TESTING CARTESIAN MESH ***" << std::endl;
    using T = double;

    auto meshes = get_cartesian_diskpp_meshes<T>();
    auto tf = get_test_functor(meshes);
    do_testing(meshes, tf);
}

void test_generic_fvca6(void)
{
    std::cout << "*** TESTING GENERIC FVCA6 MESH ***" << std::endl;
    using T = double;

    auto meshes = get_generic_fvca6_meshes<T>();
    auto tf = get_test_functor(meshes);
    do_testing(meshes, tf);
}

int main(void)
{
    //_MM_SET_EXCEPTION_MASK(_MM_GET_EXCEPTION_MASK() & ~_MM_MASK_INVALID);

    test_triangles_generic();
    test_triangles_netgen();
    test_quads();
    test_tetrahedra_netgen();
    test_cartesian_diskpp();
    test_generic_fvca6();

    return 0;
}
