#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <filesystem>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <mutex>
#include <chrono>

#pragma comment(lib, "ws2_32.lib")

constexpr int PORT = 8080;
constexpr char ROOT_DIR[] = "static";

using namespace std;
using namespace chrono;

unordered_map<string, string> fileCache;
mutex cacheMutex;

string readFileCached(const string& path) {
    lock_guard<mutex> lock(cacheMutex);
    auto it = fileCache.find(path);
    if (it != fileCache.end())
        return it->second;

    ifstream file(path, ios::binary);
    if (!file.is_open())
        return "";
    ostringstream ss;
    ss << file.rdbuf();
    string content = ss.str();
    fileCache[path] = content;
    return content;
}

bool sendAll(SOCKET sock, const char* data, int totalLen) {
    int sent = 0;
    while (sent < totalLen) {
        int n = send(sock, data + sent, totalLen - sent, 0);
        if (n == SOCKET_ERROR) return false;
        sent += n;
    }
    return true;
}

void sendResponse(SOCKET client, const string& status, const string& content, const string& contentType = "text/html") {
    ostringstream resp;
    resp << "HTTP/1.1 " << status << "\r\n"
        << "Content-Length: " << content.size() << "\r\n"
        << "Content-Type: " << contentType << "\r\n"
        << "Connection: close\r\n\r\n"
        << content;
    string s = resp.str();
    sendAll(client, s.c_str(), static_cast<int>(s.length()));
}

void logRequest(const string& path, int durationMs, const string& status) {
    ofstream log("log.txt", ios::app);
    auto now = system_clock::to_time_t(system_clock::now());
    log << "[" << now << "] "
        << path << " " << status << " " << durationMs << "ms\n";
}

void handleClient(SOCKET clientSocket) {
    auto t0 = high_resolution_clock::now();

    int flag = 1;
    setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));

    char buffer[4096];
    int received = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (received <= 0) {
        closesocket(clientSocket);
        return;
    }
    buffer[received] = '\0';

    istringstream req(buffer);
    string method, path, version;
    req >> method >> path >> version;

    if (method != "GET") {
        string body = "<html><body><h1>405 Method Not Allowed</h1></body></html>";
        sendResponse(clientSocket, "405 Method Not Allowed", body);
        auto dt = duration_cast<milliseconds>(high_resolution_clock::now() - t0).count();
        logRequest(path, (int)dt, "405");
        closesocket(clientSocket);
        return;
    }

    if (path == "/") path = "/index.html";
    string fullPath = string(ROOT_DIR) + path;

    string content = readFileCached(fullPath);

    if (content.empty()) {
        string body = "<html><body><h1>404 Not Found</h1></body></html>";
        sendResponse(clientSocket, "404 Not Found", body);
        auto dt = duration_cast<milliseconds>(high_resolution_clock::now() - t0).count();
        logRequest(path, (int)dt, "404");
    }
    else {
        string ext = fullPath.substr(fullPath.find_last_of('.') + 1);
        string mime;
        if (ext == "html") mime = "text/html";
        else if (ext == "css") mime = "text/css";
        else if (ext == "js")  mime = "application/javascript";
        else if (ext == "png") mime = "image/png";
        else if (ext == "jpg" || ext == "jpeg") mime = "image/jpeg";
        else if (ext == "svg") mime = "image/svg+xml";
        else if (ext == "json") mime = "application/json";
        else mime = "application/octet-stream";
        sendResponse(clientSocket, "200 OK", content, mime);
        auto dt = duration_cast<milliseconds>(high_resolution_clock::now() - t0).count();
        logRequest(path, (int)dt, "200");
    }

    closesocket(clientSocket);
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed.\n";
        return 1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        cerr << "Socket creation failed.\n";
        WSACleanup();
        return 1;
    }

    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);
    if (bind(serverSocket, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        cerr << "Bind failed.\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "Listen failed.\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    cout << "Server started at http://localhost:" << PORT << "\n";

    while (true) {
        SOCKET client = accept(serverSocket, nullptr, nullptr);
        if (client == INVALID_SOCKET) {
            cerr << "Accept failed.\n";
            continue;
        }
        thread(handleClient, client).detach();
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
