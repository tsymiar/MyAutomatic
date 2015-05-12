package com.TsyQi909006258.bt;

import android.app.Service;
import android.widget.Toast;
import android.content.Intent;
import android.content.*;
import java.io.*;
import android.os.*;

public class SaveToSDCard extends Service
{
	private String temp=null;
	@Override
	public IBinder onBind(Intent p1)
	{
		// TODO: Implement this method
		return null;
	}

	@Override
	public void onCreate()
	{
		// TODO: Implement this method
		super.onCreate();
	}

	@Override
	public void onStart(Intent intent, int startId)
	{
		// TODO: Implement this method
		super.onStart(intent, startId);
		Bundle bundle = intent.getExtras();
		temp = bundle.getString("temp");
		new Thread(){
            public void run() {
					Message msg=new Message();
					msg.what=0;
					handler.sendMessage(msg);
					try{
						this.sleep(50);
					}catch(InterruptedException e){
						e.printStackTrace();
					}
				}
        }.start();
	}

	Handler handler = new Handler() {  
		@Override
		public void handleMessage(Message msg) {

			if (msg.what==0) {
				try
				{
					File dir=new File(android.os.Environment.getExternalStorageDirectory()
									  + "/Device2Device");  
					if(!dir.exists())dir.mkdirs(); 
					else {
						FileOutputStream fos = new FileOutputStream(
							android.os.Environment.getExternalStorageDirectory()
							+ "/Device2Device/signal.dat",true);
						OutputStreamWriter writer = new OutputStreamWriter(fos);
						writer.write(temp+"\t");
						writer.close();
						fos.close();
						InputStream is = getResources().getAssets().open("signal.dat");
						is.close();
					}
				}
				catch (Exception e)
				{
					Toast.makeText(SaveToSDCard.this, e.getMessage()+" 已保存。", Toast.LENGTH_SHORT).show();
				} 
			} super.handleMessage(msg);
		}
	};

	@Override
	public void onDestroy()
	{
		// TODO: Implement this method
		super.onDestroy();
		stopSelf();
	}
	
}
