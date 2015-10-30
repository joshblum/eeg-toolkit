"use strict";

/* initialize all canvases */
function start() {
    /* get WebGL context and load required extensions */
    for (var i in IDS) {
        var id = IDS[i];
        SPECTROGRAMS[id] = new Spectrogram(id);
    }
}

function updateSpectrogramStartTimes() {
    for (var i in IDS) {
        var id = IDS[i];
        SPECTROGRAMS[id].updateStartRequestTime();
    }
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
        reloadSpectrogram();
        if (endTime > 70) {
            clearInterval(id);
        }
    }
    id = setInterval(pan, 15 * 1000);
}

$(document).ready(function() {
    $("body").ready(start);
    $("#patientIdentifierByMRN").keyup(submitSpectrogram);
    $("#specStartTime").keyup(submitSpectrogram);
    $("#specEndTime").keyup(submitSpectrogram);
    $("#fftLen").change(reloadSpectrogram);
    $("#specMode").change(redrawSpectrograms);
    $("#specInterpolation").change(redrawSpectrograms);
    $("#specLogarithmic").change(redrawSpectrograms);
    $("#patientIdentifierMRNSubmit").click(reloadSpectrogram);
    // the "href" attribute of .modal-trigger must specify the modal ID that wants to be triggered
    $('.modal-trigger').leanModal();
});
