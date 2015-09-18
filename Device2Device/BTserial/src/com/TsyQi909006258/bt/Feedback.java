package com.TsyQi909006258.bt;

import android.app.Activity;
import android.os.Bundle;
import android.content.Intent;
import java.io.*;
import android.os.Environment;
import android.net.Uri;

	public class Feedback extends Activity{
		@Override
		protected void onCreate(Bundle savedInstanceState) {
			super.onCreate(savedInstanceState);
			Intent data=new Intent(Intent.ACTION_SENDTO); 
			data.setData(Uri.parse("mailto:909006258@qq.com")); 
			data.putExtra(Intent.EXTRA_SUBJECT, "反馈"); 
			data.putExtra(Intent.EXTRA_TEXT, "请输入你要说的话,我应该会很快回复的。"); 
			startActivity(data); 	
			// 附件
			// File file = new File(Environment.getExternalStorageDirectory().getPath()+ File.separator + "simplenote"+ File.separator+"note.xml");
			
		}

    @Override
    protected void onStop()
    {
        super.onStop();
        this.finish();
    }
    
	}
