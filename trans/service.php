<?php
header('Content-Type: application/json;charset=utf-8');
header('Access-Control-Allow-Origin:*');
header('Access-Control-Allow-Methods:POST');
header('Access-Control-Allow-Headers:x-requested-with,content-type');
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

include_once ('utils.php');

class HandleBase {
    public static function handle_request(){}
    public static function handle_empty(){
        $json = array();
        $json["status"] = false;
        $json["message"] = "No such method.";
        $json = json_encode($json, JSON_PRETTY_PRINT);
        echo $json;
    }
}

class Regist extends HandleBase{
    public static function handle_request(){
        $register = array();
        if($_SERVER["REQUEST_METHOD"] === "POST" 
            && ($enc_key = $_POST['username'])){
            $enc_val = $_POST['passwd'];
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
                    User::errReport("user register");
                    return;
                case -1:
                    User::errReport("user exist");
                    return;
                case -2:
                    User::errReport("database");
                    return;
            }
            $js_arr = array();
            $js_arr['img'] = $_POST['icon'];
            $js_arr['date'] = $_POST['date'];
            $js_arr['zip'] = $_POST['zip'];
            $js_arr['site'] = $_POST['website'];
            $js_arr['`desc`'] = $_POST['comments'];
            $err = DBUtil::user_update(User::$table, $js_arr, $usrid);
            if($err <= 0){
                User::errReport("user update");
                return;
            }
            $js_arr['user'] = $register;
            $js_arr['code'] = 200;
            $js_arr['message'] = "OK";
            echo json_encode($js_arr, JSON_PRETTY_PRINT);
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
                    User::errReport("mkdir fail, may cause by limits of authority");
                }else{
                    /*if(file_exists("upload/".$_FILES["file"]["name"])){
                            echo $_FILES["file"]["name"]." is already exists."."\n";
                    } else {*/
                    $rslt = move_uploaded_file($_FILES["file"]["tmp_name"], "upload/".$_FILES["file"]["name"]);
                    if ($rslt) {
                        echo "Stored in: ".ROOT_PATH."/upload/".$_FILES["file"]["name"]."\n";
                    } else {
                        User::errReport("move_uploaded_file ".strval($rslt));
                    }
                }
                /*
                } else {
                    echo "ERROR: There was a problem uploading your file. Please try again.";
                }
                */
            } else {
                User::errReport("Check File ".$_FILES["file"]["error"]);
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
            exit("ERROR: open file.");
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
            ob_flush();    //release data of PHP buffer
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
