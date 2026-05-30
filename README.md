# Wrens Nest
### Description
A CLI server managment application for when things start to get messy. Wrens Nest is a TUI style application that bundles all of your sever managment and monitoring into one simple to use package, eliminating the need to constantly SSH into servers to check logs or manually using SCP to push updates.

### Why I built this
I, like many developers, have a lot of servers. I prefer to have a seperate small servers for everything. My website alone has three seperate servers, a database, server, and other tools like traffic monitoring. In the months leading up to this it started to get a bit overwhelming to manage these, so I came up with this, a simple reliable way to manage as many servers as you need in one simple to use package. This project is also a learning experience for me, I previously only knew python, javascript, and some other interpreted, high end languages. Which is why I selected C++ and go for this project. They are both compiled languages and C++ allows me to learn pointer and memory managment.

### How it works
Wrens Nest has two main components, the client side (written in C++), and the agent side (written in go). 

The agent side is what is automatically deployd to your server, it is embedded into the C++ binary and acts as a sort of mini http server. It eliminates the majority of major slowdowns by getting rid of both asymetric encyption and live feedback. Instead the agent takes in a AES encrypted http request, runs commands on its end, then send back the result. This is how Wrens Nest is able to provide high pollling rate monitoring.

The client side is what you see, it automatically dploys the agent side to selected servers, sends http commands, displays result data, and sometimes uses SSH or SCP in order to provide for more advanced features that are not possible through http.

