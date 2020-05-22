package com.example.tcptestapp;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.widget.Toast;

public class NetworkChangeReceiver extends BroadcastReceiver {
    static {
        System.loadLibrary("native-lib");
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        if (!isNetworkConnected(context))
            return;
        Toast.makeText(context, "changeNetwork!", Toast.LENGTH_LONG);
        //有连接的时候触发，加上判断逻辑
        System.out.println("change!!!");
        reset();
    }

    public static boolean isNetworkConnected(Context context) {
        ConnectivityManager connectivity = (ConnectivityManager)context.getSystemService(Context.CONNECTIVITY_SERVICE);
        if (connectivity == null)
        {
            return false;
        }
        else {
            NetworkInfo[] info = connectivity.getAllNetworkInfo();
            if (info != null) {
                for (int i = 0; i < info.length; i++) {
                    if (info[i].getState() == NetworkInfo.State.CONNECTED) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    public native void reset();

}

