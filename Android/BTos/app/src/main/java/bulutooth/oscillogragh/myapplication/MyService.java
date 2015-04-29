package bulutooth.oscillogragh.myapplication;

import android.annotation.TargetApi;
import android.app.AlertDialog;
import android.app.IntentService;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Build;
import android.os.Bundle;
import android.os.Looper;
import android.os.Handler;
import android.os.SystemClock;
import android.os.Message;
import android.util.Log;
import android.view.WindowManager;
import android.widget.Toast;
import bulutooth.oscillogragh.myapplication.WaveChart;

import java.io.IOError;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.Method;
import java.util.UUID;

@TargetApi(Build.VERSION_CODES.CUPCAKE)
public class MyService extends IntentService {

    private static final String TAG = "MyService";
    private BluetoothAdapter btAdapter = null;
    private BluetoothSocket btSocket = null;
	private OutputStream outStream = null;
	private Handler mHandler;
    // SPP UUID service
    private /*static final*/ UUID MY_UUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");
	private boolean fl= true;
	private boolean h;
	private int READ=1;
	private String s;
	private Context mBase;
	
	public MyService() {
        super("MyService");
		Log.i(TAG,this+" is constructed");
    }
	
    @TargetApi(Build.VERSION_CODES.ECLAIR)
	private BluetoothSocket createBluetoothSocket(BluetoothDevice device) throws IOException {
        if(Build.VERSION.SDK_INT >= 10) try {
            final Method m = device.getClass().getMethod("createInsecureRfcommSocketToServiceRecord", new Class[]{UUID.class});
            return (BluetoothSocket) m.invoke(device, MY_UUID);
        } catch (Exception e) {
            Log.e(TAG, "Could not create Insecure RFComm Connection", e);
        }
        return  device.createRfcommSocketToServiceRecord(MY_UUID);
    }
	@Override
    public Intent registerReceiver(
		BroadcastReceiver receiver, IntentFilter filter) {
        return mBase.registerReceiver(receiver, filter);
    }
    /*
    // 反射获取资源id
    public static int getCompentID(String packageName, String className,String idName) {
        int id = 0;
        try {
            Class<?> cls = Class.forName( packageName + ".R$" + className);
            id = cls.getField(idName).getInt(cls);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return id;
    }
    */
    @Override
    protected void onHandleIntent(Intent intent) {
        String address=intent.getStringExtra("address");
        // 定义NotificationManager
        String notice = Context.NOTIFICATION_SERVICE;
        NotificationManager mNotificationManager = (NotificationManager) getSystemService(notice);
        // 定义通知栏展现的内容信息
        CharSequence tickerText = "Wave";
        long when = System.currentTimeMillis();
        Notification notification = new Notification(R.drawable.ic_bt, tickerText, when);
        // 定义下拉通知栏时要展现的内容信息
        Context context = getApplicationContext();
        CharSequence contentTitle = "后台服务已开启。";
        CharSequence contentText = "接收数据中……地址:"+address;
        Intent notificationIntent = new Intent(this, DeviceListActivity.class);
        PendingIntent contentIntent = PendingIntent.getActivity(this, 0,notificationIntent, PendingIntent.FLAG_UPDATE_CURRENT );
        notification.setLatestEventInfo(context, contentTitle, contentText,contentIntent);
        // 用mNotificationManager的notify方法通知用户生成标题栏消息通知
        mNotificationManager.notify(0, notification);
    }
    // dialog窗口。
    private void dialog() {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("Service");
        builder.setMessage("你确定要关闭服务吗?");
        builder.setNegativeButton("×", new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
            }
        });
        builder.setPositiveButton("√", new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
				stopSelf();
            }
        });

        final AlertDialog dialog = builder.create();
        // 在dialog  show方法之前添加如下代码，表示该dialog是一个系统的dialog**
        dialog.getWindow().setType((WindowManager.LayoutParams.TYPE_SYSTEM_ALERT));
        new Thread(){
            public void run() {
                SystemClock.sleep(4000);
                Looper.prepare();
                dialog.show();
                Looper.loop();
            }
        }.start();
    }
    @TargetApi(Build.VERSION_CODES.ECLAIR)
	private void checkBTState() {

        // Check for Bluetooth support and then check to make sure it is turned on
        // Emulator doesn't support Bluetooth and will return null
        if(btAdapter==null) {
            Toast.makeText(getApplicationContext(), "Fatal Error" + "Bluetooth not support", Toast.LENGTH_SHORT).show();
        } else {
            if (btAdapter.isEnabled()) {
                Log.d(TAG, "...Bluetooth ON...");
            } else {
                // Prompt user to turn on Bluetooth
                Intent intent = new Intent(this, MainActivity.class);
                intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                startActivity(intent);
            }
        }
    }
    
    // 接收Activity传送过来的命令
    class CommandReceiver extends BroadcastReceiver{
        @Override
        public void onReceive(Context context, Intent intent) {
		}

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
			byte[] buffer = new byte[1024];  // buffer store for the stream
			int bytes; // bytes returned from read()   
			// Keep listening to the InputStream until an exception occurs
			while (true) {     
				try {                	
					// Read from the InputStream            
					bytes = mmInStream.read(buffer); // bytes数组返回值，为buffer数组的长度
					// Send the obtained bytes to the UI activity
					String str = new String(buffer);
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
	public static int byteToInt(byte[] b){
		return (((int)b[0])+((int)b[1])*256);
    }
    Handler handler = new Handler() {  //处理消息队列的Handler对象
		@Override
		public void handleMessage(Message msg) {
			super.handleMessage(msg);
			//处理消息
			if (msg.what==READ) {
				String str = (String)msg.obj;	// 类型转化
				Intent ient = new Intent(MyService.this, WaveChart.class);
				ient.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
				Bundle ble = new Bundle(); 
				ble.putString("temp", ""+str);
				ient.putExtras(ble);
				sendBroadcast(ient);
				int state=0;
				if(state==0){
				startActivity(ient);
				state++;
				}else{}
			}
			else if(h==false)
				Toast.makeText(MyService.this,"接收不成功："+s+".",Toast.LENGTH_SHORT).show();
		}
	};
	/*
	static public boolean autoBond(Class btClass, BluetoothDevice device, String strPin)
	throws Exception {
		Method autoBondMethod = btClass.getMethod("setPin", new Class[] { byte[].class });
		return (Boolean) autoBondMethod
            .invoke(device, new Object[] { strPin.getBytes() });
	}
	static public boolean createBond(Class btClass,BluetoothDevice btDevice) throws Exception {  
		Method createBondMethod = btClass.getMethod("createBond");
		return (Boolean) createBondMethod.invoke(btDevice);
	} 
    *//** Called when the service is first created. **/
    @TargetApi(Build.VERSION_CODES.GINGERBREAD)
	public void connect(Intent intent) {
        btAdapter = BluetoothAdapter.getDefaultAdapter();
        checkBTState();
		WaveChart wc=new WaveChart();
		String address = intent.getStringExtra("address");
		btAdapter.cancelDiscovery();
		BluetoothDevice device = btAdapter.getRemoteDevice(address);
		//   Two things are needed to make a connection:
		//   A MAC address, which we got above.
		//   A Service ID or UUID.  In this case we are using the
		//   UUID for SPP.
		/*	Method m;
		 BluetoothSocket socket = null; 
		 try {
		 m = device.getClass().getMethod("createRfcommSocket", new Class[] {int.class});
		 socket = (BluetoothSocket) m.invoke(device, Integer.valueOf(1));
		 socket.connect();
		 Toast.makeText(MyService.this, "\t已配对蓝牙地址为:\n"+address, Toast.LENGTH_LONG).show();
		 } catch (InvocationTargetException e) {
			 // TODO Auto-generated catch block
			 errorExit(e.getMessage());
		 } catch (IOException e) {
		 errorExit(e.getMessage());
		 wc.setTitle(e.getMessage());
		 fl=true;
		 } catch (SecurityException e) {
		 // TODO Auto-generated catch block
		 errorExit(e.getMessage());
		 } catch (NoSuchMethodException e) {
		 // TODO Auto-generated catch block
		 errorExit(e.getMessage());
		 } catch (IllegalArgumentException e) {
		 // TODO Auto-generated catch block
		 errorExit(e.getMessage());
		 } catch (IllegalAccessException e) {
		 // TODO Auto-generated catch block
		 errorExit(e.getMessage());
		 }
		 try {
		 inStream = socket.getInputStream();
		 } catch (IOException e) {
		 errorExit("Fatal Error"+"In onResume() and output stream creation failed:" + e.getMessage() + ".");
		 }
		 }*/
        try // if(address.equals("00:00:00:00:00:00"))
        {
            System.out.println("Sevice_address---------------'"+ address);
            String msg = "In onResume() and an exception occurred during receive: ";
			
            try {
                btSocket = createBluetoothSocket(device);
            } catch (IOException e) {
                if (address.equals("00:00:00:00:00:00"))
                    msg = msg + ".\n\nUpdate your server address from 00:00:00:00:00:00 to the correct address on line 35 in the java code";
                this.btAdapter.disable();
                msg = msg +  ".\n\nCheck that the SPP UUID: " + MY_UUID.toString() + " exists on server.\n\n";
                Toast.makeText(getBaseContext(), "\n"+msg + e.getMessage() + ".", Toast.LENGTH_LONG).show();
            }

            try {
				if(!fl)
				MY_UUID = UUID.fromString("fa87c0d0-afac-11de-8a39-0800200c9a55");
                btSocket = device.createRfcommSocketToServiceRecord(MY_UUID);
            } catch (IOException e) {
                Toast.makeText(getBaseContext(), "Fatal Error"+"In onResume() and socket create failed: " + e.getMessage() + ".", Toast.LENGTH_LONG).show();
            }
            // Discovery is resource intensive.  Make sure it isn't going on
            // when you attempt to connect and pass your message.
            btAdapter.cancelDiscovery();
            // Establish the connection.  This will block until it connects.
            Log.d(TAG, "...Connecting...");
            try {
				btSocket.connect();
				Toast.makeText(MyService.this, "已配对的蓝牙地址为:\n"+ address, Toast.LENGTH_LONG).show();
            } catch (IOException e) {
                try {
					btSocket.close();
					wc.setTitle(e.getMessage());
					fl=false;
					if(btAdapter.getState()!= BluetoothAdapter.STATE_CONNECTED)
						Toast.makeText(getBaseContext(), "\t连接不成功,请多试几次。\n" + e.getMessage() + ".", Toast.LENGTH_LONG).show();
				    } catch (IOException e2) {
					Toast.makeText(getBaseContext(), "Fatal Error"+"In onResume() and unable to close socket during connection failure" + e2.getMessage() + ".", Toast.LENGTH_LONG).show();
						Intent ient = new Intent(MyService.this, DeviceListActivity.class);
						ient.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
						startActivity(ient);
                }
		} catch(IOError e){}
			
            // Create a data stream so we can talk to server.
            Log.d(TAG, "...Create Socket...");
        }
        catch (IllegalArgumentException e) {
            Toast.makeText(getBaseContext(), e.getMessage(), Toast.LENGTH_SHORT).show();
        }
		try {
			outStream = btSocket.getOutputStream();
		} catch (IOException e) {
			Toast.makeText(getBaseContext(),"Fatal Error"+ "In onResume() and output stream creation failed:" + e.getMessage() + ".",Toast.LENGTH_SHORT).show();
		}
		if(!address.equals("00:00:00:00:00:00")){
			sendData("I'm ready!");
			new RecieveThread(btSocket).start(); 
		}
		
        // Set up a pointer to the remote node using it's address.
    }
	private void sendData(String message) {
		byte[] msgBuffer = message.getBytes();
		String hint="数据发送不成功。详情：";
		Log.d(TAG, "...Send data: " + message + "...");
		try {
			outStream.write(msgBuffer);
		} catch (IOException e) {
			String msg = "In onResume() and an exception occurred during write: " + e.getMessage()+"\nCheck that the SPP UUID: " + MY_UUID.toString();
			Toast.makeText(getBaseContext(), hint+"\n"+msg, Toast.LENGTH_SHORT).show();
		}
	}
	
		@Override
    public void onStart(Intent intent, int startId) throws IOError {
        // TODO: Implement this method
        super.onStart(intent, startId);
			mHandler = new Handler();
			mHandler.post(new TimerProcess()); 
			connect(intent); 
			intent = new Intent(MyService.this, WaveChart.class);
			intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
			Bundle ble = new Bundle(); 
			ble.putString("temp", " ");
			intent.putExtras(ble);
			sendBroadcast(intent);
			startActivity(intent);
		}
	private class TimerProcess implements Runnable{
		@Override 
		public void run() {
			mHandler.postDelayed(this,50);
		}
	}
    /*广播类*/
    class StaticReceiver extends BroadcastReceiver {

        @Override
        public void onReceive(Context context, Intent intent) {
            String msg = intent.getStringExtra(s);
            Toast.makeText(context, msg, Toast.LENGTH_SHORT).show();
        }
    }

/*
  服务与activity绑定时才调用
	@Override
	public IBinder onBind(Intent intent) {
		Bundle bundle = intent.getExtras();
		//接收字符串
		String address = bundle.getString("address");
		/**接收int类型数据
		 *int numVal = bundle.getInt("intValue");
		 *接收字节流，你可以把文件放入字节流
		 *byte[] bytes = bundle.getByteArray("bytesValue");
		 *//*
		return null;
	}
*/

    @Override
    public void onDestroy()
    {
        // TODO: Implement this methodD
        dialog();
        super.onDestroy();
    }
}
