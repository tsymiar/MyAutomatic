#include "CurlReqs.h"
#include <iostream>
#include "Utils.hpp"

int main()
{
    while (true) {
        std::string text;
        std::cout << "请输入聊天内容（输入 'q' 退出）: ";
        std::getline(std::cin, text);
        if (text == "q") {
            break;
        }
        if (text.empty()) {
            continue;
        }
        ReqsPara para;
        Config config("params.txt");
        int model = atoi(config.getVariable("model").c_str());
        model = (model > 4 ? 4 : model);
        para.apiPara.model = Models[model];
        std::cout << CurlReqs::processChat(text, para) << std::endl;
    }
    return 0;
}
