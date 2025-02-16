#include "CurlReqs.h"
#include <iostream>
#include "Utils.hpp"

int main()
{
    while (true) {
        std::string text;
        std::cout << "请输入聊天内容（输入 'bye' 退出）: ";
        std::getline(std::cin, text);
        if (text == "bye") {
            break;
        }
        if (text.empty()) {
            continue;
        }
        ReqsPara para;
        Config config("params.txt");
        para.balance = atoi(config.getVariable("model").c_str());
        std::cout << CurlReqs::processChat(text, para) << std::endl;
    }
    return 0;
}
