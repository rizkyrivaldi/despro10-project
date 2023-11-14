<?php

    // Define your location project directory in htdocs (EX THE FULL PATH: D:\xampp\htdocs\x-kang\simple-routing-with-php)
    // $url_path = trim( $_SERVER[ 'REQUEST_URI' ], '/' );
    // $project_location = "/x-kang/simple-routing-with-php";

    // $request = $_SERVER['REQUEST_URI'];

    // switch ($request) {
    //     case $me.'/' :
    //         require "index.html";
    //         break;
    //     case $me.'/upload' :
    //         require "upload.php";
    //         break;
    //     default:
    //         http_response_code(404);
    //         echo "404";
    //         break;
    // }

    $request = $_SERVER['REQUEST_URI'];
    $latest_image = __DIR__ . "/assets/snap.jpg";
    $latest_detected = __DIR__ . "/assets/snap_detected.jpg";
    $latest_metadata = __DIR__ . "/assets/metadata.txt";
    $json_data = __DIR__ . "/mock/mock_detection.json";
    $latest_json = __DIR__ . "/json/latest.json";
    $chart_json = __DIR__ . "/json/chart.json";

    // Anti bot logger file
    $antibot = __DIR__ . "/robots.txt";
    // var_dump($raw);

    switch ($request){
        // case "/":
            // require "index.html";
            // require "home.html";
            // break;
        case "/home":
            // require "index.html";
            require "home.html";
            break;
        case "/upload.php":
            require "upload.php";
            break;
        case "/latestpic":
            header("Content-type: image/jpeg");
            readfile($latest_image);
            break;
        case "/latestdetect":
            header("Content-type: image/jpeg");
            readfile($latest_detected);
            break;
        case "/metadata":
            header("Content-type: text/plain");
            readfile($latest_metadata);
            break;
        case "/json_data":
            header("Content-type: application/json");
            readfile($json_data);
            break;
        case "/latest_json":
            header("Content-type: application/json");
            readfile($latest_json);
            break;
        case "/chart_json":
            header("Content-type: application/json");
            readfile($chart_json);
            break;
        case "/robots.txt":
            header("Content-type: text/plain");
            readfile($antibot);
            break;
        default:
            http_response_code(404);
            // echo "404 Not Found";
            break;
    }

?>