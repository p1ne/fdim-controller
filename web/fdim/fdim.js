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
          configString.value = textDecoder.decode(data);
          parseConfigString(configString.value);
        }
        port.onReceiveError = error => {
          console.error(error);
        };
      }, error => {
        statusDisplay.textContent = error;
      });
    }

    function parseConfigString(configString) {
      let valuesArr = configString.split("|");

      configVersion.innerText = valuesArr[0];
      huType.value = valuesArr[1];
      unitsMetric.value = valuesArr[2];
      useRTC.value = valuesArr[3];
      rtcClock.value = valuesArr[4];
      timeZone.value = valuesArr[5];
      clockMode.value = valuesArr[6];
      displayPressure.value = valuesArr[7];
      pressureUnits.value = valuesArr[8];
      tpmsRequest.value = valuesArr[9];
    };

    function constructConfigString() {
      configString.value = configVersion.innerText + '|' +
      huType.value + '|' +
      unitsMetric.value + '|' +
      useRTC.value + '|' +
      rtcClock.value + '|' +
      timeZone.value + '|' +
      clockMode.value + '|' +
      displayPressure.value + '|' +
      pressureUnits.value + '|' +
      tpmsRequest.value + '|';
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
