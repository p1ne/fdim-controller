(function() {
  'use strict';

  document.addEventListener('DOMContentLoaded', event => {
    let connectButton = document.querySelector("#connect");
    let statusDisplay = document.querySelector('#status');
    let readConfigButton = document.querySelector('#readConfigButton')
    let writeConfigButton = document.querySelector('#writeConfigButton')
    let getCurrentTimeButton = document.querySelector('#getCurrentTimeButton')

    let configString = document.querySelector('#configString');

    let configVersion = document.querySelector("#configVersion");
    let huType = document.querySelector("#huType");
    let unitsMetric = document.querySelector("#unitsMetric");
    let useRTC = document.querySelector("#useRTC");
    let rtcClock = document.querySelector("#rtcClock");
    let timeZone = document.querySelector("#timeZone");
    let clockMode = document.querySelector("#clockMode");
    let displayPressure = document.querySelector("#displayPressure");
    let pressureUnits = document.querySelector("#pressureUnits");
    let tpmsRequest = document.querySelector("#tpmsRequest");

    let port;

    function connect() {
      port.connect().then(() => {
        statusDisplay.textContent = '';
        connectButton.textContent = 'Disconnect';

        port.onReceive = data => {
          let textDecoder = new TextDecoder();
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
      rtcClock.value = configString.charCodeAt(4) + ":" + configString.charCodeAt(5);
      timeZone.value = configString.charCodeAt(6);
      clockMode.value = configString.charCodeAt(7);
      displayPressure.value = configString.charCodeAt(8);
      pressureUnits.value = configString.charCodeAt(9);
      tpmsRequest.value = configString.charCodeAt(10);
    };

    function constructConfigString() {
      var rtcClockArr = rtcClock.value.split(":");

      configString.value = String.fromCharCode(configVersion.innerText) +
      String.fromCharCode(huType.value) +
      String.fromCharCode(unitsMetric.value) +
      String.fromCharCode(useRTC.value) +
      String.fromCharCode(rtcClockArr[0]) +
      String.fromCharCode(rtcClockArr[1]) +
      String.fromCharCode(timeZone.value) +
      String.fromCharCode(clockMode.value) +
      String.fromCharCode(displayPressure.value) +
      String.fromCharCode(pressureUnits.value) +
      String.fromCharCode(tpmsRequest.value);
    };

    function onGetCurrentTime() {
      var today = new Date();

      rtcClock.value = today.getHours() + ":" + today.getMinutes();
    }

    function onReadConfig() {
      if (!port) {
        return;
      }
      let toSend = "LOAD\n";
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

      constructConfigString();

      let toSend = "SAVE" + configString.value + "\n";
      var i;
      let view = new Uint8Array(toSend.length);
      for (i = 0; i < toSend.length; i++) {
        view[i] = parseInt(toSend.charCodeAt(i));
      }
      port.send(view);
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
