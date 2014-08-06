<?php
// application should check user login here
if(get_magic_quotes_gpc()){
	$_GET['content'] = stripslashes($_GET['content']);
}

$cb = urlencode($_GET['cb']);
$cname = urlencode($_GET['cname']);
$content = urlencode($_GET['content']);
$url = "http://127.0.0.1:8000/pub?cname=$cname&cb=$cb&content=$content";
$resp = http_get($url);
echo $resp;

function http_get($url){
	$ch = curl_init($url) ;
	curl_setopt($ch, CURLOPT_HEADER, 0);
	curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1) ;
	$result = curl_exec($ch) ;
	curl_close($ch) ;
	return $result;
}
