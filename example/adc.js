/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

var adc = require('adc');

// voltage
var vol = adc.open({
  id: 'ADC0'
});

var value;
// read voltage
setInterval(function() {
  // read adc value
  value = vol.readValue();

  console.log('adc value is ' + value);
}, 1000);

// vol.close();