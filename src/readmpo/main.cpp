// Copyright 2023 quocdang1998
#include <cstdlib>  // std::atoi
#include <iostream>  // std::cout
#include <string>  // std::string

#include "readmpo/master_mpo.hpp"  // readmpo::MasterMpo
#include "readmpo/h5_utils.hpp"    // readmpo::stringify

const char * help_message = R"(Retrieve microscopic cross-section from an MPO.
Options:
    Help mode:
        -h, --help: Print help message.
    Get data from MPO:
        -g, --geometry: Name of geometry.
        -e, --energymesh: Name of energy mesh.
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
    An serialized array of microscopic homogenized cross-section, which can be read with merlin::array::Stock.
)";

int main(int argc, char * argv[]) {
    using namespace readmpo;
    // parse argument
    unsigned int mode = 0;
    unsigned int xstype = 0;
    std::string geometry, energymesh, output_folder = ".";
    std::vector<std::string> filenames, isotopes, reactions, skipped_dims;
    for (int i = 1; i < argc; i++) {
        std::string argument(argv[i]);
        if (!argument.compare("-h") || !argument.compare("--help")) {
            mode |= 1;
        } else if (!argument.compare("-e") || !argument.compare("--energy-mesh")) {
            energymesh = std::string(argv[++i]);
            mode |= 2;
        } else if (!argument.compare("-g") || !argument.compare("--geometry")) {
            geometry = std::string(argv[++i]);
            mode |= 2;
        } else if (!argument.compare("-i") || !argument.compare("--isotope")) {
            isotopes.push_back(std::string(argv[++i]));
            mode |= 2;
        } else if (!argument.compare("-r") || !argument.compare("--reaction")) {
            reactions.push_back(std::string(argv[++i]));
            mode |= 2;
        } else if (!argument.compare("-o") || !argument.compare("--output")) {
            output_folder = std::string(argv[++i]);
            mode |= 2;
        } else if (!argument.compare("-sk") || !argument.compare("--skip-dims")) {
            skipped_dims.push_back(std::string(argv[++i]));
            mode |= 2;
        } else if (!argument.compare("-xs") || !argument.compare("--xs-type")) {
            xstype = std::atoi(argv[++i]);
            mode |= 2;
        } else {
            filenames.push_back(argument);
        }
    }
    // help mode
    if (mode == 1) {
        std::cout << help_message;
        return 0;
    }
    // construct master MPO and retrieve data
    MasterMpo master_mpo(filenames, geometry, energymesh);
    MpoLib microlib = master_mpo.build_microlib_xs(isotopes, reactions, skipped_dims, static_cast<XsType>(xstype));
    for (std::string & isotope : isotopes) {
        for (std::string & reaction : reactions) {
            std::string outfname = stringify(output_folder, "/", isotope, "_", reaction, ".txt");
            microlib[isotope][reaction].serialize(outfname);
        }
    }
}
