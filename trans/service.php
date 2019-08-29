<?php
header('Content-Type: application/json;charset=utf-8');
header('Access-Control-Allow-Origin:*');
header('Access-Control-Allow-Methods:POST');
header('Access-Control-Allow-Headers:x-requested-with,content-type');
/**
 * Description of Filetrans
 * @author tsymiar
 */
define("ROOT_PATH", dirname(__FILE__));
define("ACTION_REGIST", "register");
define("ACTION_SET_DBDATA", "set_dbdata");
define("ACTION_SUBMIT_ARRAY", "submit_array");
define("ACTION_FILE_UPTOLOAD", "file_upload2");
define("ACTION_FILE_DOWNLOAD", "file_download");

include_once ('utils.php');

class HandleBase {
    public static function handle_request(){}
    public static function handle_empty(){
        User::errReport("No such method.");
    }
}

class Regist extends HandleBase{
    public static function handle_request(){
        $register = array();
        if($_SERVER["REQUEST_METHOD"] === "POST" 
            && ($enc_key = $_POST['username'])){
            $enc_val = $_POST['password'];
            $psw = Crypto::aes_decrypt($enc_val, $enc_key);
            $register['user'] = $enc_key;
            $register['psw'] = substr(md5($psw),8,16);
            $register['email'] = $_POST['email'];
            foreach($register as $k=>$val){
                //echo array_values($register)['$k'];
            }
            // echo json_encode($register, JSON_PRETTY_PRINT);
            $usrid = DBUtil::user_register(User::$table, $register);
            switch($usrid){
                case 0:
                    User::errReport("User register");
                    return;
                case -1:
                    User::errReport("User exist");
                    return;
                case -2:
                    User::errReport("Database error");
                    return;
            }
            $js_arr = array();
            $js_arr['img'] = $_POST['icon'];
            $js_arr['date'] = $_POST['date'];
            if($js_arr['date'] == '-'){
                $js_arr['date'] = '0000-00-00';
            }
            $js_arr['zip'] = $_POST['zip'];
            $js_arr['site'] = $_POST['website'];
            $js_arr['`desc`'] = $_POST['comments'];
            $err = DBUtil::user_update(User::$table, $js_arr, $usrid);
            User::errReport(0, 201, "SUCCESS", NULL, $err);
        }
    }
}

class FileLoad extends HandleBase{
    public static function handle_request(){
        $action = isset($_REQUEST["action"]) ? $_REQUEST["action"] : NULL;
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
                // $allowed = array("jpg" => "image/jpg", "jpeg" => "image/jpeg", "gif" => "image/gif", "png" => "image/png");    $filename = $_FILES["file"]["name"];
                // $filetype = $_FILES["file"]["type"];
                $filename = $_FILES["file"]["name"];
                $filesize = $_FILES["file"]["size"];
                // Verify file extension
                $ext = pathinfo($filename, PATHINFO_EXTENSION);
                // if(!array_key_exists($ext, $allowed)) die("ERROR: Please select a valid file format.");
                // Verify file size - 5MB maximum
                $maxsize = 5 * 1024 * 1024;
                if ($filesize > $maxsize) {
                    die("ERROR: File size is larger than the allowed limit.");
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
                if (!is_dir($dir)) {
                    User::errReport("Exec mkdir fail, may cause by limits of authority");
                }else{
                    /*if(file_exists("upload/".$_FILES["file"]["name"])){
                            echo $_FILES["file"]["name"]." is already exists."."\n";
                    } else {*/
                    $rslt = move_uploaded_file($_FILES["file"]["tmp_name"], "upload/".$_FILES["file"]["name"]);
                    if ($rslt) {
                        echo "Stored in: ".ROOT_PATH."/upload/".$_FILES["file"]["name"]."\n";
                    } else {
                        User::errReport("Exec move_uploaded_file ".strval($rslt));
                    }
                }
                /*
                } else {
                    echo "ERROR: There was a problem uploading your file. Please try again.";
                }
                */
            } else {
                User::errReport("Check File failure: ".$_FILES["file"]["error"]);
            }
        }
    }

    private static function trans_byte($byte){
        $KB = 1024;
        $MB = 1024 * $KB;
        $GB = 1024 * $MB;
        $TB = 1024 * $GB;
        if ($byte == "") {
            return $byte;
        } elseif ($byte < $KB) {
            return $byte . "B";
        } elseif ($byte < $MB) {
            return round($byte / $KB, 2) . "KB";
        } elseif ($byte < $GB) {
            return round($byte / $MB, 2) . "MB";
        } elseif ($byte < $TB) {
            return round($byte / $GB, 2) . "GB";
        } else {
            return round($byte / $TB, 2) . "TB";
        }
    }
    
    private static function get_filename($fdir){
        $file = scandir($fdir);
        foreach($file as $k=>$v){
            if($v=="." || $v==".."){
                continue;
            }
            if(abs(filemtime("./tempory/.") - filemtime("./tempory/".$v)) < 100) {
                return $v;
            }
        }
        return NULL;
    }

    private static function handle_file_download(){
        // OS base dir </> is "../../../"(should base on httpd configure)
        // eg. server path </lib> here is <../../../lib>
        set_time_limit(0);
        $fpath = "";
        $url = isset($_REQUEST["url"]) ? $_REQUEST["url"] : NULL;
        if(!empty($url)){
            shell_exec("wget -b -nc --restrict-file-names=nocontrol -P ./tempory ".escapeshellarg($url));
            $allow_type=array("iso","xls","xlsx","cpp","pdf","gif","mp3","mp4","zip","rar","doc","docx","mov","ppt","pptx","txt","7z","jpeg","jpg","JPEG","png");
            //允许的文件类型
            $torrent = explode(".",$url);
            $file_end = end($torrent);
            $file_end = strtolower($file_end);
            //if(in_array($file_end,$allow_type))
            {
                $parent = dirname(__FILE__);
                $fdir = $parent."/tempory";
                if (!is_dir($fdir)){
                    mkdir($fdir,0777,true);
                }
                $file = scandir($fdir);
                $conname = "";//array();
                $filetime = 0;
                foreach($file as $k=>$v){
                    if($v=="." || $v==".."){
                        $filetime = filemtime("./tempory/".$v);
                        continue;
                    }
                    /*
                    $conname[] = substr($v,0,strpos($v,"."));
                    if(filemtime("./tempory/".$conname) > $filetime) {
                        $filetime = filemtime("./tempory/".$conname);
                        $fpath = './tempory/'.$conname;
                    }
                    */
                    $conname = $v;
                    $delta = abs(filemtime("./tempory/.") - filemtime("./tempory/".$v));
                    if($delta < 100) {
                        $fpath = './tempory/'.$conname;
                        break;
                    }
                }
                exit(User::errReport(0, ($file ? 201 : 404), ($conname==""?self::get_filename($fdir):$conname) , NULL, !!$file));
            }
        } else if (isset($_REQUEST["query"])) {
            $name = $_REQUEST["query"];
            if($name == "" || $name == "null")
                $name = self::get_filename("./tempory");
            $fpath = "./tempory/".$name;
            $size = filesize($fpath);
            if(!isset($_REQUEST["size"]) || $size != intval($_REQUEST["size"])){
                $ret = array();
                $ret["size"] = $size;
                exit(User::errReport(0, 300, "Downloading... ".self::trans_byte($size), $ret));
            }else{
                exit(User::errReport(0, 201, $name));
            }
        } else if (isset($_REQUEST["file"])) {
            $name = $_REQUEST["file"];
            if($name == "")
                $name = self::get_filename("./tempory");
            $ret = array();
            $ret["href"] = '<a href="trans/getfile.php?file='.$name.'">';
            exit(User::errReport(0, 200, NULL, $ret));
        } else {
            exit(User::errReport(0, 404));
        }
        echo User::errReport(0, 200);
        Header("HTTP/1.1 303 See Other"); 
        Header("Location: getfile.php?file=".$fpath);
        $file_pathinfo = pathinfo($fpath);
        $file_name = $file_pathinfo['basename'];
        //$file_extension = $file_pathinfo['extension'];
        if($fpath == ""){exit("ERROR: local file not set.");}
        $handle = fopen($fpath,"rb");
        if (FALSE === $handle){
            exit("ERROR: open local file(".$fpath.").");
        }
        $file_size = filesize($fpath);
        header("Content-type:application/octet-stream");
        header("Accept-ranges:bytes");
        header("Accept-length:".$file_size);
        header("Content-Disposition: attachment; filename=".$file_name);

        while (!feof($handle)) {
            $contents = fread($handle, 8192);
            echo $contents;
            ob_flush(); //release data of PHP buffer
            flush();    //send data to browser
        }
        fclose($handle);
    }
}

// main
$action = isset($_REQUEST["action"]) ? $_REQUEST["action"] : NULL;

if(strpos($action, "file") === false){
    switch($action){
        case NULL:
        case null:
        case "null":
        case "NULL":
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
            echo json_encode($ret, JSON_PRETTY_PRINT);
            break;
    }
} else {
    FileLoad::handle_request();
}
