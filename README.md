icomet
======

A C1000K comet server built with libevent

## Supported OS and Browsers

|| Browser || OS ||
| Safari | iPhone, Mac |
| Firefox | Windows, Mac |
| Chrome | Windows, Mac |
| IE6 | Windows |


## Usage

```shell
make
./icomet

curl -v "http://127.0.0.1:8100/sub?cid=12"
# open another terminal
curl -v "http://127.0.0.1:8000/pub?cid=12&content=hi"
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
1. Drag and drop the file web/chat.html into one web browser
1. Drag and drop the file web/chat.html into another different web browser
1. Start chatting!


## Live demo

http://www.ideawu.com/icomet/chat.html


