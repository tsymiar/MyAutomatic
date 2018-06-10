import java.io.*;
import java.lang.System;
import java.lang.String;
import java.lang.Long;
import java.net.URL;
import java.net.HttpURLConnection;

import org.apache.http.*;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.client.methods.HttpUriRequest;
import org.apache.http.impl.client.DefaultHttpClient;

public class client {

	private static String Byte2String(byte[] _bytes) { 
		String dst_str = "";
		for (byte _byte : _bytes) {   
			dst_str += (char) _byte;
  		}  
		return dst_str;
	}

	private static void printHeaders(HttpResponse response) {  
		Header[] headers = response.getAllHeaders();
	  	for (Header header : headers) {   
			System.out.println(header);
	  	} 
	}

	// @param: local file path. 
	private static int httpUpload(String file) throws Exception {  
		String arg;
	  	String url = "http://192.168.1.3:8080/file/Filetrans.php?action=file_upload";
	  	if( file == null)   
		arg = "untitled.iml";
		else
		arg = file;
	  	File binary = new File(arg);
	  	if (!binary.exists() || !binary.isFile()) {   
			throw new IOException("file don‘t exit error.");
	  	}  
		String boundary = Long.toHexString(System.currentTimeMillis());
		// Just generate some unique random value.  String CRLF = "\r\n";
		// Line separator required by multipart/form-data.
	  	HttpURLConnection connection = (HttpURLConnection)(new URL(url)).openConnection();
	  	// do necessary while default method is "GET"  connection.setRequestMethod("POST");
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
		CRLF +
				"Content-Disposition: form-data;name=\"file\";filename=\"" +
				binary.getName() +
				"\"\r\n" +    
		"Content-Type:application/octet-stream\r\n\r\n";
		byte[] head = sb.getBytes("utf-8");

	  	// get output stream  
		OutputStream output = new DataOutputStream(connection.getOutputStream());
	  	output.write(head);
	  	System.out.println(Byte2String(head));  
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
		System.out.println(Byte2String(foot));
	  
		// Request is lazily fired whenever you need to obtain information about response.  
		int response = ((HttpURLConnection) connection).getResponseCode();  
		System.out.println("Connection Code = " + response + "\n"); // Should be 200
	  
		StringBuilder buffer = new StringBuilder();  
		BufferedReader reader = null;  
		String result = null;  
		try {   
			// define BufferedReader to read the response from URL   
			reader = new BufferedReader(new InputStreamReader(connection.getInputStream()));   
			String line = null;   
			while ((line = reader.readLine()) != null) {
				    System.out.println("Message from remote server:\n" + line);
				    buffer.append(line);
			}
			result = buffer.toString();  
		} catch (IOException e) {   
			System.out.println("error send POST request! " + e);
			e.printStackTrace();
			throw new IOException("read data error.");  
		} finally {   
			if(reader!=null){    
				reader.close();   
			}  
		}  
		return response; 
	}

	// @param: remote file path; local file path. 
	private static int httpDownload(String reqsturl, String filelocal) throws Exception {  
		int cache = 10 * 1024;  
		boolean isWindows;  
		String url, head, splash, root;
		if (System.getProperty("os.name") != null && System.getProperty("os.name").toLowerCase().contains("windows")) {   
			isWindows = true;   
			splash = "\\";   
			root="D:\\home";  
		} else {   
			isWindows = false;
		    splash = "/";   
			root="./";  
		}  
		if( reqsturl == null) {   
			url = "http://192.168.1.3:8080/file/Filetrans.php?action=file_download";   
			head = "Content-Disposition";  
		} else {   
			url = reqsturl;
		    head = "Content-Type";  
		}  
		System.out.println("Request for: " + url);  

		HttpUriRequest methods;  
		try {   
			HttpClient client = new DefaultHttpClient();   
			if( reqsturl == null)
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
			} else
			    System.out.println("content header is null error.");
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

	public static void main(String[] args) throws Exception {  
		if (args.length == 0) {   
			System.out.println("Please set param [-u] or [-d].\n");   
			return;
	    }
	    switch (args[0]) {   
			case "-u":
			    httpUpload(null);
			    break;
		    case "-d":
			    httpDownload(null, null);
			    break;
		    default:
			    System.out.println("Method don't support error.\n");
			    break;
		}
	}

}
