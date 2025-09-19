# 从OkHttp到ConnectivityService的详细调用流程

## 概述

当Android应用使用OkHttp发起网络请求时，数据流并不是直接到达ConnectivityService(CS)的。实际上，OkHttp主要通过标准的Socket API与系统交互，而NetworkRequest是用于网络管理的高级API，两者在不同层面工作。

## 1. OkHttp的网络请求流程

### 1.1 OkHttp内部调用链
```java
// 应用层代码
OkHttpClient client = new OkHttpClient();
Request request = new Request.Builder()
    .url("https://api.example.com/data")
    .build();
client.newCall(request).execute();
```

### 1.2 OkHttp内部执行流程
```
OkHttpClient.newCall()
    ↓
RealCall.execute()
    ↓
Dispatcher.executed()
    ↓
RealCall.getResponseWithInterceptorChain()
    ↓
拦截器链处理:
    - RetryAndFollowUpInterceptor
    - BridgeInterceptor
    - CacheInterceptor
    - ConnectInterceptor (关键！)
    - CallServerInterceptor
```

### 1.3 ConnectInterceptor中的连接建立
```java
// OkHttp的ConnectInterceptor简化流程
public Response intercept(Chain chain) {
    // 1. 从连接池获取或创建新连接
    RealConnection connection = findConnection(...);
    
    // 2. 建立Socket连接
    connection.connect(...);
    
    // 3. 对于HTTPS，进行TLS握手
    if (request.isHttps()) {
        connection.connectTls(...);
    }
}
```

## 2. Socket层的实际调用

### 2.1 创建Socket
```java
// OkHttp最终会调用到
Socket socket = new Socket();

// 或者对于SSL
SSLSocket sslSocket = (SSLSocket) sslSocketFactory.createSocket();
```

### 2.2 Socket连接建立
```java
// 实际的连接调用
socket.connect(new InetSocketAddress(address, port), connectTimeout);
```

### 2.3 从Java到Native的调用链
```
java.net.Socket.connect()
    ↓
PlainSocketImpl.connect()
    ↓
IoBridge.connect()
    ↓
Libcore.os.connect() (Native方法)
    ↓
connect() 系统调用
```

## 3. Linux内核中的处理

### 3.1 Socket系统调用
```c
// 在内核中
sys_connect(int fd, struct sockaddr *addr, int addrlen) {
    // 1. 查找socket结构
    struct socket *sock = sockfd_lookup(fd);
    
    // 2. 调用协议特定的connect函数
    sock->ops->connect(sock, addr, addrlen, flags);
}
```

### 3.2 TCP连接建立
```
tcp_v4_connect()
    ↓
ip_route_connect() // 路由查找
    ↓
tcp_connect() // 发送SYN包
    ↓
ip_queue_xmit() // IP层处理
    ↓
dev_queue_xmit() // 发送到网络设备
```

## 4. ConnectivityService的角色

### 4.1 CS不直接参与数据传输
ConnectivityService主要负责：
- 管理网络策略
- 监控网络状态
- 处理网络切换
- 管理VPN连接

### 4.2 NetworkRequest的真正用途
```java
// NetworkRequest是用于请求特定类型网络的API
NetworkRequest request = new NetworkRequest.Builder()
    .addCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET)
    .addTransportType(NetworkCapabilities.TRANSPORT_CELLULAR)
    .build();

// 注册网络回调
ConnectivityManager cm = getSystemService(ConnectivityManager.class);
cm.requestNetwork(request, new NetworkCallback() {
    @Override
    public void onAvailable(Network network) {
        // 当满足条件的网络可用时回调
    }
});
```

## 5. 实际的网络选择机制

### 5.1 默认网络路由
当OkHttp创建Socket时，系统会自动使用默认网络：
```java
// 内核中的路由选择
// 1. 检查路由表
// 2. 选择合适的网络接口（wlan0, rmnet_data0等）
// 3. 应用防火墙规则（iptables/netfilter）
```

### 5.2 指定Network的情况
```java
// 如果应用想使用特定网络
Network network = ...; // 从NetworkCallback获得
Socket socket = network.getSocketFactory().createSocket();

// 或者绑定进程到特定网络
ConnectivityManager cm = getSystemService(ConnectivityManager.class);
cm.bindProcessToNetwork(network);
```

## 6. 完整的数据流（简化版）

```
应用层：OkHttp发起请求
    ↓
Socket API：创建和连接Socket
    ↓
Bionic库：系统调用封装
    ↓
Linux内核：
    - Socket层：管理连接
    - TCP/IP栈：协议处理
    - Netfilter：防火墙规则
    - 路由子系统：选择出口
    ↓
网络设备驱动：
    - Wi-Fi: wlan驱动
    - 移动数据: rmnet驱动
    ↓
硬件层：实际发送数据
```

## 7. ConnectivityService的监管作用

### 7.1 UID规则管理
```java
// CS通过Netd设置UID的网络访问规则
NetworkManagementService.setUidNetworkRules(uid, rules);
    ↓
Netd守护进程
    ↓
iptables规则：
    iptables -A OUTPUT -m owner --uid-owner 10001 -j DROP
```

### 7.2 网络策略执行
```java
// NetworkPolicyManager与CS协作
// 1. 设置应用的网络访问策略
// 2. CS通知Netd更新防火墙规则
// 3. 内核根据规则允许/拒绝连接
```

## 8. 关键理解点

### 8.1 OkHttp不直接与CS交互
- OkHttp使用标准Socket API
- 系统自动处理网络选择和路由
- CS在后台管理网络策略

### 8.2 NetworkRequest的用途
- 不是每个网络请求都需要NetworkRequest
- NetworkRequest用于请求特定类型的网络
- 普通应用通常使用系统默认网络

### 8.3 实际的网络决策
```
1. 应用创建Socket
2. 内核查看当前默认网络
3. 根据路由表选择网络接口
4. 检查防火墙规则（CS/Netd设置的）
5. 如果允许，建立连接
```

## 9. 代码示例：追踪网络请求

### 9.1 使用TrafficStats监控
```java
// 监控应用的网络使用
int uid = android.os.Process.myUid();
long txBytes = TrafficStats.getUidTxBytes(uid);
long rxBytes = TrafficStats.getUidRxBytes(uid);

// 执行网络请求
OkHttpClient client = new OkHttpClient();
Response response = client.newCall(request).execute();

// 检查流量变化
long newTxBytes = TrafficStats.getUidTxBytes(uid);
long newRxBytes = TrafficStats.getUidRxBytes(uid);

Log.d("Network", "Sent: " + (newTxBytes - txBytes) + " bytes");
Log.d("Network", "Received: " + (newRxBytes - rxBytes) + " bytes");
```

### 9.2 使用Network API控制路由
```java
// 强制使用移动数据网络
ConnectivityManager cm = getSystemService(ConnectivityManager.class);
NetworkRequest request = new NetworkRequest.Builder()
    .addTransportType(NetworkCapabilities.TRANSPORT_CELLULAR)
    .build();

cm.requestNetwork(request, new NetworkCallback() {
    @Override
    public void onAvailable(Network network) {
        // 方式1：绑定整个进程
        cm.bindProcessToNetwork(network);
        
        // 方式2：为OkHttp指定Network
        OkHttpClient client = new OkHttpClient.Builder()
            .socketFactory(network.getSocketFactory())
            .build();
    }
});
```

## 10. 总结

1. **OkHttp到系统的路径**：OkHttp → Socket API → Native Socket → Kernel Socket → TCP/IP Stack → Network Device

2. **CS的作用**：ConnectivityService不直接处理数据传输，而是：
   - 管理网络策略和规则
   - 通过Netd设置iptables规则
   - 处理网络切换和优先级
   - 提供NetworkRequest API供应用请求特定网络

3. **NetworkRequest不是必需的**：普通网络请求不需要创建NetworkRequest，系统会自动使用默认网络。NetworkRequest主要用于：
   - 请求特定类型的网络（如仅移动数据）
   - 监听网络状态变化
   - 绑定到特定网络

4. **实际的网络选择**：发生在内核的路由层，基于路由表和防火墙规则，而不是在应用层。