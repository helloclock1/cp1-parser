#include <fstream>

#include "parser.h"
#include "tokenizer.h"

int main() {
    std::ifstream in("file");
    Tokenizer tokenizer(&in);
    Parser parser(tokenizer);
    Module file = parser.ParseModule();
    file.Print();
    return 0;
}
