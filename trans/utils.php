<?php
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
 
 private static function errReport($msg) //报错
    {
        echo "ERROR: $msg";
    }
     
	public static function setSession() //设置session
	{
		$sid=uniqid('sid'); //生成sid
		session_id($sid);
		session_start();
		$_SESSION['user']=$this->username; //给session变量赋值
		$_SESSION['id']=$this->id; //..
		$_SESSION['level']=$this->level; //..
		if($this->usecookie) //如果使用cookie保存sid
		{
			if(!setcookie('sid',$sid,time()+$this->cookietime,$this->cookielocal))
			self::errReport("set cookie");
		}
		else
			setcookie('sid','',time()-3600); //清除cookie中的sid
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
			}
			elseif(md5($passwd)==$row['passwd']) //密码匹配
			{
				$this->id=$row['id'];
				$this->level=$row['level'];
				return true;
			}
			else //密码不匹配
			{
				self::errReport("password");
				return false;
			}
		}
		else
		{
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
?>