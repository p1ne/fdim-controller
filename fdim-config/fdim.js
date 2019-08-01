(function() {
  'use strict';

  document.addEventListener('DOMContentLoaded', event => {
    let connectButton = document.querySelector("#connect");
    let statusDisplay = document.querySelector('#status');
    let readConfigButton = document.querySelector('#readConfigButton')
    let writeConfigButton = document.querySelector('#writeConfigButton')
    let getCurrentTimeButton = document.querySelector('#getCurrentTimeButton')

    let printConfigString = document.querySelector('#printConfigString');

    let configVersion = document.querySelector("#configVersion");
    let huType = document.querySelector("#huType");
    let unitsMetric = document.querySelector("#unitsMetric");
    let useRTC = document.querySelector("#useRTC");
    let rtcClock = document.querySelector("#rtcClock");
    let timeZone = document.querySelector("#timeZone");
    let clockMode = document.querySelector("#clockMode");
    let tpmsDisplay = document.querySelector("#tpmsDisplay");
    let tpmsMode = document.querySelector("#tpmsMode");

    let port;

    let rawConfig;

    function hexEncode(str) {
      var hex, i;

      var result = "";
      for (i=0; i<str.length; i++) {
        hex = str.charCodeAt(i).toString(16);
        result += ("00" + hex + " ").slice(-3);
      }

      return result
    };

    function connect() {
      port.connect().then(() => {
        statusDisplay.textContent = '';
        connectButton.textContent = 'Disconnect';

        port.onReceive = data => {
          let textDecoder = new TextDecoder();
          rawConfig = textDecoder.decode(data);
          printConfigString.innerText = hexEncode(rawConfig);
          parseConfigString(textDecoder.decode(data));
        }
        port.onReceiveError = error => {
          console.error(error);
        };
      }, error => {
        statusDisplay.textContent = error;
      });
    }

    function parseConfigString(configString) {
      configVersion.innerText = configString.charCodeAt(0);
      huType.value = configString.charCodeAt(1);
      unitsMetric.value = configString.charCodeAt(2);
      useRTC.value = configString.charCodeAt(3);
      timeZone.value = parseInt(configString.charCodeAt(4), 10);
      timeZone.value = parseInt(timeZone.value, 10) * ((parseInt(configString.charCodeAt(5), 10) > 0) ? 1 : -1);
      clockMode.value = configString.charCodeAt(6);
      tpmsDisplay.value = configString.charCodeAt(7);
      tpmsMode.value = configString.charCodeAt(8);
      if (configString.charCodeAt(3)) {
        rtcClock.value = configString.charCodeAt(9) + ":" + configString.charCodeAt(10);
      } else {
        rtcClock.value = "";
      }
    };

    function onReadConfig() {
      if (!port) {
        return;
      }
      let toSend = "L\n";
      var i;
      let view = new Uint8Array(toSend.length);
      for (i = 0; i < toSend.length; i++) {
        view[i] = parseInt(toSend.charCodeAt(i));
      }
      port.send(view);
    };

    function onWriteConfig() {
      if (!port) {
        return;
      }
      
      var rtcClockArr = [];
      if (rtcClock.value != "") {
        rtcClockArr = rtcClock.value.split(":");
      } else {
        rtcClockArr[0] = 0;
        rtcClockArr[1] = 0;
      }
      let configVersionValue = (parseInt(configVersion.innerText, 10) > 7) ? parseInt(configVersion.innerText, 10) : 7;

      let view = new Uint8Array(13);

      let i = 0;

      view[i++] = parseInt("S".charCodeAt("0"), 10);
      view[i++] = configVersionValue
      view[i++] = parseInt(huType.value, 10);
      view[i++] = parseInt(unitsMetric.value, 10);
      view[i++] = parseInt(useRTC.value, 10);
      view[i++] = parseInt(Math.abs(timeZone.value, 10));
      view[i++] = parseInt(timeZone.value, 10) > 0 ? 1 : 0;
      view[i++] = parseInt(clockMode.value, 10);
      view[i++] = parseInt(tpmsDisplay.value, 10);
      view[i++] = parseInt(tpmsMode.value, 10);
      view[i++] = parseInt(rtcClockArr[0], 10);
      view[i++] = parseInt(rtcClockArr[1], 10);
      view[i++] = parseInt("\n".charCodeAt(0), 10);

      //printConfigString.innerText = hexEncode(currentRawConfig);

      //var i;
      //let view = new Uint8Array(13);
      //for (i = 0; i < toSend.length; i++) {
      //  view[i] = parseInt(toSend.charCodeAt(i), 10);
      //}

      //port.send(view);

      var j;
      let send = new Uint8Array(1);
      for (j = 0; j < i; j++) {
        send[0] = view[j];
        port.send(send);

      }
    };

    function onGetCurrentTime(control) {
      var today = new Date();
      var time = ("0" + today.getHours()).slice(-2) + ":" + ("0" + today.getMinutes()).slice(-2);
      rtcClock.value = time;
    };

    readConfigButton.addEventListener('click', onReadConfig);
    writeConfigButton.addEventListener('click', onWriteConfig);
    getCurrentTimeButton.addEventListener('click', onGetCurrentTime);


    connectButton.addEventListener('click', function() {
      if (port) {
        port.disconnect();
        connectButton.textContent = 'Connect';
        statusDisplay.textContent = '';
        port = null;
      } else {
        serial.requestPort().then(selectedPort => {
          port = selectedPort;
          connect();
        }).catch(error => {
          statusDisplay.textContent = error;
        });
      }
    });

    serial.getPorts().then(ports => {
      if (ports.length == 0) {
        statusDisplay.textContent = 'No device found.';
      } else {
        statusDisplay.textContent = 'Connecting...';
        port = ports[0];
        connect();
      }
    });
  });
})();
