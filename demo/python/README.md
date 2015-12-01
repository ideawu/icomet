# python icomet

Maitained by: [@yuexiajiayan](https://github.com/yuexiajiayan/)

1. 安装pycurl类库

2. 编写pytest.py

	```
	import pycurl

	class Test: 
		def body_callback(self, buf):
			if  buf!="" :
			print buf#此处为接受到数据后的逻辑处理 略。。 

	if __name__ == "__main__":   
		url = 'http://127.0.0.1:8100/stream?cname=12'  
		t = Test()
		c = pycurl.Curl()
		c.setopt(c.URL, url) 
		c.setopt(c.WRITEFUNCTION, t.body_callback)
		c.perform()
		c.close() 
	```

3. 打开浏览器请求 http://127.0.0.1:8000/push?cname=12&content=hi便会输出内容

