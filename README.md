icomet
======

A C1000K comet/push server built with libevent.

iComet is powerfull, can be used as the messaging server of many applications, such as [web chat](http://www.ideawu.com/icomet/chat.html), mobile application, desktop application etc.

iComet provides a easy-to-use JavaScript library, with iComet js lib, you can build a web app that needs server-push feature very fast.

## Supported Platforms and Browsers

| Browser | Platform |
| --------| -------- |
| Safari  | iOS(iPhone, iPod, iPad), Mac |
| Firefox | Windows, Mac |
| Chrome  | Windows, Mac |
| IE6, IE8 | Windows |


## Usage

```shell
wget --no-check-certificate https://github.com/ideawu/icomet/archive/master.zip
unzip master.zip
cd icomet-master/
make
./comet-server comet.conf

curl -v "http://127.0.0.1:8100/sub?cname=12"
# open another terminal
curl -v "http://127.0.0.1:8000/pub?cname=12&content=hi"
```

### JavaScript Library Usage

```javascript
var comet = new iComet({
    channel: 'abc',
    signUrl: 'http://127.0.0.1:8000/sign',
    subUrl: 'http://127.0.0.1:8100/sub',
    callback: function(content){
        // on server push
        alert(content);
    }
});
```

## Memory Usage

| Connections | VIRT | RES |
| ----------- | ---- | --- |
| 0 | 39m | 24m |
| 100,000 | 302m | 288m |
| 200,000 | 579m |565m |
| 500,000 | 1441m | 1427m |
| 1,000,000 | 2734m | 2720m |

2.7KB per connection.

## Run the chat demo

1. Compile and start icomet server
1. Drag and drop the file demos/web/chat.html into one web browser
1. Drag and drop the file demos/web/chat.html into another different web browser
1. Start chatting!


## Live demo

http://www.ideawu.com/icomet/chat.html


## iComet's role in a web system

![icomet's role](http://www.ideawu.com/icomet/icomet-role.png)

## Nginx + icomet

You can integrate icomet with nginx. If you are running you website on port ```80``` with domain ```www.test.com```. That is you visit your website home page with this url:

```
http://www.test.com/
```

Then you want to run icomet on the same server with port ```80```, for the concern of firewall issue. You can config nginx to pass request to icomet:

```
location ~ ^/icomet/.* {
	rewrite ^/icomet/(.*) /$1 break;

	proxy_read_timeout 60;
	proxy_connect_timeout 60;
	proxy_buffering off;
	proxy_pass   http://127.0.0.1:8100;
}   
```

Then, this url is used to subscribe to icomet channel xxx:

```
http://www.test.com/icomet/sub?cname=xxx
```

