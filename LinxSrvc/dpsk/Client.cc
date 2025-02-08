#include "CurlReq.h"
#include <iostream>
#include <string>

int main()
{
    while (true) {
        std::string text;
        std::cout << "请输入聊天内容（输入 'exit' 退出）: ";
        std::getline(std::cin, text);
        if (text == "exit") {
            break;
        }
        std::cout << CurlReq::processInput(text) << std::endl;
    }
    return 0;
}
