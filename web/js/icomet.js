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

	self.aa = null;
	self.sub = function(){
		self.url = 'http://127.0.0.1:8100/sub?id=' + self.cid + '&cb=' + self.cb;
		setTimeout(function(){
			self.stop();
			self.num_connected ++;

			var script = '\<script src="' + self.url + '">\<\/script>';
			$('body').append(script);
	
			self.last_sub_time = (new Date()).getTime();
			window[self.cb] = function(msg){
				if(self.num_connected <= 0){
					return;
				}
				self.num_connected --;
				if(self.num_connected != 0){
					self.log('ignore exceeded connections');
					return;
				}

				self.sub();
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
		}, 0);
	}
	
	self.stop = function(){
		$('script').each(function(i, e){
			if($(e).attr('src') && $(e).attr('src').indexOf(self.url) == 0){
				//console.log('remove', e.src);
				$(e).remove();
			}
		});
	}
	
	self.timer = setInterval(function(){
		var now = (new Date()).getTime();
		if(now - self.last_sub_time > self.timeout){
			self.log('timeout');
			self.last_sub_time = now;
			self.num_connected = 0;
			self.sub();
		}
	}, 1000);
	
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
}
