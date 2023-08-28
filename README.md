# SimpleChatCpp
C++ TCP 서버/클라이언트 채팅 예제입니다.\
Small personal project written in C++ to learn boost asio library and basic TCP/IP Client-Server networking.

* C++20
* Windows 10 / MSVC
* Boost 1.83.0

## Server Usage
```
server.exe <ip> <port> <channel-password>
```
All client attempt to connect are required to enter channel-password

## Client Usage
```
client.exe <ip> <port>
```
