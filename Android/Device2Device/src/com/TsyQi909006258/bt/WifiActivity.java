package com.TsyQi909006258.bt;

import java.io.IOException;
import android.app.Activity;
import android.widget.Toast;
import android.os.*;
import com.tencent.stat.StatService;
import android.content.Intent;
import android.view.*;
import android.widget.*;

public class WifiActivity extends Activity{
	
	public void onCreate(Bundle savedInstanceState){
		{
			super.onCreate(savedInstanceState);
			
	       Toast.makeText(getBaseContext(), "该功能尚在开发…请稍等", Toast.LENGTH_LONG).show();
	}
	}

	@Override
	protected void onPause()
	{
		// TODO: Implement this method
		super.onPause();
		this.finish();
	}

	@Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {

        if (keyCode == KeyEvent.KEYCODE_BACK) {
			Intent intent = new Intent(this,MainActivity.class);
			startActivity(intent);
        }     
        return super.onKeyDown(keyCode, event);
	}
}
