/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

var tcp = require('tcp');

var tcpServerHost = '';
var tcpServerPort = 0;

function onConnect() {
  tcpClient.send({
    message: 'hello, this is tcp client test',
    success: function() {
      console.log('tcp send success');
    },
    fail: function() {
      console.log('tcp send failed');
    }
  });
}
var tcpClient = tcp.createClient({
  host: tcpServerHost,
  port: tcpServerPort,
  success: function() {
    console.log('tcp client connect success');
  },
  fail: function() {
    console.log('tcp client connect failed');
  }
});

tcpClient.on('message', function(data) {
  console.log('tcp receive data: ' + data);
  tcpClient.close();
});

tcpClient.on('connect', function() {
  console.log('tcp client connected');
  onConnect();
});

tcpClient.on('close', function() {
  console.log('tcp client closed');
});

tcpClient.on('error', function(err) {
  console.log('tcp client error: ' + err);
});