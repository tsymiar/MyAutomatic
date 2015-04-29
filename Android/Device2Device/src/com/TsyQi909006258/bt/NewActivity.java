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

    public class NewActivity extends Activity {
        
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
		setContentView(R.layout.activity_new);
	}
	
	@Override
	public void onResume() {
		super.onResume();
        
        Bundle bundle=new Bundle(); 
        String address = bundle.getString("address");  
        bundle.putString("address", address);
        ImageView imageView3 = (ImageView) findViewById(R.id.imageView3);
        imageView3.setOnClickListener(new OnClickListener() {
                public void onClick(View v) {   
                    Intent intent = new Intent(NewActivity.this,ChatAct.class);
                    startActivity(intent);
                }
            });
	}

    @Override
    protected void onPause()
    {
        // TODO: Implement this method
        super.onPause();
        
    }
    
}
