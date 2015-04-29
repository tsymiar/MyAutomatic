package bulutooth.oscillogragh.myapplication;

import android.app.Activity;
import android.app.Application;

import java.util.LinkedList;
import java.util.List;

public class CloseAll extends Application {
    //save every activity by List
    private List<Activity> mList = new LinkedList<>();
    private static CloseAll instance;
    private CloseAll(){}    
    public synchronized static CloseAll getInstance(){   
        if (null == instance) {   
            instance = new CloseAll();   
        }   
        return instance;   
    }   
    // add Activity    
    public void addActivity(Activity activity) {   
        mList.add(activity);   
    }   
    //close every listed-activity  
    public void exit() {   
        try {   
            for (Activity activity:mList) {   
                if (activity != null)   
                    activity.finish();   
            }   
        } catch (Exception e) {   
            e.printStackTrace();   
        } finally {   
            System.exit(0);   
        }   
    }   
    //kill the process  
    public void onLowMemory() {   
        super.onLowMemory();       
        System.gc();   
    }    
}  
