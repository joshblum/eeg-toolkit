"use strict";
// copy pasta from https://github.com/lightning-viz/lightning-image-poly/blob/master/src/index.js

function Annotation(el) {
  var $el = $(el);
  var w = $el.width();
  var h = $el.height();

  //create the map
  if (this.map) {
      this.map.remove();
  }

  this.map = L.map(el.id, {
      minZoom: 1,
      maxZoom: 1,
      center: [w/2, h/2],
      zoom: 1,
      attributionControl: false,
      zoomControl: false,
      crs: L.CRS.Simple,
  });

  // calculate the edges of the image, in coordinate space
  var southWest = this.map.unproject([0, h], 1);
  var northEast = this.map.unproject([w, 0], 1);
  var bounds = new L.LatLngBounds(southWest, northEast);
  // tell leaflet that the map is exactly as big as the image
  this.map.setMaxBounds(bounds);

  this.freeDraw = new L.FreeDraw({
    mode: L.FreeDraw.MODES.CREATE | L.FreeDraw.MODES.EDIT | L.FreeDraw.MODES.DELETE,
  });

  // set free drawing options
  this.freeDraw.options.attemptMerge = true;
  this.freeDraw.options.setHullAlgorithm('Wildhoney/ConcaveHull');
  this.freeDraw.options.exitModeAfterCreate(false);

  // add the free drawing layer
  this.map.addLayer(this.freeDraw);
}

Annotation.prototype.exportPolygons = function() {
    // extract coordinates from regions
    var self = this;
    var n = this.freeDraw.memory.states.length;
    return this.freeDraw.memory.states[n-1].map(function(d) {
        var points = [];
        d.forEach(function (p) {
            var newpoint = self.map.project(p, 1);
            points.push([newpoint.x, newpoint.y]);
        });
        return points;
    });
}

Annotation.prototype.importPolygons = function(polygons) {
    var self = this;
    polygons = polygons.map(function (g) {
        var converted = [];
        g.map(function (p) {
            var newp = new L.point(p[0], p[1], false);
            var newpoint = self.map.unproject(newp, 1);
            converted.push(newpoint);
        });
        return converted;
    });

    self.freeDraw.options.simplifyPolygon = false;
    polygons.forEach(function (g) {
        self.freeDraw.createPolygon(g, true);
    });
    self.freeDraw.options.simplifyPolygon = true;
}

