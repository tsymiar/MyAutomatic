package com.TsyQi909006258.bt;

import android.content.Intent;
import android.app.Activity;
import android.os.Bundle;
import android.net.Uri;
import android.app.ActionBar;

public class OpenUrl extends Activity{

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);	
		Intent intent = new Intent();        
		intent.setAction("android.intent.action.VIEW");    
		Uri content_url = Uri.parse("http://adf.ly/1Gz1sP");   
		intent.setData(content_url);  
		startActivity(intent);
	}

	@Override
	protected void onStop()
	{
		super.onStop();
		this.finish();
	}
}
