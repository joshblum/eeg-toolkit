"use strict";

$(document).ready(function() {
    $("select").material_select();
    $(".button-collapse").sideNav({
        menuWidth: 275, // Default is 240
        edge: "left", // Choose the horizontal origin
        closeOnClick: true // Closes side-nav on <a> clicks, useful for Angular/Meteor
    });
    $(".menu-nav").click(function() {
        $(".button-collapse").click();
    });
});
