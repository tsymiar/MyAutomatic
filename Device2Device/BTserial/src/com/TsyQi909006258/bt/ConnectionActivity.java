package com.TsyQi909006258.bt;

import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.content.Context;  
import android.content.BroadcastReceiver;  
import android.view.View.OnClickListener;
import android.content.Intent;  
import android.content.IntentFilter;  
import android.widget.Button;
import android.widget.Toast;
import android.view.Gravity;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.Menu;
import android.view.KeyEvent;
import com.TsyQi909006258.bt.CloseAll;

public class ConnectionActivity extends Activity {
	final Activity me = this;
	public static final String TAG = "Connection";
    public static final String SYSTEM_EXIT = "exit";
    private MyReceiver receiver;  
	 // Intent request codes
    private static final int REQUEST_DISCOVER_DEVICE = 1;
    private static final int REQUEST_ENABLE_BT = 2;
    private int r=0;
    
    // Member object for the chat services
    // private BluetoothMsgService mChatService = null;
    
	// Local Bluetooth adapter
    private BluetoothAdapter mBluetoothAdapter = null;

	/** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.connection);
        IntentFilter filter = new IntentFilter();
        filter.addAction(SYSTEM_EXIT);
        receiver = new MyReceiver();
        this.registerReceiver(receiver, filter);
        Log.d(TAG, "Connection Activity Created");
		CloseAll.getInstance().addActivity(this);  
       // getWindow().setBackgroundDrawableResource(R.drawable.tetris_bg);//Draw background
        Button hostBtn = (Button)findViewById(R.id.btn_host);
        Button joinBtn = (Button)findViewById(R.id.btn_join);                
        hostBtn.setOnClickListener(new OnClickListener() {
			
			@Override
			public void onClick(View v) {
				Log.d(TAG, "ensure discoverable");
				
				//TODO pass parameter to TetrisBalst Activity
				if (mBluetoothAdapter.getScanMode() !=
					BluetoothAdapter.SCAN_MODE_CONNECTABLE_DISCOVERABLE) {
					Intent discoverableIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_DISCOVERABLE);
					discoverableIntent.putExtra(BluetoothAdapter.EXTRA_DISCOVERABLE_DURATION, 300);
					startActivityForResult(discoverableIntent, REQUEST_DISCOVER_DEVICE);
					}
				else {
					Toast toast;
					toast=Toast.makeText(ConnectionActivity.this, "可检测性已开启", Toast.LENGTH_SHORT);
					toast.show();
					toast.setGravity(Gravity.CENTER,0,0);
				}
			}
		});
        
        joinBtn.setOnClickListener(new OnClickListener() {
			
			@Override
			public void onClick(View v) {
				if (!mBluetoothAdapter.isEnabled()) {
					Toast toast;
					toast=Toast.makeText(ConnectionActivity.this, "请打开蓝牙后重试", Toast.LENGTH_SHORT);
					toast.show();
					toast.setGravity(Gravity.CENTER,0,0);
					Intent enableIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
					startActivityForResult(enableIntent, REQUEST_ENABLE_BT);
					// Otherwise, setup the chat session
				} else {
					// Launch the DeviceListActivity to see devices and do scan
					Intent serverIntent = new Intent(me, DeviceListActivity.class);
					startActivityForResult(serverIntent, REQUEST_DISCOVER_DEVICE);
				}
			}
		});
        
        // Get local Bluetooth adapter
        mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();

        // If the adapter is null, then Bluetooth is not supported
        if (mBluetoothAdapter == null) {
            Toast.makeText(this, "设备不受支持", Toast.LENGTH_SHORT).show();
            onPause();
        }       
    }

    public boolean onCreateOptionsMenu(Menu menu)
    {
		// getMenuInflater().inflate(R.menu.menu_main,menu);
        getMenuInflater().inflate(R.menu.menu_new,menu);
        return true;
    }

    public boolean onOptionsItemSelected(MenuItem item) { 
		switch (item.getItemId()) { 
			case R.id.item: 
				{
					Intent intent = new Intent(this,WifiActivity.class);
					startActivity(intent);
				}
			}
        // Handle item selection 
        switch (item.getItemId()) {
            case R.id.url: 
                openurl(); 
                return true; 
            case R.id.feedback: 
                feedback(); 
                return true; 
            case R.id.about:
                about();
                return true;
            case R.id.wificonnect:
                wificonnect();
                return true;
			case R.id.exit:
				exit();
				return true;
            default: 
                return super.onOptionsItemSelected(item); 
        } 
	}
	
    private class MyReceiver extends BroadcastReceiver {  
        @Override  
        public void onReceive(Context context, Intent intent) {  
            finish();  
        }  
    }  
	private void openurl()
    {
        Intent intent = new Intent(this,OpenUrl.class);
        startActivity(intent);
    }
    private void feedback(){
        Intent intent = new Intent(this,Feedback.class);
        startActivity(intent);
    }
    private void about(){
        Intent intent = new Intent(this,About.class);
        startActivity(intent);
    }
    private void wificonnect(){
        Intent intent = new Intent(this,WifiActivity.class);
        startActivity(intent);
    }
	private void exit() {
	CloseAll.getInstance().exit(); 
	}
   
    @Override
	public void onActivityResult(int requestCode, int resultCode, Intent data) {
        Log.d(TAG, "onActivityResult " + resultCode);
		r++;
        switch (requestCode) {
			case REQUEST_DISCOVER_DEVICE:
				// When the request to enable Bluetooth returns
				if (resultCode != 0) { } 
				else {
					// User did not enable Bluetooth or an error occured
					Log.d(TAG, "Device not Available");
				}
				break;
			case REQUEST_ENABLE_BT:
				// When the request to enable Bluetooth returns
				if (resultCode == Activity.RESULT_OK) {
					// Bluetooth is now enabled, so set up a chat session
				} else {
					// User did not enable Bluetooth or an error occured
					Log.d(TAG, "BT not enabled");
					Toast.makeText(this, "请打开蓝牙" , Toast.LENGTH_SHORT).show();
					if(r<=2) {
						Intent enableIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
						startActivityForResult(enableIntent, REQUEST_ENABLE_BT);
					}
				}
        }
    }
	
    @Override
    public void onStart() {
        super.onStart();
        // If BT is not on, request that it be enabled.
        // setupChat() will then be called during onActivityResult
        if (!mBluetoothAdapter.isEnabled()) {
            Intent enableIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivityForResult(enableIntent, REQUEST_ENABLE_BT);
        // Otherwise, setup the chat session
        } else {
           // if (mChatService == null) setupChat();
        }
    }
	/*
	private void setupChat() {
        Log.d(TAG, "setupChat()");
    }*/
  
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {

        if (keyCode == KeyEvent.KEYCODE_BACK) {
			Intent intent = new Intent(this,MainActivity.class);
			startActivity(intent);
			finish();
        }     
        return super.onKeyDown(keyCode, event);
	}

    @Override
    protected void onDestroy()
    {
        this.unregisterReceiver(receiver); 
        // TODO: Implement this method
        super.onDestroy();
    }   
}
