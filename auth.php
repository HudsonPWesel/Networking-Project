<?php
// Check user in DB
if ($valid_user) {
    setcookie("session_token", $token, time()+3600, "/");
    header("Location: /Networking-Project/home.html"); 
    exit();
} else {
    echo "Login failed!";
}
?>
