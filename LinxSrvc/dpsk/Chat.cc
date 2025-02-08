#include "CurlReqs.h"
#include <iostream>

int main()
{
    while (true) {
        std::string text;
        std::cout << "请输入聊天内容（输入 'exit' 退出）: ";
        std::getline(std::cin, text);
        if (text == "exit") {
            break;
        }
        ReqsPara para;
        para.apiPara.temperature = 1.5;
        std::cout << CurlReqs::processChat(text, para) << std::endl;
    }
    return 0;
}
