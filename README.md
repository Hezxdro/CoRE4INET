# Phone-Telematics 序列号处理方案

## 概述
本方案用于解决Phone进程和Telematics进程之间通过Binder进行网络状态通信时的消息顺序问题。

## 序列号机制
- 使用0-99的环形序列号
- 环形比较采用"半环原则"：
  - diff = (new - last + 100) % 100
  - diff > 0 && diff < 50：接受（更新的消息）
  - diff == 0：丢弃（重复消息）
  - diff >= 50：丢弃（旧消息）

## Crash场景处理

### 场景1：Telematics Crash后重启
**问题**：Telematics丢失last sequence记录，Phone端sequence继续累加

**解决方案**：
1. Telematics重启后将last sequence初始化为-1
2. 收到的第一个消息无条件接受
3. Phone检测到新listener注册，立即推送所有APN当前状态

**关键代码**：
```java
// Telematics端
if (lastSequence == -1) {
    lastSequence = sequence;
    processNetworkStateChange(phoneId, apnNumber, state);
    return true;
}
```

### 场景2：Phone Crash后重启
**问题**：Phone端sequence重置为0，Telematics端还记着旧的last sequence

**解决方案**：
1. Phone重启后发送特殊广播通知
2. Telematics收到广播后重置last sequence为-1
3. Phone等待Telematics重新注册后推送所有状态

**关键代码**：
```java
// Phone端
public void onPhoneProcessStart() {
    currentSequence = 0;
    sendPhoneRestartBroadcast();
}

// Telematics端
private void handlePhoneRestart() {
    sequenceHandler.reset(); // 重置为-1
    connectToPhoneService();
}
```

## 优势
1. **简单可靠**：逻辑清晰，易于实现和维护
2. **状态同步**：crash后能快速恢复到正确状态
3. **向后兼容**：不影响正常运行时的消息处理

## 注意事项
1. Phone重启广播必须可靠送达
2. Telematics需要持久化关键业务数据，不依赖sequence
3. 建议在关键状态变化时增加日志，便于问题排查