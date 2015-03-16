package com.TsyQi909006258.bt;

import java.io.IOException;
import java.io.OutputStream;
import java.lang.String;
import java.util.UUID;
import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.View.OnClickListener;
import android.view.*;
import android.widget.*;
import android.widget.TextView.OnEditorActionListener;
import java.lang.reflect.*;

public class ChatAct extends Activity {
    private static final String TAG = "ChatAct";
    private BluetoothAdapter btAdapter = null;
    private BluetoothSocket btSocket = null;
    private OutputStream outStream = null;
    private String data="1";
    private Toast toast;
    private boolean ok;

    // SPP UUID service 
    private static final UUID MY_UUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");

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

    private void checkBTState() {

        // Check for Bluetooth support and then check to make sure it is turned on
        // Emulator doesn't support Bluetooth and will return null
        if(btAdapter==null) { 
            errorExit("Fatal Error", "Bluetooth not support");
        } else {
            if (btAdapter.isEnabled()) {
                Log.d(TAG, "...Bluetooth ON...");
            } else {

                // Prompt user to turn on Bluetooth
                Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
                startActivityForResult(enableBtIntent, 1);
                Intent intent=new Intent(this, MainActivity.class); 
                startActivity(intent);
            }
        }
    }

    private void errorExit(String title, String message){
        Toast.makeText(getBaseContext(), title + " - " + message, Toast.LENGTH_LONG).show();
        finish();
    }

    private void sendData(String message) {
        byte[] msgBuffer = message.getBytes();
        Intent intent=this.getIntent();  
        Bundle bundle=intent.getExtras(); 
        String address = bundle.getString("address");  
        String hint="数据发送不成功。详情：";
        Log.d(TAG, "...Send data: " + message + "...");

        try {
            outStream.write(msgBuffer);
        } catch (IOException e) {
            String msg = "In onResume() and an exception occurred during write: " + e.getMessage();
            if (address.equals("00:00:00:00:00:00")) 
                msg = msg + ".\n\nUpdate your server address from 00:00:00:00:00:00 to the correct address on line 35 in the java code";
            this.btAdapter.disable(); 
            msg = msg +  ".\n\nCheck that the SPP UUID: " + MY_UUID.toString() + " exists on server.\n\n";
            Toast.makeText(getBaseContext(), hint+"\n"+msg, Toast.LENGTH_LONG).show();
            errorExit("Fatal Error", msg);   
        }
    }

    public boolean onCreateOptionsMenu(Menu menu)
    {
        getMenuInflater().inflate(R.menu.menu_new,menu);
        return true;
    }

    public boolean onOptionsItemSelected(MenuItem item) { 
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
            default: 
                return super.onOptionsItemSelected(item); 
        } }

    /** Called when the activity is first created. **/
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.dialog_new);
        ok=true;
    }

    @Override
    public void onResume() {
        super.onResume();

        btAdapter = BluetoothAdapter.getDefaultAdapter();
        checkBTState(); 

        if(ok){
            /*****
             */////
            final EditText editText=(EditText)findViewById(R.id.edit_text);  
            // 监听回车键  
            editText.setOnEditorActionListener(new OnEditorActionListener() {  
                    @Override  
                    public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {  
                        data=editText.getText().toString();
                        if(data.equals(null)||data.equals(" ")||data.equals("")){Toast.makeText(ChatAct.this,"未输入", Toast.LENGTH_SHORT).show();}
                        else{
                            toast=Toast.makeText(ChatAct.this,data/*String.valueOf(actionId)*/, Toast.LENGTH_SHORT);
                            toast.show();  
                            toast.setGravity(Gravity.BOTTOM,0,30);
                        }
                        return false;  
                    }   
                });
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
                        if(data.equals(null)||data.equals(" ")||data.equals(""))data="null";
                        toast=Toast.makeText(getBaseContext(), data+"\n已发送", Toast.LENGTH_SHORT);
                        toast.show();
                        toast.setGravity(Gravity.TOP|Gravity.RIGHT,50,100);
                    }
                });

            Log.d(TAG, "...onResume - try connect...");
            Intent intent=this.getIntent();  
            Bundle bundle=intent.getExtras(); 
            String address = bundle.getString("address");  
            // Set up a pointer to the remote node using it's address.
            BluetoothDevice device = btAdapter.getRemoteDevice(address);
            Toast.makeText(ChatAct.this, "\t蓝牙地址为:\n"+address, Toast.LENGTH_LONG).show();

            //   Two things are needed to make a connection:
            //   A MAC address, which we got above.
            //   A Service ID or UUID.  In this case we are using the
            //   UUID for SPP.
            try {
                btSocket = createBluetoothSocket(device);
            } catch (IOException e1) {
                errorExit("Fatal Error", "In onResume() and socket create failed: " + e1.getMessage() + ".");
            }

            try {
                btSocket = device.createRfcommSocketToServiceRecord(MY_UUID);
            } catch (IOException e) {
                errorExit("Fatal Error", "In onResume() and socket create failed: " + e.getMessage() + ".");
            }

            // Discovery is resource intensive.  Make sure it isn't going on
            // when you attempt to connect and pass your message.
            btAdapter.cancelDiscovery();

            // Establish the connection.  This will block until it connects.
            Log.d(TAG, "...Connecting...");
            try {
                btSocket.connect();
                Log.d(TAG, "...Connection ok...");
            } catch (IOException e) {
                try {
                    btSocket.close();
                } catch (IOException e2) {
                    errorExit("Fatal Error", "In onResume() and unable to close socket during connection failure" + e2.getMessage() + ".");
                }
            }

            // Create a data stream so we can talk to server.
            Log.d(TAG, "...Create Socket...");

            try {
                outStream = btSocket.getOutputStream();
            } catch (IOException e) {
                errorExit("Fatal Error", "In onResume() and output stream creation failed:" + e.getMessage() + ".");
            }
        }ok=false;      
    }

    @Override
    protected void onPause()
    {
        // TODO: Implement this method
        super.onPause();
    }
}
