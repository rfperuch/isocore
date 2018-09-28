#include <isolario/plus/json.hpp>

#include <cstdio>
int main()
{
    using namespace isolario;

    json_enc miao;
    miao.new_object();
    miao.new_field("value"); miao.new_value(0.5);
    miao.close_object();

    std::printf("%s\n", miao.text());
    return 0;
}
