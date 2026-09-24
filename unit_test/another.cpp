#include "another.h"
#include <vector>
#include <string>
#include "bokumeido/core.hpp"

using std::string;
using std::vector;
using namespace meido;

//std::ostream& operator<<(std::ostream& out, const test::Complex& c1)
//{
//    std::cout << c1.a;
//    return out;
//}
struct MyStruct {};

void anotherTest()
{
    io::print("anotherTest");
    printf("Author check! Expected| [ERROR] Failed to open nonexistent_config.ini (file does not exist, expected behavior)\n");
    io::IniFile nonexistent_ini;
    nonexistent_ini.open("nonexistent_config.ini");
    nonexistent_ini.close();
    std::vector<int> nums = {1, 2, 3};
    std::map<int, std::string> kv = {{1, "a"}, {2, "b"}};
    std::vector<std::map<int, std::string>> nested = {{{1, "a"}, {2, "b"}}, {{3, "c"}}};
    MyStruct s ;    // 假设有个没适配 operator<< 的自定义类型

    meido::io::print(nums, kv, nested, 3.14, s);
    MEIDO_INFO("anotherTest out\n");
}