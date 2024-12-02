$(document).ready(function () {
    var icon_reset = "#303030o";
    var icon_on = "#d81b60";

    // Audio player.
    //
    // - Each link with the "audio-play" class is a player,
    // - It wraps an audio SVG icon,
    // - Target link is the audio file to be played,
    // - Multiple players supported, they need to point to != targets.
    //
    var setUpAudio = function () {
	var audio_preload = {};

	$(".audio-play").each(function () {
	    var target = $(this).attr("href");

	    audio_preload[target] = new Audio(target);
	    audio_preload[target].loop = true;
	});

	$(".audio-play").click(function () {
	    var target = $(this).attr("href");

	    if (audio_preload[target].paused) {
		audio_preload[target].play();
		$(this).find("g").attr("fill", icon_on);
	    } else {
		audio_preload[target].pause();
		$(this).find("g").attr("fill", icon_reset);
	    }

	    return false;
	});
    };

    setUpAudio();

});
