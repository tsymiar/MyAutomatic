package com.TsyQi909006258.bt;

import android.graphics.PixelFormat;
import android.app.Service;
import android.app.Notification;
import android.content.Intent;
import android.app.PendingIntent;
import android.view.*;
import android.widget.*;
import android.os.*;
import android.widget.ActionMenuView.*;

public class FloatWindow extends Service
{
	WindowManager wm = null;

	WindowManager.LayoutParams wmParams = null;

	View view;
	boolean added=false;

	private String temp=null;
	private float mTouchStartX;
	private float mTouchStartY;
	private TextView myview=null;
	private float x;
	private float y;

	@Override
	public void onCreate() {

		super.onCreate();
		Notification notification = new Notification(R.drawable.bt, getText(R.string.edit_text),
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
				//////**
				myview.setText(temp+"");
			} super.handleMessage(msg);
		}
	};


	private void createView() {
		wm = (WindowManager) getApplicationContext().getSystemService("window");

		wmParams = new WindowManager.LayoutParams();
		wmParams.type = WindowManager.LayoutParams.TYPE_SYSTEM_ERROR;
		wmParams.flags |= WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE;
		wmParams.gravity = Gravity.LEFT | Gravity.TOP;

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
		view.setOnTouchListener(new OnTouchListener() {

				public boolean onTouch(View v, MotionEvent event) {

					x = event.getRawX();
					y = event.getRawY();

					switch (event.getAction()) {

						case MotionEvent.ACTION_DOWN:
							mTouchStartX = event.getX();
							mTouchStartY = event.getY() + view.getHeight() / 2;
							break;

						case MotionEvent.ACTION_MOVE:
							updateViewPosition();
							break;

						case MotionEvent.ACTION_UP:
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
