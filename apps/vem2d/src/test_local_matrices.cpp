

#include <iostream>
#include <iomanip>
#include <regex>

#include <unistd.h>

#include "diskpp/loaders/loader.hpp"
#include "diskpp/methods/vem"
#include "diskpp/mesh/meshgen.hpp"

int main(int argc, char **argv)
{
    using T = double;
    size_t degree = 2;
    disk::generic_mesh<T,2> msh;
    using point_type = disk::generic_mesh<T, 2>::point_type;

    std::string mesh_filename;
    if (argc > 1)
    {
        mesh_filename = argv[1];

        /* Single element CSV 2D */
        if (std::regex_match(mesh_filename, std::regex(".*\\.csv$") ))
        {
            std::cout << "Guessed mesh format: CSV 2D" << std::endl;
            disk::load_single_element_csv(msh, mesh_filename);
        }
    }
    else
    {
        double radius = 1.0;
        size_t num_faces = 5;
        disk::make_single_element_mesh(msh, radius, num_faces);
    }

    std::cout << "LEX ordering" << std::endl;
    for (auto& cl : msh) {
        auto fcs = faces(msh, cl);
        for (auto& fc : fcs)
            std::cout << fc << std::endl;
    }

    std::cout << "CCW ordering" << std::endl;
    for (auto& cl : msh) {
        auto fcs_ccw = faces_ccw(msh, cl);
        for (auto& [fc, flip] : fcs_ccw)
            std::cout << fc << " " << flip << std::endl;
    }

    for(const auto& cl : msh)
        std::cout<< "G matrix: \n"
        << disk::vem_2d::matrix_G(msh, cl, degree) << "\n";


    for(const auto& cl : msh)
        std::cout<< "B matrix: \n"
            << disk::vem_2d::matrix_B(msh, cl, degree) << "\n";

    for(const auto& cl : msh)
        std::cout<< "D matrix: \n"
            << disk::vem_2d::matrix_D(msh, cl, degree) << "\n";


    auto rhs_fun = [](const point_type& p) -> T {

        return  32.0 * (p.x() *(1.0 - p.x())+ p.y() * (1.0 - p.y()));
        };

    for (const auto& cl : msh)
    {
        auto [A, rhs] = disk::vem_2d::compute_local(msh, cl, degree, rhs_fun);

        std::cout << "A matrix: \n" << A << "\n";
        std::cout << "RHS vector: \n" << rhs << "\n";

    }


    return 0;
}