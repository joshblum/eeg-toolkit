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
   payload   binary data as ArrayBuffer.
*/
function sendMessage(type, content, payload) {
    if (payload === undefined) {
        ws.send(JSON.stringify({
            type: type,
            content: content
        }));
    } else {
        var headerString = JSON.stringify({
            type: type,
            content: content
        });
        // append enough spaces so that the payload starts at an 8-byte
        // aligned position. The first four bytes will be the length of
        // the header, encoded as a 32 bit signed integer:
        var alignmentBytes = 8 - ((headerString.length + 4) % 8);
        for (var i = 0; i < alignmentBytes; i++) {
            headerString += " ";
        }

        var message = new ArrayBuffer(4 + headerString.length + payload.byteLength);

        // write the length of the header as a binary 32 bit signed integer:
        var prefixData = new Int32Array(message, 0, 4);
        prefixData[0] = headerString.length;

        // write the header data
        var headerData = new Uint8Array(message, 4, headerString.length);
        for (i = 0; i < headerString.length; i++) {
            headerData[i] = headerString.charCodeAt(i);
        }

        // write the payload data
        var payloadData = new Uint8Array(message, 4 + headerString.length, payload.byteLength);
        payloadData.set(new Uint8Array(payload));
        ws.send(message);
    }
}

/* Request the spectrogram for a file.

   Arguments:
   mrn       the medical record number from which to load data.
   nfft      the FFT length used for calculating the spectrogram.
   startTime the start time for the calculation
   endTime   the end time for the calculation
   overlap   the amount of overlap between consecutive spectra.
   channel   the channel (LP, LP, ...) we are requesting
*/
function requestFileSpectrogram(mrn, nfft, startTime, endTime, overlap, channel) {
    updateSpectrogramStartTimes();
    // TODO (joshblum): need a new field for request action to allow updates for panning
    sendMessage("request_file_spectrogram", {
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
        // spectrogram messages can either be a `new` message or an `update` action.
        var action = content.action;
        if (action === "new") {
            // First let"s setup the canvas
            getElementById("specStartTime").value = content.startTime.toFixed(3);
            getElementById("specEndTime").value = content.endTime.toFixed(3);
            spectrogram.newSpectrogram(content.nblocks,
                content.nfreqs, content.fs,
                content.startTime, content.endTime);
        } else if (action === "update") {
            // Now lets update the next frame in the spectrogram
            // TODO (joshblum) Instead of allocating new arrays here
            // it might be worth it to have some of the things preallocated and cached.
            // 1. Figure out if it is a bottleneck. When things are chunked it should be easier to reuse
            spectrogram.updateStartLoadTime();
            spectrogram.updateSpectrogram(new Float32Array(event.data, headerLen + 4),
                content.nblocks, content.nfreqs);
            spectrogram.logElaspedTime();
        } else {
            console.log(action);
        }
    } else if (type === "loading_progress") {
        spectrogram.updateProgressBar(content.progress);
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
    reloadSpectrogram();
};

/* Test the keyup event for a submission and then reload the spectrogram.
 */
function submitSpectrogram(e) {
    var which = e.which || e.keyCode;
    if (which === 13) {
        reloadSpectrogram();
    }
}

/* Loads the spectrogram for the currently seleced file/FFT-length.

   Reads the input field to get the current file and the
   select field to get the current FFT length.

   This only sends the request for a spectrogram. Delivering the
   spectrogram is up to the server.
*/
function reloadSpectrogram() {
    var patientIdentifier = getElementById("patientIdentifierByMRN").value;
    var fftLen = parseFloat(getElementById("fftLen").value);
    var startTime = parseFloat(getElementById("specStartTime").value);
    var endTime = parseFloat(getElementById("specEndTime").value);
    // first we try to load a file
    if (patientIdentifier) {
        console.log("Requesting spectrogram for: " + patientIdentifier);
        for (var ch = 0; ch < IDS.length; ch++) {
          requestFileSpectrogram(patientIdentifier, fftLen, startTime, endTime,
            OVERLAP, ch);
        }
    }
    if (!patientIdentifier) {
        console.log("Could not load spectrogram: No file selected");
        return;
    }
}
