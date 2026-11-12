package com.omnipay.sdk.net;

import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * Monitors mobile connectivity transitions (Wi-Fi, 5G, Offline).
 * Emits callbacks to trigger offline queue re-synchronization.
 */
public final class NetworkStateMonitor {
    public interface ConnectivityListener {
        void onConnectivityChanged(boolean isConnected);
    }

    private final AtomicBoolean isConnected = new AtomicBoolean(true);
    private final CopyOnWriteArrayList<ConnectivityListener> listeners = new CopyOnWriteArrayList<>();

    public void addListener(ConnectivityListener listener) {
        if (listener != null) listeners.add(listener);
    }

    public void removeListener(ConnectivityListener listener) {
        listeners.remove(listener);
    }

    public boolean isConnected() {
        return isConnected.get();
    }

    public void setConnected(boolean connected) {
        boolean old = isConnected.getAndSet(connected);
        if (old != connected) {
            for (ConnectivityListener listener : listeners) {
                listener.onConnectivityChanged(connected);
            }
        }
    }
}
