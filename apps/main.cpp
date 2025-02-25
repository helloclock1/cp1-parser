#include <parser/formatter.h>
#include <parser/parser.h>
#include <parser/tokenizer.h>

#include <fstream>
#include <iostream>

void usage() {
    std::cout << "Usage: beautify read_from [write_to]\n";
    std::cout << "If write_to is not specified, a beautified file is printed to stdout.\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 3) {
        usage();
        return 0;
    }
    std::string filename = argv[1];
    std::ifstream in(filename);
    if (in.fail()) {
        std::cerr << "File `" << filename << "` does not exist.\n";
        return 1;
    }
    Tokenizer tokenizer(&in);
    Parser parser(tokenizer);
    Module file = parser.ParseModule();
    if (argc == 2) {
        CodeGenerator gen(std::cout);
        gen.Generate(file);
    } else {
        std::string write_to = argv[2];
        std::ofstream out(write_to);
        CodeGenerator gen(out);
        gen.Generate(file);
    }
    return 0;
}
