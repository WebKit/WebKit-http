<?php
/**
 * Defines the Gears manifest file for Google Gears offline storage.
 *
 * @package WordPress
 * @subpackage Administration
 */

/**
 * Disable error reporting
 *
 * Set this to error_reporting( E_ALL ) or error_reporting( E_ALL | E_STRICT ) for debugging
 */
error_reporting(0);

/** Set ABSPATH for execution */
define( 'ABSPATH', dirname(dirname(__FILE__)) . '/' );

require(ABSPATH . '/wp-admin/includes/manifest.php');

$files = get_manifest();

header( 'Expires: Wed, 11 Jan 1984 05:00:00 GMT' );
header( 'Last-Modified: ' . gmdate( 'D, d M Y H:i:s' ) . ' GMT' );
header( 'Cache-Control: no-cache, must-revalidate, max-age=0' );
header( 'Pragma: no-cache' );
header( 'Content-Type: application/x-javascript; charset=UTF-8' );
?>
{
"betaManifestVersion" : 1,
"version" : "<?php echo $man_version; ?>",
"entries" : [
<?php
$entries = '';

foreach ( $files as $file ) {
	// Set ignoreQuery, defaulting to true
	$ignore_query = ( isset($file[2]) && !$file[2] ) ? '' : ', "ignoreQuery" : true ';

	// If version is not set, just output the file
	if ( !isset($file[1]) )
		$entries .= '{ "url" : "' . $file[0] . '"' . $ignore_query . ' }' . "\n";
	// Output url and src
	else
		$entries .= '{ "url" : "' . $file[0] . '", "src" : "' . $file[0] . '?' . $file[1] . '"' . $ignore_query . ' },' . "\n";
}

echo trim( trim($entries), ',' );
?>

]}
