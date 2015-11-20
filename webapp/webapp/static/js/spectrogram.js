"use strict";

var IDS = ["LL", "LP", "RP", "RL"];

// global data structure which holds spectrogram objects.
// When a request for update comes in the sender must specify an id to update
var SPECTROGRAMS = {};

// global visgoth object
var visgoth = new Visgoth();

/*
 * Helper function to wrap `document.getElementById`
 */
function getElementById(type, id) {
    id = id ? "-" + id : "";
    return document.getElementById(type + id);
}

/*
 * Redraw all spectrograms on the page.
 * Used when options such as the mode or scale are chagned
 */
function redrawSpectrograms() {
    for (var key in SPECTROGRAMS) {
        if (SPECTROGRAMS.hasOwnProperty(key)) {
            SPECTROGRAMS[key].drawScene();
        }
    }
}

/*
 * A Spectogram object is used to hold various attributes
 * of the spectrogram in a single object. This includes
 * the webGL context and the different canvases associated
 * with a view.
 */
function Spectrogram(id) {
    this.id = id;
    this.specContainer = getElementById("specContainer", id);
    this.specView = getElementById("spectrogram", id);
    this.specTimeScale = getElementById("specTimeScale", id);
    this.specFrequencyScale = getElementById("specFreqScale", id);
    this.specDataView = getElementById("specDataView", id);
    this.progressBar = getElementById("progressBar", id);
    this.annotation = new Annotation(this.specContainer);

    this.gl = null; // the WebGL instance

    // shader attributes:
    this.vertexPositionAttribute = null;
    this.textureCoordAttribute = null;

    // shader uniforms:
    this.samplerUniform = null;
    this.ampRangeUniform = null;
    this.zoomUniform = null;
    this.specSizeUniform = null;
    this.specDataSizeUniform = null;
    this.specModeUniform = null;
    this.specLogarithmicUniform = null;

    // for performance stats
    this.bufferLoadTimeStat = new VisgothStat(VisgothLabels.bufferLoadTime, this.id);
    this.networkLatencyStat = new VisgothStat(VisgothLabels.networkLatency, this.id);
    this.networkBufferSizeStat = new VisgothStat(VisgothLabels.networkBufferSize, this.id);
    this.fpsStat = new VisgothStat(VisgothLabels.fps, this.id);
    visgoth.registerStat(this.bufferLoadTimeStat);
    visgoth.registerStat(this.networkLatencyStat);
    visgoth.registerStat(this.networkBufferSizeStat);
    visgoth.registerStat(this.fpsStat);

    // vertex buffer objects
    this.vertexPositionBuffers = null;
    this.textureCoordBuffer = null;

    // textures objects
    this.spectrogramTextures = null;

    // Spectrogram meta data
    this.specSize = 0; // total size of the spectrogram
    this.specViewSize = 0; // visible size of the spectrogram
    this.init();
}

Spectrogram.prototype.init = function() {
    try {
        this.gl = this.specView.getContext("webgl");
    } catch (e) {
        console.log("Could not initialize WebGL");
        console.log(e);
        this.gl = null;
        return;
    }

    // needed for floating point textures
    this.gl.getExtension("OES_texture_float");
    var error = this.gl.getError();
    if (error !== this.gl.NO_ERROR) {
        console.log("Could not enable float texture extension");
        return;
    }

    // needed for linear filtering of floating point textures
    this.gl.getExtension("OES_texture_float_linear");
    if (error !== this.gl.NO_ERROR) {
        console.log("Could not enable float texture linear extension");
        return;
    }

    // 2D-drawing only
    this.gl.disable(this.gl.DEPTH_TEST);

    // get shaders ready
    this.loadSpectrogramShaders();

    // load dummy data
    this.render(new Float32Array(), 0, 0, 200, 0, 1);

    this.addListeners();
};

Spectrogram.prototype.addListeners = function() {
    window.addEventListener("resize", this.updateCanvasResolutions, false);
    this.updateCanvasResolutions();
    var self = this;
    this.specContainer.onwheel = function(wheel) {
        self.onwheel(wheel);
    };
    this.specContainer.onmousemove = function(mouse) {
        self.onmousemove(mouse);
    };
};

/* set resolution of all canvases to native resolution */
Spectrogram.prototype.updateCanvasResolutions = function() {
    this.specView.width = this.specView.clientWidth;
    this.specView.height = this.specView.clientHeight;
    this.gl.viewport(0, 0, this.specView.width, this.specView.height);
    this.specTimeScale.width = this.specTimeScale.clientWidth;
    this.specTimeScale.height = this.specTimeScale.clientHeight;
    this.specFrequencyScale.width = this.specFrequencyScale.clientWidth;
    this.specFrequencyScale.height = this.specFrequencyScale.clientHeight;
    var self = this;
    window.requestAnimationFrame(function() {
        self.drawScene();
    });
};

/* log version and memory information about WebGL */
Spectrogram.prototype.logGLInfo = function() {
    if (this.gl === null) {
        sendMessage("information", "could not initialize webGl context");
    } else {
        sendMessage("information",
            "version: " + this.gl.getParameter(this.gl.VERSION) + "\n" +
            "shading language version: " + this.gl.getParameter(this.gl.SHADING_LANGUAGE_VERSION) + "\n" +
            "vendor: " + this.gl.getParameter(this.gl.VENDOR) + "\n" +
            "renderer: " + this.gl.getParameter(this.gl.RENDERER) + "\n" +
            "max texture size: " + this.gl.getParameter(this.gl.MAX_TEXTURE_SIZE) + "\n" +
            "max combined texture image units: " + this.gl.getParameter(this.gl.MAX_COMBINED_TEXTURE_IMAGE_UNITS));
    }
};

/* Update the start time when asking for a new spectrogram */
Spectrogram.prototype.updateStartRequestTime = function() {
    console.log("updating startRequestTime");
    this.networkLatencyStat.markStart();
};

Spectrogram.prototype.updateStartLoadTime = function() {
    console.log("updating startLoadTime");
    this.bufferLoadTimeStat.markStart();
};

/* Log the elapsed time to generate a spectrogram */
Spectrogram.prototype.logElaspedTime = function() {
    this.networkLatencyStat.measurePerformance();
    this.bufferLoadTimeStat.measurePerformance();
};

/*
 * Sets the progress bar
 * If progress is 0 or 1, the progress bar will be turned invisible.
 */
Spectrogram.prototype.updateProgressBar = function(progress) {
    if (progress === 0 || progress === 1) {
        this.progressBar.hidden = true;
    } else {
        this.progressBar.hidden = false;
        this.progressBar.value = progress;
    }
};

/* link shaders and save uniforms and attributes

   saves the following attributes to global scope:
   - vertexPositionAttribute: aVertexPosition
   - textureCoordAttribute: aTextureCoord

   saves the following uniforms to global scope:
   - samplerUniform: uSampler
   - zoomUniform: mZoom
   - ampRangeUniform: vAmpRange
   - specSizeUniform: vSpecSize
   - specModeUniform: uSpecMode
*/
Spectrogram.prototype.loadSpectrogramShaders = function() {
    var fragmentShader = this.getShader("fragmentShader");
    var vertexShader = this.getShader("vertexShader");

    var shaderProgram = this.gl.createProgram();
    this.gl.attachShader(shaderProgram, vertexShader);
    this.gl.attachShader(shaderProgram, fragmentShader);
    this.gl.linkProgram(shaderProgram);

    if (!this.gl.getProgramParameter(shaderProgram, this.gl.LINK_STATUS)) {
        console.log("unable to link shader program.");
    }

    this.gl.useProgram(shaderProgram);

    this.vertexPositionAttribute = this.gl.getAttribLocation(shaderProgram, "aVertexPosition");
    this.gl.enableVertexAttribArray(this.vertexPositionAttribute);

    this.textureCoordAttribute = this.gl.getAttribLocation(shaderProgram, "aTextureCoord");
    this.gl.enableVertexAttribArray(this.textureCoordAttribute);

    this.samplerUniform = this.gl.getUniformLocation(shaderProgram, "uSampler");
    this.zoomUniform = this.gl.getUniformLocation(shaderProgram, "mZoom");
    this.ampRangeUniform = this.gl.getUniformLocation(shaderProgram, "vAmpRange");
    this.specSizeUniform = this.gl.getUniformLocation(shaderProgram, "vSpecSize");
    this.specDataSizeUniform = this.gl.getUniformLocation(shaderProgram, "vDataSize");
    this.specModeUniform = this.gl.getUniformLocation(shaderProgram, "uSpecMode");
    this.specLogarithmicUniform = this.gl.getUniformLocation(shaderProgram, "bSpecLogarithmic");
};

/* load and compile a shader

   Attributes:
   id    the id of a script element that contains the shader source.

   Returns the compiled shader.
*/
Spectrogram.prototype.getShader = function(id) {
    var script = getElementById(id);
    var shader;
    if (script.type === "x-shader/x-fragment") {
        shader = this.gl.createShader(this.gl.FRAGMENT_SHADER);
    } else if (script.type === "x-shader/x-vertex") {
        shader = this.gl.createShader(this.gl.VERTEX_SHADER);
    } else {
        return null;
    }

    this.gl.shaderSource(shader, script.innerHTML);
    this.gl.compileShader(shader);

    if (!this.gl.getShaderParameter(shader, this.gl.COMPILE_STATUS)) {
        console.log("An error occurred compiling the shaders: " + this.gl.getShaderInfoLog(shader));
        return null;
    }

    return shader;
};


/* loads a spectrogram into video memory and fills VBOs

   If there is more data than fits into a single texture, several
   textures are allocated and the data is written into consecutive
   textures. According vertex positions are saved into an equal number
   of VBOs.

   - saves textures into a global array `spectrogramTextures`.
   - saves vertexes into a global array `vertexPositionBuffers`.
   - saves texture coordinates into global `textureCoordBuffer`.

   Attributes:
   data       a Float32Array containing nblocks x nfreqs values.
   nblocks    the width of the data, the number of blocks.
   nfreqs     the height of the data, the number of frequency bins.
*/
Spectrogram.prototype.render = function(data, nblocks, nfreqs, fs, startTime, endTime) {
    this.networkBufferSizeStat.addValue(data.byteLength);

    // calculate the number of textures needed
    var maxTexSize = this.gl.getParameter(this.gl.MAX_TEXTURE_SIZE);
    var numTextures = nblocks / maxTexSize;

    // bail if too big for video memory
    if (Math.ceil(numTextures) > this.gl.getParameter(this.gl.MAX_COMBINED_TEXTURE_IMAGE_UNITS)) {
        console.log("Not enough texture units to display spectrogram");
        return;
    }

    // delete previously allocated textures and VBOs
    for (var i in this.spectrogramTextures) {
        this.gl.deleteBuffer(this.vertexPositionBuffers[i]);
        this.gl.deleteTexture(this.spectrogramTextures[i]);
    }
    this.gl.deleteBuffer(this.textureCoordBuffer);

    this.vertexPositionBuffers = new Array(Math.ceil(numTextures));
    this.spectrogramTextures = new Array(Math.ceil(numTextures));
    // texture coordinates for all textures are identical
    this.textureCoordBuffer = this.gl.createBuffer();
    this.gl.bindBuffer(this.gl.ARRAY_BUFFER, this.textureCoordBuffer);
    var textureCoordinates = new Float32Array([
        1.0, 1.0,
        1.0, 0.0,
        0.0, 1.0,
        0.0, 0.0
    ]);
    this.gl.bufferData(this.gl.ARRAY_BUFFER, textureCoordinates, this.gl.STATIC_DRAW);

    // for every texture, calculate vertex indices and texture content
    for (i = 0; i < numTextures; i++) {
        // texture position in 0..1:
        var minX = i / numTextures;
        var maxX = ((i + 1) < numTextures) ? (i + 1) / numTextures : 1;

        // calculate vertex positions, scaled to -1..1
        this.vertexPositionBuffers[i] = this.gl.createBuffer();
        this.gl.bindBuffer(this.gl.ARRAY_BUFFER, this.vertexPositionBuffers[i]);
        var vertices = new Float32Array([
            maxX * 2 - 1, 1.0,
            maxX * 2 - 1, -1.0,
            minX * 2 - 1, 1.0,
            minX * 2 - 1, -1.0
        ]);
        this.gl.bufferData(this.gl.ARRAY_BUFFER, vertices, this.gl.STATIC_DRAW);

        var blocks = ((i + 1) < numTextures) ? maxTexSize : (numTextures % 1) * maxTexSize;
        var chunk = data.subarray(i * maxTexSize * nfreqs, (i * maxTexSize + blocks) * nfreqs);
        var tmp = new Float32Array(chunk.length);
        for (var x = 0; x < blocks; x++) {
            for (var y = 0; y < nfreqs; y++) {
                tmp[x + blocks * y] = chunk[y + nfreqs * x];
            }
        }
        this.spectrogramTextures[i] = this.gl.createTexture();
        this.gl.activeTexture(this.gl.TEXTURE0 + i);
        this.gl.bindTexture(this.gl.TEXTURE_2D, this.spectrogramTextures[i]);
        this.gl.texParameteri(this.gl.TEXTURE_2D, this.gl.TEXTURE_WRAP_S, this.gl.CLAMP_TO_EDGE);
        this.gl.texParameteri(this.gl.TEXTURE_2D, this.gl.TEXTURE_WRAP_T, this.gl.CLAMP_TO_EDGE);
        this.gl.texImage2D(this.gl.TEXTURE_2D, 0, this.gl.LUMINANCE, blocks, nfreqs, 0, this.gl.LUMINANCE, this.gl.FLOAT, tmp);
        this.gl.texParameteri(this.gl.TEXTURE_2D, this.gl.TEXTURE_MAG_FILTER, this.gl.NEAREST);
        this.gl.texParameteri(this.gl.TEXTURE_2D, this.gl.TEXTURE_MIN_FILTER, this.gl.LINEAR_MIPMAP_NEAREST);
    }

    // save spectrogram sizes
    var minT = hoursToSeconds(startTime);
    var maxT = hoursToSeconds(endTime);
    this.specSize = new SpecSize(minT, maxT, 0, nfreqs - 1);
    this.specSize.numT = nblocks;
    this.specSize.numF = nfreqs;
    this.specViewSize = new SpecSize(minT, maxT, 0, nfreqs - 1, -45, 45);
    var self = this;
    window.requestAnimationFrame(function() {
        self.drawScene();
    });
};

function hoursToSeconds(time){
  return 60 * 60 * time;
}

/* updates the spectrogram and the scales */
Spectrogram.prototype.drawScene = function() {
    this.fpsStat.markStart();

    this.drawSpectrogram();
    this.drawSpecTimeScale();
    this.drawSpecFrequencyScale();

    this.fpsStat.measurePerformance();
};


/* draw the zoomed spectrogram, one texture at a time */
Spectrogram.prototype.drawSpectrogram = function() {
    // load the texture coordinates VBO
    this.gl.bindBuffer(this.gl.ARRAY_BUFFER, this.textureCoordBuffer);
    this.gl.vertexAttribPointer(this.textureCoordAttribute, 2, this.gl.FLOAT, false, 0, 0);

    // set the current model view matrix
    var panX = (this.specViewSize.centerT() - this.specSize.centerT()) / this.specSize.widthT();
    var panY = (this.specViewSize.centerF() - this.specSize.centerF()) / this.specSize.widthF();
    var zoomX = this.specSize.widthT() / this.specViewSize.widthT();
    var zoomY = this.specSize.widthF() / this.specViewSize.widthF();
    var zoomMatrix = [
        zoomX, 0.0, -2 * panX * zoomX,
        0.0, zoomY, -2 * panY * zoomY,
        0.0, 0.0, 1.0
    ];
    this.gl.uniformMatrix3fv(this.zoomUniform, this.gl.FALSE, zoomMatrix);

    // set the current amplitude range to display
    this.gl.uniform2f(this.ampRangeUniform, this.specViewSize.minA, this.specViewSize.maxA);
    // set the size of the spectrogram
    this.gl.uniform2f(this.specSizeUniform, this.specSize.widthT(), this.specSize.widthF());
    this.gl.uniform2f(this.specDataSizeUniform, this.specSize.numT, this.specSize.numF);
    // set the spectrogram display mode
    var specMode = getElementById("specMode").value;
    this.gl.uniform1i(this.specModeUniform, specMode);
    var specLogarithmic = getElementById("specLogarithmic").checked;
    this.gl.uniform1i(this.specLogarithmicUniform, specLogarithmic);

    // switch interpolation on or off
    var interpolate = getElementById("specInterpolation").checked;

    // draw the spectrogram textures
    for (var i = 0; i < this.spectrogramTextures.length; i++) {
        this.gl.activeTexture(this.gl.TEXTURE0 + i);
        this.gl.texParameteri(this.gl.TEXTURE_2D, this.gl.TEXTURE_MAG_FILTER, interpolate ? this.gl.LINEAR : this.gl.NEAREST);
        this.gl.texParameteri(this.gl.TEXTURE_2D, this.gl.TEXTURE_MIN_FILTER, interpolate ? this.gl.LINEAR : this.gl.NEAREST);
        this.gl.bindTexture(this.gl.TEXTURE_2D, this.spectrogramTextures[i]);
        this.gl.uniform1i(this.samplerUniform, i);

        this.gl.bindBuffer(this.gl.ARRAY_BUFFER, this.vertexPositionBuffers[i]);
        this.gl.vertexAttribPointer(this.vertexPositionAttribute, 2, this.gl.FLOAT, false, 0, 0);

        this.gl.drawArrays(this.gl.TRIANGLE_STRIP, 0, 4);
    }
};

/* format a time in mm:ss.ss

   Attributes:
   seconds    a time in seconds.

   returns    a formatted string containing minutes, seconds, and
     hundredths.
*/
Spectrogram.prototype.formatTime = function(seconds) {
    var minutes = Math.floor(seconds / 60);
    var hours = Math.floor(minutes / 60);
    minutes = minutes % 60;
    seconds = seconds % 60;
    minutes = minutes.toString();
    if (minutes.length === 1) {
        minutes = "0" + minutes;
    }
    seconds = seconds.toFixed(2);
    if (seconds.length === 4) {
        seconds = "0" + seconds;
    }
    return hours + ":" + minutes + ":" + seconds;
};

/* draw the time scale canvas

   The time scale prints the minimum and maximum currently visible
   time and an axis with two ticks. Minimum and maximum time are taken
   from specViewSize.(min|max)T.
*/
Spectrogram.prototype.drawSpecTimeScale = function() {
    var ctx = this.specTimeScale.getContext("2d");
    ctx.clearRect(0, 0, ctx.canvas.width, ctx.canvas.height);
    // draw axis line and two ticks
    ctx.fillStyle = "black";
    ctx.fillRect(10, 2, ctx.canvas.width - 20, 1);
    ctx.fillRect(10, 2, 1, 5);
    ctx.fillRect(ctx.canvas.width - 10, 3, 1, 5);
    // draw lower time bound
    ctx.font = "9px sans-serif";
    var text = this.formatTime(this.specViewSize.minT);
    var textWidth = ctx.measureText(text).width;
    ctx.fillText(text, 0, ctx.canvas.height - 2);
    // draw upper time bound
    text = this.formatTime(this.specViewSize.maxT);
    textWidth = ctx.measureText(text).width;
    ctx.fillText(text, ctx.canvas.width - textWidth, ctx.canvas.height - 2);
};

/* convert a linear frequency coordinate to logarithmic frequency */
Spectrogram.prototype.freqLin2Log = function(f) {
    return Math.pow(this.specSize.widthF(), f / this.specSize.widthF());
};

/* format a frequency

   Attributes:
   frequency   a frequency in Hz.

   returns     a formatted string with the frequency in Hz or kHz,
     with an appropriate number of decimals. If logarithmic
     frequency is enabled, the returned frequency will be
     appropriately distorted.
*/
Spectrogram.prototype.formatFrequency = function(frequency) {
    frequency = getElementById("specLogarithmic").checked ? this.freqLin2Log(frequency) : frequency;

    if (frequency < 10) {
        return frequency.toFixed(2) + " Hz";
    } else if (frequency < 100) {
        return frequency.toFixed(1) + " Hz";
    } else if (frequency < 1000) {
        return Math.round(frequency).toString() + " Hz";
    } else if (frequency < 10000) {
        return (frequency / 1000).toFixed(2) + " kHz";
    } else {
        return (frequency / 1000).toFixed(1) + " kHz";
    }
};

/* draw the frequency scale canvas

   The frequency scale prints the minimum and maximum currently
   visible frequency and an axis with two ticks. Minimum and maximum
   frequency are taken from specViewSize.(min|max)F.
*/
Spectrogram.prototype.drawSpecFrequencyScale = function() {
    var ctx = this.specFrequencyScale.getContext("2d");
    ctx.clearRect(0, 0, ctx.canvas.width, ctx.canvas.height);
    // draw axis line and two ticks
    ctx.fillStyle = "black";
    ctx.fillRect(2, 10, 1, ctx.canvas.height - 20);
    ctx.fillRect(2, 10, 5, 1);
    ctx.fillRect(2, ctx.canvas.height - 10, 5, 1);
    // draw upper frequency bound
    ctx.font = "9px sans-serif";
    var text = this.formatFrequency(this.specViewSize.maxF);
    var textWidth = ctx.measureText(text).width;
    ctx.fillText(text, 8, 14);
    // draw lower frequency bound
    text = this.formatFrequency(this.specViewSize.minF);
    textWidth = ctx.measureText(text).width;
    ctx.fillText(text, 8, ctx.canvas.height - 8);
};

/* zoom or pan when on scrolling

   If no modifier is pressed, scrolling scrolls the spectrogram.

   If alt or shift is pressed, scrolling changes the displayed amplitude range.
   Pressing shift as well switches X/Y scrolling.

   If ctrl is pressed, scrolling zooms in or out. If ctrl and shift is
   pressed, scrolling only zooms the time axis.

   At no time will any of this zoom or pan outside of the spectrogram
   view area.
*/
Spectrogram.prototype.onwheel = function(wheel) {
    var stepF = this.specViewSize.widthF() / 100;
    var stepT = this.specViewSize.widthT() / 100;
    var deltaT, deltaF;
    if (wheel.altKey || wheel.shiftKey) {
        var center = this.specViewSize.centerA();
        var range = this.specViewSize.widthA();
        range += wheel.shiftKey ? wheel.deltaY / 10 : wheel.deltaX / 10;
        range = Math.max(range, 1);
        center += wheel.shiftKey ? wheel.deltaX / 10 : wheel.deltaY / 10;
        this.specViewSize.minA = center - range / 2;
        this.specViewSize.maxA = center + range / 2;
    } else if (wheel.ctrlKey) {
        deltaT = wheel.deltaY * stepT;
        if (this.specViewSize.widthT() + 2 * deltaT > this.specSize.widthT()) {
            deltaT = (this.specSize.widthT() - this.specViewSize.widthT()) / 2;
        }
        deltaF = wheel.shiftKey ? 0 : wheel.deltaY * stepF;
        if (this.specViewSize.widthF() + 2 * deltaF > this.specSize.widthF()) {
            deltaF = (this.specSize.widthF() - this.specViewSize.widthF()) / 2;
        }
        this.specViewSize.minF -= deltaF;
        this.specViewSize.maxF += deltaF;
        this.specViewSize.minT -= deltaT;
        this.specViewSize.maxT += deltaT;
        if (this.specViewSize.minT < this.specSize.minT) {
            this.specViewSize.maxT += this.specSize.minT - this.specViewSize.minT;
            this.specViewSize.minT += this.specSize.minT - this.specViewSize.minT;
        }
        if (this.specViewSize.maxT > this.specSize.maxT) {
            this.specViewSize.minT += this.specSize.maxT - this.specViewSize.maxT;
            this.specViewSize.maxT += this.specSize.maxT - this.specViewSize.maxT;
        }
        if (this.specViewSize.minF < this.specSize.minF) {
            this.specViewSize.maxF += this.specSize.minF - this.specViewSize.minF;
            this.specViewSize.minF += this.specSize.minF - this.specViewSize.minF;
        }
        if (this.specViewSize.maxF > this.specSize.maxF) {
            this.specViewSize.minF += this.specSize.maxF - this.specViewSize.maxF;
            this.specViewSize.maxF += this.specSize.maxF - this.specViewSize.maxF;
        }
    } else {
        deltaT = (wheel.shiftKey ? -wheel.deltaY : wheel.deltaX) * stepT / 10;
        if (this.specViewSize.maxT + deltaT > this.specSize.maxT) {
            deltaT = this.specSize.maxT - this.specViewSize.maxT;
        }
        if (this.specViewSize.minT + deltaT < this.specSize.minT) {
            deltaT = this.specSize.minT - this.specViewSize.minT;
        }
        deltaF = (wheel.shiftKey ? wheel.deltaX : -wheel.deltaY) * stepF / 10;
        if (this.specViewSize.maxF + deltaF > this.specSize.maxF) {
            deltaF = this.specSize.maxF - this.specViewSize.maxF;
        }
        if (this.specViewSize.minF + deltaF < this.specSize.minF) {
            deltaF = this.specSize.minF - this.specViewSize.minF;
        }
        this.specViewSize.minF += deltaF;
        this.specViewSize.maxF += deltaF;
        this.specViewSize.minT += deltaT;
        this.specViewSize.maxT += deltaT;
    }
    wheel.preventDefault();
    this.specContainer.onmousemove(wheel);
    var self = this;
    window.requestAnimationFrame(function() {
        self.drawScene();
    });
};

/* update the specDataView with cursor position.

   The specDataView should contain the current cursor position in
   frequency/time coordinates. It is updated every time the mouse is
   moved on the spectrogram.
*/
Spectrogram.prototype.onmousemove = function(mouse) {
    var t = this.specViewSize.scaleT(mouse.layerX / this.specView.clientWidth);
    var f = this.specViewSize.scaleF(1 - mouse.layerY / this.specView.clientHeight);
    this.specDataView.innerHTML = this.formatTime(t) + ", " + this.formatFrequency(f) + "<br/>" +
        this.specViewSize.centerA().toFixed(2) + " dB " +
        "&plusmn; " + (this.specViewSize.widthA() / 2).toFixed(2) + " dB";
};

/* update spectrogram display mode on keypress */
window.onkeypress = function(e) {
    // prevent the default action of submitting the GET parameters.
    var which = e.which || e.keyCode;
    if (which === 13) {
        e.preventDefault();
    }
};
