/////////////////////////////////////////////////////////
//  Bare Bones Browser Launch                          //
//  Version 3.2 (October 24, 2010)                     //
//  By Dem Pilafian                                    //
//  Supports:                                          //
//     Mac OS X, GNU/Linux, Unix, Windows XP/Vista/7   //
//  Example Usage:                                     //
//     String url = "http://centerkey.com/";           //
//     BareBonesBrowserLaunch.openURL(url);            //
//  WTFPL -- Free to use as you like                   //
/////////////////////////////////////////////////////////

import javax.swing.JOptionPane;
import java.io.*;
import java.util.Arrays;
import java.util.Map;
import java.util.Objects;

class StreamThread extends Thread {
    private String type;
    private InputStream is;
    StreamThread(InputStream is, String type) {
        this.is = is;
        this.type = type;
    }
    private static String toUtf8(String str) {
        try {
            return new String(str.getBytes("UTF-8"),"UTF-8");
        } catch (UnsupportedEncodingException e) {
            e.printStackTrace();
        }
        return str;
    }
    public void run() {
        try {
            InputStreamReader isr = new InputStreamReader(is);
            BufferedReader br = new BufferedReader(isr);
            String line;
            while ((line = br.readLine()) != null)
                System.out.println(type + ">\t" + toUtf8(line));
        } catch (IOException ioe) {
            ioe.printStackTrace();
        }
    }
}

public class BareBonesBrowserLaunch {

   static final String[] browsers = {"x-www-browser", "google-chrome",
           "firefox", "opera", "epiphany", "konqueror", "conkeror", "midori",
           "kazehakase", "mozilla"};
   static final String errMsg = "Error attempting to launch web browser";

   public static void openURL(String url) {
      try {  //attempt to use Desktop library from JDK 1.6+
         Class<?> d = Class.forName("java.awt.Desktop");
         d.getDeclaredMethod("browse", new Class[]{java.net.URI.class}).invoke(
                 d.getDeclaredMethod("getDesktop").invoke(null),
                 new Object[]{java.net.URI.create(url)});
         //above code mimicks:  java.awt.Desktop.getDesktop().browse()
      } catch (Exception ignore) {  //library not available or failed
         String osName = System.getProperty("os.name");
         try {
            if (osName.startsWith("Mac OS")) {
               Class.forName("com.apple.eio.FileManager").getDeclaredMethod(
                       "openURL", new Class[]{String.class}).invoke(null,
                       new Object[]{url});
            } else if (osName.startsWith("Windows"))
               Runtime.getRuntime().exec(
                       "rundll32 url.dll,FileProtocolHandler " + url);
            else { //assume Unix or Linux
               String browser = null;
               for (String b : browsers)
                  if (browser == null && Runtime.getRuntime().exec(new String[]
                          {"which", b}).getInputStream().read() != -1)
                     Runtime.getRuntime().exec(new String[]{browser = b, url});
               if (browser == null)
                  throw new Exception(Arrays.toString(browsers));
            }
         } catch (Exception e) {
            JOptionPane.showMessageDialog(null, errMsg + "\n" + e.toString());
         }
      }
   }

   public static void execWinCmd(String cmd) throws IOException {
       String[] env_arr = {null};
       Map<String, String> map = System.getenv();
       int i = 0;
       Process proc = null;
       String text;
       for (String key : map.keySet()) {
           String env = map.get(key);
           if (key.equals("Path")) {
               env_arr[i] = key + "=" + env;
           }
       }
       try{
           String osName = System.getProperty("os.name");
           System.out.println(osName);
           String[] cmd_arr = new String[3];
           if (osName.equals("Windows 95")) {
               cmd_arr[0] = "command.com";
           }else if (osName.contains("Windows")) {
               cmd_arr[0] = "cmd.exe";
            }else{
                System.out.println("Support on Windows OS ONLY!");
                return;
            }
           cmd_arr[1] = "/C";
           cmd_arr[2] = cmd;
           System.out.println(Arrays.toString(env_arr));
           System.out.println(Arrays.toString(cmd_arr));
           try {
               proc = Runtime.getRuntime().exec(cmd_arr, env_arr);
           } catch (IOException e) {
               e.printStackTrace();
           }
		   StreamThread msgStream = new StreamThread(Objects.requireNonNull(proc).getErrorStream(), "MSG");
           StreamThread outStream = new StreamThread(proc.getInputStream(), "OUT");
           msgStream.start();
           outStream.start();
           int exitVal = proc.waitFor();
           System.out.println("exitValue: " + exitVal);
       } catch (Throwable t) {
           t.printStackTrace();
       }
       BufferedReader br = new BufferedReader(new
               InputStreamReader(Objects.requireNonNull(proc)
               .getInputStream()));
       // BufferedInputStream bis = new  BufferedInputStream(proc.getInputStream());
       while ((text = br.readLine()) != null) {
           System.out.println(text);
       }
    }

}
