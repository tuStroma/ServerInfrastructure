# Server Infrastructure

This project was created to help in development of C++ projects based on Client - Server architecture. It serves as a C++ library, which handles comunication with server application on the client side, and multiple clients on the server side. The server and client applications comunicate with TCP/IP protocol over web sockets.

## Table of contents

1. [Getting started](#1-getting-started)
    * [Dependencies](#dependencies)
1. [Manual](#2-manual)
    * [Overview of classes and methods](#overview-of-classes-and-methods)
        * [Message](#class-messagetype)
        * IClient
        * IServer
    * Developing an aplication
1. [Setting up your server](#3-setting-up-your-server)

---
---

## 1. Getting Started

To add Server Infrastructure to your server and client projects:
1. Download the release package in your desired version to your computer. 
2. Add path to `ServerInfrastructure/` directory (where the `server_infrastructure.h` header file is located) to build settings of both server and client projects.
3. Include the `server_infrastructure.h` header file in your project with line `#include <server_infrastructure.h>` to your projects.

4. The Server Infrastructure is built with `c++ 20 Standard` version of C++. Make sure to set the right C++ version in your build settings.

5. Download and link all dependencies to your projects.

### Dependencies

* Boost.Asio 1.24.0 - Server Infrastructure is built on top of the ASIO library, which you can download from https://think-async.com/Asio.

## 2. Manual

### Overview of classes and methods

> #### Class **Message\<Type\>**

Message objects carry data between server and clients. It consists of the header and the body. Header carries data about type of message and size of message body. 

Message objects should be declared with template argument, which is a type of [communication context](#communication-context) declared by the user.

**Namespace:** net::common

**Constructors:**

* `Message<Type>(Type type, size_t size)`

Creates message of given type and size (size is a size of body in bytes).

* `Message<Type>(Header<Type> header)`

Creates message based on the given header.

* `Message<Type>(Message<Type>& msg)`

Copies message `msg`.

**Functions:**

* `bool put(void* source, size_t size)`

Inserts data of given size to the message. If the message has less free space than given size the return value is `false`, if data is inserted correctly the return value is `true`.

Data can be inserted in steps by several `put()` function calls given that their overall size is not greater than message size. This data can be later extracted by the `get()` function calls in the same order (with the same sizes).

* `bool putString(const char* source)`

Inserts given string to the message. If the string is bigger than remaining space returns `false` and if inserted correctly returns `true`.

* `bool get(void* destination, size_t size)`

Extracts data of given size from message to the buffer specified by the `destination` pointer. If specified size is greater than the size of remaining data returns `false`, if extracted correctly returns `true`.

Data can be extracted in steps by several `get()` function calls, preferably matching with order of `put()` function calls.

* `bool getString(char* destination)`

Extracts string to the buffer specified by the `destination` pointer. If the string isn't terminated by `\0` before the end of message buffer returns `false`, if extracted correctly returns `true`.

**Warning:** Make sure to check if the string buffer has big enough size to store string from message. Too small buffer can lead to buffer overflow error, which can cause the server crush.

* `int getStringLen()`

This function assumes that the next data to pull from message is string and calculates it's length (without the `\0` terminating character). If the string is greater than size of remaining data returns `0`.


> #### Communication context

Communication context is a type **declared by the user**, which has defined all of the possible message types.

It is advised to use an **enum type** based on 32 byte int.

Example
```c++
enum ChatContext {
	AcceptClient,
	Disconnect,
	Message,
    Ping
};
```

## 3. Setting up your server