# iComet APIs

## Public port

The public(or front) port is the port that clients can connect to and receive messages from the comet server. The default port number is ```8100```.

### Subscribe to a channel

__Request:__

__Long-polling__

	http://127.0.0.1:8100/poll?cname=$channel&seq=$next_seq&token=$token

__Forever iframe__

	http://127.0.0.1:8100/iframe?cname=$channel&seq=$next_seq&token=$token

__Streaming(HTTP endless chunk)__

	http://127.0.0.1:8100/stream?cname=$channel&seq=$last_seq&token=$token

__Parameters:__

* cname: The name of the channel this client will subscribe to.
* token: Token is returned by ```/sign```.
* seq: The sequence number of the next data message that will sent by iComet server. iComet client must remember every message's sequence number, plus it with 1, and used for next polling or reconnect streaming.

__Response:__

If the HTTP response tatus code is not ```200```, it is likely that the iComet server is down.

__Long-polling__

	icomet_cb({type: "data", cname: "a", seq: "1", content: "a"});

__Forever iframe__

	<script>parent.icomet_cb({type: "data", cname: "a", seq: "0", content: "a"});</script>

__Streaming(HTTP endless chunk)__

	{type: "data", cname: "a", seq: "1", content: "a"}

__Parameters:__

* type: The type of this message
	* data: A normal message.
	* noop: An empty message.
	* 429: Error message, too many channels/subscribers.
	* 401: Error message, token error.

## Administration port

The administration(or admin) port is the port that internal services will connect to, to create a channel, publish a message, close a channel, etc. Clients are forbidden to connect to this port. The default port number is ```8000```.

### ```/sign``` Create channel

__Request:__

	http://127.0.0.1:8000/sign?cname=$channel[&expires=60]

The created channel will be waiting for 60 seconds to expire before any subscriber arrives.

__Response:__

	{type: "sign", cname: "a", seq: 0, token: "36289dcb55bc35aa6893f7557b7fc28c", expires: 30, sub_timeout: 10}

### ```/pub``` Publish/Push message

__Request:__

	http://127.0.0.1:8000/pub?cname=$channel&content=$content

__Response:__

	{type: "ok"}

$content must be json encoded string, without the leading and trailing quote pair.
