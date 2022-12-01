#include <iostream>
#include <common/json/impl/boost/boost_json.hpp>
#include <boost/json/src.hpp>

namespace json
{

void BoostJson::hello_world()
{
    boost::json::value hello_world = {{"hello", "Boost"}};

    boost::json::serializer sr;
    sr.reset(&hello_world);
    do
    {
        char buf[16];
        std::cout << sr.read(buf);
    } while (!sr.done());
}

}
