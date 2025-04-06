<?php
if (isset($_GET['file'])) {
    $name = $_GET['file'];
    if($name == NULL || name == "")
        $name = self::get_filename("./tempory");
    $file_path = "./tempory/".$name;
    $file_info = pathinfo($file_path);
    $file_name = $file_info['basename'];
    $handle = fopen($file_path,"rb");
    if (FALSE === $handle){
        exit("ERROR: open local file(".$file_path.").");
    }
    $file_size = filesize($file_path);

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
} else {
    exit("ERROR: download file name not set.");
}
