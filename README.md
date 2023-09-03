# SimpleChatCpp
Boost Asio 라이브러리를 이용한 간단한 TCP 채팅 예제입니다.\
헤더/바디 구조로 패킷을 송수신하는 방식으로 통신합니다.

A simple example of TCP Client-Server chat program using boost asio library.\
Each client and server communicate by transfering packets consists of header and body.

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
After successful connection, each client are requested to enter channel-password(passkey) and nickname.
