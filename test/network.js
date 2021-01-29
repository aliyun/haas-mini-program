/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

var network = require('network');

var net = network.openNetWorkClient();

function netinfo_print(){
    var info = net.getInfo();
    /* imsi 国际移动用户识别码 */
    console.log('net imsi is: ' + info.imsi);

    /* imei 国际移动设备识别码 */
    console.log('net imei is: ' + info.imei);

    /* iccid 集成电路卡识别码 */
    console.log('net iccid is: ' + info.iccid);

    /* cid 基站编号 */
    console.log('net cid is: ' + info.cid);

    /* lac 位置区域码 */
    console.log('net lac is: ' + info.lac);

    /* mcc 移动国家代码（中国的为460 */
    console.log('net mcc is: ' + info.mcc);

    /* mnc 移动网络号码（中国移动为00，中国联通为01） */
    console.log('net mnc is: ' + info.mnc);

    /* rssi 接收的信号强度值 */
    console.log('net rssi is: ' + info.rssi);
}


/** 获取网络类型
* 目前支持两种类型：wifi cellular（蜂窝网）
*/
var type = net.getType();
console.log('net type is: ' + type);

/** 获取网络状态
* 目前支持两种状态：connect disconnect（蜂窝网）
*/
var status = net.getStatus();
console.log('net status is: ' + status);

if (status == 'connect') {
    netinfo_print();
} else {
    net.on('connect', function() {
        netinfo_print();
    })
}

net.on('disconnect', function(){
    console.log('device is disconnect');
});