/*  BokuMeidoCpp 单元测试主文件
    以下命令在 unit_test/ 目录下执行：
    cd unit_test
    g++ bokumeido_unit_test.cpp another.cpp -o bokumeido_unit_test -I.. -lpthread --std=c++11 -g -Wall -Wextra -Wno-unused
    g++ bokumeido_unit_test.cpp another.cpp -o bokumeido_unit_test -I.. -lpthread --std=c++11 -O3 -DNDEBUG -D_GLIBCXX_ASSERTIONS -Wall -Wextra -Wno-unused

    运行：./bokumeido_unit_test
*/
// #include "bokumeido/core.hpp"
#include "core/base_test.hpp"
#include "core/type_test.hpp"
#include "core/str_test.hpp"
#include "core/datastruct_test.hpp"
#include "core/math_test.hpp"
#include "core/path_test.hpp"
#include "core/thread_test.hpp"
#include "core/time_test.hpp"
#include "core/io_test.hpp"
#include "core/mem_test.hpp"
#include "core/log_test.hpp"
#include "core/type_traits_ambiguity_test.hpp"

#include "another.h"

using namespace meido;

int main()
{
    meido::base::logVersion("bokumeido_unit_test");

    _meidobasecheck::check();
    _meidotypecheck::check();
    _meidostrcheck::check();
    _meidodscheck::check();
    _meidomathcheck::check();
    _meidopathcheck::check();
    _meidothreadcheck::check();
    _meidotimecheck::check();
    _meidoiocheck::check();
    _meidomemcheck::check();
    _meidologcheck::check();
    _meidotypeambiguitycheck::check();

    anotherTest();

    return 0;
}
