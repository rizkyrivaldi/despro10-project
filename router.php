<?php
// router.php

// Get the url path and trim leading slash
$url_path = trim( $_SERVER[ 'REQUEST_URI' ], '/' );

// If url_path is empty, it is root, so call index.html
if ( ! $url_path ) {
    include( 'index.html' );
    return;
}

// If url_path has no dot, it is a post permalink, so add .html extension
if( ! preg_match( '/[.]/', $url_path ) ) {
    include( $url_path . '.html' );
    return;
}

// In case of css files, add the appropriate header
if( preg_match( '/[.css]/', $url_path ) ) {
    header("Content-type: text/css");
    include( $url_path );
    // You can do the same for other file types as well
}

if( preg_match( '/[upload.php]/', $url_path ) ) {
    include('upload.php');
}

if( preg_match( '/[uploads]/', $url_path ) ) {
    include('index.html');
}

?>