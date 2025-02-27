#include <parser/formatter.h>
#include <parser/parser.h>
#include <parser/tokenizer.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

void usage() {
    std::cout << "Usage: ./beautify read_from [write_to] [OPTIONS]\n";
    std::cout << "\n";
    std::cout << "Description: this program accepts a file as input and outputs the same file but formatted either to "
                 "another file or stdout.\n";
    std::cout << "\n";
    std::cout << "Options:\n";
    std::cout << "  --help                         Shows this message\n";
    std::cout << "  --spaces-per-tab -t            Specifies amount of spaces a tab should be expanded as "
                 "(defaults to 8)";
}

// Returns an object of structure {{input filename, output filename (stdout if empty)}, spaces per tab}
std::pair<std::pair<std::string, std::string>, size_t> ParseArgs(int argc, char* argv[]) {
    std::string in_filename;
    std::string out_filename;
    size_t spaces = 8;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help") {
            usage();
            exit(0);
        } else if (arg == "--spaces-per-tab" || arg == "-t") {
            if (i + 1 < argc) {
                try {
                    spaces = std::stoull(argv[i + 1]);
                } catch (const std::invalid_argument&) {
                    std::cerr << "Invalid value for spaces per tab argument: " << argv[i + 1] << ".\n";
                    exit(1);
                }
                ++i;
            } else {
                std::cerr << "No spaces per tab argument value was provided.\n";
                exit(1);
            }
        } else if (in_filename.empty()) {
            in_filename = arg;
        } else if (out_filename.empty()) {
            out_filename = arg;
        } else {
            std::cerr << "Unknown argument: " << arg << ".\n";
            exit(1);
        }
    }
    if (in_filename.empty()) {
        std::cerr << "No input filename was provided.\n";
        exit(1);
    }
    return {{in_filename, out_filename}, spaces};
}

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 4) {
        usage();
        return 0;
    }
    auto [filenames, spaces] = ParseArgs(argc, argv);
    auto& [in_filename, out_filename] = filenames;
    std::ifstream in(in_filename);
    if (in.fail()) {
        std::cerr << "File `" << in_filename << "` does not exist.\n";
        return 1;
    }
    Tokenizer tokenizer(&in, spaces);
    Parser parser(tokenizer);
    Module file = parser.ParseModule();
    if (out_filename.empty()) {
        CodeGenerator gen(std::cout);
        gen.Generate(file);
    } else {
        std::ofstream out(out_filename);
        CodeGenerator gen(out);
        gen.Generate(file);
    }
    return 0;
}
