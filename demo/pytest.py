#author 609419323@qq.com
import pycurl
       
class Test: 
    def body_callback(self, buf):
        if  buf!="" :
            print buf#此处为接受到数据后的逻辑处理 略。。 
            
if __name__ == "__main__":   
        url = 'http://127.0.0.1:8100/stream?cname=12'#stream不能变 cname可变
        t = Test()
        c = pycurl.Curl()
        c.setopt(c.URL, url) 
        c.setopt(c.WRITEFUNCTION, t.body_callback)
        c.perform()
        c.close() 
