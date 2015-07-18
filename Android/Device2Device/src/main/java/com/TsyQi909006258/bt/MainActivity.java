package com.TsyQi909006258.bt;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.support.annotation.NonNull;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.widget.ImageView;
import android.widget.Toast;

public class MainActivity extends Activity {

    protected Animation animation;
    private ImageView toTestAnimation;
    private long mCurTime;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // TODO: Implement this method
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        findMyImg();
        Animation(R.anim.animation);
        ExitApplication.getInstance().addActivity(this);
    }

    @Override
    protected void onResume() {
        // TODO: Implement this method
        super.onResume();

        ViewGroup vg = (ViewGroup) findViewById(R.id.anim);
        new Handler().postDelayed(new Runnable() {
            @Override
            public void run() {
                startActivity(new Intent(MainActivity.this, ConnectActivity.class));
            }
        }, 2000);
        assert vg != null;
        vg.setOnClickListener(new View.OnClickListener() {

            @Override
            public void onClick(View arg0) {
                // TODO Auto-generated method stub
                //startActivity(new Intent(MainActivity.this, ConnectActivity.class));
                Animation(0);
                //MainActivity.this.finish();
            }
        });
    }

    private void findMyImg() {
        // TODO Auto-generated method stub
        toTestAnimation = (ImageView) findViewById(R.id.ooopic);
    }

    private void Animation(int i) {

        if (i != 0) {
            animation = AnimationUtils.loadAnimation(this, i);
            toTestAnimation.startAnimation(animation);
        }
        else {
            AnimationUtils.makeOutAnimation(this,true);
        }
    }

    public void exit(Activity activity) {
        Intent rs = new Intent(activity, RecieveService.class);
        Intent ss = new Intent(activity, SavefileService.class);
        Bundle bundle = new Bundle();
        String temp = "stop";
        bundle.putString("temp", temp);
        rs.putExtras(bundle);
        ss.putExtras(bundle);
        activity.sendBroadcast(rs);
        activity.sendBroadcast(ss);
        activity.stopService(rs);
        activity.stopService(ss);
        ExitApplication.getInstance().exit();
    }

    public void writer(Activity activity) {
        Intent intent = new Intent(activity, WriterActivity.class);
        activity.startActivity(intent);
    }

    public void feedback(Activity activity) {
        Intent intent = new Intent(activity, FeedbackActivity.class);
        activity.startActivity(intent);
    }

    public void about(Activity activity) {
        Intent intent = new Intent(activity, AboutActivity.class);
        activity.startActivity(intent);
    }

    @Override
    public boolean onKeyDown(int keyCode, @NonNull KeyEvent event) {

        /* Called when the activity is first created. */
        long mLastTime = mCurTime;
        mCurTime = System.currentTimeMillis();

        if (keyCode == KeyEvent.KEYCODE_BACK) {

            if (mCurTime - mLastTime < 800) {
                Intent intent = new Intent(MainActivity.this, RecieveService.class);
                Bundle bundle = new Bundle();
                String temp = "stop";
                bundle.putString("temp", temp);
                intent.putExtras(bundle);
                sendBroadcast(intent);
                stopService(intent);
                ExitApplication.getInstance().exit();
                return true;
            } else if (mCurTime - mLastTime >= 800) {
                Toast.makeText(MainActivity.this, "双击完全退出程序", Toast.LENGTH_SHORT).show();
                return true;
            }

        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    protected void onPause() {
        // TODO: Implement this method
        super.onPause();
        this.finish();
    }
}
