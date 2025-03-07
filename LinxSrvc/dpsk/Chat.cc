#include "CurlReqs.h"
#include <iostream>
#include "Utils.hpp"
#include <thread>

enum State { Normal, AwaitingFile, FileReady };

int main()
{
    ReqsPara para;
    char status = Normal;
    while (true) {
        std::string text = "";
        if (status == AwaitingFile) {
            std::cout << ">> 请输入文件位置: ";
        } else {
            status = Normal;
            std::cout << "请输入请求内容（输入 'q' 退出，输入 'f' 读取文件）: ";
        }
        std::getline(std::cin, text);
        if (text.empty()) {
            continue;
        }
        if (text == "q") {
            std::cout << "聊天退出！" << std::endl;
            break;
        }
        if (text == "f" && status == Normal) {
            status = AwaitingFile;
            continue;
        }
        if (text == "e" && status == FileReady) {
            status = Normal;
            continue;
        }
        if (std::cin.eof()) {
            std::cout << std::endl;
            std::cin.clear();
            continue;
        }
        if (status == AwaitingFile) {
            para.apiPara.file_content = getFileContent(text);
            status = FileReady;
            continue;
        }
        para.setModel(std::stoi(Configs::getConfig().getVariable("model")));
        para.apiPara.stream = Configs::getConfig().getVariable("stream") == "true";
        if (para.apiPara.stream) {
            std::thread task([]()->void {
                CurlReqs creq;
                std::string content;
                do {
                    if (!content.empty()) {
                        std::cout << content;
                        content = "";
                    }
                    msWait(50);
                } while (creq.getContents(content));
                std::cout << content << std::endl;
                });
            CurlReqs::processChat(text, para);
            task.join();
        } else {
            std::cout << CurlReqs::processChat(text, para) << std::endl;
        }
        para.unset();
    }
    return 0;
}
