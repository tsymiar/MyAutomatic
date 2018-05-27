<?php

ini_set('display_errors',1);
ini_set('display_startup_errors',1);
error_reporting(-1);
ini_set('error_log', dirname(__FILE__) . '/error_log.txt');
header('content-type:text/html; charset=utf8'); 

class User {
    
    static $table="glkline";  //数据表
    static $err_user="bannd";  //用户无效(被封禁)

    var $id;
    var $level;
    var $usecookie=true;    //使用cookie保存sessionid
    var $cookielocal='/';   //cookie路径
    var $cookietime=108000; //cookie有效时间

    var $username;  //用户名
    var $passwd;    //密码
    var $email;     //邮箱
    var $date;  //生日
    var $icon;  //头像
    var $website; //主页
    var $zip;  //手机号吗
    var $comments; //签名 
 
    public static function errReport($err,$code,$msg,$data) { //报错
        if (!$code && !$msg) {
            echo "ERROR: $err.\n";
        } else {
            $json = array();
            $json["code"] = $code;
            $json["message"] = $msg;
            if($data){
                $json["data"] = $data;
            }
            echo json_decode ($json, JSON_PRETTY_PRINT);
        }
    }
     
    public static function setSession() //设置session
    {
        $sid=uniqid('sid'); //生成sid
        session_id($sid);
        session_start();
        $_SESSION['user']=$this->username; //给session变量赋值
        $_SESSION['id']=$this->id; //..
        $_SESSION['level']=$this->level; //..
        if ($this->usecookie) { //如果使用cookie保存sid
            if (!setcookie('sid', $sid, time() + $this->cookietime, $this->cookielocal)) {
                self::errReport("set cookie");
            }
        } else {
            setcookie('sid', '', time() - 3600);
        } //清除cookie中的sid
    }

    public static function userAuth($username,$passwd) //用户认证
    {
        $this->username = $username;
        $this->passwd = $passwd;
        $row = DBUtil::query_user($username);
        if($row != null)
        {
            if($row['bannd']==1) //用户被封禁
            {
                self::errReport(self::$err_user);
                return false;
            } elseif(md5($passwd)==$row['passwd']) //密码匹配
            {
                $this->id=$row['id'];
                $this->level=$row['level'];
                return true;
            } else //密码不匹配
            {
                self::errReport("password");
                return false;
            }
        } else {
            self::errReport("username");
            return false;
        }
    }
}

class DBUtil{
 
    // connect func
    private static function get_sql_link(){
       $link = mysqli_connect("localhost","root","Psw123$") or die("ERROR connect to database:".mysql_errno().":".mysql_error());
       mysqli_select_db($link, "custominfo") or die("open `custominfo` error.");
       $sql = 'set names utf8';
       mysqli_query($link, $sql) or die ("set charset error.");
       return $link;
   }

    public static function user_register($table, $array){
        $link = self::get_sql_link();
        $key = join(",", array_keys($array));
        $val = "'".join("','", array_values($array))."'";
        $sql = "select count(*) from {$table} as total where user='".$array["user"]."'";
        $res = mysqli_query($link, $sql);
        if(!$res){
            mysqli_close($link);
            return -2;
        }
        $arr = mysqli_fetch_array($res);
        $num = $arr[0];
        if($num > 0){
            mysqli_close($link);
            return -1;
        }
        $sql = "insert into {$table} ({$key}) values ({$val})";
        mysqli_query($link, $sql);
        $id = mysqli_insert_id($link);
        mysqli_close($link);
        return $id;
    }
 
    public static function user_update($table,$array,$id){
        $link = self::get_sql_link();
        $key = join(",", array_keys($array));
        $val = "'".join("','", array_values($array))."'";
        $sql = "update {$table} set ";
        for ($i = 0; $i < count($array); $i++)
        {
            $key = array_keys($array)[$i];
            $val = array_values($array)[$i];
            if($i >= 1){
                $sql .= ", ";
            }
            $sql .= "{$key}='{$val}'";
        }
        $sql .= " where idx=".$id;
        $res = mysqli_query($link, $sql);
        mysqli_close($link);
        return $res;
    }
 
    public static function query_user($username){
        $link = self::get_sql_link();
        $query = "select * from `".User::$table."` where `username`='$username'";
        $user = mysqli_query($query);
        $row = null;
        if(mysqli_num_rows($user) != 0)
        {
                $row = mysqli_fetch_array($user);
        }
        mysqli_close($link);
        return $row;
    }
}

class Crypto
{
    private static $iv = "a6afbbcbf8be7668";

    public static function init()
    {
        if(0){
            self::$iv = self::astr2bin(self::$iv);
        }
    }
    
    private static function astr2bin($ina){  
        // (?<!^)后瞻消极断言
        // (?!$) 前瞻消极断言
        $arr = preg_split('/(?<!^)(?!$)/u', $ina);
        /**
         * unpack：将二进制字符串解包(Unpack data from binary string)
         * H: Hex string, high nibble first 
         */
        foreach($arr as &$v){
            $temp = unpack('H*', $v);
            $v = base_convert($temp[1], 16, 2);  
            unset($temp);  
        }  
        return join(' ', $arr);  
    }
    
    public static function bstr2bin($inb){
        // Binary representation of a binary-string
        if (!is_string($inb)) {
            return null; // Sanity check
        }
        // Unpack as a hexadecimal string
        $val = unpack('H*', $inb);
        // Output binary representation
        $arr = str_split($val[1], 1);
        $bin = '';
        foreach ($arr as $v)
        {
            $b = str_pad(base_convert($v, 16, 2), 4, '0', STR_PAD_LEFT);
            $bin .= $b;
        }
        return $bin;
    }
    
    public static function stripPkcs7Padding($string) {
        $strpad = $string;
        $last = ord(substr($strpad, -1));  
        $slastc = chr($last);  
        $check = substr($strpad, -$last);  
        if (!$check) {
            return null;
        }
        if (preg_match("/$slastc{" . $last . "}/", $strpad)) {  
            $strpad = substr($strpad, 0, strlen($strpad) - $last);  
            return $strpad;  
        } else {
            return false;  
        }  
    }  
    public static function hexStringify($hex){
        $string = '';
        for ($i=0; $i < strlen($hex)-1; $i+=2){
            $string .= chr(hexdec($hex[$i].$hex[$i+1]));
        }
        return $string;
    }

    public static function aes_encrypt($text, $key)
    {       
        $enc_key = md5($key);
        $encrypted_data = mcrypt_encrypt(MCRYPT_RIJNDAEL_128, $enc_key, $text, MCRYPT_MODE_CBC, self::$iv);  
        // openssl_encrypt($text, 'aes-128-cbc', $enc_key, OPENSSL_ZERO_PADDING, self::$iv);
        return base64_encode($encrypted_data);
    }

    public static function aes_decrypt($data, $key)
    {
//        $dec_key = pack('H*',md5(strtoupper($key)));
//        $iv_len = mcrypt_get_iv_size(MCRYPT_RIJNDAEL_128, MCRYPT_MODE_CBC);
//        $decrypted = mcrypt_decrypt(MCRYPT_RIJNDAEL_128, $dec_key, base64_decode(str_replace(' ', '+', $data)), MCRYPT_MODE_CBC, self::astr2bin(self::$iv));
//        $bindec = rtrim($decrypted, "\0");
//        $strpad = self::stripPkcs7Padding($bindec);
//        if (!$strpad) {
//            $bindec = $strpad;
//        }
//        return self::hexStringify(bin2hex(trim($bindec)));
        $dec_key = md5($key);
        $ciphertext = base64_decode(str_replace(' ', '+', $data));
        $dec_val = base64_decode(str_replace(' ', '+', $ciphertext));
        $decrypted = mcrypt_decrypt(MCRYPT_RIJNDAEL_128, $dec_key, $dec_val, MCRYPT_MODE_CBC, self::$iv);
        // openssl_decrypt($dec_val, 'aes-128-cbc', $dec_key, OPENSSL_ZERO_PADDING, self::$iv);
        return $decrypted;
    }
}
