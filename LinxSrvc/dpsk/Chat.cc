#include "CurlReqs.h"
#include <iostream>
#include "Parser.hpp"

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
        std::cout << Markdown::Parse(CurlReqs::processChat(text, para)) << std::endl;
    }
    return 0;
}
