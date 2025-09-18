package com.example.telephony;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.RemoteException;
import android.telephony.data.IMultiInternetCallback;

/**
 * Telematics服务端实现
 */
public class TelematicsService {
    private static final String ACTION_PHONE_RESTARTED = "android.telephony.action.PHONE_RESTARTED";
    
    private Context context;
    private TelematicsSequenceHandler sequenceHandler;
    private IPhoneService phoneService; // Phone服务的接口
    
    // 回调实现
    private final IMultiInternetCallback callback = new IMultiInternetCallback.Stub() {
        @Override
        public void onNetworkStateChanged(int apnNumber, int state, int phoneId, int sequence) {
            // 使用序列号处理器判断是否处理该消息
            boolean shouldProcess = sequenceHandler.handleMessage(phoneId, apnNumber, state, sequence);
            
            if (shouldProcess) {
                // 处理网络状态变化
                handleNetworkStateChange(phoneId, apnNumber, state);
            }
        }
    };
    
    // Phone重启广播接收器
    private final BroadcastReceiver phoneRestartReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (ACTION_PHONE_RESTARTED.equals(intent.getAction())) {
                handlePhoneRestart();
            }
        }
    };
    
    public TelematicsService(Context context) {
        this.context = context;
        this.sequenceHandler = new TelematicsSequenceHandler();
    }
    
    /**
     * 启动Telematics服务
     */
    public void start() {
        // 注册Phone重启广播
        IntentFilter filter = new IntentFilter(ACTION_PHONE_RESTARTED);
        context.registerReceiver(phoneRestartReceiver, filter);
        
        // 连接到Phone服务并注册listener
        connectToPhoneService();
    }
    
    /**
     * Telematics crash后重启的处理
     */
    public void onTelematicsRestart() {
        // 序列号处理器已经自动初始化为-1，无需额外处理
        
        // 重新连接到Phone服务并注册listener
        connectToPhoneService();
    }
    
    /**
     * 处理Phone重启
     */
    private void handlePhoneRestart() {
        System.out.println("Detected Phone restart, resetting sequence handler");
        
        // 重置序列号处理器
        sequenceHandler.reset();
        
        // 重新注册listener
        if (phoneService != null) {
            try {
                phoneService.unregisterCallback(callback);
            } catch (RemoteException e) {
                // Phone已经重启，忽略异常
            }
        }
        
        // 重新连接
        connectToPhoneService();
    }
    
    /**
     * 连接到Phone服务并注册listener
     */
    private void connectToPhoneService() {
        try {
            // 获取Phone服务（具体实现依赖于系统）
            phoneService = getPhoneService();
            
            if (phoneService != null) {
                // 注册回调
                phoneService.registerCallback(callback);
                System.out.println("Successfully registered callback to Phone service");
            }
        } catch (RemoteException e) {
            System.err.println("Failed to connect to Phone service: " + e.getMessage());
            // 可以设置重试机制
        }
    }
    
    /**
     * 处理网络状态变化的业务逻辑
     */
    private void handleNetworkStateChange(int phoneId, int apnNumber, int state) {
        System.out.println(String.format(
            "Network state changed: Phone=%d, APN=%d, State=%s",
            phoneId, apnNumber, state == 1 ? "Connected" : "Disconnected"
        ));
        
        // 实际的业务处理逻辑
        // ...
    }
    
    /**
     * 获取Phone服务（需要根据实际系统实现）
     */
    private IPhoneService getPhoneService() {
        // 这里需要根据实际的系统服务获取方式来实现
        // 例如通过ServiceManager.getService("phone")等
        return null;
    }
    
    /**
     * 停止服务
     */
    public void stop() {
        context.unregisterReceiver(phoneRestartReceiver);
        
        if (phoneService != null) {
            try {
                phoneService.unregisterCallback(callback);
            } catch (RemoteException e) {
                // 忽略异常
            }
        }
    }
}