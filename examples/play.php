<?php

 // load the file with sound/fx/etc
$ma = new \MiniAudio(__DIR__ . "/button.mp3");

// play
$ma->play();

// just debug if is playing
var_dump($ma->isPlaying());

 // wait audio to finish
sleep(3);

// just another debug if is playing
var_dump($ma->isPlaying());