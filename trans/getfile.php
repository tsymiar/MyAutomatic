<?php
if (isset($_GET['file'])) {
    $name = $_GET['file'];
    if($name == NULL || name == "")
        $name = self::get_filename("./tempory");
    $fpath = "./tempory/".$name;
    $file_pathinfo = pathinfo($fpath);
    $file_name = $file_pathinfo['basename'];
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
        ob_flush();
        flush();
    }
    fclose($handle);
}else{
    exit("ERROR: download file name not set.");
}