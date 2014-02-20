Welcome to the icomet wiki!

***

# APIs

## ```/sub, /poll``` Subscribe to a channel

### With long-polling

```
http://127.0.0.1:8100/sub?cname=$channel_name&seq=$last_seq&cb=$jsonp_callback&token=$token
or
http://127.0.0.1:8100/poll?cname=$channel_name&seq=$last_seq&cb=$jsonp_callback&token=$token
```

### With streaming(endless HTTP chunk)

```
http://127.0.0.1:8100/stream?cname=$channel_name&seq=$last_seq&token=$token
```

The $token is returned by ```/sign```.

***

## ```/sign``` Create channel

__Request:__

```
http://127.0.0.1:8000/sign?cname=$channel_name[&expires=60]
```

The created channel will be waiting for 60 seconds to expire before any subscriber arrives.

__Response:__

```
{type: "sign", cname: "a", seq: 0, token: "36289dcb55bc35aa6893f7557b7fc28c", expires: 30, sub_timeout: 10}
```

## ```/pub``` Publish/Push message

```
http://127.0.0.1:8000/pub?cname=$channel_name&content=$content
```

$content must be json encoded string, without the leading and trailing quote pair.
