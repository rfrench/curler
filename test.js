var curler = require("./build/Release/curler.node").Curler;
var curlClient = new curler();

var options = {
	method: "GET",
	url: 'http://www.google.com'
};

var startDate = Date.now();
curlClient.request(options, function(err, res, bodyData) {
	var duration = (Date.now() - startDate);
	if (err) {
		console.log(err);
	}
	else {
		console.log('statusCode: %s', res.statusCode);
		console.log('bodyData: %s', bodyData);
	}
	console.log("curler (libcurl) performed http request in %s ms. dnsTime: %s, connectTime: %s, preTransferTime: %s, startTransferTime: %s, totalTime: %s", duration, res.dnsTime, res.connectTime, res.preTransferTime, res.startTransferTime, res.totalTime);
});