import java.io.BufferedReader;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;

import java.net.HttpURLConnection;
import java.net.URL;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;

import org.apache.http.Header;
import org.apache.http.HeaderElement;
import org.apache.http.HttpEntity;
import org.apache.http.HttpResponse;
import org.apache.http.NameValuePair;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.client.methods.HttpUriRequest;
import org.apache.http.impl.client.DefaultHttpClient;

class FileType {

    private final static Map<String, String> MAP_FILE_TYPE = new HashMap<>();

    static {
        setAllFileType();
    }

    private static void setAllFileType() {
        MAP_FILE_TYPE.put("jpg", "FFD8FF"); //JPEG (jpg)
        MAP_FILE_TYPE.put("png", "89504E47");  //PNG (png)
        MAP_FILE_TYPE.put("gif", "47494638");  //GIF (gif)
        MAP_FILE_TYPE.put("tif", "49492A00");  //TIFF (tif)
        MAP_FILE_TYPE.put("bmp", "424D"); //Windows Bitmap (bmp)
        MAP_FILE_TYPE.put("dwg", "41433130"); //CAD (dwg)
        MAP_FILE_TYPE.put("html", "68746D6C3E");  //HTML (html)
        MAP_FILE_TYPE.put("rtf", "7B5C727466");  //Rich Text Format (rtf)
        MAP_FILE_TYPE.put("xml", "3C3F786D6C");
        MAP_FILE_TYPE.put("zip", "504B0304");
        MAP_FILE_TYPE.put("rar", "52617221");
        MAP_FILE_TYPE.put("psd", "38425053");  //Photoshop (psd)
        MAP_FILE_TYPE.put("eml", "44656C69766572792D646174653A");  //Email [thorough only] (eml)
        MAP_FILE_TYPE.put("dbx", "CFAD12FEC5FD746F");  //Outlook Express (dbx)
        MAP_FILE_TYPE.put("pst", "2142444E");  //Outlook (pst)
        MAP_FILE_TYPE.put("xls", "D0CF11E0");  //MS Word
        MAP_FILE_TYPE.put("doc", "D0CF11E0");  //MS Excel
        MAP_FILE_TYPE.put("mdb", "5374616E64617264204A");  //MS Access (mdb)
        MAP_FILE_TYPE.put("wpd", "FF575043"); //WordPerfect (wpd)
        MAP_FILE_TYPE.put("eps", "252150532D41646F6265");
        MAP_FILE_TYPE.put("ps", "252150532D41646F6265");
        MAP_FILE_TYPE.put("pdf", "255044462D312E");  //Adobe Acrobat (pdf)
        MAP_FILE_TYPE.put("qdf", "AC9EBD8F");  //Quicken (qdf)
        MAP_FILE_TYPE.put("pwl", "E3828596");  //Windows Password (pwl)
        MAP_FILE_TYPE.put("wav", "57415645");  //Wave (wav)
        MAP_FILE_TYPE.put("avi", "41564920");
        MAP_FILE_TYPE.put("ram", "2E7261FD");  //Real Audio (ram)
        MAP_FILE_TYPE.put("rm", "2E524D46");  //Real Media (rm)
        MAP_FILE_TYPE.put("mpg", "000001BA");  //
        MAP_FILE_TYPE.put("mov", "6D6F6F76");  //Quicktime (mov)
        MAP_FILE_TYPE.put("asf", "3026B2758E66CF11"); //Windows Media (asf)
        MAP_FILE_TYPE.put("mid", "4D546864");  //MIDI (mid)
    }

    private static String getFileTypeByStream(byte[] hex) {
        String fileTypeHex = String.valueOf(getFileHexString(hex));
        for (Entry<String, String> entry : MAP_FILE_TYPE.entrySet()) {
            String fileTypeHexValue = entry.getValue();
            if (fileTypeHex.toUpperCase().startsWith(fileTypeHexValue)) {
                return entry.getKey();
            }
        }
        return null;
    }

    private static String getFileHexString(byte[] b) {
        StringBuilder stringBuilder = new StringBuilder();
        if (b == null || b.length <= 0) {
            return null;
        }
        for (byte a : b) {
            int v = a & 0xFF;
            String hv = Integer.toHexString(v);
            if (hv.length() < 2) {
                stringBuilder.append(0);
            }
            stringBuilder.append(hv);
        }
        return stringBuilder.toString();
    }

    public static String getFileType(File file) {
        String type = null;
        byte[] b = new byte[50];
        try {
            InputStream is = new FileInputStream(file);
            int len = is.read(b);
            System.out.println("getFileType len = " + len);
            type = getFileTypeByStream(b);
            is.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
        return type;
    }
}

public class Client {

    private static final byte[] CRLF = new byte[]{13, 10};

    private static String byte2String(byte[] _bytes) {
        StringBuilder dest_ = new StringBuilder();
        for (byte _byte : _bytes) {
            dest_.append((char) _byte);
        }
        return dest_.toString();
    }

    private static void printHeaders(HttpResponse response) {
        Header[] headers = response.getAllHeaders();
        for (Header header : headers) {
            System.out.println(header);
        }
    }

    // @param: local file path.
    private static int httpUpload(String file) throws Exception {
        String arg;
        String url = "http://127.0.0.1:8080/MyAutomatic/trans/service.php?action=file_upload2";
        if(file == null)
            arg = "untitled.iml";
        else
            arg = file;
        File binary = new File(arg);
        if (!binary.exists() || !binary.isFile()) {
            throw new IOException("file don‘t exit error.");
        }
        String boundary = Long.toHexString(System.currentTimeMillis());
        // Just generate some unique random value.  String CRLF = "\r\n";
        // Line separator required by multipart/form-data.
        HttpURLConnection connection = (HttpURLConnection)(new URL(url)).openConnection();
        // do necessary while default method is "GET"  connection.setRequestMethod("POST");
        connection.setDoInput(true);
        connection.setDoOutput(true);
        connection.setUseCaches(false); // POST should have no cache
        System.out.println("Show 'head' & 'foot' we want to send:\n");

        // set head
        connection.setRequestProperty("Connection", "Keep-Alive");
        connection.setRequestProperty("Charset", "UTF-8");
        // set boundary
        connection.setRequestProperty("Content-Type", "multipart/form-data;boundary=" + boundary);
        // set message
        String sb = "--" + // --
                boundary +
                Arrays.toString(CRLF) + //?
                "Content-Disposition: form-data;name=\"file\";filename=\"" +
                binary.getName() +
                "\"\r\n" +
                "Content-Type:application/octet-stream\r\n\r\n";
        byte[] head = sb.getBytes("utf-8");

        // get output stream
        OutputStream output = new DataOutputStream(connection.getOutputStream());
        output.write(head);
        System.out.println(byte2String(head));
        // push file to the server as stream
        DataInputStream input = new DataInputStream(new FileInputStream(binary));
        int bytes = 0;
        byte[] bufferOut = new byte[1024];
        while ((bytes = input.read(bufferOut)) != -1) {
            output.write(bufferOut, 0, bytes);
        }
        input.close();
        // End of multipart/form-data.
        byte[] foot = ("\r\n--" + boundary + "--\r\n").getBytes("utf-8");
        output.write(foot);
        output.flush();
        output.close();
        System.out.println(byte2String(foot));

        // Request is lazily fired whenever you need to obtain information about response.
        int response = ((HttpURLConnection) connection).getResponseCode();
        System.out.println("Connection Code = " + response + "\n"); // Should be 200

        try (BufferedReader reader = new BufferedReader(new InputStreamReader(connection.getInputStream()))) {
            StringBuilder buffer = new StringBuilder();
            // define BufferedReader to read the response from URL
            String line;
            while ((line = reader.readLine()) != null) {
                System.out.println("Message from remote server:\n" + line);
                buffer.append(line);
            }
        } catch (IOException e) {
            System.out.println("error send POST request! " + e);
            e.printStackTrace();
            throw new IOException("read data error.");
        }
        return response;
    }

    // @param: remote file path; local file path.
    private static int httpDownload(String reqsturl, String local) throws Exception {
        String url;
        String head;
        if (reqsturl == null) {
            url = "http://127.0.0.1:8080/MyAutomatic/trans/service.php?action=file_download";
            head = "Content-Disposition";
        } else {
            url = reqsturl;
            head = "Content-Type";
        }
        System.out.println("Request for: " + url);

        HttpUriRequest methods;
        try {
            HttpClient client = new DefaultHttpClient();
            if(reqsturl == null)
                methods = new HttpPost(url);
            else
                methods = new HttpGet(url);

            HttpResponse response = client.execute(methods);
            HttpEntity entity = response.getEntity();
            InputStream input = entity.getContent();
            Header content = response.getFirstHeader(head);
            String filename = null;
            String[] array;
            if (reqsturl != null) {
                array = reqsturl.split("/");
                filename = array[array.length-1];
            }
            printHeaders(response);
            if (content != null) {
                HeaderElement[] values = content.getElements();
                if (values.length == 1) {
                    NameValuePair param = values[0].getParameterByName("filename");
                    if (param != null) {
                        try {
                            filename = param.getValue();
                        } catch (Exception e) {
                            e.printStackTrace();
                        }
                    }
                } else
                    System.out.println("values.length =" + values.length);
            } else {
                System.out.println("content header is null error.");
            }
            String root;
            String splash;
            if (System.getProperty("os.name") != null && System.getProperty("os.name").toLowerCase().contains("windows")) {
                splash = "\\";
                root = "D:\\home";
            } else {
                splash = "/";
                root = "./";
            }
            String filelocal = local;
            if (filelocal == null) {
                filelocal = root + splash;
                if (filename != null) {
                    filelocal += filename;
                } else {
                    filelocal += String.valueOf(System.currentTimeMillis());
                }
            }

            File file = new File(filelocal);
            file.getParentFile().mkdirs();
            FileOutputStream fileout = new FileOutputStream(file);
            /*set buffer size refer to real size of file.*/
            int cache = 10 * 1024;
            byte[] buffer = new byte[cache];
            int ch = 0;
            while ((ch = input.read(buffer)) != -1) {
                fileout.write(buffer, 0, ch);
            }
            input.close();
            fileout.flush();
            fileout.close();
            System.out.println("\nHave download '" + filename + "' to:\n" + filelocal);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return 0;
    }

    public static void main(String[] args) throws Exception {
        if (args.length == 0) {
            System.out.println("Please set param [-u] or [-d].\n");
            return;
        }
        String file = null;
        String url = null;
        if (args.length == 2){
            file = args[1];
        }
        if (args.length >= 3){
            url = args[2];
        }
        switch (args[0]) {
            case "-u":
                httpUpload(file);
                break;
            case "-d":
                httpDownload(url, file);
                break;
            default:
                System.out.println("Method don't support error.\n");
                break;
        }
    }
}
