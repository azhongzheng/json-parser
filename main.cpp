#include "json.hpp"

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