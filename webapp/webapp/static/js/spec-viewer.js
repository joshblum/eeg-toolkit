"use strict"

/* initialize all canvases */
function initSpectrograms() {
    /* get WebGL context and load required extensions */
    for (var i in IDS) {
        var id = IDS[i];
        SPECTROGRAMS[id] = new Spectrogram(id);
    }
}

function reloadSpectrograms() {
    var patientIdentifier = getElementById("patientIdentifierByMRN").value;
    var fftLen = parseFloat(getElementById("fftLen").value);
    var startTime = parseFloat(getElementById("specStartTime").value);
    var endTime = parseFloat(getElementById("specEndTime").value);
    // first we try to load a file
    if (patientIdentifier) {
        console.log("Requesting spectrogram for: " + patientIdentifier);
        for (var ch = 0; ch < IDS.length; ch++) {
          requestSpectrogram(patientIdentifier, fftLen, startTime, endTime,
            OVERLAP, ch);
        }
    }
    if (!patientIdentifier) {
        console.log("Could not load spectrogram: No file selected");
        return;
    }
}

/*
 *  Arguments:
 *  mrn       the medical record number from which to load data.
 *  nfft      the FFT length used for calculating the spectrogram.
 *  startTime the start time for the calculation
 *  endTime   the end time for the calculation
 *  overlap   the amount of overlap between consecutive spectra.
 *  channel   the channel (LL, LP, ...) we are requesting
*/
function requestSpectrogram(mrn, nfft, startTime, endTime, overlap, channel) {
    var spectrogram = SPECTROGRAMS[IDS[channel]];
    spectrogram.updateStartRequestTime();
    spectrogram.updateProgressBar(0);
    visgoth.sendProfiledMessage("spectrogram", {
        mrn: mrn,
        nfft: nfft,
        startTime: startTime,
        endTime: endTime,
        overlap: overlap,
        channel: channel,
        maxWidth: spectrogram.specView.width,
        maxHeight: spectrogram.specView.height,
    });
}

/*
 * Set the time of the given `timeId` as a formatted float.
 */
function setTime(timeId, timeValue) {
    getElementById(timeId).value = parseFloat(timeValue.toFixed(3));
}

/*
 * Increase the time interval in viewing range
 */
function increaseInterval() {
    var interval = parseFloat(getElementById("specInterval").value);
    var specStartTime = parseFloat(getElementById("specStartTime").value);
    var specEndTime = parseFloat(getElementById("specEndTime").value);
    if (specEndTime - specStartTime === interval) {
      setTime("specStartTime", specStartTime + interval);
      setTime("specEndTime", specEndTime + interval);
    } else {
      setTime("specEndTime", specStartTime + interval);
    }
    reloadSpectrograms();
}

/*
 * Decrease the time interval in viewing range
 */
function decreaseInterval() {
    var interval = parseFloat(getElementById("specInterval").value);
    var specStartTime = parseFloat(getElementById("specStartTime").value);
    var specEndTime = parseFloat(getElementById("specEndTime").value);
    if (specEndTime - specStartTime === interval) {
      setTime("specStartTime", Math.max(0, specStartTime - interval));
      setTime("specEndTime", Math.max(interval, specEndTime - interval));
    } else {
      setTime("specStartTime", Math.max(0, specStartTime - interval));
    }
    reloadSpectrograms();
}

function testPanning() {
    var mrn = "005";
    var startTime = 0;
    var endTime = 1;
    var interval = 0.5;
    getElementById("patientIdentifierByName").value = mrn;
    var id;

    function pan() {
        getElementById("specStartTime").value = startTime;
        getElementById("specEndTime").value = endTime;
        console.log("startTime: " + startTime + " endTime: " + endTime);
        startTime += interval;
        endTime += interval;
        reloadSpectrograms();
        if (endTime > 70) {
            clearInterval(id);
        }
    }
    id = setInterval(pan, 15 * 1000);
}

/*
 * Test the keyup event for a submission and then reload the spectrogram.
 */
function submitSpectrogram(e) {
    var which = e.which || e.keyCode;
    if (which === 13) {
        reloadSpectrograms();
    }
}

$(document).ready(function() {
    $("body").ready(initSpectrograms);
    $("#patientIdentifierByMRN").keyup(submitSpectrogram);
    $("#specStartTime").keyup(submitSpectrogram);
    $("#specEndTime").keyup(submitSpectrogram);
    $("#fftLen").change(reloadSpectrograms);
    $("#specMode").change(redrawSpectrograms);
    $("#specInterpolation").change(redrawSpectrograms);
    $("#specLogarithmic").change(redrawSpectrograms);
    $("#patientIdentifierMRNSubmit").click(reloadSpectrograms);
    $("#increaseInterval").click(increaseInterval);
    $("#decreaseInterval").click(decreaseInterval);
    // the "href" attribute of .modal-trigger must specify the modal ID that wants to be triggered
    $(".modal-trigger").leanModal();
    $("#specInterval").keyup(function(e) {
      var which = e.which || e.keyCode;
      if (which === 13) {
          $("#settingsModal").closeModal();
      }
    });
});
