<?php
// Basic FastCGI / PHP-FPM test

echo "<h1>PHP-FastCGI Test</h1>";

// Check if running under FPM/FastCGI
echo "<h2>Server API</h2>";
echo "<pre>";
echo php_sapi_name() . "\n";  // should show something like "fpm-fcgi"
echo "</pre>";

// Show key FastCGI / server variables
echo "<h2>Server Variables</h2>";
$vars = [
    'SERVER_SOFTWARE',
    'SERVER_NAME',
    'SERVER_PROTOCOL',
    'REQUEST_METHOD',
    'REQUEST_URI',
    'SCRIPT_FILENAME',
    'DOCUMENT_ROOT',
    'REMOTE_ADDR',
];

echo "<pre>";
foreach ($vars as $var) {
    echo $var . ": " . ($_SERVER[$var] ?? 'N/A') . "\n";
}
echo "</pre>";

// Show PHP version
echo "<h2>PHP Version</h2>";
echo phpversion();

// Optional: full phpinfo (uncomment if needed)
// phpinfo();
