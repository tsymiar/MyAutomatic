<?php
header('Content-Type: application/json;charset=utf-8');
/**
 * Description of Filetrans
 *
 * @author tsymiar
 */
define("ROOT_PATH", dirname(__FILE__));
define("ACTION_REGIST", "register");
define("ACTION_SET_DBDATA", "set_dbdata");
define("ACTION_SUBMIT_ARRAY", "submit_array");
define("ACTION_FILE_UPTOLOAD", "file_uptoload");
define("ACTION_FILE_DOWNLOAD", "file_download");

class HandleBase {
	public static function handle_request(){}
	public static function handle_empty(){
		$ret = array();
		 $ret["status"] = false;
		 $ret = json_encode($ret, JSON_PRETTY_PRINT);
		 echo $ret;
	 }
}
class Regist extends HandleBase{
	public static function handle_request(){
		$user = array();
		//echo "<a href='index.html'><input id=\"reset\" type=\"button\" value=\"重置\"></a><br>";
		if($_SERVER["REQUEST_METHOD"] === "POST" && $user = $_POST['user']){
			print_r($user);
			foreach($user as $k=>$val){
			 	//echo array_values($user)['$k'];
			}
			//echo $user["'name'"];
		}
		if($_SERVER["REQUEST_METHOD"] === "POST")
		{
			$detail = array();
			if($detail['comments'] = $_POST['comments']){
				
			}
			if($$detail['website'] = $_POST['website']){
				
			}
			if($detail['date'] = $_POST['date']){
				
			}
			if($detail['zip'] = $_POST['zip']){
				
			}
			print_r($detail);
		}
	}
}

class FileLoad extends HandleBase{
	public static function handle_request($action){
		switch($action){
			case ACTION_FILE_UPTOLOAD:
				self::handle_file_uptoload();
				 break;
			 case ACTION_FILE_DOWNLOAD:
				self::handle_file_download();
				 break;
			 default :
				 self::handle_empty();
				 break;
		 }
	}

	private static function create_folders($dir){
		return is_dir($dir) or (self::create_folders(dirname($dir)) and mkdir($dir, 0777));
 	}

 	 private static function handle_file_uptoload(){
		// Check if the form was submitted
		if($_SERVER["REQUEST_METHOD"] === "POST"){
			// Check if file was uploaded without errors
			if(isset($_FILES["file"]) && $_FILES["file"]["error"] == 0){
				// TODO: use php_fileinfo func to judge file type instead.
				// $allowed = array("jpg" => "image/jpg", "jpeg" => "image/jpeg", "gif" => "image/gif", "png" => "image/png");	$filename = $_FILES["file"]["name"];
				// $filetype = $_FILES["file"]["type"];
				$filesize = $_FILES["file"]["size"];
				// Verify file extension
				$ext = pathinfo($filename, PATHINFO_EXTENSION);
				// if(!array_key_exists($ext, $allowed)) die("Error: Please select a valid file format.");
				// Verify file size - 5MB maximum
				$maxsize = 5 * 1024 * 1024;
				if ($filesize > $maxsize) {
					die("Error: File size is larger than the allowed limit.");
				}
				// Verify MYME type of the file
				// if(in_array($filetype, $allowed)){
				echo "File: ".$_FILES["file"]["name"]."\n";
				echo "Type: ".$_FILES["file"]["type"]."\n";
				echo "Size: ".($_FILES["file"]["size"] / 1024)." kB\n";
				echo "Temp: ".$_FILES["file"]["tmp_name"]."\n";
				// Check whether path/file exists before uploading it
				$dir="upload";
				self::create_folders($dir);
				if (!is_dir($dir))
					echo "mkdir fail, may cause by limits of authority.\n";
				/*if(file_exists("upload/".$_FILES["file"]["name"])){
						echo $_FILES["file"]["name"]." is already exists."."\n";
				} else*/ {
					$rslt = move_uploaded_file($_FILES["file"]["tmp_name"], "upload/".$_FILES["file"]["name"]);
					if ($rslt) {
						echo "Stored in: ".ROOT_PATH."/upload/".$_FILES["file"]["name"]."\n";
					} else {
						echo "Upload Error: ".$rslt."\n";
					}
				}
				/*
				} else {
					echo "Error: There was a problem uploading your file. Please try again.";
				}
				*/
			} else {
				echo "Check File Error: ".$_FILES["file"]["error"];
			}
		}
	}

	private static function handle_file_download(){

		set_time_limit(0);
		// OS base dir </> is "../../../"(should base on httpd configure)
		// eg. server path </lib> here is <../../../lib>
		$fpath = './file.html';
		$file_pathinfo = pathinfo($fpath);
		$file_name = $file_pathinfo['basename'];
		//$file_extension = $file_pathinfo['extension'];
		$handle = fopen($fpath,"rb");
		if (FALSE === $handle){
			exit("open file error.");
		}
		$filesize = filesize($fpath);

		header("Content-type:multipart/form-data");
		header("Accept-Ranges:bytes");
		header("Accept-Length:".$filesize);
		header("Content-Disposition: attachment; filename=".$file_name);

		$contents = '';
		while (!feof($handle)) {
			$contents = fread($handle, 8192);
			echo $contents;
			ob_flush();	//release data of PHP buffer
			flush();	//send data to browser
		}
		fclose($handle);
		}
	}

// main
$action = isset($_REQUEST["action"]) ? $_REQUEST["action"] : NULL;

if(strpos($action, "file") === false){
	switch($action){
		case NULL:
			break;
		case ACTION_REGIST:
			Regist::handle_request();
			break;
		case ACTION_SET_DBDATA:
			// $file = file_get_contents('set2mysql.php');
			// eval($file);
			system("/usr/bin/php set2mysql.php");
			break;
		case ACTION_SUBMIT_ARRAY:
			Regist::handle_request();
			break;
		default:
			$ret = array();
			$ret["status"] = false;
			$ret = json_encode($ret, JSON_PRETTY_PRINT);
			echo $ret;
			break;
	}
} else {
	FileLoad::handle_request($action);
}
