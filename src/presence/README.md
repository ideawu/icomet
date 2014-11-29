Connect to comet servers, subscribe presence activities to build presence table.

# How to use?

Write HTTP client to connect to the icomet-server, and subscribe to presence events like:

	curl "http://127.0.0.1:8000/psub"

The HTTP client will receive endless chunks like this:

	1 public
	0 public

* 0: offline event
* 1: online event
* public: channel name

Online events will be received periodically(since version 0.2.3, configured with ServerConfig::polling_timeout)

