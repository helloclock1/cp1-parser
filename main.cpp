#include <fstream>

#include "formatter.h"
#include "parser.h"
#include "tokenizer.h"

int main(int argc, char* argv[]) {
    std::ifstream in("file");
    Tokenizer tokenizer(&in);
    Parser parser(tokenizer);
    Module file = parser.ParseModule();
    CodeGenerator gen(std::cout);
    gen.Generate(file);
    return 0;
}
