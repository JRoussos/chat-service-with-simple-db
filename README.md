# Chat Servise

This is a semester assignment for the class of System Programming. It's a chat service app with the ability to chat on a group conversation or with individual users. On this implementation of a chatroom there is  a database logic for storing all the information as requested by my professor. This project was made to run on linux and unix based systems.

## Description

A Client can register or log in on the chat service with a username and a password. Then has the choice of chating with a contact or in a group chat, add a new friend or group, create a new group that other users can join in and also to detele groups that has create.

## Requirements

The requirements of the assignment are as follow:

* Sing in to the service
* Log in using a username and a password
* Creation of groups
* Delete of a group from its admin
* Every group member can send messages
* Every group member can receive messages
* Maintain contacts
* Send personal messages to a user on the contacts page of the sender
* One server will handle a set of groups
* Retrive old messages (optional)

## Usage
Compile the server and the client as such

```
gcc -o server.out server.c -lpthread -lrt
gcc -o client.out client.c -lpthread
```

First run the server and then as many clients as you want. When executing the client you can optionaly pass as an argument the ip address of the server if it's running on a different machine than the client (works only on local network), on the other hand if no arguments are passed the client is running on localhost.


```
./server.out
./client.out (optionaly you can type the address of the server, otherwise its localhost)
```

## MIT License

Copyright (c) 2020 John Roussos

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.