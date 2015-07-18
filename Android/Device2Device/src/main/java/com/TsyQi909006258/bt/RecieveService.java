package com.TsyQi909006258.bt;

import android.annotation.SuppressLint;
import android.app.Notification;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.PixelFormat;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.TextView;

public class RecieveService extends Service
{
	WindowManager wm = null;

	WindowManager.LayoutParams wmParams = null;

	View view;
	Bundle bundle=new Bundle();
	boolean added=false;

	private String temp=null;
	private float mTouchStartX;
	private float mTouchStartY;
	private TextView myview=null;
	private float x;
	private float y;
	private long mLastTime, mCurTime ;

	@SuppressLint("InflateParams")
	@Override
	public void onCreate() {

		super.onCreate();
		Notification notification = new Notification(R.drawable.ooopic, getText(R.string.edit_text),
													 System.currentTimeMillis());
		Intent notificationIntent = new Intent(this, DeviceListActivity.class);
		PendingIntent pendingIntent = PendingIntent.getActivity(this, 0, notificationIntent, 0);
		notification.setLatestEventInfo(this, getText(R.string.recieve),
										getText(R.string.back), pendingIntent);
		startForeground(1, notification);
		view = LayoutInflater.from(this).inflate(R.layout.float_window, null);
		myview=(TextView)view.findViewById(R.id.notification);
	}

	@Override
	public void onStart(Intent intent, int startId) {
		super.onStart(intent, startId);
		createView();
		bundle = intent.getExtras();
		temp = bundle.getString("temp");
		if ((bundle != null)&&(temp!=null)) {
			new Thread() {
				public void run() {
					Message msg = new Message();
					msg.what = 0;
					handler.sendMessage(msg);
					try {
						sleep(50);
					} catch (InterruptedException e) {
						e.printStackTrace();
					}
				}
			}.start();
		}
		myview.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				mLastTime = mCurTime;
				mCurTime = System.currentTimeMillis();
				if (mCurTime - mLastTime < 300) {
					wm.removeView(view);
				}
			}
		});
	}

	Handler handler = new Handler() {  
		@Override
		public void handleMessage(Message msg) {

			if (msg.what==0) {
				myview.setText(temp+"");
			} super.handleMessage(msg);
		}
	};

	private void createView() {
		wm = (WindowManager) getApplicationContext().getSystemService(Context.WINDOW_SERVICE);

		wmParams = new WindowManager.LayoutParams();
		wmParams.type = WindowManager.LayoutParams.TYPE_SYSTEM_ERROR;
		wmParams.flags |= WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE;
		wmParams.gravity = 51;

		wmParams.x = 250;
		wmParams.y = 0;

		wmParams.width = WindowManager.LayoutParams.WRAP_CONTENT;
		wmParams.height = WindowManager.LayoutParams.WRAP_CONTENT;
		wmParams.format = PixelFormat.RGBA_8888;
		if(added)
		{
			wm.updateViewLayout(view,wmParams);
			}
		else
		{
			wm.addView(view, wmParams);
			added=true;
		}
		view.setOnTouchListener(new View.OnTouchListener() {

			public boolean onTouch(View v, MotionEvent event) {

				x = event.getRawX();
				y = event.getRawY();

				switch (event.getAction()) {

					case MotionEvent.ACTION_DOWN:
						mTouchStartX = event.getX();
						mTouchStartY = event.getY() + view.getHeight() / 2;
						break;
					case MotionEvent.ACTION_MOVE:
						myview.setTextColor(Color.rgb(0, 255, 0));
						updateViewPosition();
						break;
					case MotionEvent.ACTION_UP:
						myview.setTextColor(Color.rgb(255, 255, 255));
						updateViewPosition();
						mTouchStartX = mTouchStartY = 0;
						break;
				}
				return true;
			}
		});
	}

	private void updateViewPosition() {

		wmParams.x = (int) (x - mTouchStartX);
		wmParams.y = (int) (y - mTouchStartY);
		wm.updateViewLayout(view, wmParams);
	}

	@Override
	public IBinder onBind(Intent intent) {
		return null;
	}
}
