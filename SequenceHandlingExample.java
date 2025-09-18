package com.example.telephony;

/**
 * 序列号处理的完整示例
 */
public class SequenceHandlingExample {
    
    public static void main(String[] args) {
        // 演示各种场景
        demonstrateNormalFlow();
        System.out.println("\n" + "=".repeat(50) + "\n");
        
        demonstrateTelematicsCrash();
        System.out.println("\n" + "=".repeat(50) + "\n");
        
        demonstratePhoneCrash();
    }
    
    /**
     * 正常流程演示
     */
    private static void demonstrateNormalFlow() {
        System.out.println("=== 正常流程演示 ===");
        
        TelematicsSequenceHandler handler = new TelematicsSequenceHandler();
        
        // 模拟正常的消息序列
        System.out.println("接收消息 seq=0: " + handler.handleMessage(0, 1, 1, 0)); // true
        System.out.println("接收消息 seq=1: " + handler.handleMessage(0, 1, 0, 1)); // true
        System.out.println("接收消息 seq=0: " + handler.handleMessage(0, 2, 1, 0)); // false (旧消息)
        System.out.println("接收消息 seq=2: " + handler.handleMessage(0, 2, 1, 2)); // true
        
        // 环形边界测试
        System.out.println("\n环形边界测试:");
        handler = new TelematicsSequenceHandler();
        handler.handleMessage(0, 1, 1, 98);  // 初始化为98
        System.out.println("当前 last=98, 接收 seq=99: " + handler.handleMessage(0, 1, 1, 99));  // true
        System.out.println("当前 last=99, 接收 seq=0: " + handler.handleMessage(0, 1, 1, 0));   // true (环形递增)
        System.out.println("当前 last=0, 接收 seq=50: " + handler.handleMessage(0, 1, 1, 50));  // false (diff=50，视为旧消息)
        System.out.println("当前 last=0, 接收 seq=49: " + handler.handleMessage(0, 1, 1, 49));  // true (diff=49)
    }
    
    /**
     * Telematics crash场景演示
     */
    private static void demonstrateTelematicsCrash() {
        System.out.println("=== Telematics Crash 场景演示 ===");
        
        // Phone端序列号管理器
        MockPhoneSequenceManager phoneMgr = new MockPhoneSequenceManager();
        
        // Telematics端序列号处理器
        TelematicsSequenceHandler telematicsHandler = new TelematicsSequenceHandler();
        
        // 正常运行一段时间
        System.out.println("正常运行阶段:");
        for (int i = 0; i < 5; i++) {
            int seq = phoneMgr.getAndIncrement();
            boolean handled = telematicsHandler.handleMessage(0, 1, 1, seq);
            System.out.println("Phone发送 seq=" + seq + ", Telematics处理: " + handled);
        }
        
        // Telematics crash并重启
        System.out.println("\nTelematics crash并重启:");
        telematicsHandler = new TelematicsSequenceHandler(); // 新实例，last=-1
        
        // Phone继续发送（序列号继续）
        System.out.println("Phone继续发送:");
        for (int i = 0; i < 3; i++) {
            int seq = phoneMgr.getAndIncrement();
            boolean handled = telematicsHandler.handleMessage(0, 1, 1, seq);
            System.out.println("Phone发送 seq=" + seq + ", Telematics处理: " + handled);
        }
    }
    
    /**
     * Phone crash场景演示
     */
    private static void demonstratePhoneCrash() {
        System.out.println("=== Phone Crash 场景演示 ===");
        
        // 初始状态
        MockPhoneSequenceManager phoneMgr = new MockPhoneSequenceManager();
        TelematicsSequenceHandler telematicsHandler = new TelematicsSequenceHandler();
        
        // 正常运行到seq=50
        System.out.println("正常运行阶段:");
        phoneMgr.setSequence(48);
        for (int i = 0; i < 3; i++) {
            int seq = phoneMgr.getAndIncrement();
            boolean handled = telematicsHandler.handleMessage(0, 1, 1, seq);
            System.out.println("Phone发送 seq=" + seq + ", Telematics处理: " + handled);
        }
        
        // Phone crash并重启
        System.out.println("\nPhone crash并重启:");
        phoneMgr = new MockPhoneSequenceManager(); // 新实例，seq从0开始
        System.out.println("Phone发送重启广播");
        telematicsHandler.reset(); // Telematics收到广播后重置
        
        // Phone从0开始发送
        System.out.println("Phone重新开始发送:");
        for (int i = 0; i < 3; i++) {
            int seq = phoneMgr.getAndIncrement();
            boolean handled = telematicsHandler.handleMessage(0, 1, 1, seq);
            System.out.println("Phone发送 seq=" + seq + ", Telematics处理: " + handled);
        }
    }
    
    /**
     * 模拟的Phone序列号管理器
     */
    private static class MockPhoneSequenceManager {
        private int sequence = 0;
        
        public int getAndIncrement() {
            int current = sequence;
            sequence = (sequence + 1) % 100;
            return current;
        }
        
        public void setSequence(int seq) {
            this.sequence = seq;
        }
    }
}