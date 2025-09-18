package com.example.telephony;

import android.content.Intent;
import android.os.RemoteCallbackList;
import android.telephony.data.IMultiInternetCallback;

/**
 * Phone端的序列号管理器
 */
public class PhoneSequenceManager {
    private static final int SEQUENCE_MAX = 100;
    private static final String ACTION_PHONE_RESTARTED = "android.telephony.action.PHONE_RESTARTED";
    
    private int currentSequence = 0;
    private RemoteCallbackList<IMultiInternetCallback> callbacks = new RemoteCallbackList<>();
    
    // 网络状态缓存，用于新listener注册时推送当前状态
    private NetworkStateCache stateCache = new NetworkStateCache();
    
    /**
     * Phone进程启动时调用
     */
    public void onPhoneProcessStart() {
        // 重置序列号
        currentSequence = 0;
        
        // 发送重启广播
        sendPhoneRestartBroadcast();
    }
    
    /**
     * 注册新的listener
     */
    public void registerCallback(IMultiInternetCallback callback) {
        callbacks.register(callback);
        
        // 立即推送所有APN的当前状态给新注册的listener
        pushAllCurrentStates(callback);
    }
    
    /**
     * 网络状态变化时调用
     */
    public void notifyNetworkStateChanged(int phoneId, int apnNumber, int state) {
        // 更新缓存
        stateCache.updateState(phoneId, apnNumber, state);
        
        // 获取当前序列号
        int sequence = getCurrentSequence();
        
        // 广播给所有注册的listener
        int count = callbacks.beginBroadcast();
        for (int i = 0; i < count; i++) {
            try {
                callbacks.getBroadcastItem(i).onNetworkStateChanged(
                    apnNumber, state, phoneId, sequence);
            } catch (Exception e) {
                // 处理远程调用异常
            }
        }
        callbacks.finishBroadcast();
        
        // 序列号递增
        incrementSequence();
    }
    
    /**
     * 获取当前序列号（用于发送）
     */
    private synchronized int getCurrentSequence() {
        return currentSequence;
    }
    
    /**
     * 序列号递增
     */
    private synchronized void incrementSequence() {
        currentSequence = (currentSequence + 1) % SEQUENCE_MAX;
    }
    
    /**
     * 推送所有当前状态给特定listener
     */
    private void pushAllCurrentStates(IMultiInternetCallback callback) {
        for (int phoneId = 0; phoneId <= 1; phoneId++) {
            for (int apnNumber = 1; apnNumber <= 3; apnNumber++) {
                NetworkState state = stateCache.getState(phoneId, apnNumber);
                if (state != null) {
                    try {
                        int sequence = getCurrentSequence();
                        callback.onNetworkStateChanged(
                            apnNumber, state.getState(), phoneId, sequence);
                        incrementSequence();
                    } catch (Exception e) {
                        // 处理远程调用异常
                    }
                }
            }
        }
    }
    
    /**
     * 发送Phone重启广播
     */
    private void sendPhoneRestartBroadcast() {
        Intent intent = new Intent(ACTION_PHONE_RESTARTED);
        // 发送广播的具体实现
    }
    
    /**
     * 网络状态缓存
     */
    private static class NetworkStateCache {
        // states[phoneId][apnNumber-1] 存储每个APN的状态
        private NetworkState[][] states = new NetworkState[2][3];
        
        public void updateState(int phoneId, int apnNumber, int state) {
            if (phoneId >= 0 && phoneId <= 1 && apnNumber >= 1 && apnNumber <= 3) {
                states[phoneId][apnNumber - 1] = new NetworkState(state);
            }
        }
        
        public NetworkState getState(int phoneId, int apnNumber) {
            if (phoneId >= 0 && phoneId <= 1 && apnNumber >= 1 && apnNumber <= 3) {
                return states[phoneId][apnNumber - 1];
            }
            return null;
        }
    }
    
    private static class NetworkState {
        private final int state;
        
        public NetworkState(int state) {
            this.state = state;
        }
        
        public int getState() {
            return state;
        }
    }
}