var curler = require("./build/Release/curler.node").Curler;
var curl = new curler();

var options = {
  method: "GET",
  userAgent: 'Mozilla/5.0 (iPhone; CPU iPhone OS 5_0 like Mac OS X) AppleWebKit/534.46 (KHTML, like Gecko) Version/5.1 Mobile/9A334 Safari/7534.48.3',
  url: 'http://www.google.com'
};

var startDate = Date.now();
curl.request(options, function(err, res, bodyData) {
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