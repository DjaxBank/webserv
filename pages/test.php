<?php
header('Content-Type: text/html; charset=UTF-8');

function dump_value($value)
{
    return htmlspecialchars(print_r($value, true), ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8');
}

$rawBody = file_get_contents('php://input');

echo "<!DOCTYPE html>\n";
echo "<html lang=\"en\">\n<head><meta charset=\"UTF-8\"><title>PHP CGI Test</title></head>\n<body>\n";
echo "<h1>PHP CGI Request Test</h1>";

echo "<h2>Request Line Data</h2>";
echo "<pre>";
echo "REQUEST_METHOD: " . dump_value($_SERVER['REQUEST_METHOD'] ?? 'N/A') . "\n";
echo "REQUEST_URI: " . dump_value($_SERVER['REQUEST_URI'] ?? 'N/A') . "\n";
echo "QUERY_STRING: " . dump_value($_SERVER['QUERY_STRING'] ?? 'N/A') . "\n";
echo "</pre>";

echo "<h2>Parsed Query Params</h2>";
echo "<pre>" . dump_value($_GET) . "</pre>";

echo "<h2>Parsed POST Params</h2>";
echo "<pre>" . dump_value($_POST) . "</pre>";

echo "<h2>Raw Request Body</h2>";
echo "<pre>" . dump_value($rawBody) . "</pre>";

echo "<h2>PHP Version</h2>";
echo "<pre>" . dump_value(phpversion()) . "</pre>";
echo "</body>\n</html>";
