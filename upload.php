<?php
// Rui Santos
// Complete project details at https://RandomNerdTutorials.com/esp32-cam-post-image-photo-server/
// Code Based on this example: w3schools.com/php/php_file_upload.asp

date_default_timezone_set('Asia/Jakarta');

$target_dir = "uploads/";
$datum = mktime(date('H')+0, date('i'), date('s'), date('m'), date('d'), date('y'));
$target_file = $target_dir . date('Y.m.d_H-i-s_', $datum) . basename($_FILES["imageFile"]["name"]);
$uploadOk = 1;
$imageFileType = strtolower(pathinfo($target_file,PATHINFO_EXTENSION));

$latest_file = "assets/" . "snap.jpg";
$latest_metadata = "assets/" . "metadata.txt";

// Check if image file is a actual image or fake image
if(isset($_POST["submit"])) {
  $check = getimagesize($_FILES["imageFile"]["tmp_name"]);
  if($check !== false) {
    echo "File is an image - " . $check["mime"] . ".";
    $uploadOk = 1;
  }
  else {
    echo "File is not an image.";
    $uploadOk = 0;
  }
}

// Check if file already exists
if (file_exists($target_file)) {
  echo "Sorry, file already exists.";
  $uploadOk = 0;
}

// Check file size
if ($_FILES["imageFile"]["size"] > 500000) {
  echo "Sorry, your file is too large.";
  $uploadOk = 0;
}

// Allow certain file formats
if($imageFileType != "jpg" && $imageFileType != "png" && $imageFileType != "jpeg"
&& $imageFileType != "gif" ) {
  echo "Sorry, only JPG, JPEG, PNG & GIF files are allowed.";
  $uploadOk = 0;
}

// Check if $uploadOk is set to 0 by an error
if ($uploadOk == 0) {
  echo "Sorry, your file was not uploaded.";
// if everything is ok, try to upload file
}
else {
  $destination_path = getcwd().DIRECTORY_SEPARATOR;
  // $target_path = $destination_path . basename( $_FILES["imageFile"]["name"]);
  $target_path = $destination_path . "uploads" . DIRECTORY_SEPARATOR . date('Y-m-d_H-i-s_', $datum) . basename( $_FILES["imageFile"]["name"]);
  @move_uploaded_file($_FILES['imageFile']['tmp_name'], $target_path);
  echo "successful, file at " . $target_path;
  // if (move_uploaded_file($_FILES["imageFile"]["tmp_name"], $target_file)) {
  //   echo "The file ". basename( $_FILES["imageFile"]["name"]). " has been uploaded.";
  // }
  // else {
  //   echo "Sorry, there was an error uploading your file.";
  // }
  copy($target_path, $latest_file);
  file_put_contents($latest_metadata, date('Y-m-d_H-i-s', $datum));

  system("START /B python img_processing.py");
}
?>