package bulutooth.oscillogragh.myapplication;

import android.annotation.TargetApi;
import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.DashPathEffect;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PathEffect;
import android.graphics.PointF;
import android.graphics.Rect;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Vibrator;
import android.os.Looper;
import android.os.Message;
import android.os.SystemClock;
import android.util.FloatMath;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.KeyEvent;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import java.io.IOException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.UUID;
import java.io.*;

//import dalvik.system.VMRuntime;
/*
 import org.achartengine.model.*;
 import org.achartengine.ChartFactory;
 import org.achartengine.chart.PointStyle;
 import org.achartengine.GraphicalView;
 import org.achartengine.model.XYMultipleSeriesDataset;
 import org.achartengine.model.XYSeries;
 import org.achartengine.renderer.XYMultipleSeriesRenderer;
 import org.achartengine.renderer.XYSeriesRenderer;
 import org.achartengine.renderer.XYSeriesRenderer.FillOutsideLine;
 */

public class WaveChart extends Activity {

    private Handler mHandler;
    private NewChart mView;
	private Chart chart0;
	private Bitmap mBitmap;
	private Matrix matrix = new Matrix();
	private SensorManager sensorManager;
	private SurfaceHolder holder = null;
	private PointF mid = new PointF();
	private TextView myview=null;
	private Button stop_bn=null;
	private long lastUpdateTime;
	private boolean flag=true;
	private boolean fg=false;
	private int mode=0;
	private float distance0;
	private float n=1,p=1;
	private float mx=0;
	private float my=0;
	private float x2,tx;
	private float y2,ty;
	private float disx;
	private float disy;
	private String temp;
	// 传感器管理器
    public double[] linear_acceleration = new double[3];
    public double[] gravity=new double[3];
	// private PointF[] mPoints = new PointF[200]; // 声明一个对象，泛型是PointF，并实例化这个泛型对象
    private ArrayList<Integer> xlist=new ArrayList<>();
	public static final String TAG = "Canvas";
    // 检测的时间间隔
    private static final int UPTATE_INTERVAL_TIME = 30;
	/*private static final*/ UUID MY_UUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");

	@TargetApi(Build.VERSION_CODES.HONEYCOMB)
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
		setContentView(R.layout.canvas_chart);
		Button begin =(Button)findViewById(R.id.draw); 
		stop_bn=(Button)findViewById(R.id.stop_bn);
		stop_bn.setOnClickListener(new ButtonStopListener());
		// 初始化SurfaceHolder对象
        SurfaceView surface = (SurfaceView) findViewById(R.id.show);
		holder = surface.getHolder();  
		holder.setFixedSize(9, 9);  //设 小，要比实际的绘图位置大一点
        sensorManager = (SensorManager) getSystemService(Context.SENSOR_SERVICE);
		CloseAll.getInstance().addActivity(this);  
        chart0 = new Chart(sensorManager.getDefaultSensor(Sensor.TYPE_LINEAR_ACCELERATION));
		matrix.set(surface.getMatrix());
        mView=new NewChart(this);
        mView.invalidate();
		myview=(TextView)findViewById(R.id.myview);
		LinearLayout mLayout = (LinearLayout) findViewById(R.id.toor);
        mLayout.addView(mView);
        mHandler = new Handler();
        mHandler.post(new TimerProcess()); 
		begin.setOnClickListener(new OnClickListener(){
				public void onClick(View v) {
					// TODO Auto-generated method stub
					fg=true;
				}
			});
	}
	
// 通过蓝牙对点的采集
	@TargetApi(Build.VERSION_CODES.ECLAIR)
    public BluetoothSocket createBluetoothSocket(BluetoothDevice device) throws IOException {
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

    public void errorExit(String message){
        Toast.makeText(getBaseContext(), " - " + message, Toast.LENGTH_SHORT).show();
        finish();
    }

    @TargetApi(Build.VERSION_CODES.ECLAIR)
    @Override
    protected void onResume() {
		// TODO Auto-generated method stub
		super.onResume();
	
		chart0.register();
		/* new Thread(){
            public void run() {*/
				Intent intent = getIntent();
				Bundle bundle=intent.getExtras();
				temp = bundle.getString("temp");
				/*while(true){
				Message msg=new Message();
				msg.what=0;
				handler.sendMessage(msg);
                try{
					this.sleep(50);
				}catch(InterruptedException e){
					e.printStackTrace();
				}
				}
            }
        }.start();
	};
	
	Handler handler = new Handler() {  
		@Override
		public void handleMessage(Message msg) {

			if (msg.what==0) {*/
				myview.setText(" " + temp);
				// myview.invalidate();
			/*} super.handleMessage(msg);
		};*/
	}
	
    @Override
    protected void onPause() {
        // TODO Auto-generated method stub
        super.onPause();
        chart0.unregister();
    }

	public Bitmap CreatNewPhoto() {
		int width = 900;
		int height = 600;
		Paint mPaint=new Paint();
		Bitmap bitmap = Bitmap.createBitmap(width, height,
						Bitmap.Config.ARGB_8888); // 背景图片
		Canvas canvas = new Canvas(bitmap);
		canvas.drawBitmap(mBitmap, matrix, mPaint);
		canvas.save(Canvas.ALL_SAVE_FLAG);
		canvas.restore();
		return bitmap;
	}
	private class TimerProcess implements Runnable{
		@Override 
		public void run() {
			mHandler.postDelayed(this,50);
			mView.invalidate();
		}
	}

	class ButtonStopListener implements OnClickListener{

		@Override
		public void onClick(View v) {
			// TODO Auto-generated method stub
			if(flag){
				flag= false;
				stop_bn.setText(R.string.begin);}
			else
			{
				flag=true;
				stop_bn.setText(R.string.stop);
			}
		}
	}
	class NewChart extends View {

        private int CHARTH = 800;// 画布高
        private int CHARTW = 600;// 画布宽
        private int OFFSET_LEFT = 70;// 距离左边70
        private int OFFSET_TOP = 80;// 距离顶部80
        private int TEXT_OFFSET = 30;// 文字距离中心位置30
        private int X_INTERVAL = 9;

        private ArrayList<PointF> mPlist;
        private ArrayList<PointF> mPlist0;

		private Paint mPaint=new Paint();
		private double M;

        public NewChart(Context context) {
            super(context);
            // TODO Auto-generated constructor stub
            mPlist = new ArrayList<>();
            mPlist0 = new ArrayList<>();
        }
        @Override
        protected void onDraw(Canvas canvas) {
            super.onDraw(canvas);
			drawTable(canvas);
            legend(canvas);
			if(fg){
				holder.lockCanvas(new Rect(0,0,0,0));
				mPaint.setColor(Color.CYAN);
				int height=getWindowManager().getDefaultDisplay().getHeight();
				int width=getWindowManager().getDefaultDisplay().getWidth();
				if(flag){
					drawDegree(canvas, 1/n, 1/n);
				    canvas.drawText("f(x)="+Float.toString(my*n),mx,my,mPaint);
					canvas.scale(n,n,0,width/2);
				}
				else {
					switch(mode){
						case(1):
						drawDegree(canvas, disy, disx);
						canvas.scale(p,p,mid.x,mid.y);
						canvas.translate(tx-2*width/5,ty-2*height/5);
					    break;
						case 2:
                        drawDegree(canvas, 1/p, 1/p);
					    canvas.scale(p,p,mid.x,mid.y);	
						break;
					   }
					 }
				Point(n);
				drawCurve(canvas);
				Line0(n);
				drawCurve0(canvas);
			 }
			else{
				this.invalidate(); 
				//	CreatNewPhoto();
			}
		}

		private void drawTable(Canvas canvas){

            mPaint.setColor(Color.WHITE);// 设置要画出图形的颜色

            mPaint.setStyle(Paint.Style.STROKE);
            mPaint.setStrokeWidth(2);
            
            Rect chartRec = new Rect(OFFSET_LEFT,OFFSET_TOP+3,CHARTW+OFFSET_LEFT,CHARTH+OFFSET_TOP+3);
            // 顶端的文字
            Path textPaint = new Path();
            mPaint.setStyle(Paint.Style.FILL);// 设置画笔的样式
            textPaint.moveTo(250, 44);// 文字排版起始点
            textPaint.lineTo(500, 44);// 结束点
            mPaint.setTextSize(27);// 字号
            mPaint.setAntiAlias(true);// 设置锯齿效果，true消除。
            canvas.drawTextOnPath("电力波形曲线", textPaint, 0, 0, mPaint);
            // 左侧数字
			Paint mpt=new Paint();
            mpt.setColor(Color.LTGRAY);
            mpt.setStrokeWidth(5);
			mpt.setTextSize(23);

            // canvas.drawText("9.8(m/s²)", CHARTW/2, OFFSET_TOP + CHARTH/2-60, mpt);
            canvas.drawText("0", OFFSET_LEFT-TEXT_OFFSET-17, OFFSET_TOP + CHARTH/2-5, mpt);
            canvas.drawText("幅度", OFFSET_LEFT-TEXT_OFFSET-30, OFFSET_TOP + CHARTH/2+30, mpt);

			// 底部单位
			Path oinsPath = new Path();
			Paint oinPaint = new Paint();
			oinPaint.setTextSize(22);
			oinPaint.setStrokeWidth(5);
			oinPaint.setColor(Color.WHITE);
			oinPaint.setStyle(Paint.Style.FILL);
            oinsPath.moveTo(OFFSET_LEFT-TEXT_OFFSET-30, CHARTH+OFFSET_TOP*2-33);
			oinsPath.lineTo(660, CHARTH+OFFSET_TOP*2-33);
			canvas.drawTextOnPath("(ms)  0", oinsPath, 0, 0, oinPaint);
			Path oPath = new Path();
			Paint oPaint = new Paint();
			oPaint.setTextSize(30);
			oPaint.setStrokeWidth(5);
			oPaint.setAntiAlias(true);
			oPaint.setColor(Color.LTGRAY);
			oPaint.setStyle(Paint.Style.FILL);
			oPath.moveTo(OFFSET_LEFT-8, CHARTH+OFFSET_TOP*2-60);
            oPath.lineTo(690, CHARTH+OFFSET_TOP*2-60);
			canvas.drawTextOnPath("⇂     ⇂     ⇂     ⇂     ⇂     ⇂     ⇂     ⇂     ⇂     ⇂     ⇂     ⇂     ⇂", oPath, 0, 0, oPaint);
            // 画表格中的虚线
            Path mPath = new Path();
			Paint aint=new Paint();
            PathEffect effect = new DashPathEffect(new float[]{2,2,2,2}, 1);
            aint.setStyle(Paint.Style.FILL_AND_STROKE);
            aint.setAntiAlias(true);
            aint.setStrokeWidth(2);
            aint.setPathEffect(effect);
			aint.setColor(Color.GRAY);
			canvas.drawRect(chartRec, aint);
            for (int i = 0; i <=9; i++) {
                mPath.moveTo(OFFSET_LEFT, OFFSET_TOP + CHARTH/10*i+3);
                mPath.lineTo(OFFSET_LEFT + CHARTW, OFFSET_TOP + CHARTH/10*i+3);
                canvas.drawPath(mPath, aint);
            }
        }
		// 图例
        private void legend(Canvas canvas){

            mPaint.setColor(Color.GREEN);
            Path legPaint = new Path();
            mPaint.setStyle(Paint.Style.FILL);
            legPaint.moveTo(90, CHARTH+OFFSET_TOP*2+7);
            legPaint.lineTo(200, CHARTH+OFFSET_TOP*2+7);
            mPaint.setTextSize(25);
            canvas.drawTextOnPath("图例：", legPaint, 0, 0, mPaint);
            // 图线
            mPaint.setColor(Color.YELLOW);
            canvas.drawLine(80, CHARTH+OFFSET_TOP*3-40, 200, CHARTH+OFFSET_TOP*3-40, mPaint);
            // 文字
            mPaint.setColor(Color.CYAN);
            Path accPath = new Path();
            mPaint.setStyle(Paint.Style.FILL);
            accPath.moveTo(220, CHARTH+OFFSET_TOP*3-33);
            accPath.lineTo(400, CHARTH+OFFSET_TOP*3-33);
            mPaint.setTextSize(20);
            canvas.drawTextOnPath("CH1", accPath, 0, 0, mPaint);
            /////
            mPaint.setColor(Color.MAGENTA);
            canvas.drawLine(80, CHARTH+OFFSET_TOP*3+20, 200, CHARTH+OFFSET_TOP*3+20, mPaint);
            mPaint.setColor(Color.CYAN);
            Path graPath = new Path();
            mPaint.setStyle(Paint.Style.FILL);
            graPath.moveTo(220, CHARTH+OFFSET_TOP*3+25);
            graPath.lineTo(400, CHARTH+OFFSET_TOP*3+25);
            mPaint.setTextSize(20);
            canvas.drawTextOnPath("CH2", graPath, 0, 0, mPaint);
        }
        /*
		 private PointF[] getpoints(ArrayList<PointF> dlk, ArrayList<Integer> xlist)
		 {
		 int widt=getWidth();
		 final float scale = SensorChart.this.getResources().getDisplayMetrics().density;
		 int widh=(int) (50* scale + 0.5f);
		 for(int i=0;i<dlk.size();i++)
		 {
		 xlist.add(widh+(widt-widh)/dlk.size()*i);
		 }
		 PointF[] mPoints=new PointF[dlk.size()];
		 for(int i=0;i<dlk.size();i++)
		 {
		 int ph=(int)(gravity[0]*100);

		 mPoints[i]=new PointF(xlist.get(i),ph);

		 // invalidate();
		 }
		 return mPoints;
		 }

		 private void drawscrollline(PointF[] ps,Canvas canvas,Paint paint)
		 {
		 PointF startp=new PointF();
		 PointF endp=new PointF();
		 paint.setStyle(Paint.Style.FILL);
		 for(int i=0;i<ps.length-1;i++)
		 {
		 startp=ps[i];
		 endp=ps[i+1];
		 float wt=(startp.x+endp.x)/2;
		 PointF p3=new PointF();
		 PointF p4=new PointF();
		 p3.y=startp.y;
		 p3.x=wt;
		 p4.y=endp.y;
		 p4.x=wt;
		 Path path = new Path();
		 path.moveTo(startp.x,startp.y);
		 // 贝塞尔三次曲线
		 path.cubicTo(p3.x, p3.y, p4.x, p4.y,endp.x, endp.y);
		 // 贝塞尔二次 path.quadTo();
		 canvas.drawPath(path, paint);
		 }
		 }
		 */
		@Override
		public boolean dispatchTouchEvent(MotionEvent ev) {
			try{

				return super.dispatchTouchEvent(ev);
			}catch (IllegalArgumentException exception){
				Log.d(TAG, "dispatch key event exception");
			}
			return false;
		}
		@TargetApi(Build.VERSION_CODES.ECLAIR)
		private float distance(MotionEvent event) {
			float x = event.getX(0) - event.getX(1);
			float y = event.getY(0) - event.getY(1);
			return FloatMath.sqrt(x * x + y * y);
		}
		@TargetApi(Build.VERSION_CODES.ECLAIR)
		public void midPoint(PointF point, MotionEvent event) {
			try {
				float x = event.getX(0) + event.getX(1);
				float y = event.getY(0) + event.getY(1);
				point.set(x / 2, y / 2);
			} catch (IllegalArgumentException e) {
				e.printStackTrace();
			}
		}
        // 拖动事件
        @TargetApi(Build.VERSION_CODES.ECLAIR)
        @Override
		public boolean onTouchEvent(MotionEvent event)
        {
			Paint pt=new Paint();
			pt.setTextSize(20);
			pt.setStrokeWidth(5);
			pt.setColor(Color.WHITE);
			pt.setStyle(Paint.Style.FILL);
			// if(mBitmap==null||mBitmap!=null){
            switch (event.getAction() & MotionEvent.ACTION_MASK) {
                case MotionEvent.ACTION_OUTSIDE:
					Toast.makeText(WaveChart.this,"OUTSIDE",Toast.LENGTH_SHORT).show();
                    break;
				case MotionEvent.ACTION_DOWN:
					Vibrator vibrator = (Vibrator) getSystemService(VIBRATOR_SERVICE);  
					vibrator.vibrate(50); 
					x2=event.getX();
					y2=event.getY();
					mode = 1;  
					break;  
                case MotionEvent.ACTION_POINTER_DOWN:
			    	distance0 = distance(event);
					mode += 1;
                    break;
				case MotionEvent.ACTION_MOVE:
					if(mode==1){
				    	matrix.postTranslate(event.getX(),event.getY());
						tx = event.getX();
						ty = event.getY();
						disx=x2/tx;
						disy=y2/ty;
					}
					if(mode>=2){
						midPoint(mid,event);
						float distance2 = distance(event);
						if(flag)n=distance2 /distance0;
						else p=distance2 /distance0;
						matrix.setScale(n,n,mid.x,mid.x);
						matrix.postScale(n, n, mid.x, mid.y);
					}
                    break;
                case MotionEvent.ACTION_CANCEL:
                    break;
            }
		    //  通知改组件重绘
            this.invalidate(); 
			return true;
		}

		// 刻度
		private void drawDegree(Canvas canvas,float m, float w){
			Paint mpt=new Paint();
            mpt.setColor(Color.LTGRAY);
            mpt.setStrokeWidth(5);
			mpt.setTextSize(21);
			// 左侧数字
            for(int j = 5; j >= 1 ; j-- ){
                canvas.drawText("+"+0.5*m*j, OFFSET_LEFT-TEXT_OFFSET-23,OFFSET_TOP + CHARTH/10*(5-j) , mpt);
            }
			for(int i = 1; i <= 5 ; i++ ){
                canvas.drawText("-"+0.5*m* i, OFFSET_LEFT-TEXT_OFFSET-20,OFFSET_TOP + CHARTH/10*(i+5) , mpt);
            }
			for(int j = 1; j <= 12 ; j++ ){
				canvas.drawText("  "+(int)(50*w* j), OFFSET_LEFT+20+50*(j-1), CHARTH+OFFSET_TOP*2-33, mpt);
            }
		}
		// 点的操作--
		private void drawCurve(Canvas canvas) {
		    mPaint.setColor(Color.YELLOW);
            mPaint.setStrokeWidth(3);
            mPaint.setAntiAlias(true);
			
            if(mPlist.size() >= 2){
                for (int i = 0; i < mPlist.size()-1; i++) {
					canvas.drawCircle(mPlist.get(i).x, mPlist.get(i).y, 4,mPaint);
	                 if(i==0){}else{
                    canvas.drawLine(mPlist.get(i).x, mPlist.get(i).y,mPlist.get(i-1).x ,mPlist.get(i-1).y, mPaint);
					//mPath.moveTo(mPlist.get(i).x, mPlist.get(i).y);
					 }
				}
            }
        }
		
        private void parallel(float xe,float ye,float x0,float y0,float xb,float yb) {
			float m=(xb+x0)/2;
			float n=(yb+y0)/2;
			float p=(xe+x0)/2;
			float q=(ye+y0)/2;
			float k=(q-n)/(p-m);
			float b=(n*p-m*q)/(p-m);
			float s=y0-k*x0;
			float z=b+s;
		}
		
        private void drawCurve0(Canvas canvas) {
            mPaint.setColor(Color.MAGENTA);
            mPaint.setStrokeWidth(3);
            mPaint.setAntiAlias(true);

            if(mPlist0.size() >= 2){
                for (int i = 0; i < mPlist0.size()-1; i++){
					canvas.drawCircle(mPlist0.get(i).x, mPlist0.get(i).y, 4, mPaint);
                    canvas.drawLine(mPlist0.get(i).x, mPlist0.get(i).y,mPlist0.get(i+1).x ,mPlist0.get(i+1).y, mPaint);
				 }
            }
        }
        public void Point(float m){
			
            float y=(float)(linear_acceleration[0]+0.55f);

            float Py = OFFSET_TOP + y*(CHARTH - OFFSET_TOP);
                  my=Py;
            PointF p = new PointF((float)OFFSET_LEFT/m/*+ CHARTW*/, Py);

			if(flag){

				if(mPlist.size() > 300){
					mPlist.remove(0);
					for (int i = 0; i < 300; i++) {
						{
						if(i == 0) mPlist.get(i).x += (X_INTERVAL - 1);
						else mPlist.get(i).x += X_INTERVAL;
						}
						mx=mPlist0.get(i).x;
					}
					mPlist.add(p);
				}
				else{
					for (int i = 0; i < mPlist.size(); i++) {
						mPlist.get(i).x += X_INTERVAL;
						mx=mPlist0.get(i).x;
					}
					mPlist.add(p);
				}
				M=linear_acceleration[0];
				if(M>=7.0f)
				{
					Intent i =new Intent(WaveChart.this,MyService.class);
					String s = "A";
					i.putExtra("s", s);
					i.setAction("broadcast");
					sendBroadcast(i);
				}
			}
		}

        public void Line0(float m){
			
            float y0=(float)(9.8f-gravity[2])/20;

            float Py0 = OFFSET_TOP + (y0*(CHARTH - OFFSET_TOP));

            PointF p0 = new PointF((float)OFFSET_LEFT/m , Py0);

			if(flag){
 
				if(mPlist0.size() > 300){
					mPlist0.remove(0);
					for (int i = 0; i < 300; i++) {
						{
						if(i == 0) mPlist0.get(i).x += (X_INTERVAL - 1);
						else mPlist0.get(i).x += X_INTERVAL;
						}
					}
					mPlist0.add(p0);
				}
				else{
					for (int i = 0; i < mPlist0.size(); i++) {
						mPlist0.get(i).x += X_INTERVAL;
					}
					mPlist0.add(p0);
				}
			}
		}
	}

	// 三点菜单
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        //noinspection SimplifiableIfStatement
		switch (item.getItemId()) {
			case  R.id.action_settings:CloseAll.getInstance().exit();
		}

        return super.onOptionsItemSelected(item);
    }

    @TargetApi(Build.VERSION_CODES.CUPCAKE)
    class Chart implements SensorEventListener {

        Sensor sensor;

        public Chart(Sensor sensor) {

            this.sensor = sensor;
        }

        public void register() {
            sensorManager.registerListener(this, sensor, SensorManager.SENSOR_DELAY_GAME);
        }

        public void unregister() {
            sensorManager.unregisterListener(this);
        }
        @Override
        public void onAccuracyChanged(Sensor arg0, int arg1) {
            // TODO Auto-generated method stub
        }
        @Override
        public void onSensorChanged(SensorEvent arg) {
            // TODO Auto-generated method stub
            // 检测时间
            long currentUpdateTime = System.currentTimeMillis();
            // 检测的时间间隔
            long timeInterval = currentUpdateTime - lastUpdateTime;
            // 判断是否达到了检测时间间隔
            if (timeInterval < UPTATE_INTERVAL_TIME)
                return;
            // 现在的时间变成last时间
            lastUpdateTime = currentUpdateTime;

            final float alpha = 0.8f;

            gravity[0] = alpha * gravity[SensorManager.DATA_X] + (1 - alpha) * arg.values[SensorManager.DATA_X];
            gravity[1] = alpha * gravity[SensorManager.DATA_Y] + (1 - alpha) * arg.values[SensorManager.DATA_Y];
            gravity[2] = alpha * gravity[SensorManager.DATA_Z] + (1 - alpha) * arg.values[SensorManager.DATA_Z];

            linear_acceleration[0] = arg.values[SensorManager.DATA_X] - gravity[SensorManager.DATA_X];
            linear_acceleration[1] = arg.values[SensorManager.DATA_Y] - gravity[SensorManager.DATA_Y];
            linear_acceleration[2] = arg.values[SensorManager.DATA_Z] - gravity[SensorManager.DATA_Z];

        }
	}
	@Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {

        if (keyCode == KeyEvent.KEYCODE_BACK) {
			Intent ient = new Intent(WaveChart.this, MainActivity.class);
			startActivity(ient);
		    this.finish();
		  } return true;
	  }
}

