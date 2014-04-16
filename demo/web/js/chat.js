// misc func
function htmlEntities(str) {
	return String(str).replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');
}
function sleep(time, el) {
	setTimeout(el, time);
}

var EmoticonAdv = {
	activeTextAreaId : "",
	showDropDown : function (btnid, txtid) {
		this.activeTextAreaId = txtid;
		var buttonPosition = $("#" + btnid).offset();
		$("#emoticonDropDown").css({
			left : buttonPosition.left - 3.5,
			top : buttonPosition.top - 1.5
		});

		$("#emoticonDropDown").fadeIn();
	},
	hideDropDown : function () {
		$("#emoticonDropDown").fadeOut();
	},
	insertSmiley : function (smiley) {
		$("#emoticonDropDown").fadeOut();
		var x = $("#" + this.activeTextAreaId);
		x.val(x.val() + " " + smiley + " ").focus();
	}
};

//bind mouse out event to automatically close the menu
$(function () {
	$("#emoticonDropDown").bind("mouseleave", function () {
		EmoticonAdv.hideDropDown();
	});
	$("#emoticonDropDown *").click(function (event) {
		event.stopPropagation(); //this is needed to prevent the reply area to collapse on click outside it
	});
});

var emoticons = [{
		pattern : ["-_-;"],
		css : "background-position:0 -420px; width:18px"
	}, {
		pattern : ["._.;"],
		css : "background-position:0 -380px; width:18px"
	}, {
		pattern : ["^_^;", "^^;"],
		css : "background-position:0 -120px; width:18px"
	}, {
		pattern : ["-_-"],
		css : "background-position:0 -80px"
	}, {
		pattern : ["&gt;_&lt;"],
		css : "background-position:0 -140px"
	}, {
		pattern : ["&lt;_&lt;"],
		css : "background-position:0 -200px"
	}, {
		pattern : ["&gt;_&gt;"],
		css : "background-position:0 -220px"
	}, {
		pattern : ["x_x"],
		css : "background-position:0 -240px"
	}, {
		pattern : ["\(o^_^o\)"],
		css : "background-position:0 -440px"
	}, {
		pattern : ["._."],
		css : "background-position:0 -500px"
	}, {
		pattern : ["T_T"],
		css : "background-position:0 -520px"
	}, {
		pattern : ["XD"],
		css : "background-position:0 -540px"
	}, {
		pattern : ["\('&lt;"],
		css : "background-position:0 -560px"
	}, {
		pattern : ["B\)"],
		css : "background-position:0 -580px"
	}, {
		pattern : ["XP"],
		css : "background-position:0 -600px"
	}, {
		pattern : [";_;", ":'\("],
		css : "background-position:0 -280px"
	}, {
		pattern : [":[", "=["],
		css : "background-position:0 -340px"
	}, {
		pattern : [":3", "=3"],
		css : "background-position:0 -360px"
	}, {
		pattern : [":S", "=S"],
		css : "background-position:0 -620px"
	}, {
		pattern : ["&gt;:\(", "&gt;:\["],
		css : "background-position:0 -480px"
	}, {
		pattern : ["&gt;:\)", "&gt;:D"],
		css : "background-position:0 -640px"
	}, {
		pattern : ["^_^", "^^"],
		css : "background-position:0 -40px"
	}, {
		pattern : [":\(", ":-\(", "=\("],
		css : "background-position:0 -60px"
	}, {
		pattern : [":\)", ":-\)", "=\)"],
		css : "background-position:0 0"
	}, {
		pattern : [":-D", ":D", "=D"],
		css : "background-position:0 -20px"
	}, {
		pattern : ["\(^_^\)/", "o/"],
		css : "background-position:0 -460px; width:19px"
	}, {
		pattern : ["&lt;\(^.^\)&gt;", "\(&gt;'.'\)&gt;", "\(&gt;^.^\)&gt;"],
		css : "background-position:0 -400px; width:19px"
	}, {
		pattern : [":O", "=O", ":o", "=o"],
		css : "background-position:0 -300px"
	}, {
		pattern : ["o_O", "O_o", "o_0", "0_o"],
		css : "background-position:0 -260px"
	}, {
		pattern : [":x", "=x", ":|", "=|", "'_'"],
		css : "background-position:0 -180px"
	}, {
		pattern : [":-/", "=/", ":\\", "=\\"],
		css : "background-position:0 -160px"
	}, {
		pattern : [";-\)", ";\)", "^_-", "~_-", "-_^", "-_~"],
		css : "background-position:0 -100px"
	}, {
		pattern : [":-P", ":P", ":p", ":-p", "=P", ";P", ";-P"],
		css : "background-position:0 -320px"
	}
];

function toemote(content, format) {
	for (var i = 0; i < emoticons.length; i++) {
		for (var p = 0; p < emoticons[i].pattern.length; p++) {
			content = content.replace(new RegExp(emoticons[i].pattern[p].replace(/[\-\[\]\/\{\}\(\)\*\+\?\.\\\^\$\|]/g, "\\$&"), "g"), format.replace("$$EMOTE$$", emoticons[i].css));
		}
	}

	return content
}

// detect tab changes
var isTabOpen = 0;
var timer;
$(window).on("blur focus", function (e) {
	var prevType = $(this).data("prevType");

	if (prevType != e.type) { //  reduce double fire issues
		switch (e.type) {
		case "blur":
			isTabOpen = 0;
			break;
		case "focus":
			isTabOpen = 1;
			break;
		}
	}

	$(this).data("prevType", e.type);
});

function flashTitle(initial, to) {
	if (document.title == initial)
		document.title = to;
	else
		document.title = initial;

	if (isTabOpen == 1) {
		document.title = initial;
		return 0;
	}
	sleep(800, function () {
		flashTitle(initial, to)
	});
}

function convertToLinks(text) {
	var replaceText,
	replacePattern1;

	//URLs starting with http://, https://
	replacePattern1 = /(\b(https?):\/\/[-A-Z0-9+&amp;@#\/%?=~_|!:,.;]*[-A-Z0-9+&amp;@#\/%=~_|])/ig;
	replacedText = text.replace(replacePattern1, '<a class="colored-link-1" title="$1" href="$1" target="_blank">$1</a>');

	//URLs starting with "www."
	replacePattern2 = /(^|[^\/])(www\.[\S]+(\b|$))/gim;
	replacedText = replacedText.replace(replacePattern2, '$1<a class="colored-link-1" href="http://$2" target="_blank">$2</a>');

	//returns the text result
	return replacedText;
}

function random_str(size) {
	size = size || 20;
	var time = (new Date().getTime() + '').substr(3);
	var channel = time + Math.random() + '';
	return channel.replace('.', '').substr(0, size);
}

var uid = 'u' + (Math.random() + '').replace('.', '').substr(1, 6);
var nickname = '';

var comet;
var app_host = '127.0.0.1:8080';
var admin_host = '127.0.0.1:8000';
var icomet_host = '127.0.0.1:8100';

// for testing only
var n = location.href.match(/http[s]?:\/\/([^\/]*)\//);
if (n && n.length == 2) {
	app_host = n[1];
	var ps = n[1].split(':');
	admin_host = ps[0] + ':8000';
	icomet_host = ps[0] + ':8100';
}

var testing = location.href.indexOf('http') != 0;
if (testing) {
	// This is for test purpose only
	var sign_url = 'http://' + admin_host + '/sign';
	var pub_url = 'http://' + admin_host + '/push?cb=?';
} else {
	// In real world business, signUrl/pubUrl should link to an application server
	// which will do neccessary authentication.
	var n = location.href.match(/http[s]?:\/\/[^\/]*\/(.*)\/.*/);
	var path = '';
	if(n && n.length == 2){
		path = '/' + n[1];
	}
	var sign_url = 'http://' + app_host + path + '/php/sign.php';
	var pub_url = 'http://' + app_host + path + '/php/push.php?cb=?';
}
var sub_url = 'http://' + icomet_host + '/sub';

var msgs = [];
function addmsg(euid, name, content, is) {
	is = is || false
		var l = 'm' + (Math.random() + '').replace('.', '').substr(1, 6);
	content = content.replace(/</g, '&lt;').replace(/>/g, '&gt;');
	content = content.replace(/\n/g, '<br/>');

	var html = '';
	content = toemote(content, "<span class=\"emoticon\" style=\"$$EMOTE$$\"></span>");
	content = convertToLinks(content);

	if (euid != uid || is) {
		if (euid == uid) {
			msgs[content] = l;
		}
		html += "<tr>";
		html += '<td id="' + l + '"' + (euid == uid ? ' class="po sent"' : '') + ' width="20px">&nbsp;&nbsp;&nbsp;&nbsp;</td>';
		switch ($.trim(content).split(" ")[0]) {
		case "/me":
			html += '<td class="chat_names">*</td>';
			html += '<td class="chat_content"><span class="chat_names">' + name + '</span> ' + content.substr(4, content.length) + '</td>';
			break;

		default:
			html += '<td class="chat_names">' + name + '</td>';
			html += '<td class="chat_content">' + content + '</td>';
			break;
		}
		html += "</tr>";

		flashTitle('iComet Example Chat', '* People said stuff *');
	} else {
		$("#" + msgs[content]).removeClass("sent");
	}

	$('#chat').append(html);
	$('#recv_chat_window').scrollTop($('#recv_chat_window')[0].scrollHeight);
}

function join() {
	nickname = $.trim($('*[name=nickname]').val());
	nickname = nickname.replace(/</g, '&lt;').replace(/>/g, '&gt;');
	if (nickname.length == 0) {
		alert('Please enter your nickname!');
		return false;
	}

	var channel = $('*[name=channel]').val();
	comet = new iComet({
		channel : channel,
		signUrl : sign_url,
		subUrl : sub_url,
		pubUrl : pub_url,
		callback : function (content) {
			var msg = JSON.parse(content);
			addmsg(msg.uid, msg.nickname, msg.content, false);
		}
	});
	$('#login_form').hide();
	$('#chat_window').show();
	$('#chat_window h3').html('Channel: ' + channel);
	$('#nickname').html(nickname);

	var url = location.href.replace(/channel=[^&]*/, '').replace(/\?$/, '');
	url += ((url.indexOf('?') == -1) ? '?' : '&') + 'channel=' + channel;
	var html = '';
	html += 'Tell your friend to join this chat with this link: '
	html += '<a target="_blank" href="' + url + '">' + url + '</a>';
	$('#share').html(html);
}

function send() {
	var t = $('#chat_window textarea[name=content]');
	var content = $.trim(t.val());
	content = htmlEntities(content);
	t.val('');
	if (content.length == 0) {
		$('#errors').html('content empty!');
		return false;
	}
	if (content.length > 1000) {
		$('#errors').html('content too long!');
		return false;
	}
	$('#errors').html('');

	addmsg(uid, nickname, content, true);
	
	var msg = {
		'uid' : uid,
		'nickname' : nickname,
		'content' : content
	};
	comet.pub(JSON.stringify(msg));
}

var w = window,
x = w.innerWidth,
y = w.innerHeight;
$(function () {
	$("#chat_window").width(x - 40);
	$("#chat_window textarea").width(x - 40 - 5);
	$(".chat_window").height(y - 190);

	var url = location.href;
	var channel = url.match(/channel=[^&]*/);
	if (channel != null) {
		var channel = channel[0].replace('channel=', '');
		channel = channel.replace(/</g, '&lt;').replace(/>/g, '&gt;');
	} else {
		var channel = random_str();
	}
	$('*[name=channel]').val(channel);
	$('*[name=nickname]').val(uid);

	$('#text').keypress(function (e) { // Attach the form handler to the keypress event
		if (e.keyCode == 13 && !e.shiftKey) { // If the the enter key was pressed without the shift key.
			$('#post').click(); // Trigger the button(elementId) click event.
			return e.preventDefault(); // Prevent the form submit.
		}
	});

	isTabOpen = 1;

	var content = "";
	var e = "";
	for (var i = 0; i < emoticons.length; i++) {
		e = (emoticons[i].pattern[0]).replace(/&lt;/g, "<").replace(/&gt;/g, '>');
		content += "<li><a href=\"javascript:EmoticonAdv.insertSmiley('" + e.replace("'", "\\'") + "');void(0)\" title=\"" + htmlEntities(emoticons[i].pattern[0]) + "\" style=\"" + emoticons[i].css + "\" class=\"emoticon\" alt=\"" + htmlEntities(emoticons[i].pattern[0]) + "\">\"" + htmlEntities(emoticons[i].pattern[0]) + "\"</a></li>";
	}
	$('#connies').html(content);
});

