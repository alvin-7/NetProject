# NetProject
> 基于select/epoll的网络通信架构

### 使用方式

1. 在系统变量中添加MinGW环境变量 `MinGW : D\Env\MinGW64\mingw32`

### 服务端

1. 使用Socket通信
2. 使用Select网络模型
3. 消息接收缓冲区和消息发送缓冲区
4. 多线程设计，4个接收线程，1个发送线程

### 客户端

1. Socket通信
2. 使用Select网络模型
3. 消息接收缓存区

