#include "../thirdparty/httplib.h"

#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#include <atomic>
#include <shared_mutex>

// 全局变量，用于服务注册表和配置映射
std::unordered_map<std::string, uint16_t> service_registry;
std::unordered_map<std::string, std::function<void()>> service_config_map;
std::shared_mutex registry_mutex;
std::mutex config_mutex;

std::atomic<uint16_t> next_port(8050);

// 查找可用端口
uint16_t find_available_port(uint16_t start_port) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 0;
    }

    for (uint16_t port = start_port; port < 8100; ++port) {
        SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET) {
            continue;
        }

        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        if (bind(sock, (sockaddr*)&addr, sizeof(addr)) == 0) {
            closesocket(sock);
            WSACleanup();
            return port;
        }

        closesocket(sock);
    }

    WSACleanup();
    return 0; // 如果没有找到可用端口，返回 0
}

// 启动服务
void start_service(const std::string& service_name, const uint16_t port) {
    if (service_name == "echo") {
        std::string command = "echo.exe --port " + std::to_string(port);
        STARTUPINFO si;
        PROCESS_INFORMATION pi;

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        if (!CreateProcess(NULL,
                           (LPSTR)command.c_str(),
                           NULL,
                           NULL,
                           FALSE,
                           0,
                           NULL,
                           NULL,
                           &si,
                           &pi)) {
            std::cerr << "CreateProcess failed (" << GetLastError() << ")\n";
            return;
        }

        // 等待进程完成
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "Local service started successfully.\n";
    } else {
        std::string image_name =
            (service_name == "docker-echo") ? "alinche/echo" : service_name;
        std::string port_forwarding = std::to_string(port) + ":3000";
        std::string command = "docker run -p " + port_forwarding +
                              " --pull=missing --rm -d " + image_name;
        STARTUPINFO si;
        PROCESS_INFORMATION pi;

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        if (!CreateProcess(NULL,
                           (LPSTR)command.c_str(),
                           NULL,
                           NULL,
                           FALSE,
                           0,
                           NULL,
                           NULL,
                           &si,
                           &pi)) {
            std::cerr << "CreateProcess failed (" << GetLastError() << ")\n";
            return;
        }

        std::this_thread::sleep_for(std::chrono::seconds(3));
        std::cout << "Local service started successfully.\n";
    }
}

// 获取或启动服务
uint16_t get_or_start_service(const std::string& service_name) {
    std::shared_lock<std::shared_mutex> lock(registry_mutex);
    if (service_registry.find(service_name) != service_registry.end()) {
        return service_registry[service_name];
    }

    lock.unlock();

    uint16_t port = find_available_port(next_port.fetch_add(1));
    if (port == 0) {
        throw std::runtime_error("No available ports in range 8050-8100");
    }

    std::unique_lock<std::shared_mutex> unique_lock(registry_mutex);
    service_registry[service_name] = port;
    unique_lock.unlock();

    start_service(service_name, port);

    return port;
}

// 从 URL 中提取第一个路径并注入到请求头中
void inject_service_name_header(httplib::Request& req) {
    if (req.path.empty()) {
        return;
    }
    if (req.path[0] != '/') {
        std::cerr << "Path does not start with '/'" << std::endl;
        return;
    }

    size_t first_slash = req.path.find('/');
    if (first_slash != std::string::npos) {
        size_t second_slash = req.path.find('/', first_slash + 1);
        if (second_slash == std::string::npos) {
            second_slash = req.path.length();
        }
        std::string service_name =
            req.path.substr(first_slash + 1, second_slash - first_slash - 1);
        req.headers.insert({"service_name", service_name});
        req.path = req.path.substr(second_slash);
        if (req.path.empty()) {
            req.path = "/";
        }
        std::cout << "service_name: " << service_name
                  << ", rest_path: " << req.path << '\n';
    }
}

void check_result(httplib::Result& result, httplib::Response& resp) {
    if (result) {
        resp.status = result->status;
        for (auto& header : result->headers) {
            resp.set_header(header.first, header.second);
        }
        resp.set_content(result->body, "text/plain");
    } else {
        resp.status = 500;
        resp.set_content("Internal Server Error", "text/plain");
    }
}

// 代理请求
void proxy_request_get(httplib::Server& server,
                       const httplib::Request& req,
                       httplib::Response& resp) {
    try {
        // 从 URL 中提取第一个路径并注入到请求头中
        httplib::Request modified_req = req;
        inject_service_name_header(modified_req);

        std::string service_name =
            modified_req.get_header_value("service_name");
        uint16_t port = get_or_start_service(service_name);
        std::string target_url = "http://localhost:" + std::to_string(port);
        std::cout << "target_url: " << target_url << '\n';

        httplib::Client client(target_url);
        client.set_follow_location(true);
        auto result = client.Get(modified_req.path);
        check_result(result, resp);

    } catch (const std::out_of_range& e) {
        // 如果 path_params 中没有 service_name，返回 404
        resp.status = 404;
        resp.set_content("Service not found", "text/plain");
    } catch (const std::exception& e) {
        // 其他异常
        resp.status = 500;
        resp.set_content("Internal Server Error", "text/plain");
    }
}
void proxy_request_post(httplib::Server& server,
                        const httplib::Request& req,
                        httplib::Response& resp) {
    try {
        // 从 URL 中提取第一个路径并注入到请求头中
        httplib::Request modified_req = req;
        inject_service_name_header(modified_req);

        std::string service_name =
            modified_req.get_header_value("service_name");
        uint16_t port = get_or_start_service(service_name);
        std::string target_url = "http://localhost:" + std::to_string(port);
        std::cout << "target_url: " << target_url << '\n';

        httplib::Client client(target_url);
        client.set_follow_location(true);
        auto result =
            client.Post(modified_req.path,
                        modified_req.body,
                        modified_req.get_header_value("Content-Type"));
        check_result(result, resp);

    } catch (const std::out_of_range& e) {
        // 如果 path_params 中没有 service_name，返回 404
        resp.status = 404;
        resp.set_content("Service not found", "text/plain");
    } catch (const std::exception& e) {
        // 其他异常
        resp.status = 500;
        resp.set_content("Internal Server Error", "text/plain");
    }
}

int main() {
    httplib::Server server;
    server.Get("/heathl",
        [](const httplib::Request& req, httplib::Response& res) {
            std::cout << "alive\n";
        });
    server.Get("/(.*)",
        [&server](const httplib::Request& req, httplib::Response& resp) {
            proxy_request_get(server, req, resp);
        });
    server.Post(
        "/(.*)",
        [&server](const httplib::Request& req, httplib::Response& resp) {
            proxy_request_post(server, req, resp);
        });

    uint16_t port = 8090;
    std::cout << "Server started on port " << port << "\n\n";
    if (!server.listen("0.0.0.0", port)) {
        std::cerr << "Failed to start server on port " << port << '\n';
        return 1;
    }

    return 0;
}