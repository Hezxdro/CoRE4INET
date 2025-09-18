package com.example.telephony;

/**
 * Telematics端的序列号处理器
 */
public class TelematicsSequenceHandler {
    private static final int SEQUENCE_MAX = 100;
    private static final int HALF_RING = 50;
    
    // 使用-1表示未初始化状态
    private int lastSequence = -1;
    
    /**
     * 处理接收到的消息
     * @param phoneId 卡槽ID (0/1)
     * @param apnNumber APN编号 (1/2/3)
     * @param state 连接状态 (0断开/1连接)
     * @param sequence 序列号 (0-99)
     * @return true表示消息被接受处理，false表示丢弃
     */
    public boolean handleMessage(int phoneId, int apnNumber, int state, int sequence) {
        // 第一次接收消息（crash重启后的情况）
        if (lastSequence == -1) {
            lastSequence = sequence;
            processNetworkStateChange(phoneId, apnNumber, state);
            return true;
        }
        
        // 环形序号比较
        int diff = (sequence - lastSequence + SEQUENCE_MAX) % SEQUENCE_MAX;
        
        if (diff > 0 && diff < HALF_RING) {
            // 更新的消息，接受处理
            lastSequence = sequence;
            processNetworkStateChange(phoneId, apnNumber, state);
            return true;
        } else if (diff == 0) {
            // 重复消息，丢弃
            System.out.println("Duplicate message, sequence: " + sequence);
            return false;
        } else {
            // 旧消息（diff >= 50），丢弃
            System.out.println("Old message, sequence: " + sequence + ", last: " + lastSequence);
            return false;
        }
    }
    
    /**
     * 重置序列号处理器（用于Phone crash场景）
     */
    public void reset() {
        lastSequence = -1;
    }
    
    private void processNetworkStateChange(int phoneId, int apnNumber, int state) {
        // 实际的业务处理逻辑
        System.out.println(String.format("Processing: Phone=%d, APN=%d, State=%d", 
                          phoneId, apnNumber, state));
    }
}