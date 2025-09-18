# Mermaid时序图使用说明

## 场景1：Telematics Crash后重启

```mermaid
sequenceDiagram
    title 场景1：Telematics Crash后重启的序列号处理
    
    participant Phone as Phone进程
    participant Telematics as Telematics进程
    
    Note over Phone: 正常运行中<br/>当前sequence=45
    Note over Telematics: 正常运行中<br/>lastSequence=44
    
    Phone->>Telematics: onNetworkStateChanged(apn=1, state=1, phone=0, seq=45)
    Note over Telematics: 接受消息<br/>lastSequence=45
    
    Phone->>Telematics: onNetworkStateChanged(apn=2, state=0, phone=0, seq=46)
    Note over Telematics: 接受消息<br/>lastSequence=46
    
    Note over Telematics: ❌ Crash!
    Note over Phone: 继续运行<br/>sequence继续累加
    
    Note over Telematics: ✅ 重启完成<br/>lastSequence=-1<br/>(未初始化状态)
    
    Telematics->>Phone: registerListener(callback)
    Note over Phone: 检测到新listener注册<br/>当前sequence=52
    
    rect rgb(200, 230, 200)
        Note over Phone,Telematics: Phone推送所有APN当前状态
        Phone->>Telematics: onNetworkStateChanged(apn=1, state=1, phone=0, seq=52)
        Note over Telematics: lastSequence=-1<br/>无条件接受第一个消息<br/>更新lastSequence=52
        
        Phone->>Telematics: onNetworkStateChanged(apn=2, state=0, phone=0, seq=53)
        Note over Telematics: 正常环形比较<br/>(53-52+100)%100=1<br/>接受，lastSequence=53
        
        Phone->>Telematics: onNetworkStateChanged(apn=3, state=1, phone=0, seq=54)
        Note over Telematics: 正常环形比较<br/>(54-53+100)%100=1<br/>接受，lastSequence=54
        
        Phone->>Telematics: onNetworkStateChanged(apn=1, state=1, phone=1, seq=55)
        Note over Telematics: 继续正常处理...
    end
    
    Note over Phone,Telematics: 恢复正常运行状态
```

## 场景2：Phone Crash后重启

```mermaid
sequenceDiagram
    title 场景2：Phone Crash后重启的序列号处理
    
    participant Phone as Phone进程
    participant Telematics as Telematics进程
    participant System as 系统广播
    
    Note over Phone: 正常运行中<br/>当前sequence=88
    Note over Telematics: 正常运行中<br/>lastSequence=87
    
    Phone->>Telematics: onNetworkStateChanged(apn=1, state=1, phone=0, seq=88)
    Note over Telematics: 接受消息<br/>lastSequence=88
    
    Phone->>Telematics: onNetworkStateChanged(apn=2, state=0, phone=0, seq=89)
    Note over Telematics: 接受消息<br/>lastSequence=89
    
    Note over Phone: ❌ Crash!
    Note over Telematics: 继续运行<br/>lastSequence=89
    
    Note over Phone: ✅ 重启完成<br/>sequence=0<br/>(重新开始)
    
    rect rgb(255, 220, 200)
        Note over Phone,System: Phone发送重启广播
        Phone->>System: sendBroadcast(ACTION_PHONE_RESTARTED)
        System->>Telematics: onReceive(ACTION_PHONE_RESTARTED)
        
        Note over Telematics: 收到Phone重启广播<br/>重置lastSequence=-1
    end
    
    Telematics->>Phone: unregisterListener(旧callback)
    Note over Telematics: 清理旧连接
    
    Telematics->>Phone: registerListener(新callback)
    Note over Phone: 接受新listener注册
    
    rect rgb(200, 230, 200)
        Note over Phone,Telematics: Phone推送所有APN当前状态（从seq=0开始）
        Phone->>Telematics: onNetworkStateChanged(apn=1, state=1, phone=0, seq=0)
        Note over Telematics: lastSequence=-1<br/>无条件接受第一个消息<br/>更新lastSequence=0
        
        Phone->>Telematics: onNetworkStateChanged(apn=2, state=0, phone=0, seq=1)
        Note over Telematics: 正常环形比较<br/>(1-0+100)%100=1<br/>接受，lastSequence=1
        
        Phone->>Telematics: onNetworkStateChanged(apn=3, state=1, phone=0, seq=2)
        Note over Telematics: 正常环形比较<br/>(2-1+100)%100=1<br/>接受，lastSequence=2
        
        Note over Phone,Telematics: 继续推送其他APN状态...
    end
    
    Note over Phone,Telematics: 恢复正常运行状态<br/>Phone从0继续递增<br/>Telematics正常处理
```

## 使用方法

1. 将上面的mermaid代码复制到支持Mermaid的Markdown编辑器中
2. 或者使用在线工具如 [Mermaid Live Editor](https://mermaid.live/)
3. 也可以在支持Mermaid的文档工具中直接使用（如GitHub、GitLab等）

## 关键点说明

### 场景1关键点：
- Telematics crash后lastSequence重置为-1
- 第一个消息无条件接受，不进行环形比较
- Phone检测到新listener后主动推送全量状态

### 场景2关键点：
- Phone crash后发送重启广播
- Telematics收到广播后主动重置lastSequence为-1
- Phone sequence从0重新开始
- 通过广播机制保证双方状态同步