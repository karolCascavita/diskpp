/*
 *       /\
 *      /__\       Matteo Cicuttin (C) 2016, 2017 - matteo.cicuttin@enpc.fr
 *     /_\/_\      École Nationale des Ponts et Chaussées - CERMICS
 *    /\    /\
 *   /__\  /__\    DISK++, a template library for DIscontinuous SKeletal
 *  /_\/_\/_\/_\   methods.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * If you use this code for scientific publications, you are required to
 * cite it.
 */

/*
 * This source file is part of EMT, the ElectroMagneticTool.
 *
 * Copyright (C) 2013-2015, Matteo Cicuttin - matteo.cicuttin@uniud.it
 * Department of Electrical Engineering, University of Udine
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University of Udine nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(s) ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR(s) BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#pragma once

#include <iostream>
#include <vector>
#include <complex>
#include <array>
#include <type_traits>
#include <fstream>

#include "common/eigen.hpp"

/****************************************************************************/
// feast prototypes
extern "C" {

void feastinit(int *);

void sfeast_scsrev(const char& uplo, const int& n,
                   const float * a, const int * ia, const int * ja,
                   int * fpm, float& epsout, int& loop,
                   const float& emin, const float& emax, int& m0,
                   float * e, float * x, int& m, float * res, int& info);

void dfeast_scsrev(const char& uplo, const int& n,
                   const double * a, const int * ia, const int * ja,
                   int * fpm, double& epsout, int& loop,
                   const double& emin, const double& emax, int& m0,
                   double * e, double * x, int& m, double * res, int& info);

void cfeast_hcsrev(const char& uplo, const int& n,
                   const std::complex<float> * a, const int * ia, const int * ja,
                   int * fpm, float& epsout, int& loop,
                   const float& emin, const float& emax, int& m0,
                   float * e, std::complex<float> * x, int& m,
                   float * res, int& info);

void zfeast_hcsrev(const char& uplo, const int& n,
                   const std::complex<double> * a, const int * ia, const int * ja,
                   int * fpm, double& epsout, int& loop,
                   const double& emin, const double& emax, int& m0,
                   double * e, std::complex<double> * x, int& m,
                   double * res, int& info);

void sfeast_scsrgv(const char& uplo, const int& n,
                   const float * a, const int * ia, const int * ja,
                   const float * b, const int * ib, const int * jb,
                   int * fpm, float& epsout, int& loop,
                   const float& emin, const float& emax, int& m0,
                   float * e, float * x, int& m, float * res, int& info);

void dfeast_scsrgv(const char& uplo, const int& n,
                   const double * a, const int * ia, const int * ja,
                   const double * b, const int * ib, const int * jb,
                   int * fpm, double& epsout, int& loop,
                   const double& emin, const double& emax, int& m0,
                   double * e, double * x, int& m, double * res, int& info);

void cfeast_hcsrgv(const char& uplo, const int& n,
                   const std::complex<float> * a, const int * ia, const int * ja,
                   const std::complex<float> * b, const int * ib, const int * jb,
                   int * fpm, float& epsout, int& loop,
                   const float& emin, const float& emax, int& m0,
                   float * e, std::complex<float> * x, int& m,
                   float * res, int& info);

void zfeast_hcsrgv(const char& uplo, const int& n,
                   const std::complex<double> * a, const int * ia, const int * ja,
                   const std::complex<double> * b, const int * ib, const int * jb,
                   int * fpm, double& epsout, int& loop,
                   const double& emin, const double& emax, int& m0,
                   double * e, std::complex<double> * x, int& m,
                   double * res, int& info);

} // extern "C"

#define FEASTPARM_LEN   128

template<typename T>
class eigen_sparse_raw
{
    bool m_one_based, m_already_restored;
public:

    T *     data;
    int *   ia;
    int *   ja;
    int     nnz;
    int     n;

    template<int Options, typename Index>
    eigen_sparse_raw(Eigen::SparseMatrix<T, Options, Index>& A, bool one_based = false)
        : m_one_based(one_based), m_already_restored(false)
    {
        A.makeCompressed();

        data    = A.valuePtr();
        ia      = A.outerIndexPtr();    //colmaj size(ia) == N+1
        ja      = A.innerIndexPtr();    //colmaj size(ja) == size(data) == nnz
        nnz     = A.nonZeros();
        n       = A.rows();

        if (one_based)
        {
            for (size_t i = 0; i < n+1; i++)
                ia[i] += 1;

            for (size_t i = 0; i < nnz; i++)
                ja[i] += 1;
        }
    }

    void restore(void)
    {
        if ( !m_one_based || m_already_restored )
            return;

        for (size_t i = 0; i < n+1; i++)
            ia[i] -= 1;

        for (size_t i = 0; i < nnz; i++)
            ja[i] -= 1;

        m_already_restored = true;
    }

    void show(void)
    {
        std::cout << "A: ";
        for (size_t i = 0; i < nnz; i++)
            std::cout << data[i] << " ";
        std::cout << std::endl;

        std::cout << "ja: ";
        for (size_t i = 0; i < nnz; i++)
            std::cout << ja[i] << " ";
        std::cout << std::endl;

        std::cout << "ia: ";
        for (size_t i = 0; i < n+1; i++)
            std::cout << ia[i] << " ";
        std::cout << std::endl;
    }

    ~eigen_sparse_raw()
    {
        if ( m_one_based && !m_already_restored )
            restore();
    }
};

void dump_sparse_matrix(Eigen::SparseMatrix<double>& M, const std::string& filename)
{
    std::ofstream ofs(filename);

    for (int k=0; k < M.outerSize(); ++k)
        for (Eigen::SparseMatrix<double>::InnerIterator it(M,k); it; ++it)
            ofs << it.row() << " " << it.col() << " " << it.value() << std::endl;

    ofs.close();
}

template<typename T>
struct feast_eigensolver_params
{
    bool    verbose;
    int     tolerance;
    T       min_eigval, max_eigval;
    int     subspace_size;
    int     eigvals_found;
    int     feast_info;
};

template<int _Options, typename _Index>
bool
generalized_eigenvalue_solver(feast_eigensolver_params<double>& params,
                              Eigen::SparseMatrix<double, _Options, _Index>& L,
                              Eigen::SparseMatrix<double, _Options, _Index>& R,
                              Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>& eigvecs,
                              Eigen::Matrix<double, Eigen::Dynamic, 1>& eigvals)
{
    if ( L.rows() != R.rows() && L.cols() != R.cols() && L.rows() == L.cols() )
    {
        std::cout << "Two square matrices of the same size are needed." << std::endl; 
        return false;
    }

    std::array<int, FEASTPARM_LEN>    fpm;
    feastinit(fpm.data());

    auto Lc = eigen_sparse_raw<double>(L, true);
    auto Rc = eigen_sparse_raw<double>(R, true);
    int N = Lc.n;

    if (params.verbose)
        fpm.at(0) = 1;

    if (params.tolerance > 0 && params.tolerance < 16)
        fpm.at(2) = params.tolerance;
    else
    {
        std::cout << "Invalid tolerance." << std::endl;
        return false;
    }

    if (params.subspace_size < 1 || params.subspace_size > N)
    {
        std::cout << "Invalid subspace size." << std::endl;
        return false;
    }

    double  trace, eps;
    int     loop;

    Eigen::Matrix<double, Eigen::Dynamic, 1> res;
    res     = Eigen::Matrix<double, Eigen::Dynamic, 1>::Zero(params.subspace_size);
    eigvals = Eigen::Matrix<double, Eigen::Dynamic, 1>::Zero(params.subspace_size);
    eigvecs = Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>::Zero(N, params.subspace_size);

    dfeast_scsrgv('F', N, Lc.data, Lc.ia, Lc.ja, Rc.data, Rc.ia, Rc.ja,
                       fpm.data(), eps, loop,
                       params.min_eigval,
                       params.max_eigval,
                       params.subspace_size,
                       eigvals.data(),
                       eigvecs.data(),
                       params.eigvals_found,
                       res.data(),
                       params.feast_info);

    return true;
}

/*

bool test_eigenvalue_solver(void)
{
    int N = 256;

    int aN = N-2;
    double h = 1./N;
    Eigen::SparseMatrix<double> K(aN, aN);
    Eigen::SparseMatrix<double> M(aN, aN);

    typedef Eigen::Triplet<double> triplet_type;

    std::vector<triplet_type> tK, tM;

    for (size_t i = 0; i < aN; i++)
    {
        if (i > 0)
        {
            tK.push_back( triplet_type(i-1, i, -1.0) );
            tM.push_back( triplet_type(i-1, i,  1.0/6.0) );
        }

        tK.push_back( triplet_type(i, i, 2.0) );
        tM.push_back( triplet_type(i, i, 4.0/6.0) );

        if (i < aN-1)
        {
            tK.push_back( triplet_type(i+1, i, -1.0) );
            tM.push_back( triplet_type(i+1, i,  1.0/6.0) );
        }
    }

    K.setFromTriplets(tK.begin(), tK.end());
    M.setFromTriplets(tM.begin(), tM.end());

    K *= 1./(h*h);

    Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> eigvecs;
    Eigen::Matrix<double, Eigen::Dynamic, 1> eigvals;

    generalized_eigenvalue_solver(K, M, eigvecs, eigvals);

    std::cout << eigvals << std::endl;

    return true;
}

*/














