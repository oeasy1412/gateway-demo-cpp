#include "../thirdparty/httplib.h"
#include <string>

// 全局变量port
uint16_t port = 3000;

// 处理根路径的请求
void handleRoot(const httplib::Request& req, httplib::Response& resp) {
    // 构造 JSON 响应
    resp.status = 200;
    resp.set_content(
        "{\"status\":\"success\","
        "\"port\":" + std::to_string(port) + ","
        "\"message\":\"Service started successfully\"}", 
        "application/json"
    );
}

void handleEcho(const httplib::Request& req, httplib::Response& resp) {
    // 构造 JSON 响应
    resp.status = 200;
    resp.set_content(
        "{\"status\":\"success\","
        "\"port\":" + std::to_string(port) + ","
        "\"received_message\":\"" + req.body + "\"}", 
        "application/json"
    );
}

// 解析命令行参数中的端口号
void parseCommandLineArguments(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--port" && i + 1 < argc) {
            port = static_cast<uint16_t>(std::stoi(argv[i + 1]));
            break;
        }
    }
}

int main(int argc, char* argv[]) {
    // 解析命令行参数以获取端口号
    parseCommandLineArguments(argc, argv);

    httplib::Server server;

    server.Get("/", [](const httplib::Request& req, httplib::Response& resp) {
        handleRoot(req, resp);
    });
    server.Post("/echo", [](const httplib::Request& req, httplib::Response& resp) {
        handleEcho(req, resp);
    });

    server.listen("0.0.0.0", port);

    return 0;
}