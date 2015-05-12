package com.TsyQi909006258.bt;

import android.annotation.TargetApi;
import java.io.IOException;
import java.io.OutputStream;
import java.lang.String;
import java.util.UUID;
import android.os.Message;
import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.os.Looper;
import android.os.Handler;
import android.view.View.OnClickListener;
import android.view.*;
import android.widget.*;
import android.widget.TextView.OnEditorActionListener;
import android.app.Dialog;
import java.lang.reflect.*;
import java.io.*;
import java.util.*;
import android.view.inputmethod.*;

public class SendRecieve extends Activity {
    private static final String TAG = "ChatAct";
    private BluetoothAdapter btAdapter = null;
    private BluetoothSocket btSocket = null;
    private OutputStream outStream = null;
    private String data="1";
    private Toast toast;
    private boolean ok;
	private int READ=1;
	private String s;
	private boolean h;
    // SPP UUID service 
    private static final UUID MY_UUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");
    /** Called when the activity is first created. **/
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
		setFinishOnTouchOutside(false);
        setContentView(R.layout.dialog_new);
		getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_VISIBLE);
		final EditText editText=(EditText)findViewById(R.id.edit_text);  
		Timer timer=new Timer();
		timer.schedule(new TimerTask()
			{
				public void run()
				{
					InputMethodManager inputmanager=(InputMethodManager)editText.getContext().getSystemService(SendRecieve.INPUT_METHOD_SERVICE);
					inputmanager.showSoftInput(editText,0);
				}
			},200);
		// 监听回车键
		editText.setOnEditorActionListener(new OnEditorActionListener() {  
				@Override  
				public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {  
					data=editText.getText().toString();
					if(data.equals(null)||data.equals(" ")||data.equals("")){Toast.makeText(SendRecieve.this,"未输入", Toast.LENGTH_SHORT).show();}
					else{
						toast=Toast.makeText(SendRecieve.this,data/*String.valueOf(actionId)*/, Toast.LENGTH_SHORT);
						toast.show();  
						toast.setGravity(Gravity.BOTTOM,0,30);
					}
					return false;  
				}   
			});
		Intent iitt = new Intent(SendRecieve.this, FloatWindow.class);
		Bundle ble = new Bundle();  
		ble.putString("temp", "接收的数据");
		iitt.putExtras(ble);
		sendBroadcast(iitt);
		startService(iitt);
    }
	
	@Override
    public void onResume() {
        super.onResume();
		ok=true;
        btAdapter = BluetoothAdapter.getDefaultAdapter();
        checkBTState(); 
        if(ok){
            /*****
             */////
            Log.d(TAG, "...onResume - try connect...");
            Intent intent=this.getIntent();  
            Bundle bundle=intent.getExtras(); 
            String address = bundle.getString("address");  
            // Set up a pointer to the remote node using it's address.
            BluetoothDevice device = btAdapter.getRemoteDevice(address);
            //   Two things are needed to make a connection:
            //   A MAC address, which we got above.
            //   A Service ID or UUID.  In this case we are using the
            //   UUID for SPP.

			try
			{
				System.out.println("Sevice_address---------------'"+address);
				String msg = "接收意外中断: ";

				try {
					btSocket = createBluetoothSocket(device);
				} catch (IOException e) {
					this.btAdapter.disable();
					msg = msg +  "串口已关闭。";
					Toast.makeText(getBaseContext(), "\n"+msg + e.getMessage() + ".", Toast.LENGTH_SHORT).show();
				}

				try {
					btSocket = device.createRfcommSocketToServiceRecord(MY_UUID);
				} catch (IOException e) {
					Toast.makeText(getBaseContext(), e.getMessage() + ".", Toast.LENGTH_SHORT).show();
				}
				// Discovery is resource intensive.  Make sure it isn't going on
				// when you attempt to connect and pass your message.
				btAdapter.cancelDiscovery();
				// Establish the connection.  This will block until it connects.
				Log.d(TAG, "...Connecting...");
				try {
					btSocket.connect();
					Toast.makeText(SendRecieve.this, "蓝牙地址为:\n"+address, Toast.LENGTH_SHORT).show();
					Log.d(TAG, "...Connection ok...");
				} catch (IOException e) {
					try {
						btSocket.close();
						if(btAdapter.getState()==btAdapter.STATE_DISCONNECTED)
							Toast.makeText(getBaseContext(), "\t本次连接不成功,请返回多试几次。\n" + e.getMessage() + ".", Toast.LENGTH_SHORT).show();
                    } catch (IOException e2) {
						Toast.makeText(getBaseContext(), e2.getMessage() + ".", Toast.LENGTH_SHORT).show();
					}
				}
				// Create a data stream so we can talk to server.
				Log.d(TAG, "...Create Socket...");

				try {
					outStream = btSocket.getOutputStream();
				} catch (IOException e) {
					errorExit( e.getMessage() , ".");
				}
				if(!address.equals("00:00:00:00:00:00"))
					new RecieveThread(btSocket).start(); 
			   } catch(IOError e) {}
		}
		ImageView imageView2 = (ImageView) findViewById(R.id.imageView2);
		Button btn0;
		btn0 = (Button) findViewById(R.id.btn0);

		btn0.setOnClickListener(new OnClickListener() {

				public void onClick(View v) {
					sendData("0");
					toast=Toast.makeText(getBaseContext(), "Pause", Toast.LENGTH_SHORT);
					toast.show();
					toast.setGravity(Gravity.TOP|Gravity.RIGHT,50,100);
				}
			});

		imageView2.setOnClickListener(new OnClickListener() {
				public void onClick(View v) {
					sendData(data);
				}
			});
		
    }
	
	class RecieveThread extends Thread {

		private final InputStream mmInStream;
	    @TargetApi(Build.VERSION_CODES.ECLAIR)
        protected RecieveThread(BluetoothSocket socket) {
			InputStream tmpIn = null;
			// Get the input and output streams, using temp objects because
			// member streams are final
			try {
				tmpIn = socket.getInputStream(); // 获取输入流
			} catch (IOException ignored) { }
			mmInStream = tmpIn;
		}

		public void run() {
			Looper.prepare();
			byte[] buffer = new byte[1024]; // buffer store for the stream
			int bytes; // bytes returned from read()   
			// Keep listening to the InputStream until an exception occurs
			while (true) {     
				try {                	
					// Read from the InputStream            
					bytes = mmInStream.read(buffer); // bytes数组返回值，为buffer数组的长度
					// Send the obtained bytes to the UI activity
					String str = new String(buffer, 0, bytes);
					handler.obtainMessage(READ, bytes, -1, str)
						.sendToTarget();     // 压入消息队列
				} catch (Exception e) {
					s=e.getMessage();
					System.out.print(s);
					h=false;
					break;
				}
			}
			Looper.loop(); 
		}    
	}

    Handler handler = new Handler() {  //处理消息队列的Handler对象
		@Override
		public void handleMessage(Message msg) {
			super.handleMessage(msg);
			//处理消息
			if (msg.what==READ) {
				String str = (String)msg.obj;	// 类型转化
				/*myview.append(" "+str);	  // TextView*/
				Intent it = new Intent(SendRecieve.this, SaveToSDCard.class);
				Intent iitt = new Intent(SendRecieve.this, FloatWindow.class);
				Bundle ble = new Bundle();  
				ble.putString("temp", ""+str);
				iitt.putExtras(ble);
				sendBroadcast(iitt);
				startService(iitt);
				it.putExtras(ble);
				sendBroadcast(it);
				startService(it);
			}
			else if(h==false)
				Toast.makeText(SendRecieve.this,"接收不成功："+s+".",Toast.LENGTH_SHORT).show();
		}
	};
	
	private BluetoothSocket createBluetoothSocket(BluetoothDevice device) throws IOException {
        if(Build.VERSION.SDK_INT >= 10){
            try {
                final Method  m = device.getClass().getMethod("createInsecureRfcommSocketToServiceRecord", new Class[] { UUID.class });
                return (BluetoothSocket) m.invoke(device, MY_UUID);
            } catch (Exception e) {
                Log.e(TAG, "Could not create Insecure RFComm Connection",e);
            }
        }
        return  device.createRfcommSocketToServiceRecord(MY_UUID);
    }
	
    private void checkBTState() {

        // Check for Bluetooth support and then check to make sure it is turned on
        // Emulator doesn't support Bluetooth and will return null
        if(btAdapter==null) { 
            errorExit("Fatal Error", "Bluetooth not support");
        } else {
            if (btAdapter.isEnabled()) {
                Log.d(TAG, "...Bluetooth ON...");
            } else {
				this.finish();
				// Prompt user to turn on Bluetooth
                Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
                startActivityForResult(enableBtIntent, 1);
            }
        }
    }

    private void errorExit(String title, String message){
        Toast.makeText(getBaseContext(), title + " - " + message, Toast.LENGTH_LONG).show();
        finish();
    }
	
    private void sendData(String message) {
        byte[] msgBuffer = message.getBytes();
        String hint="数据发送不成功。";
        Log.d(TAG, "...Send data: " + message + "...");
        try {
            outStream.write(msgBuffer);
			if(message.equals(null)||message.equals(" ")||message.equals(""))message="null";
			if(!(btAdapter.getState()==btAdapter.STATE_DISCONNECTED)) {
				toast=Toast.makeText(getBaseContext(), message+"\n已发送。", Toast.LENGTH_SHORT);
				toast.setGravity(Gravity.TOP|Gravity.RIGHT,50,100);
				toast.show();
			}
        } catch (IOException e) {
            this.btAdapter.disable(); 
			Intent enableIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivityForResult(enableIntent, 2);
			this.finish();
            Toast.makeText(getBaseContext(), hint, Toast.LENGTH_LONG).show();
        }
    }
}
