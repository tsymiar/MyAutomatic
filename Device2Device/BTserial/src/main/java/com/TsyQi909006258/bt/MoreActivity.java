package com.TsyQi909006258.bt;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.view.KeyEvent;
import android.widget.Toast;

public class MoreActivity extends Activity{

	public void onCreate(Bundle savedInstanceState){
		{
			super.onCreate(savedInstanceState);

			Toast.makeText(getBaseContext(), "该功能尚在开发…请稍等", Toast.LENGTH_SHORT).show();
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
	public boolean onKeyDown(int keyCode, @NonNull KeyEvent event) {

		if (keyCode == KeyEvent.KEYCODE_BACK) {
			Intent intent = new Intent(this,MainActivity.class);
			startActivity(intent);
		}
		return super.onKeyDown(keyCode, event);
	}
}

