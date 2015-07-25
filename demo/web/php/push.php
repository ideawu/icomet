<?php
// application should check user login here
if(get_magic_quotes_gpc()){
	$_GET['content'] = stripslashes($_GET['content']);
}
$cname = trim($_GET['cname']);
$content = trim($_GET['content']);

echo icomet_push($cname, $content);

function icomet_push($cname, $content){
	$cname = urlencode($cname);
	$content = urlencode($content);
	$url = "http://127.0.0.1:8000/push?cname=$cname&content=$content";

	$ch = curl_init($url) ;
	curl_setopt($ch, CURLOPT_HEADER, 0);
	curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1) ;
	$result = @curl_exec($ch) ;
	curl_close($ch) ;
	return $result;
}

