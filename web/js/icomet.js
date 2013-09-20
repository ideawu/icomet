function iComet(cid, callback){
	if(iComet.id__ == undefined){
		iComet.id__ = 0;
	}
	
	var self = this;
	self.id = iComet.id__++;
	self.cb = 'icomet_cb_' + self.id;
	self.last_sub_time = 0;
	self.timeout = (12 + 5) * 1000;
	self.num_connected = 0;
	self.cid = cid;
	self.url = '#';
	self.sub_cb = callback;
	self.timer = null;
	self.url = 'http://127.0.0.1:8100/sub?id=' + self.cid + '&cb=' + self.cb;
	self.stopped = true;

	var self_sub = function(){
		self.stopped = false;
		setTimeout(function(){
			self.num_connected ++;
			self.last_sub_time = (new Date()).getTime();

			$('script.' + self.cb).remove();
			var url = self.url + '&_=' + new Date().getTime();
			var script = '\<script class="' + self.cb + '\" src="' + url + '">\<\/script>';
			$('body').append(script);
		}, 0);
	}

	window[self.cb] = function(msg){
		if(self.stopped){
			return;
		}
		if(self.num_connected <= 0){
			return;
		}
		self.num_connected --;
		if(self.num_connected != 0){
			self.log('ignore exceeded connections');
			return;
		}
		if(msg){
			if(msg.type != 'data'){
				self.log(msg);
				self.stop();
			}
			if(msg.type == '404'){
				//
			}else if(msg.type == '429'){
				setTimeout(self._start_timeout_checker, self.timeout * 2);
			}
			if(msg.type != 'data'){
				return;
			}
		}

		self_sub();
		if(!msg){
			self.log('noop');
			return;
		}
		self.log(msg);

		// TODO:
		if(msg.seq != self.last_seq + 1){
			// return;
		}
		self.last_seq = msg.seq;
		if(self.sub_cb){
			self.sub_cb(msg);
		}
	}

	self.stop = function(){
		self.stopped = true;
		if(self.timer){
			clearTimeout(self.timer);
		}
	}
	
	self._start_timeout_checker = function(){
		self.timer = setInterval(function(){
			var now = (new Date()).getTime();
			if(now - self.last_sub_time > self.timeout){
				self.log('timeout');
				self.last_sub_time = now;
				self.num_connected = 0;
				self_sub();
			}
		}, 1000);
	}
	
	self.log = function(){
		var v = arguments;
		var p = 'icomet[' + self.id + ']';
		if(v.length == 1){
			console.log(p, v[0]);
		}else if(v.length == 2){
			console.log(p, v[0], v[1]);
		}else if(v.length == 3){
			console.log(p, v[0], v[1], v[2]);
		}else if(v.length == 4){
			console.log(p, v[0], v[1], v[2], v[3]);
		}else if(v.length == 5){
			console.log(p, v[0], v[1], v[2], v[3], v[4]);
		}else{
			console.log(p, v);
		}
	}
	
	self._start_timeout_checker();
	self_sub();
}
