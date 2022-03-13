#include "search_server.h"
#include "request_queue.h"
#include "test_example_functions.h"
#include "log_duration.h"

using namespace std;

int main()
{
    setlocale( LC_ALL, "Russian" );
    try
    {
        TestSearchServer();
    }
    catch ( const std::exception& e )
    {
        std::cout << e.what() << std::endl;
    }
    // Если вы видите эту строку, значит все тесты прошли успешно
    std::cout << "Search server testing finished"s << std::endl;
}