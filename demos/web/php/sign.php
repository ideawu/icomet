<?php
// application should check user login here

$cb = urlencode($_GET['cb']);
$cname = urlencode($_GET['cname']);
$url = "http://127.0.0.1:8000/sign?cname=$cname&cb=$cb";
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
