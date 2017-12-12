<?php
header('content-type:text/html; charset=utf8');

define("DB_HOST","localhost");
define("DB_USER","root");
define("DB_PWD","psw123");
define("DB_DATABASE","custominfo");
define("DB_CHARSET","utf8");
define("DB_TABLE","myweb");

$file = "game.txt";
$link = ""; //global mysql descriptor
// connect func
function connect(){
	global $link;
	$link = mysqli_connect(DB_HOST,DB_USER,DB_PWD) or die("ERROR connect to database:".mysql_errno().":".mysql_error());
	mysqli_select_db($link, DB_DATABASE) or die("open DB_DATABASE error.");
	$sql = 'set names '.DB_CHARSET;
	mysqli_query($link, $sql) or die ("set charset error.");
	return $link;
}
/* deal func, array is not a turely array.*/
function insert($table, $array){
	global $link;
	$key = join(",", array_keys($array));
	$val = "'".join("','", array_values($array))."'";
	$id = $val[$key][0];
	# $sql = "insert into {$table} /*({$key})*/ values ({$val})";
	// $sql = "UPDATE {$table} set ({$val}) where id={$id}";
	// mysqli_query($link, $sql);
	for ($i = 1; $i < count($array); $i++)
	{
		$vol =  array_keys($array)[$i];
		$vlu = array_values($array)[$i];
        $sql = "update {$table} set {$vol}='{$vlu}' where id={$id};";
        mysqli_query($link, $sql);
	}
	return mysqli_insert_id($link);
}

function one_param_query($conn, $sql, $param, $type = "s"){
	if ($stmt = mysqli_prepare($conn, $sql)){
		$stmt->bind_param($type, $param);
		$stmt->execute();
		$rslt = $stmt->get_result();
		return $rslt;
	} else {
		echo "'mysqli_prepare' got an issue.\n";
		mysqli_close($conn);
		return NULL;
	}
}

function select_images($table)
{
	global $link;
	$value = "";
	$sql = "select img from ? order by id, name";
	if($rslt = one_param_query($link, $sql, $table)){
		$val = mysqli_fetch_assoc($link, $rslt);
		$value = array_pop($val);
	} else {
		echo "function 'one_param_query' error.\n";
	}
	echo $value;return $value;}
	// set file content as php object
	$jscnt = file_get_contents($file);
	// encode json string as php array
	// $jscnt = utf8_encode($jscnt);
	$jsarr = json_decode($jscnt,true);
	if (!is_array($jsarr))
		die("set data as php array NOT successful.\n");
	// connect with mysql
	connect();
	// 2-dimensional array
	$flag = 0;
	foreach($jsarr as $k=>$val){
		for($j=0; $j<count($val); $j++){
			// 1-dimensional array
			$i = insert(DB_TABLE, $val[$j]);
			if ($i != 0) {
				echo "maybe some error in inserting ".$val[$j]." to ".$file."\n";
				$flag = 1;
			}
		}
	}
	if($flag == 0)
	echo "fill data of ".$file." OK.\n";
	// select_images(DB_TABLE);
	if(!$link){
		mysqli_close($link);
	}
?>

