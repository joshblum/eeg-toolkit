"use strict";

// http://stackoverflow.com/questions/105034/create-guid-uuid-in-javascript
function guid() {
  function s4() {
    return Math.floor((1 + Math.random()) * 0x10000)
      .toString(16)
      .substring(1);
  }
  return s4() + s4() + "-" + s4() + "-" + s4() + "-" +
    s4() + "-" + s4() + s4() + s4();
}

var VisgothLabels = {
  "bandwidth": "bandwidth",
  "networkLatency": "networkLatency",
  "fps": "fps",
  "bufferLoadTime": "bufferLoadTime",
  "networkBufferSize": "networkBufferSize",
  "extent": "extent",
};

// ************** //
//  Visgoth Stat  //
// ************** //
function VisgothStat(label, id) {
  this.label = label;
  this.id = id;
  this.start = 0;
  this.end = 0;
  this.state = [];
}

VisgothStat.prototype.addValue = function(value) {
  this.state.push(value);
};

VisgothStat.prototype.markStart = function() {
  this.start = performance.now();
};

VisgothStat.prototype.markEnd = function() {
  this.end = performance.now();
};

VisgothStat.prototype.measurePerformance = function(shouldEnd) {
  if (shouldEnd !== false) {
    this.markEnd();
  }
  this.addValue(this.end - this.start);
};

// ***************** //
//  Visgoth Profiler //
// ***************** //
function VisgothProfiler(label, getProfileDataFn) {
  this.label = label;
  this.getProfileData = getProfileDataFn;
}

VisgothProfiler.prototype.sumArray = function(array) {
  return array.reduce(function(x, y) {
    return x + y;
  }, 0);
};

VisgothProfiler.prototype.avgArray = function(array) {
  var length = array.length;
  if (length === 0) {
    return 0;
  } else {
    return this.sumArray(array) / length;
  }
};

VisgothProfiler.prototype.sumStatArray = function(statArray) {
  return this.sumArray(statArray.reduce(function(x, y) {
    return x.concat(y.state);
  }, []));
};

VisgothProfiler.prototype.avgStatArray = function(statArray) {
  return this.avgArray(statArray.reduce(function(x, y) {
    return x.concat(y.state);
  }, []));
};

var NetworkLatencyProfiler = new VisgothProfiler(VisgothLabels.networkLatency, function(stats) {
  stats = stats[this.label];
  return this.avgStatArray(stats);
});

var BandwidthProfiler = new VisgothProfiler(VisgothLabels.bandwidth, function(stats) {
  var networkLatencyStats = stats[VisgothLabels.networkLatency];
  var networkBufferSizeStats = stats[VisgothLabels.networkBufferSize];
  // should this instead be the average per request?
  return this.avgStatArray(networkBufferSizeStats) / this.avgStatArray(networkLatencyStats);
});

var FpsProfiler = new VisgothProfiler(VisgothLabels.fps, function(stats) {
  stats = stats[this.label];
  // events per second
  return 1000 / this.avgStatArray(stats);
});

var MockExtentProfiler = new VisgothProfiler(VisgothLabels.extent, function(stats) {
  // Index 0 is hard-coded, corresponds to the 'LL' spectrogram
  var extent_state = stats[this.label][0].state;
  return extent_state[extent_state.length - 1];
});


// ********* //
//  Visgoth  //
// ********* //
function Visgoth() {
  this.profilers = {};
  this.profilers[VisgothLabels.bandwidth] = BandwidthProfiler;
  this.profilers[VisgothLabels.networkLatency] = NetworkLatencyProfiler;
  this.profilers[VisgothLabels.fps] = FpsProfiler;
  this.profilers[VisgothLabels.extent] = MockExtentProfiler;

  this.stats = {};
  var profiler;
  for (var label in this.profilers) {
    profiler = this.profilers[label];
    this.stats[label] = [];
  }

  this.clientId = guid();
  this.profileDumps = {};
}

Visgoth.prototype.registerStat = function(visgothStat) {
  if (this.stats[visgothStat.label] === undefined) {
    this.stats[visgothStat.label] = [];
  }
  this.stats[visgothStat.label].push(visgothStat);
};

Visgoth.prototype.getProfileData = function() {
  var profile = {};
  var profiler;
  for (var label in this.profilers) {
    profiler = this.profilers[label];
    profile[label] = profiler.getProfileData(this.stats);
  }
  return profile;
};

Visgoth.prototype.getMetadata = function() {
  return {
    "client_id": this.clientId,
    "timestamp": Date.now(),
  };
};

Visgoth.prototype.sendProfiledMessage = function(type, content) {
  // TODO: When running future experiments where we vary all parameters,
  // this.getProfileData() cannot synchronously compute the profile data
  // (varying extent is okay because it's synchronous with sending messages).
  var visgoth_content = {
    "metadata": this.getMetadata(),
    "profile": this.getProfileData(),
  };
  sendMessage(type, content, visgoth_content);
};

Visgoth.prototype.clearStats = function() {
  var profilerStats;
  for (var label in this.profilers) {
    profilerStats = this.stats[label];
    for (var j in profilerStats) {
      profilerStats[j].state = [];
    }
  }
}

// Aggregate and dump state for a single profile stat during a given request.
Visgoth.prototype.dumpProfileStat = function(key, profilerLabel) {
  if (!(key in this.profileDumps)) {
    this.profileDumps[key] = {};
  }

  this.profileDumps[key][profilerLabel] = this.profilers[profilerLabel].getProfileData(this.stats);
}

// **** Visgoth experiment **** //
Visgoth.prototype.EXTENTS = [1, 2, 4];
Visgoth.prototype.TRIALS = 10;  // Number of times each extent should be run.
Visgoth.prototype.DELAY = 1000;  // Time to wait before trying another profile.

Visgoth.prototype.setExtent = function(extent) {
  // Only necessary for the first spectrogram since visgoth is hard-coded to
  // only look at that one, but oh well.
  var extentState = this.stats[VisgothLabels.extent];
  for (var i in extentState) {
    extentState[i].addValue(extent);
  }
}

// The test function for Visgoth experiments.
Visgoth.prototype.sample = function() {
  requestSpectrogram("007", 1024, 0, 1, OVERLAP, 0);
}

// Run j-th trial for i-th extent. Try to schedule the next one after
// Visgoth.DELAY ms.
Visgoth.prototype.runOnce = function(extents, i, j) {
  // We're done running all extents.
  if (i >= extents.length) {
    return;
  }

  // We're done running one extent.
  if (j >= this.TRIALS) {
    i = i + 1;
    j = 0;
  }

  this.clearStats();
  this.setExtent(extents[i]);
  this.sample();

  setTimeout(Visgoth.prototype.runOnce.bind(this), this.DELAY, extents, i, j+1);
}

Visgoth.prototype.run = function(extent) {
  var extents = [extent];
  if (isNaN(extent)) {
    extents = this.EXTENTS;
  }

  this.profileDumps = {};
  spectrogramRequestCount = 0;
  
  // Synchronously initialize the "extent" field for all profile dumps.
  for (var i in extents) {
    for (var j = 0; j < this.TRIALS; j++) {
      this.setExtent(extents[i]);
      this.dumpProfileStat(i * this.TRIALS + j, VisgothLabels.extent);
    }
  }

  this.runOnce(extents, 0, 0);
}

Visgoth.prototype.sendProfileDumps = function() {
  $.post('/dump-visgoth', {
    'profileDumps': JSON.stringify(this.profileDumps)
  }, function(data) {
    if (!data.success) {
      console.log(data);
    }
  });
}

