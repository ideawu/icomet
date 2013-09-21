function iComet(cid, callback){
	if(iComet.id__ == undefined){
		iComet.id__ = 0;
	}
	
	var self = this;
	self.id = iComet.id__++;
	self.cb = 'icomet_cb_' + self.id;
	self.sub_timeout = 35 * 1000;
	self.cid = cid;
	self.timer = null;
	self.ping_timer = null;
	self.stopped = true;
	self.last_sub_time = 0;
	self.need_fast_reconnect = true;

	self.data_seq = 0;
	self.noop_seq = 0;
	self.sub_cb = callback;
	self.ping_cb = null;
	self.sub_url = 'http://127.0.0.1:8100/sub?id=' + self.cid + '&cb=' + self.cb;
	self.ping_url = 'http://127.0.0.1:8100/ping?cb=' + self.cb;

	window[self.cb] = function(msg, in_batch){
		// batch repsonse
		if(msg instanceof Array){
			self.log('batch response', msg.length);
			for(var i in msg){
				if(msg[i] && msg[i].type == 'data'){
					if(i == msg.length - 1){
						window[self.cb](msg[i]);
					}else{
						window[self.cb](msg[i], true);
					}
				}
			}
			return;
		}
		//self.log('resp', msg);
		if(self.stopped){
			return;
		}
		if(!msg){
			return;
		}
		if(msg.type == '404'){
			// TODO
			return;
		}
		if(msg.type == '429'){
			self.stop();
			self.start();
			return;
		}
		if(msg.type == 'ping'){
			self.log('proc', msg);
			try{
				var a = parseInt(msg.sub_timeout);
				self.sub_timeout = (a + 5) * 1000;
			}catch(e){}
			if(self.ping_cb){
				self.ping_cb(msg);
			}
			return;
		}
		if(msg.type == 'noop'){
			if(msg.seq == self.noop_seq){
				self.log('proc', msg);
				if(self.noop_seq == 2147483647){
					self.noop_seq = -2147483648;
				}else{
					self.noop_seq ++;
				}
				// if the channel is empty, it is probably empty next time,
				// so pause some seconds before sub again
				setTimeout(self_sub, 2000 + Math.random() * 3000);
			}else{
				self.log('ignore exceeded connections');
			}
			return;
		}
		if(msg.type == 'data'){
			if(msg.seq != self.data_seq){
				if(msg.seq < self.data_seq){
					self.log('drop', msg);
				}else{
					self.log('msg lost', msg);
					// TODO: lost_cb(msg);
					if(self.sub_cb){
						self.sub_cb(msg);
					}
				}
				
				self.data_seq = msg.seq;
				if(self.data_seq == 2147483647){
					self.data_seq = -2147483648;
				}else{
					self.data_seq ++;
				}
				if(!in_batch){
					// fast reconnect
					var now = new Date().getTime();
					if(self.need_fast_reconnect || now - self.last_sub_time > 3 * 1000){
						self.log('fast reconnect');
						self.need_fast_reconnect = false;
						self_sub();
					}
				}
			}else{
				self.log('proc', msg);
				if(self.data_seq == 2147483647){
					self.data_seq = -2147483648;
				}else{
					self.data_seq ++;
				}
				if(self.sub_cb){
					self.sub_cb(msg);
				}
				if(!in_batch){
					self_sub();
				}
			}
			return;
		}
	}
	
	self.ping = function(callback){
		self.log('ping icomet server...');
		self.ping_cb = callback;
		var url = self.ping_url + '&_=' + new Date().getTime();
		var script = '\<script class="' + self.cb + '\" src="' + url + '">\<\/script>';
		$('body').append(script);
	}

	var self_sub = function(){
		//self.log('sub');
		self.stopped = false;
		self.last_sub_time = (new Date()).getTime();
		$('script.' + self.cb).remove();
		var url = self.sub_url
			 + '&seq=' + self.data_seq
			 + '&noop=' + self.noop_seq
			 + '&_=' + new Date().getTime();
		var script = '\<script class="' + self.cb + '\" src="' + url + '">\<\/script>';
		setTimeout(function(){
			$('body').append(script);
		}, 0);
	}
	
	self.start = function(){
		self.stopped = false;
		self.ping(function(){
			if(self.ping_timer){
				clearTimeout(self.ping_timer);
				self.ping_timer = null;
			}else{
				return;
			}
			if(!self.stopped){
				self.log('start sub, timeout=' + self.sub_timeout + 'ms');
				self._start_timeout_checker();
				self_sub();
			}
		});
		if(!self.ping_timer){
			self.ping_timer = setInterval(self.start, 3000 + Math.random() * 2000);
		}
		if(self.timer){
			clearTimeout(self.timer);
			self.timer = null;
		}
	}

	self.stop = function(){
		self.last_sub_time = 0;
		self.need_fast_reconnect = true;
		self.stopped = true;
		if(self.timer){
			clearTimeout(self.timer);
			self.timer = null;
		}
		if(self.ping_timer){
			clearTimeout(self.ping_timer);
			self.ping_timer = null;
		}
	}
	
	self._start_timeout_checker = function(){
		if(self.timer){
			clearTimeout(self.timer);
		}
		self.timer = setInterval(function(){
			var now = (new Date()).getTime();
			if(now - self.last_sub_time > self.sub_timeout){
				self.log('timeout');
				self.stop();
				self.start();
			}
		}, 1000);
	}
	
	self.log = function(){
		var v = arguments;
		var p = 'icomet[' + self.id + ']';
		var t = new Date().toTimeString().substr(0, 8);
		if(v.length == 1){
			console.log(t, p, v[0]);
		}else if(v.length == 2){
			console.log(t, p, v[0], v[1]);
		}else if(v.length == 3){
			console.log(t, p, v[0], v[1], v[2]);
		}else if(v.length == 4){
			console.log(t, p, v[0], v[1], v[2], v[3]);
		}else if(v.length == 5){
			console.log(t, p, v[0], v[1], v[2], v[3], v[4]);
		}else{
			console.log(t, p, v);
		}
	}

	self.start();

}
