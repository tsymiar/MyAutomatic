package com.TsyQi909006258.bt;

import java.io.IOException;
import android.app.Activity;
import android.widget.Toast;
import android.os.*;
import android.view.*;
import android.widget.*;
import com.tencent.stat.StatService;
import android.content.Intent;
import android.app.AlertDialog;
import android.view.KeyEvent;
import java.util.List;
import java.util.LinkedList;
import android.app.Application;
import android.content.DialogInterface;
import android.view.View.OnTouchListener;  
import android.graphics.ColorMatrixColorFilter; 

public class MainActivity extends Activity {

	/** Called when the activity is first created. */
    private long mLastTime;
    private long mCurTime;
    public static final String SYSTEM_EXIT = "exit";
	@Override
	
	public void onCreate(Bundle savedInstanceState){
			super.onCreate(savedInstanceState);
			setContentView(R.layout.activity_main);
			StatService.trackCustomEvent(this, "onCreate", ""); 
			Button imageView3 = (Button) findViewById(R.id.imageView3);
			Button imageView4 = (Button) findViewById(R.id.imageView4);
		imageView3.setOnClickListener(new View.OnClickListener()
			{
				public void onClick(View v)
				{
					startActivity(new Intent(MainActivity.this, ConnectionActivity.class));
					// 2.0(SdkVersion版本号为5)以后的版本才支持 
					int version = Integer.valueOf(android.os.Build.VERSION.SDK);    
					if(version >= 5) {    
						overridePendingTransition(android.R.anim.fade_in,android.R.anim.fade_out);   
						overridePendingTransition(android.R.anim.slide_in_left,android.R.anim.slide_out_right); 
					}  
				}
			});

		imageView4.setOnClickListener(new View.OnClickListener()
			{
				public void onClick(View v)
				{
					startActivity(new Intent(MainActivity.this, WifiActivity.class));
					int version = Integer.valueOf(android.os.Build.VERSION.SDK);    
					if(version  >= 5) {    
						overridePendingTransition(android.R.anim.fade_in,android.R.anim.fade_out);   
						overridePendingTransition(android.R.anim.slide_in_left,android.R.anim.slide_out_right); 
					}  
				}
			});
		}
    
    public static final OnTouchListener VIEW_TOUCH_DARK = new OnTouchListener() {
        public final float[] BT_SELECTED = new float[] { 1, 0, 0, 0, -50, 0, 1,  
            0, 0, -50, 0, 0, 1, 0, -50, 0, 0, 0, 1, 0 };  
        public final float[] BT_NOT_SELECTED = new float[] { 1, 0, 0, 0, 0, 0,  
            1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0 };  
        public void addTouchDrak(View view , boolean isClick){  
            view.setOnTouchListener( VIEW_TOUCH_DARK ) ;   
            if(!isClick){  
                view.setOnClickListener(new View.OnClickListener() {  
                        @Override  
                        public void onClick(View v) {  
                        }  
                    });  
            }  
        }  
        
        @Override  
        public boolean onTouch(View v, MotionEvent event) {  
            if (event.getAction() == MotionEvent.ACTION_DOWN) {  
                if(v instanceof ImageView){  
                    ImageView iv = (ImageView) v;  
                    iv.setDrawingCacheEnabled(true);   

                    iv.setColorFilter( new ColorMatrixColorFilter(BT_SELECTED) ) ;   
                }else{  
                    v.getBackground().setColorFilter( new ColorMatrixColorFilter(BT_SELECTED) );  
                    v.setBackgroundDrawable(v.getBackground());  
                }  
            } else if (event.getAction() == MotionEvent.ACTION_UP) {  
                if(v instanceof ImageView){  
                    ImageView iv = (ImageView) v;   
                    iv.setColorFilter( new ColorMatrixColorFilter(BT_NOT_SELECTED) ) ;   
                    System.out.println( "back" );   
                }else{  
                    v.getBackground().setColorFilter(  
                        new ColorMatrixColorFilter(BT_NOT_SELECTED));  
                    v.setBackgroundDrawable(v.getBackground());  
                }  
            }  
            return false;  
        }  
    };  
    
    @Override
    protected void onPause()
    {
        // TODO: Implement this method
        super.onPause();
		this.finish();
    }
    
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {

        mLastTime = mCurTime;
        mCurTime = System.currentTimeMillis();

        if (keyCode == KeyEvent.KEYCODE_BACK) {

            if(keyCode == KeyEvent.KEYCODE_BACK&&mCurTime - mLastTime < 800) {
                Intent intent = new Intent();  
                intent.setAction(MainActivity.SYSTEM_EXIT);  
                sendBroadcast(intent);  
                this.finish();
                return true;
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
