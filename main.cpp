#include "json.hpp"

std::string read_file(const std::string &filename)
{
    std::ifstream ifs(filename);
    if (ifs.bad())
        throw std::runtime_error("open " + filename + " failed");

    std::string res;
    std::string line;
    while (getline(ifs, line))
        res += line;
    return res;
}


int main(int argc, char const *argv[])
{
    /* code */

    using namespace Lexer;
    auto ts = build_token_stream("{133\"hey\ng\tgg\"}");
    // ts.print();

    try
    {

        // JSON js("[12, [1, 23]]]");
        JSON json(read_file("test.json"));
        std::cout << "host: " << json["db"]["host"].get_str() << std::endl;
        std::cout << "port: " << json["db"]["port"].get_int() << std::endl;
       
    }
    catch (const std::exception &e)
    {
        std::cout << e.what() << std::endl;
    }

    return 0;
}