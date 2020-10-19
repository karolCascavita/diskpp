/*
 * DISK++, a template library for DIscontinuous SKeletal methods.
 *  
 * Matteo Cicuttin (C) 2020
 * matteo.cicuttin@uliege.be
 *
 * University of Liège - Montefiore Institute
 * Applied and Computational Electromagnetics group  
 */

#include <iomanip>
#include <iostream>
#include <regex>

#include <unistd.h>

#include "bases/bases.hpp"
#include "methods/hho"
#include "methods/implementation_hho/curl.hpp"
#include "quadratures/quadratures.hpp"

#include "core/loaders/loader.hpp"

#include "common.hpp"

template<typename Mesh, bool mixed>
struct test_functor_curl_reconstruction
{
    typename Mesh::coordinate_type
    operator()(const Mesh& msh, size_t degree) const
    {
        return 0;
    }

    size_t
    expected_rate(size_t k)
    {
        return k + 50;
    } 
};

template<template<typename, size_t, typename> class Mesh, typename T, typename Storage, bool mixed>
struct test_functor_curl_reconstruction<Mesh<T,2,Storage>, mixed>
{
    using mesh_type = Mesh<T,2,Storage>;
    /* Expect k+1 convergence (hho stabilization) */
    typename mesh_type::coordinate_type
    operator()(const mesh_type& msh, size_t degree) const
    {
        typedef typename mesh_type::cell            cell_type;
        typedef typename mesh_type::face            face_type;
        typedef typename mesh_type::coordinate_type scalar_type;
        typedef typename mesh_type::point_type      point_type;

        typedef Matrix<scalar_type, mesh_type::dimension, 1> ret_type;

        auto f = [](const point_type& pt) -> T {
            return std::sin(M_PI*pt.x())*std::sin(M_PI*pt.y());
        };

        size_t fd = degree;
        size_t cd = mixed ? degree+1 : degree;
        size_t rd = degree+1;

        disk::hho_degree_info hdi( { .rd = rd, .cd = cd, .fd = fd } );

        scalar_type error = 0.0;
        for (auto& cl : msh)
        {
            auto CR = disk::curl_reconstruction(msh, cl, hdi);
            auto proj = disk::project_tangent(msh, cl, hdi, f);
            Matrix<T, Dynamic, 1> rf = CR.first * proj;
            auto rb = disk::make_scalar_monomial_basis(msh, cl, rd);

            Matrix<T, Dynamic, Dynamic> mass         = disk::make_mass_matrix(msh, cl, rb);
            Matrix<T, Dynamic, 1>       rhs          = disk::make_rhs(msh, cl, rb, f);
            Matrix<T, Dynamic, 1>       exp_reconstr = mass.ldlt().solve(rhs);
            Matrix<T, Dynamic, 1>       diff = exp_reconstr;
            diff.head(1) = exp_reconstr.head(1);
            diff.tail(rb.size()-1) -= rf;
        
            Matrix<T, Dynamic, Dynamic> CC = make_curl_curl_matrix(msh, cl, rb);
            error += diff.dot(CC * diff);

        }

        return std::sqrt(error);
    }

    size_t
    expected_rate(size_t k)
    {
        return k + 1;
    }
};

template<template<typename, size_t, typename> class Mesh, typename T, typename Storage, bool mixed>
struct test_functor_curl_reconstruction<Mesh<T,3,Storage>, mixed>
{
    using mesh_type = Mesh<T,3,Storage>;
    /* Expect k+1 convergence (hho stabilization) */
    typename mesh_type::coordinate_type
    operator()(const mesh_type& msh, size_t degree) const
    {
        typedef typename mesh_type::cell            cell_type;
        typedef typename mesh_type::face            face_type;
        typedef typename mesh_type::coordinate_type scalar_type;
        typedef typename mesh_type::point_type      point_type;

        typedef Matrix<scalar_type, mesh_type::dimension, 1> ret_type;

        auto f = [](const point_type& pt) -> Matrix<T,3,1> {
            Matrix<T,3,1> ret;
            ret(0) = std::sin(M_PI*pt.y());
            ret(1) = std::sin(M_PI*pt.z());
            ret(2) = std::sin(M_PI*pt.x());
            return ret;
        };

        size_t fd = degree;
        size_t cd = mixed ? degree+1 : degree;
        size_t rd = degree+1;

        disk::hho_degree_info hdi( { .rd = rd, .cd = cd, .fd = fd } );

        scalar_type error = 0.0;
        for (auto& cl : msh)
        {
            auto CR = disk::curl_reconstruction(msh, cl, hdi);
            auto proj = disk::project_tangent(msh, cl, hdi, f, 2);
            Matrix<T, Dynamic, 1> rf = CR.first * proj;
            auto rb = disk::make_vector_monomial_basis(msh, cl, rd);

            Matrix<T, Dynamic, Dynamic> mass         = disk::make_mass_matrix(msh, cl, rb);
            Matrix<T, Dynamic, 1>       rhs          = disk::make_rhs(msh, cl, rb, f);
            Matrix<T, Dynamic, 1>       exp_reconstr = mass.ldlt().solve(rhs);
            Matrix<T, Dynamic, 1>       diff = exp_reconstr;
            diff.head(3) = exp_reconstr.head(3);
            diff.tail(rb.size()-3) -= rf;
        
            //Matrix<T, Dynamic, Dynamic> CC = make_curl_curl_matrix(msh, cl, rb);
            //error += diff.dot(CC * diff);

            auto cb = disk::make_vector_monomial_basis(msh, cl, cd);
            Matrix<T, Dynamic, Dynamic> cc = make_curl_curl_matrix(msh, cl, cb);

            Matrix<T, Dynamic, Dynamic> tr = Matrix<T, Dynamic, Dynamic>::Zero(cb.size(), rb.size());
            Matrix<T, Dynamic, Dynamic> mm = Matrix<T, Dynamic, Dynamic>::Zero(cb.size(), cb.size());

            auto qps = integrate(msh, cl, 2*rd);
            for (auto& qp : qps)
            {
                auto r_phi = rb.eval_functions(qp.point());
                auto c_phi = cb.eval_functions(qp.point());
                tr += qp.weight() * c_phi * r_phi.transpose();
                mm += qp.weight() * c_phi * c_phi.transpose();
            }

            Matrix<T, Dynamic, Dynamic> H = mm.ldlt().solve(tr);

            Matrix<T, Dynamic, 1> dh = H*diff;

            error += dh.dot(cc*dh);


        }

        return std::sqrt(error);
    }

    size_t
    expected_rate(size_t k)
    {
        return k + 1;
    }
};

template<typename Mesh>
using test_functor_curl_reconstruction_eo = test_functor_curl_reconstruction<Mesh, false>;

template<typename Mesh>
using test_functor_curl_reconstruction_mo = test_functor_curl_reconstruction<Mesh, true>;

int
main(void)
{
    std::cout << red << "Test HHO curl reconstruction operator" << std::endl;
    // face order: k, cell order: k
    std::cout << cyan << "Face order: k and Cell order: k" << std::endl;
    tester<test_functor_curl_reconstruction_eo> tstr1;
    tstr1.run();

    std::cout << cyan << "Face order: k and Cell order: k+1" << std::endl;
    tester<test_functor_curl_reconstruction_mo> tstr2;
    tstr2.run();

    return 0;
}
