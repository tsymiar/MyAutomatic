package bulutooth.oscillogragh.myapplication;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.app.Activity;
import android.app.Fragment;
import android.bluetooth.BluetoothAdapter;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.Switch;
import android.widget.Toast;

@TargetApi(Build.VERSION_CODES.ECLAIR)
public class MainActivity extends Activity {

    private static final String TAG = "MainActivity";
	final Activity me = this;
	private long mCurTime;
	private static final int REQUEST_ENABLE_BT = 2;
	private static final int REQUEST_DISCOVER_DEVICE = 1;
	// Local Bluetooth adapter
    private BluetoothAdapter mBtAdapter = BluetoothAdapter.getDefaultAdapter();
	
	@Override
        protected void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
			CloseAll.getInstance().addActivity(this);  
            // Setup the window
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
				
				Window window = getWindow();
				// Translucent status bar
				window.setFlags(
					WindowManager.LayoutParams.FLAG_TRANSLUCENT_NAVIGATION,
					WindowManager.LayoutParams.FLAG_TRANSLUCENT_NAVIGATION);
				// Translucent navigation bar
				window.setFlags(
					WindowManager.LayoutParams.FLAG_TRANSLUCENT_NAVIGATION,
					WindowManager.LayoutParams.FLAG_TRANSLUCENT_NAVIGATION);
				getWindow().addFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS);
    		}
            requestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);
            setContentView(R.layout.fragment_main);

            // Set result CANCELED incase the user backs out
            setResult(Activity.RESULT_CANCELED);

            // Initialize the button to perform device discovery
            Button joinBtn = (Button)findViewById(R.id.btn_join);
			Button hostBtn = (Button)findViewById(R.id.btn_host);
            Switch BT = (Switch)findViewById(R.id.fragmentmainSwitch1);
			
            joinBtn.setOnClickListener(new OnClickListener() {
					@TargetApi(Build.VERSION_CODES.ECLAIR)
					@Override
					public void onClick(View v) {
						if (!mBtAdapter.isEnabled()) {
							Intent enableIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
							startActivityForResult(enableIntent, REQUEST_ENABLE_BT);
							// Otherwise, setup the chat session
						} else {
							Intent serverIntent = new Intent(me, /*DrawChart*/DeviceListActivity.class);
							startActivityForResult(serverIntent, REQUEST_DISCOVER_DEVICE);
						}
					}
				});
				
			hostBtn.setOnClickListener(new OnClickListener() {
					@Override
					public void onClick(View v) {
						Log.d(TAG, "ensure discoverable");

						//TODO pass parameter to Activity
						if (mBtAdapter.getScanMode() !=
							BluetoothAdapter.SCAN_MODE_CONNECTABLE_DISCOVERABLE) {
							Intent discoverableIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_DISCOVERABLE);
							discoverableIntent.putExtra(BluetoothAdapter.EXTRA_DISCOVERABLE_DURATION, 300);
							startActivityForResult(discoverableIntent, REQUEST_DISCOVER_DEVICE);
						}
						else {
							Toast.makeText(me, "可检测性已开启", Toast.LENGTH_SHORT).show();
						}
					}
				});
			BT.setOnClickListener(new OnClickListener() {
					@TargetApi(Build.VERSION_CODES.ECLAIR)
					@Override
					public void onClick(View v) {
						if (!mBtAdapter.isEnabled()) {
							Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
							startActivity(enableBtIntent);
						}
						else{
							mBtAdapter.disable(); 
						}
					}
				});
			// If the adapter is null, then Bluetooth is not supported
			if (mBtAdapter == null) {
				Toast.makeText(this, "设备不受支持", Toast.LENGTH_SHORT).show();
				onPause();
			}       
		}
		class MyReceiver extends BroadcastReceiver {
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
						Intent intt = new Intent(me, MyService.class);
						startService(intt);
					} 
					else {
					// User did not enable Bluetooth or an error occured
						Log.d(TAG, "Device not Available");
					}
					break;
				case REQUEST_ENABLE_BT:
					// When the request to enable Bluetooth returns
					switch (resultCode) {
						case Activity.RESULT_OK:
							break;
						default:
							// User did not enable Bluetooth or an error occured
							Log.d(TAG, "BT not enabled");
							Toast.makeText(this, "请打开蓝牙", Toast.LENGTH_SHORT).show();
						/* Intent mainIntent = new Intent(me, MainActivity.class);
						startActivity(mainIntent); */
							break;
					}
			}
		}
        /**
     * A placeholder fragment containing a simple view.
     */
    @SuppressLint("ValidFragment")
    @TargetApi(Build.VERSION_CODES.HONEYCOMB)
    static class PlaceholderFragment extends Fragment {

        public PlaceholderFragment() {
        }

        @Override
        public View onCreateView(LayoutInflater inflater, ViewGroup container,
                                 Bundle savedInstanceState) {
            View rootView;
            rootView = inflater.inflate(R.layout.fragment_main, container, false);
            return rootView;
        }
    }
	
	@Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {

		long mLastTime = mCurTime;
        mCurTime = System.currentTimeMillis();

        if (keyCode == KeyEvent.KEYCODE_BACK) {

            if(mCurTime - mLastTime < 800) {
				CloseAll.getInstance().exit();
            }
            else if(mCurTime - mLastTime >= 800)
            {
                Toast.makeText(MainActivity.this,"双击退出", Toast.LENGTH_SHORT).show();
                return true;
            }

        }
        return super.onKeyDown(keyCode, event);
	}
}
