var ws = new WebSocket("ws://localhost:" + getURLParameters()['port'] + "/spectrogram");
ws.binaryType = 'arraybuffer';

OVERLAP = 0.5;

/* return an object containing the URL parameters */
function getURLParameters() {
    var params = {};
    if (location.search) {
        var parts = location.search.substring(1).split('&');
        for (var i = 0; i < parts.length; i++) {
            var pair = parts[i].split('=');
            if (!pair[0]) continue;
            params[pair[0]] = pair[1] || "";
        }
    }
    return params
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
        var headerData = new Uint8Array(message, 4, headerString.length)
        for (var i = 0; i < headerString.length; i++) {
            headerData[i] = headerString.charCodeAt(i);
        }

        // write the payload data
        payloadData = new Uint8Array(message, 4 + headerString.length, payload.byteLength);
        payloadData.set(new Uint8Array(payload));
        ws.send(message);
    }
}

/* Request the spectrogram for a file.

   Arguments:
   filename  the file name from which to load audio data.
   nfft      the FFT length used for calculating the spectrogram.
   overlap   the amount of overlap between consecutive spectra.
*/
function requestFileSpectrogram(filename, nfft, overlap, dataType) {
    sendMessage("request_file_spectrogram", {
        filename: filename,
        nfft: nfft,
        overlap: overlap,
        dataType: dataType
    });
}

/* Request the spectrogram for a file.

   Arguments:
   data      the content of a file from which to load audio data.
   nfft      the FFT length used for calculating the spectrogram.
   overlap   the amount of overlap between consecutive spectra.
*/
function requestDataSpectrogram(data, nfft, overlap, dataType) {
    sendMessage("request_data_spectrogram", {
        nfft: nfft,
        overlap: overlap,
        dataType: dataType
    }, data);
}

/* Parses a message

   Each message must contain the message type, the message content,
   and an optional binary payload. The decoded message will be
   forwarded to different functions based on the message type.

   Arguments:
   event     the message, either as string or ArrayBuffer.
*/
ws.onmessage = function(event) {
    if (event.data.constructor.name === "ArrayBuffer") {
        var headerLen = new Int32Array(event.data, 0, 1)[0];
        var header = String.fromCharCode.apply(null, new Uint8Array(event.data, 4, headerLen));
    } else {
        var header = event.data;
    }
    var msg;
    try {
        msg = JSON.parse(header);
    } catch (e) {
        console.error("Message", e.message, "is not a valid JSON object");
        return
    }

    var type = msg.type;
    var content = msg.content;
    var canvasId = content.canvas_id || IDS[0];
    var spectrogram = SPECTROGRAMS[canvasId];
    if (type === "spectrogram") {
        spectrogram.loadSpectrogram(new Float32Array(event.data, headerLen + 4), content.extent[0], content.extent[1], content.fs, content.length);
    } else if (type === "loading_progress") {
        spectrogram.updateProgressBar(content.progress);
    } else {
        console.log(type, content);
    }
}

/* log some info about the GL, then display spectrogram */
ws.onopen = function() {
  for (var i in SPECTROGRAMS) {
    var spectrogram = SPECTROGRAMS[i];
    spectrogram.logGLInfo();
  }
  reloadSpectrogram();
}

/* Test the keyup event for a submission and then reload the spectrogram.
 */
function submitSpectrogram(e) {
    e.which = e.which || e.keyCode;
    if (e.which == 13) {
        reloadSpectrogram();
    }
}

/* Loads the spectrogram for the currently seleced file/FFT-length.

   Reads the audioFile input field to get the current file and the
   select field to get the current FFT length.

   This only sends the request for a spectrogram. Delivering the
   spectrogram is up to the server.
*/
function reloadSpectrogram() {
    var audioFile = document.getElementById('audioFileByData').files[0];
    var fftLen = parseFloat(document.getElementById('fftLen').value);
    var dataType;
    // first we try to load a file
    if (audioFile) {
        var reader = new FileReader();
        dataType = getDataType(audioFile.name);
        reader.readAsArrayBuffer(audioFile);
        reader.onloadend = function() {
            requestDataSpectrogram(reader.result, fftLen, OVERLAP, dataType);
        }
    } else { // otherwise see if there is a filename
        audioFile = document.getElementById('audioFileByName').value;
        if (audioFile) {
            dataType = getDataType(audioFile);
            console.log("Requesting spectrogram for: " + audioFile);
            requestFileSpectrogram(audioFile, fftLen, OVERLAP, dataType);
        }
    }
    if (!audioFile) {
        console.log("Could not load spectrogram: No file selected");
        return;
    }
}

/*
 * Determines the type of input file the server has to handle based on the filename.
 *
 * Currently handles audio and eeg type files.
 */
function getDataType(inputFile) {
    var fileName = inputFile.split('.')
    var fileExt = fileName[fileName.length - 1];
    return fileExt === 'eeg' ? 'eeg' : 'audio';
}
