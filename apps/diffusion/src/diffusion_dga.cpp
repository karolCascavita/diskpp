/*
 * DISK++, a template library for DIscontinuous SKeletal methods.
 *
 * Matteo Cicuttin (C) 2024
 * matteo.cicuttin@polito.it
 *
 * Politecnico di Torino - DISMA
 * Dipartimento di Matematica
 */
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

#include "diskpp/methods/dga"
#include "diskpp/loaders/loader.hpp"
#include "diskpp/mesh/meshgen.hpp"
#include "diskpp/output/silo.hpp"

#include "mumps.hpp"

int main(int argc, char **argv)
{
    using T = double;

    auto rhs_fun = [](const typename disk::simplicial_mesh<T, 3>::point_type& pt) -> auto {
        return 3.0 * M_PI * M_PI * sin(M_PI*pt.x()) * sin(M_PI*pt.y()) * sin(M_PI*pt.z());
    };

    auto sol_fun = [](const typename disk::simplicial_mesh<T, 3>::point_type& pt) -> auto {
        return sin(M_PI*pt.x()) * sin(M_PI*pt.y()) * sin(M_PI*pt.z());
    };

    /*
    if (argc != 2)
    {
        std::cout << "Please specify file name." << std::endl;
        return 1;
    }

    char *meshfile = argv[1];

    typedef disk::simplicial_mesh<T, 3>  mesh_type;

    mesh_type msh;
    disk::netgen_mesh_loader<T, 3> loader;

    if (!loader.read_mesh(meshfile))
    {
        std::cout << "Problem loading mesh." << std::endl;
        return 1;
    }
    loader.populate_mesh(msh);
    */

    using mesh_type = disk::simplicial_mesh<T, 3>;

    mesh_type msh;
    auto mesher = make_simple_mesher(msh);
    for (int i = 0; i < 4; i++)
        mesher.refine();

    msh.statistics();

    typedef Eigen::SparseMatrix<T>  sparse_matrix_type;
    typedef Eigen::Triplet<T>       triplet_type;

    std::vector<bool> dirichlet_nodes( msh.points_size() );
    for (auto itor = msh.boundary_faces_begin(); itor != msh.boundary_faces_end(); itor++)
    {
        auto fc = *itor;
        auto ptids = fc.point_ids();
        dirichlet_nodes.at(ptids[0]) = true;
        dirichlet_nodes.at(ptids[1]) = true;
        dirichlet_nodes.at(ptids[2]) = true;
    }

    std::vector<size_t> compress_map, expand_map;
    compress_map.resize( msh.points_size() );
    size_t system_size = std::count_if(dirichlet_nodes.begin(), dirichlet_nodes.end(), [](bool d) -> bool {return !d;});
    expand_map.resize( system_size );

    auto nnum = 0;
    for (size_t i = 0; i < msh.points_size(); i++)
    {
        if ( dirichlet_nodes.at(i) )
            continue;

        expand_map.at(nnum) = i;
        compress_map.at(i) = nnum++;
    }

    sparse_matrix_type              gA(system_size, system_size);
    disk::dynamic_vector<T>               gb(system_size), gx(system_size);

    gb = disk::dynamic_vector<T>::Zero(system_size);


    std::vector<triplet_type>       triplets;


    for (auto& cl : msh)
    {
        auto G = grad_matrix(msh, cl);
        auto E = edge_matrix(msh, cl, 1.0);
        Eigen::Matrix<T,4,4> lapl = G.transpose() * E * G;

        auto ptids = cl.point_ids();
        auto pts = points(msh, cl);
        auto vol = disk::priv::volume_unsigned(msh, cl);

        for (size_t i = 0; i < lapl.rows(); i++)
        {
            if ( dirichlet_nodes.at(ptids[i]) )
                continue;

            for (size_t j = 0; j < lapl.cols(); j++)
            {
                if ( dirichlet_nodes.at(ptids[j]) )
                    continue;

                triplets.push_back( triplet_type(compress_map.at(ptids[i]),
                                                 compress_map.at(ptids[j]),
                                                 lapl(i,j)) );
            }

            auto rhs_val = rhs_fun(pts[i]);
            gb(compress_map.at(ptids[i])) += rhs_val * vol * 0.25;
        }
    }

    gA.setFromTriplets(triplets.begin(), triplets.end());

    std::cout << "Mesh elements: " << msh.cells_size() << std::endl;
    std::cout << "Dofs: " << gA.rows() << std::endl;

    std::cout << "Running MUMPS" << std::endl;
    gx = mumps_lu(gA, gb);

    disk::dynamic_vector<T> sol = disk::dynamic_vector<T>::Zero( msh.points_size() );

    for (size_t i = 0; i < gx.size(); i++)
        sol(expand_map.at(i)) = gx(i);

    std::ofstream ofs("lapl.dat");
    for (size_t i = 0; i < msh.points_size(); i++)
    {
        auto pt = *std::next(msh.points_begin(), i);
        ofs << pt.x() << " " << pt.y() << " " << pt.z() << " " << sol(i) << std::endl;
    }

    ofs.close();

    disk::silo_database silo;
    silo.create("diffusion_dga.silo");
    silo.add_mesh(msh, "mesh");
    silo.add_variable("mesh", "u", sol, disk::nodal_variable_t);

    T error = 0.0;
    for (auto& cl : msh)
    {
        auto G = grad_matrix(msh, cl);
        auto E = edge_matrix(msh, cl, 1.0);
        Eigen::Matrix<T,4,4> lapl = G.transpose() * E * G;

        auto ptids = cl.point_ids();
        auto pts = points(msh, cl);

        Eigen::Matrix<T,4,1> realsol;
        Eigen::Matrix<T,4,1> compsol;

        for (size_t i = 0; i < 4; i++)
        {
            realsol(i) = sol_fun(pts[i]);
            compsol(i) = sol( size_t(ptids[i]) );

        }

        auto diff = realsol - compsol;
        //std::cout << "***" << std::endl;
        //std::cout << realsol.transpose() << std::endl;
        //std::cout << compsol.transpose() << std::endl;

        error += diff.dot(lapl*diff);
    }


    std::cout << "Error: " << std::sqrt(error) << std::endl;

    return 0;
}
