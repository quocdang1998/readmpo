// Copyright 2023 quocdang1998
#include <cstdlib>   // std::atoi, std::atol
#include <iostream>  // std::cout
#include <iterator>  // std::make_move_iterator
#include <filesystem>  // std::filesystem::exists
#include <string>    // std::string

#include "readmpo/glob.hpp"        // readmpo::glob
#include "readmpo/h5_utils.hpp"    // readmpo::stringify
#include "readmpo/master_mpo.hpp"  // readmpo::MasterMpo
#include "readmpo/query_mpo.hpp"   // readmpo::query_mpo

const char * help_message = R"(Retrieve microscopic cross-section from an MPO.
Options:
    Help mode:
        -h, --help: Print help message.
    Query mode: get names of geometries and energy meshes presenting in the MPO.
        -q, --query: Query the MPO
    Get data from MPO:
        -g, --geometry: Name of geometry.
        -e, --energy-mesh: Name of energy mesh.
        -i, --isotope: Name of isotope (multiple calls allowed).
        -r, --reaction: Name of reaction (multiple calls allowed).
        -o, --output: Name of output folder. Default: ".".
        -sk, --skip-dims: Name (in lowercase) of parameter that should be ignored (multiple calls allowed).
        -xs, --xs-type: Type of cross section. Possible value:
            0: micro (default)
            1: macro
            2: zoneflux
            3: reaction rate.
Result:
    Serialized arrays of homogenized cross-section, which can be read with merlin::array::Stock.
)";

int main(int argc, char * argv[]) {
    using namespace readmpo;
    // parse argument
    unsigned int mode = 0;
    unsigned int xstype = 0;
    std::uint64_t max_anisotropy_order = 1;
    std::string geometry, energymesh, output_folder = ".";
    std::vector<std::string> filenames, isotopes, reactions, skipped_dims;
    bool reload = false;
    std::string mastermpo_name = "master_mpo.txt";
    for (int i = 1; i < argc; i++) {
        std::string argument(argv[i]);
        if (!argument.compare("-h") || !argument.compare("--help")) {
            mode |= 1;
        } else if (!argument.compare("-q") || !argument.compare("--query")) {
            mode |= 2;
        } else if (!argument.compare("-e") || !argument.compare("--emesh")) {
            energymesh = std::string(argv[++i]);
            mode |= 4;
        } else if (!argument.compare("-g") || !argument.compare("--geom")) {
            geometry = std::string(argv[++i]);
            mode |= 4;
        } else if (!argument.compare("-i") || !argument.compare("--iso")) {
            isotopes.push_back(std::string(argv[++i]));
            mode |= 4;
        } else if (!argument.compare("-r") || !argument.compare("--reac")) {
            reactions.push_back(std::string(argv[++i]));
            mode |= 4;
        } else if (!argument.compare("-o") || !argument.compare("--outdir")) {
            output_folder = std::string(argv[++i]);
            mode |= 4;
        } else if (!argument.compare("-sk") || !argument.compare("--skipdims")) {
            skipped_dims.push_back(std::string(argv[++i]));
            mode |= 4;
        } else if (!argument.compare("-xs") || !argument.compare("--type")) {
            xstype = std::atoi(argv[++i]);
            mode |= 4;
        } else if (!argument.compare("-mao") || !argument.compare("--maxanisop")) {
            max_anisotropy_order = std::atoi(argv[++i]);
            mode |= 4;
        } else if (!argument.compare("-l") || !argument.compare("--reload")) {
            reload = true;
            mode |= 4;
        } else {
            // filenames.push_back(argument);
            std::vector<std::string> glob_expanded = glob(argument);
            filenames.insert(filenames.end(), std::make_move_iterator(glob_expanded.begin()),
                             std::make_move_iterator(glob_expanded.end()));
        }
    }
    // help mode
    if (mode == 1) {
        std::cout << help_message;
        return 0;
    }
    // query mode
    if (mode == 2) {
        for (std::string & fname : filenames) {
            std::cout << "Filename: " << fname << "\n";
            std::map<std::string, std::vector<std::string>> queried_list = query_mpo(fname);
            for (auto & [key, values] : queried_list) {
                std::cout << "    " << key << ": " << values << "\n";
            }
        }
        return 0;
    }
    // construct master MPO and retrieve data
    if (mode == 4) {
        MasterMpo master_mpo;
        if (reload) {
            if (!std::filesystem::exists(mastermpo_name)) {
                throw std::runtime_error("Executed in reload mode, but unable to open master_mpo.txt.\n");
            }
            master_mpo.deserialize(mastermpo_name);
        } else {
            master_mpo = MasterMpo(filenames, geometry, energymesh);
        }
        std::cout << master_mpo.str() << "\n";
        MpoLib microlib = master_mpo.build_microlib_xs(isotopes, reactions, skipped_dims, static_cast<XsType>(xstype));
        for (auto & [isotope, rlib] : microlib) {
            for (auto & [reaction, lib] : rlib) {
                std::string outfname = stringify(output_folder, "/", isotope, "_", reaction, ".txt");
                lib.serialize(outfname);
            }
        }
        if (!reload) {
            master_mpo.serialize(mastermpo_name);
        }
        return 0;
    }
    // argument not match together
    throw std::runtime_error("Argument options not match. Execute \"readmpo --help\" for more information.\n");
    return 1;
}
