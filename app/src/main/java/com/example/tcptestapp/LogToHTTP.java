package com.example.tcptestapp;

import android.util.Log;

import java.io.File;
import java.io.IOException;
import java.util.Random;

import okhttp3.MediaType;
import okhttp3.MultipartBody;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;
import okhttp3.ResponseBody;

public class LogToHTTP {

    private static final String TAG = "============";

    private static String url="http://202.120.38.131:14673/uploadfile/";
//    private static String url="http://192.168.1.161:9999/uploadfile/";

    public static String  uploadLog(String filename)  {

        OkHttpClient client=new OkHttpClient();

        File logFile=new File(filename);

        if(logFile.exists()){
            System.out.println("文件已经存在！");
        }else{
            //文件不存在则创建该文件
            try {
                logFile.createNewFile();
                System.out.println("文件创建成功!");
            } catch (IOException e) {
                e.printStackTrace();
            }
        }

        Log.d(TAG, "uploadLog: 上传文件的路径："+logFile.getAbsolutePath());

        long time = System.currentTimeMillis();

        RequestBody requestBody=new MultipartBody.Builder()
                .setType(MultipartBody.FORM)
                .addFormDataPart("myfile","TCP" + time +".log",RequestBody.create(MediaType.parse("multipart/form-data"), logFile))
                .build();

        Request request=new Request.Builder()
                .url(url)
                .post(requestBody)
                .build();

        String responseString="";

        Response response=null;

        try {
            Random r=new Random();
            int delay=r.nextInt(1000);
            Log.d(TAG, "run: 等待随机秒数："+delay);

            Thread.sleep(delay);
            response=client.newCall(request).execute();
            responseString=response.body().string();
            Log.d(TAG, "run: 上传结果："+response.body().string());
        } catch (Exception e) {
            e.printStackTrace();
        }
        return responseString;
    }

}
