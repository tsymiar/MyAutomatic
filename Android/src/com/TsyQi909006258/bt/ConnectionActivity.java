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

public class ConnectionActivity extends Activity {
	final Activity me = this;
	public static final String TAG = "TetrisBlast";
    public static final String SYSTEM_EXIT = "exit";
    private MyReceiver receiver;  
	 // Intent request codes
    private static final int REQUEST_DISCOVER_DEVICE = 1;
    private static final int REQUEST_ENABLE_BT = 2;
    
    
    // Member object for the chat services
    private BluetoothMsgService mChatService = null;
    
	// Local Bluetooth adapter
    private BluetoothAdapter mBluetoothAdapter = null;
	
	public boolean onCreateOptionsMenu(Menu menu)
	{
		getMenuInflater().inflate(R.menu.menu_main,menu);
	    return true;
	}
	public boolean onOptionsItemSelected(MenuItem item) { 
    switch (item.getItemId()) { 
	case R.id.item: 
	{
        Intent intent = new Intent(this,WifiActivity.class);
	    startActivity(intent);
		}
		}return true;
		}
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
					me.finish();
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
				// Launch the DeviceListActivity to see devices and do scan
	            Intent serverIntent = new Intent(me, DeviceListActivity.class);
	            startActivityForResult(serverIntent, REQUEST_DISCOVER_DEVICE);
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
    private class MyReceiver extends BroadcastReceiver {  
        @Override  
        public void onReceive(Context context, Intent intent) {  
            finish();  
        }  
    }  
    @Override
	public void onActivityResult(int requestCode, int resultCode, Intent data) {
        Log.d(TAG, "onActivityResult " + resultCode);
        switch (requestCode) {
        case REQUEST_DISCOVER_DEVICE:
          	// When the request to enable Bluetooth returns
           if (resultCode != 0) {
            	Intent intt = new Intent(me, NewActivity.class);
				startActivity(intt);
            } 
		else {
                // User did not enable Bluetooth or an error occured
			Log.d(TAG, "Device not Available");
             }
            break;
        case REQUEST_ENABLE_BT:
            // When the request to enable Bluetooth returns
            if (resultCode == Activity.RESULT_OK) {
                // Bluetooth is now enabled, so set up a chat session
                setupChat();
            } else {
                // User did not enable Bluetooth or an error occured
                Log.d(TAG, "BT not enabled");
                Toast.makeText(this, "请打开蓝牙" , Toast.LENGTH_SHORT).show();
				Intent mainIntent = new Intent(me, MainActivity.class);
				startActivity(mainIntent);
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
            if (mChatService == null) setupChat();
        }
    }
    
    private void setupChat() {
        Log.d(TAG, "setupChat()");
    }

    @Override
    protected void onPause()
    {
        // TODO: Implement this method
        super.onPause();
        
    }
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {

        if (keyCode == KeyEvent.KEYCODE_BACK) {

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
