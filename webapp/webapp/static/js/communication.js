"use strict";

var wsEndpoint = "/compute/spectrogram/";
var wsPort = 8080;
var ws = new WebSocket(getwsUrl(wsPort));
ws.binaryType = "arraybuffer";

var OVERLAP = 0.5;

function getwsUrl(wsPort) {
    var loc = window.location,
        newUri;
    if (loc.protocol === "https:") {
        newUri = "wss:";
    } else {
        newUri = "ws:";
    }
    newUri += "//" + loc.hostname + ":" + wsPort;
    newUri += wsEndpoint;
    return newUri;
}

/* return an object containing the URL parameters */
function getURLParameters() {
    var params = {};
    if (location.search) {
        var parts = location.search.substring(1).split("&");
        for (var i = 0; i < parts.length; i++) {
            var pair = parts[i].split("=");
            if (!pair[0]) {
                continue;
            }
            params[pair[0]] = pair[1] || "";
        }
    }
    return params;
}

/* Send a message.

   Arguments:
   type      the message type as string.
   content   the message content as json-serializable data.
*/
function sendMessage(type, content, visgoth_content) {
  ws.send(JSON.stringify({
      "type": type,
      "content": content,
      "visgoth_content": visgoth_content,
  }));
}

/*
   Arguments:
   mrn       the medical record number from which to load data.
   nfft      the FFT length used for calculating the spectrogram.
   startTime the start time for the calculation
   endTime   the end time for the calculation
   overlap   the amount of overlap between consecutive spectra.
   channel   the channel (LL, LP, ...) we are requesting
*/
function requestSpectrogram(mrn, nfft, startTime, endTime, overlap, channel) {
    var spectrogram = SPECTROGRAMS[IDS[channel]];
    spectrogram.updateStartRequestTime();
    spectrogram.updateProgressBar(0);
    // TODO (joshblum): need a new field for request action to allow updates for panning
    visgoth.sendProfiledMessage("spectrogram", {
        mrn: mrn,
        nfft: nfft,
        startTime: startTime,
        endTime: endTime,
        overlap: overlap,
        channel: channel,
    });
}

/* Parses a message

   Each message must contain the message type, the message content,
   and an optional binary payload. The decoded message will be
   forwarded to different functions based on the message type.

   Arguments:
   event     the message, either as string or ArrayBuffer.
*/
ws.onmessage = function(event) {
    var header, headerLen;
    if (event.data.constructor.name === "ArrayBuffer") {
        headerLen = new Int32Array(event.data, 0, 1)[0];
        header = String.fromCharCode.apply(null, new Uint8Array(event.data, 4, headerLen));
    } else {
        header = event.data;
    }
    var msg;
    try {
        msg = JSON.parse(header);
    } catch (e) {
        console.error("Message", e.message, "is not a valid JSON object");
        return;
    }

    var type = msg.type;
    var content = msg.content;
    var canvasId = content.canvasId || IDS[0];
    var spectrogram = SPECTROGRAMS[canvasId];
    if (type === "spectrogram") {
        getElementById("specStartTime").value = content.startTime.toFixed(3);
        getElementById("specEndTime").value = content.endTime.toFixed(3);

        spectrogram.updateStartLoadTime();
        spectrogram.render(new Float32Array(event.data, headerLen + 4),
            content.nblocks, content.nfreqs, content.fs,
            content.startTime, content.endTime);
        spectrogram.logElaspedTime();
        spectrogram.updateProgressBar(1);
    } else {
        console.log(type, content);
    }
};

/* log some info about the GL, then display spectrogram */
ws.onopen = function() {
    for (var i in SPECTROGRAMS) {
        var spectrogram = SPECTROGRAMS[i];
        spectrogram.logGLInfo();
    }
    reloadSpectrograms();
};

