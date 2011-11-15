<?php
/**
 * Main WordPress API
 *
 * @package WordPress
 */

/**
 * Converts MySQL DATETIME field to user specified date format.
 *
 * If $dateformatstring has 'G' value, then gmmktime() function will be used to
 * make the time. If $dateformatstring is set to 'U', then mktime() function
 * will be used to make the time.
 *
 * The $translate will only be used, if it is set to true and it is by default
 * and if the $wp_locale object has the month and weekday set.
 *
 * @since 0.71
 *
 * @param string $dateformatstring Either 'G', 'U', or php date format.
 * @param string $mysqlstring Time from mysql DATETIME field.
 * @param bool $translate Optional. Default is true. Will switch format to locale.
 * @return string Date formated by $dateformatstring or locale (if available).
 */
function mysql2date( $dateformatstring, $mysqlstring, $translate = true ) {
	$m = $mysqlstring;
	if ( empty( $m ) )
		return false;

	if ( 'G' == $dateformatstring )
		return strtotime( $m . ' +0000' );

	$i = strtotime( $m );

	if ( 'U' == $dateformatstring )
		return $i;

	if ( $translate )
		return date_i18n( $dateformatstring, $i );
	else
		return date( $dateformatstring, $i );
}

/**
 * Retrieve the current time based on specified type.
 *
 * The 'mysql' type will return the time in the format for MySQL DATETIME field.
 * The 'timestamp' type will return the current timestamp.
 *
 * If $gmt is set to either '1' or 'true', then both types will use GMT time.
 * if $gmt is false, the output is adjusted with the GMT offset in the WordPress option.
 *
 * @since 1.0.0
 *
 * @param string $type Either 'mysql' or 'timestamp'.
 * @param int|bool $gmt Optional. Whether to use GMT timezone. Default is false.
 * @return int|string String if $type is 'gmt', int if $type is 'timestamp'.
 */
function current_time( $type, $gmt = 0 ) {
	switch ( $type ) {
		case 'mysql':
			return ( $gmt ) ? gmdate( 'Y-m-d H:i:s' ) : gmdate( 'Y-m-d H:i:s', ( time() + ( get_option( 'gmt_offset' ) * 3600 ) ) );
			break;
		case 'timestamp':
			return ( $gmt ) ? time() : time() + ( get_option( 'gmt_offset' ) * 3600 );
			break;
	}
}

/**
 * Retrieve the date in localized format, based on timestamp.
 *
 * If the locale specifies the locale month and weekday, then the locale will
 * take over the format for the date. If it isn't, then the date format string
 * will be used instead.
 *
 * @since 0.71
 *
 * @param string $dateformatstring Format to display the date.
 * @param int $unixtimestamp Optional. Unix timestamp.
 * @param bool $gmt Optional, default is false. Whether to convert to GMT for time.
 * @return string The date, translated if locale specifies it.
 */
function date_i18n( $dateformatstring, $unixtimestamp = false, $gmt = false ) {
	global $wp_locale;
	$i = $unixtimestamp;

	if ( false === $i ) {
		if ( ! $gmt )
			$i = current_time( 'timestamp' );
		else
			$i = time();
		// we should not let date() interfere with our
		// specially computed timestamp
		$gmt = true;
	}

	// store original value for language with untypical grammars
	// see http://core.trac.wordpress.org/ticket/9396
	$req_format = $dateformatstring;

	$datefunc = $gmt? 'gmdate' : 'date';

	if ( ( !empty( $wp_locale->month ) ) && ( !empty( $wp_locale->weekday ) ) ) {
		$datemonth = $wp_locale->get_month( $datefunc( 'm', $i ) );
		$datemonth_abbrev = $wp_locale->get_month_abbrev( $datemonth );
		$dateweekday = $wp_locale->get_weekday( $datefunc( 'w', $i ) );
		$dateweekday_abbrev = $wp_locale->get_weekday_abbrev( $dateweekday );
		$datemeridiem = $wp_locale->get_meridiem( $datefunc( 'a', $i ) );
		$datemeridiem_capital = $wp_locale->get_meridiem( $datefunc( 'A', $i ) );
		$dateformatstring = ' '.$dateformatstring;
		$dateformatstring = preg_replace( "/([^\\\])D/", "\\1" . backslashit( $dateweekday_abbrev ), $dateformatstring );
		$dateformatstring = preg_replace( "/([^\\\])F/", "\\1" . backslashit( $datemonth ), $dateformatstring );
		$dateformatstring = preg_replace( "/([^\\\])l/", "\\1" . backslashit( $dateweekday ), $dateformatstring );
		$dateformatstring = preg_replace( "/([^\\\])M/", "\\1" . backslashit( $datemonth_abbrev ), $dateformatstring );
		$dateformatstring = preg_replace( "/([^\\\])a/", "\\1" . backslashit( $datemeridiem ), $dateformatstring );
		$dateformatstring = preg_replace( "/([^\\\])A/", "\\1" . backslashit( $datemeridiem_capital ), $dateformatstring );

		$dateformatstring = substr( $dateformatstring, 1, strlen( $dateformatstring ) -1 );
	}
	$timezone_formats = array( 'P', 'I', 'O', 'T', 'Z', 'e' );
	$timezone_formats_re = implode( '|', $timezone_formats );
	if ( preg_match( "/$timezone_formats_re/", $dateformatstring ) ) {
		$timezone_string = get_option( 'timezone_string' );
		if ( $timezone_string ) {
			$timezone_object = timezone_open( $timezone_string );
			$date_object = date_create( null, $timezone_object );
			foreach( $timezone_formats as $timezone_format ) {
				if ( false !== strpos( $dateformatstring, $timezone_format ) ) {
					$formatted = date_format( $date_object, $timezone_format );
					$dateformatstring = ' '.$dateformatstring;
					$dateformatstring = preg_replace( "/([^\\\])$timezone_format/", "\\1" . backslashit( $formatted ), $dateformatstring );
					$dateformatstring = substr( $dateformatstring, 1, strlen( $dateformatstring ) -1 );
				}
			}
		}
	}
	$j = @$datefunc( $dateformatstring, $i );
	// allow plugins to redo this entirely for languages with untypical grammars
	$j = apply_filters('date_i18n', $j, $req_format, $i, $gmt);
	return $j;
}

/**
 * Convert integer number to format based on the locale.
 *
 * @since 2.3.0
 *
 * @param int $number The number to convert based on locale.
 * @param int $decimals Precision of the number of decimal places.
 * @return string Converted number in string format.
 */
function number_format_i18n( $number, $decimals = 0 ) {
	global $wp_locale;
	$formatted = number_format( $number, absint( $decimals ), $wp_locale->number_format['decimal_point'], $wp_locale->number_format['thousands_sep'] );
	return apply_filters( 'number_format_i18n', $formatted );
}

/**
 * Convert number of bytes largest unit bytes will fit into.
 *
 * It is easier to read 1kB than 1024 bytes and 1MB than 1048576 bytes. Converts
 * number of bytes to human readable number by taking the number of that unit
 * that the bytes will go into it. Supports TB value.
 *
 * Please note that integers in PHP are limited to 32 bits, unless they are on
 * 64 bit architecture, then they have 64 bit size. If you need to place the
 * larger size then what PHP integer type will hold, then use a string. It will
 * be converted to a double, which should always have 64 bit length.
 *
 * Technically the correct unit names for powers of 1024 are KiB, MiB etc.
 * @link http://en.wikipedia.org/wiki/Byte
 *
 * @since 2.3.0
 *
 * @param int|string $bytes Number of bytes. Note max integer size for integers.
 * @param int $decimals Precision of number of decimal places. Deprecated.
 * @return bool|string False on failure. Number string on success.
 */
function size_format( $bytes, $decimals = 0 ) {
	$quant = array(
		// ========================= Origin ====
		'TB' => 1099511627776,  // pow( 1024, 4)
		'GB' => 1073741824,     // pow( 1024, 3)
		'MB' => 1048576,        // pow( 1024, 2)
		'kB' => 1024,           // pow( 1024, 1)
		'B ' => 1,              // pow( 1024, 0)
	);
	foreach ( $quant as $unit => $mag )
		if ( doubleval($bytes) >= $mag )
			return number_format_i18n( $bytes / $mag, $decimals ) . ' ' . $unit;

	return false;
}

/**
 * Get the week start and end from the datetime or date string from mysql.
 *
 * @since 0.71
 *
 * @param string $mysqlstring Date or datetime field type from mysql.
 * @param int $start_of_week Optional. Start of the week as an integer.
 * @return array Keys are 'start' and 'end'.
 */
function get_weekstartend( $mysqlstring, $start_of_week = '' ) {
	$my = substr( $mysqlstring, 0, 4 ); // Mysql string Year
	$mm = substr( $mysqlstring, 8, 2 ); // Mysql string Month
	$md = substr( $mysqlstring, 5, 2 ); // Mysql string day
	$day = mktime( 0, 0, 0, $md, $mm, $my ); // The timestamp for mysqlstring day.
	$weekday = date( 'w', $day ); // The day of the week from the timestamp
	if ( !is_numeric($start_of_week) )
		$start_of_week = get_option( 'start_of_week' );

	if ( $weekday < $start_of_week )
		$weekday += 7;

	$start = $day - 86400 * ( $weekday - $start_of_week ); // The most recent week start day on or before $day
	$end = $start + 604799; // $start + 7 days - 1 second
	return compact( 'start', 'end' );
}

/**
 * Unserialize value only if it was serialized.
 *
 * @since 2.0.0
 *
 * @param string $original Maybe unserialized original, if is needed.
 * @return mixed Unserialized data can be any type.
 */
function maybe_unserialize( $original ) {
	if ( is_serialized( $original ) ) // don't attempt to unserialize data that wasn't serialized going in
		return @unserialize( $original );
	return $original;
}

/**
 * Check value to find if it was serialized.
 *
 * If $data is not an string, then returned value will always be false.
 * Serialized data is always a string.
 *
 * @since 2.0.5
 *
 * @param mixed $data Value to check to see if was serialized.
 * @return bool False if not serialized and true if it was.
 */
function is_serialized( $data ) {
	// if it isn't a string, it isn't serialized
	if ( ! is_string( $data ) )
		return false;
	$data = trim( $data );
 	if ( 'N;' == $data )
		return true;
	$length = strlen( $data );
	if ( $length < 4 )
		return false;
	if ( ':' !== $data[1] )
		return false;
	$lastc = $data[$length-1];
	if ( ';' !== $lastc && '}' !== $lastc )
		return false;
	$token = $data[0];
	switch ( $token ) {
		case 's' :
			if ( '"' !== $data[$length-2] )
				return false;
		case 'a' :
		case 'O' :
			return (bool) preg_match( "/^{$token}:[0-9]+:/s", $data );
		case 'b' :
		case 'i' :
		case 'd' :
			return (bool) preg_match( "/^{$token}:[0-9.E-]+;\$/", $data );
	}
	return false;
}

/**
 * Check whether serialized data is of string type.
 *
 * @since 2.0.5
 *
 * @param mixed $data Serialized data
 * @return bool False if not a serialized string, true if it is.
 */
function is_serialized_string( $data ) {
	// if it isn't a string, it isn't a serialized string
	if ( !is_string( $data ) )
		return false;
	$data = trim( $data );
	$length = strlen( $data );
	if ( $length < 4 )
		return false;
	elseif ( ':' !== $data[1] )
		return false;
	elseif ( ';' !== $data[$length-1] )
		return false;
	elseif ( $data[0] !== 's' )
		return false;
	elseif ( '"' !== $data[$length-2] )
		return false;
	else
		return true;
}

/**
 * Retrieve option value based on name of option.
 *
 * If the option does not exist or does not have a value, then the return value
 * will be false. This is useful to check whether you need to install an option
 * and is commonly used during installation of plugin options and to test
 * whether upgrading is required.
 *
 * If the option was serialized then it will be unserialized when it is returned.
 *
 * @since 1.5.0
 * @package WordPress
 * @subpackage Option
 * @uses apply_filters() Calls 'pre_option_$option' before checking the option.
 * 	Any value other than false will "short-circuit" the retrieval of the option
 *	and return the returned value. You should not try to override special options,
 * 	but you will not be prevented from doing so.
 * @uses apply_filters() Calls 'option_$option', after checking the option, with
 * 	the option value.
 *
 * @param string $option Name of option to retrieve. Expected to not be SQL-escaped.
 * @return mixed Value set for the option.
 */
function get_option( $option, $default = false ) {
	global $wpdb;

	// Allow plugins to short-circuit options.
	$pre = apply_filters( 'pre_option_' . $option, false );
	if ( false !== $pre )
		return $pre;

	$option = trim($option);
	if ( empty($option) )
		return false;

	if ( defined( 'WP_SETUP_CONFIG' ) )
		return false;

	if ( ! defined( 'WP_INSTALLING' ) ) {
		// prevent non-existent options from triggering multiple queries
		$notoptions = wp_cache_get( 'notoptions', 'options' );
		if ( isset( $notoptions[$option] ) )
			return $default;

		$alloptions = wp_load_alloptions();

		if ( isset( $alloptions[$option] ) ) {
			$value = $alloptions[$option];
		} else {
			$value = wp_cache_get( $option, 'options' );

			if ( false === $value ) {
				$row = $wpdb->get_row( $wpdb->prepare( "SELECT option_value FROM $wpdb->options WHERE option_name = %s LIMIT 1", $option ) );

				// Has to be get_row instead of get_var because of funkiness with 0, false, null values
				if ( is_object( $row ) ) {
					$value = $row->option_value;
					wp_cache_add( $option, $value, 'options' );
				} else { // option does not exist, so we must cache its non-existence
					$notoptions[$option] = true;
					wp_cache_set( 'notoptions', $notoptions, 'options' );
					return $default;
				}
			}
		}
	} else {
		$suppress = $wpdb->suppress_errors();
		$row = $wpdb->get_row( $wpdb->prepare( "SELECT option_value FROM $wpdb->options WHERE option_name = %s LIMIT 1", $option ) );
		$wpdb->suppress_errors( $suppress );
		if ( is_object( $row ) )
			$value = $row->option_value;
		else
			return $default;
	}

	// If home is not set use siteurl.
	if ( 'home' == $option && '' == $value )
		return get_option( 'siteurl' );

	if ( in_array( $option, array('siteurl', 'home', 'category_base', 'tag_base') ) )
		$value = untrailingslashit( $value );

	return apply_filters( 'option_' . $option, maybe_unserialize( $value ) );
}

/**
 * Protect WordPress special option from being modified.
 *
 * Will die if $option is in protected list. Protected options are 'alloptions'
 * and 'notoptions' options.
 *
 * @since 2.2.0
 * @package WordPress
 * @subpackage Option
 *
 * @param string $option Option name.
 */
function wp_protect_special_option( $option ) {
	$protected = array( 'alloptions', 'notoptions' );
	if ( in_array( $option, $protected ) )
		wp_die( sprintf( __( '%s is a protected WP option and may not be modified' ), esc_html( $option ) ) );
}

/**
 * Print option value after sanitizing for forms.
 *
 * @uses attr Sanitizes value.
 * @since 1.5.0
 * @package WordPress
 * @subpackage Option
 *
 * @param string $option Option name.
 */
function form_option( $option ) {
	echo esc_attr( get_option( $option ) );
}

/**
 * Loads and caches all autoloaded options, if available or all options.
 *
 * @since 2.2.0
 * @package WordPress
 * @subpackage Option
 *
 * @return array List of all options.
 */
function wp_load_alloptions() {
	global $wpdb;

	if ( !defined( 'WP_INSTALLING' ) || !is_multisite() )
		$alloptions = wp_cache_get( 'alloptions', 'options' );
	else
		$alloptions = false;

	if ( !$alloptions ) {
		$suppress = $wpdb->suppress_errors();
		if ( !$alloptions_db = $wpdb->get_results( "SELECT option_name, option_value FROM $wpdb->options WHERE autoload = 'yes'" ) )
			$alloptions_db = $wpdb->get_results( "SELECT option_name, option_value FROM $wpdb->options" );
		$wpdb->suppress_errors($suppress);
		$alloptions = array();
		foreach ( (array) $alloptions_db as $o ) {
			$alloptions[$o->option_name] = $o->option_value;
		}
		if ( !defined( 'WP_INSTALLING' ) || !is_multisite() )
			wp_cache_add( 'alloptions', $alloptions, 'options' );
	}

	return $alloptions;
}

/**
 * Loads and caches certain often requested site options if is_multisite() and a peristent cache is not being used.
 *
 * @since 3.0.0
 * @package WordPress
 * @subpackage Option
 *
 * @param int $site_id Optional site ID for which to query the options. Defaults to the current site.
 */
function wp_load_core_site_options( $site_id = null ) {
	global $wpdb, $_wp_using_ext_object_cache;

	if ( !is_multisite() || $_wp_using_ext_object_cache || defined( 'WP_INSTALLING' ) )
		return;

	if ( empty($site_id) )
		$site_id = $wpdb->siteid;

	$core_options = array('site_name', 'siteurl', 'active_sitewide_plugins', '_site_transient_timeout_theme_roots', '_site_transient_theme_roots', 'site_admins', 'can_compress_scripts', 'global_terms_enabled' );

	$core_options_in = "'" . implode("', '", $core_options) . "'";
	$options = $wpdb->get_results( $wpdb->prepare("SELECT meta_key, meta_value FROM $wpdb->sitemeta WHERE meta_key IN ($core_options_in) AND site_id = %d", $site_id) );

	foreach ( $options as $option ) {
		$key = $option->meta_key;
		$cache_key = "{$site_id}:$key";
		$option->meta_value = maybe_unserialize( $option->meta_value );

		wp_cache_set( $cache_key, $option->meta_value, 'site-options' );
	}
}

/**
 * Update the value of an option that was already added.
 *
 * You do not need to serialize values. If the value needs to be serialized, then
 * it will be serialized before it is inserted into the database. Remember,
 * resources can not be serialized or added as an option.
 *
 * If the option does not exist, then the option will be added with the option
 * value, but you will not be able to set whether it is autoloaded. If you want
 * to set whether an option is autoloaded, then you need to use the add_option().
 *
 * @since 1.0.0
 * @package WordPress
 * @subpackage Option
 *
 * @uses apply_filters() Calls 'pre_update_option_$option' hook to allow overwriting the
 * 	option value to be stored.
 * @uses do_action() Calls 'update_option' hook before updating the option.
 * @uses do_action() Calls 'update_option_$option' and 'updated_option' hooks on success.
 *
 * @param string $option Option name. Expected to not be SQL-escaped.
 * @param mixed $newvalue Option value. Expected to not be SQL-escaped.
 * @return bool False if value was not updated and true if value was updated.
 */
function update_option( $option, $newvalue ) {
	global $wpdb;

	$option = trim($option);
	if ( empty($option) )
		return false;

	wp_protect_special_option( $option );

	if ( is_object($newvalue) )
		$newvalue = clone $newvalue;

	$newvalue = sanitize_option( $option, $newvalue );
	$oldvalue = get_option( $option );
	$newvalue = apply_filters( 'pre_update_option_' . $option, $newvalue, $oldvalue );

	// If the new and old values are the same, no need to update.
	if ( $newvalue === $oldvalue )
		return false;

	if ( false === $oldvalue )
		return add_option( $option, $newvalue );

	$notoptions = wp_cache_get( 'notoptions', 'options' );
	if ( is_array( $notoptions ) && isset( $notoptions[$option] ) ) {
		unset( $notoptions[$option] );
		wp_cache_set( 'notoptions', $notoptions, 'options' );
	}

	$_newvalue = $newvalue;
	$newvalue = maybe_serialize( $newvalue );

	do_action( 'update_option', $option, $oldvalue, $_newvalue );
	if ( ! defined( 'WP_INSTALLING' ) ) {
		$alloptions = wp_load_alloptions();
		if ( isset( $alloptions[$option] ) ) {
			$alloptions[$option] = $_newvalue;
			wp_cache_set( 'alloptions', $alloptions, 'options' );
		} else {
			wp_cache_set( $option, $_newvalue, 'options' );
		}
	}

	$result = $wpdb->update( $wpdb->options, array( 'option_value' => $newvalue ), array( 'option_name' => $option ) );

	if ( $result ) {
		do_action( "update_option_{$option}", $oldvalue, $_newvalue );
		do_action( 'updated_option', $option, $oldvalue, $_newvalue );
		return true;
	}
	return false;
}

/**
 * Add a new option.
 *
 * You do not need to serialize values. If the value needs to be serialized, then
 * it will be serialized before it is inserted into the database. Remember,
 * resources can not be serialized or added as an option.
 *
 * You can create options without values and then add values later. Does not
 * check whether the option has already been added, but does check that you
 * aren't adding a protected WordPress option. Care should be taken to not name
 * options the same as the ones which are protected and to not add options
 * that were already added.
 *
 * @package WordPress
 * @subpackage Option
 * @since 1.0.0
 *
 * @uses do_action() Calls 'add_option' hook before adding the option.
 * @uses do_action() Calls 'add_option_$option' and 'added_option' hooks on success.
 *
 * @param string $option Name of option to add. Expected to not be SQL-escaped.
 * @param mixed $value Optional. Option value, can be anything. Expected to not be SQL-escaped.
 * @param mixed $deprecated Optional. Description. Not used anymore.
 * @param bool $autoload Optional. Default is enabled. Whether to load the option when WordPress starts up.
 * @return null returns when finished.
 */
function add_option( $option, $value = '', $deprecated = '', $autoload = 'yes' ) {
	global $wpdb;

	if ( !empty( $deprecated ) )
		_deprecated_argument( __FUNCTION__, '2.3' );

	$option = trim($option);
	if ( empty($option) )
		return false;

	wp_protect_special_option( $option );

	if ( is_object($value) )
		$value = clone $value;

	$value = sanitize_option( $option, $value );

	// Make sure the option doesn't already exist. We can check the 'notoptions' cache before we ask for a db query
	$notoptions = wp_cache_get( 'notoptions', 'options' );
	if ( !is_array( $notoptions ) || !isset( $notoptions[$option] ) )
		if ( false !== get_option( $option ) )
			return;

	$_value = $value;
	$value = maybe_serialize( $value );
	$autoload = ( 'no' === $autoload ) ? 'no' : 'yes';
	do_action( 'add_option', $option, $_value );
	if ( ! defined( 'WP_INSTALLING' ) ) {
		if ( 'yes' == $autoload ) {
			$alloptions = wp_load_alloptions();
			$alloptions[$option] = $value;
			wp_cache_set( 'alloptions', $alloptions, 'options' );
		} else {
			wp_cache_set( $option, $value, 'options' );
		}
	}

	// This option exists now
	$notoptions = wp_cache_get( 'notoptions', 'options' ); // yes, again... we need it to be fresh
	if ( is_array( $notoptions ) && isset( $notoptions[$option] ) ) {
		unset( $notoptions[$option] );
		wp_cache_set( 'notoptions', $notoptions, 'options' );
	}

	$result = $wpdb->query( $wpdb->prepare( "INSERT INTO `$wpdb->options` (`option_name`, `option_value`, `autoload`) VALUES (%s, %s, %s) ON DUPLICATE KEY UPDATE `option_name` = VALUES(`option_name`), `option_value` = VALUES(`option_value`), `autoload` = VALUES(`autoload`)", $option, $value, $autoload ) );

	if ( $result ) {
		do_action( "add_option_{$option}", $option, $_value );
		do_action( 'added_option', $option, $_value );
		return true;
	}
	return false;
}

/**
 * Removes option by name. Prevents removal of protected WordPress options.
 *
 * @package WordPress
 * @subpackage Option
 * @since 1.2.0
 *
 * @uses do_action() Calls 'delete_option' hook before option is deleted.
 * @uses do_action() Calls 'deleted_option' and 'delete_option_$option' hooks on success.
 *
 * @param string $option Name of option to remove. Expected to not be SQL-escaped.
 * @return bool True, if option is successfully deleted. False on failure.
 */
function delete_option( $option ) {
	global $wpdb;

	wp_protect_special_option( $option );

	// Get the ID, if no ID then return
	$row = $wpdb->get_row( $wpdb->prepare( "SELECT autoload FROM $wpdb->options WHERE option_name = %s", $option ) );
	if ( is_null( $row ) )
		return false;
	do_action( 'delete_option', $option );
	$result = $wpdb->query( $wpdb->prepare( "DELETE FROM $wpdb->options WHERE option_name = %s", $option) );
	if ( ! defined( 'WP_INSTALLING' ) ) {
		if ( 'yes' == $row->autoload ) {
			$alloptions = wp_load_alloptions();
			if ( is_array( $alloptions ) && isset( $alloptions[$option] ) ) {
				unset( $alloptions[$option] );
				wp_cache_set( 'alloptions', $alloptions, 'options' );
			}
		} else {
			wp_cache_delete( $option, 'options' );
		}
	}
	if ( $result ) {
		do_action( "delete_option_$option", $option );
		do_action( 'deleted_option', $option );
		return true;
	}
	return false;
}

/**
 * Delete a transient
 *
 * @since 2.8.0
 * @package WordPress
 * @subpackage Transient
 *
 * @uses do_action() Calls 'delete_transient_$transient' hook before transient is deleted.
 * @uses do_action() Calls 'deleted_transient' hook on success.
 *
 * @param string $transient Transient name. Expected to not be SQL-escaped.
 * @return bool true if successful, false otherwise
 */
function delete_transient( $transient ) {
	global $_wp_using_ext_object_cache;

	do_action( 'delete_transient_' . $transient, $transient );

	if ( $_wp_using_ext_object_cache ) {
		$result = wp_cache_delete( $transient, 'transient' );
	} else {
		$option_timeout = '_transient_timeout_' . $transient;
		$option = '_transient_' . $transient;
		$result = delete_option( $option );
		if ( $result )
			delete_option( $option_timeout );
	}

	if ( $result )
		do_action( 'deleted_transient', $transient );
	return $result;
}

/**
 * Get the value of a transient
 *
 * If the transient does not exist or does not have a value, then the return value
 * will be false.
 *
 * @uses apply_filters() Calls 'pre_transient_$transient' hook before checking the transient.
 * 	Any value other than false will "short-circuit" the retrieval of the transient
 *	and return the returned value.
 * @uses apply_filters() Calls 'transient_$option' hook, after checking the transient, with
 * 	the transient value.
 *
 * @since 2.8.0
 * @package WordPress
 * @subpackage Transient
 *
 * @param string $transient Transient name. Expected to not be SQL-escaped
 * @return mixed Value of transient
 */
function get_transient( $transient ) {
	global $_wp_using_ext_object_cache;

	$pre = apply_filters( 'pre_transient_' . $transient, false );
	if ( false !== $pre )
		return $pre;

	if ( $_wp_using_ext_object_cache ) {
		$value = wp_cache_get( $transient, 'transient' );
	} else {
		$transient_option = '_transient_' . $transient;
		if ( ! defined( 'WP_INSTALLING' ) ) {
			// If option is not in alloptions, it is not autoloaded and thus has a timeout
			$alloptions = wp_load_alloptions();
			if ( !isset( $alloptions[$transient_option] ) ) {
				$transient_timeout = '_transient_timeout_' . $transient;
				if ( get_option( $transient_timeout ) < time() ) {
					delete_option( $transient_option  );
					delete_option( $transient_timeout );
					return false;
				}
			}
		}

		$value = get_option( $transient_option );
	}

	return apply_filters( 'transient_' . $transient, $value );
}

/**
 * Set/update the value of a transient
 *
 * You do not need to serialize values. If the value needs to be serialized, then
 * it will be serialized before it is set.
 *
 * @since 2.8.0
 * @package WordPress
 * @subpackage Transient
 *
 * @uses apply_filters() Calls 'pre_set_transient_$transient' hook to allow overwriting the
 * 	transient value to be stored.
 * @uses do_action() Calls 'set_transient_$transient' and 'setted_transient' hooks on success.
 *
 * @param string $transient Transient name. Expected to not be SQL-escaped.
 * @param mixed $value Transient value. Expected to not be SQL-escaped.
 * @param int $expiration Time until expiration in seconds, default 0
 * @return bool False if value was not set and true if value was set.
 */
function set_transient( $transient, $value, $expiration = 0 ) {
	global $_wp_using_ext_object_cache;

	$value = apply_filters( 'pre_set_transient_' . $transient, $value );

	if ( $_wp_using_ext_object_cache ) {
		$result = wp_cache_set( $transient, $value, 'transient', $expiration );
	} else {
		$transient_timeout = '_transient_timeout_' . $transient;
		$transient = '_transient_' . $transient;
		if ( false === get_option( $transient ) ) {
			$autoload = 'yes';
			if ( $expiration ) {
				$autoload = 'no';
				add_option( $transient_timeout, time() + $expiration, '', 'no' );
			}
			$result = add_option( $transient, $value, '', $autoload );
		} else {
			if ( $expiration )
				update_option( $transient_timeout, time() + $expiration );
			$result = update_option( $transient, $value );
		}
	}
	if ( $result ) {
		do_action( 'set_transient_' . $transient );
		do_action( 'setted_transient', $transient );
	}
	return $result;
}

/**
 * Saves and restores user interface settings stored in a cookie.
 *
 * Checks if the current user-settings cookie is updated and stores it. When no
 * cookie exists (different browser used), adds the last saved cookie restoring
 * the settings.
 *
 * @package WordPress
 * @subpackage Option
 * @since 2.7.0
 */
function wp_user_settings() {

	if ( ! is_admin() )
		return;

	if ( defined('DOING_AJAX') )
		return;

	if ( ! $user = wp_get_current_user() )
		return;

	$settings = get_user_option( 'user-settings', $user->ID );

	if ( isset( $_COOKIE['wp-settings-' . $user->ID] ) ) {
		$cookie = preg_replace( '/[^A-Za-z0-9=&_]/', '', $_COOKIE['wp-settings-' . $user->ID] );

		if ( ! empty( $cookie ) && strpos( $cookie, '=' ) ) {
			if ( $cookie == $settings )
				return;

			$last_time = (int) get_user_option( 'user-settings-time', $user->ID );
			$saved = isset( $_COOKIE['wp-settings-time-' . $user->ID]) ? preg_replace( '/[^0-9]/', '', $_COOKIE['wp-settings-time-' . $user->ID] ) : 0;

			if ( $saved > $last_time ) {
				update_user_option( $user->ID, 'user-settings', $cookie, false );
				update_user_option( $user->ID, 'user-settings-time', time() - 5, false );
				return;
			}
		}
	}

	setcookie( 'wp-settings-' . $user->ID, $settings, time() + 31536000, SITECOOKIEPATH );
	setcookie( 'wp-settings-time-' . $user->ID, time(), time() + 31536000, SITECOOKIEPATH );
	$_COOKIE['wp-settings-' . $user->ID] = $settings;
}

/**
 * Retrieve user interface setting value based on setting name.
 *
 * @package WordPress
 * @subpackage Option
 * @since 2.7.0
 *
 * @param string $name The name of the setting.
 * @param string $default Optional default value to return when $name is not set.
 * @return mixed the last saved user setting or the default value/false if it doesn't exist.
 */
function get_user_setting( $name, $default = false ) {

	$all = get_all_user_settings();

	return isset($all[$name]) ? $all[$name] : $default;
}

/**
 * Add or update user interface setting.
 *
 * Both $name and $value can contain only ASCII letters, numbers and underscores.
 * This function has to be used before any output has started as it calls setcookie().
 *
 * @package WordPress
 * @subpackage Option
 * @since 2.8.0
 *
 * @param string $name The name of the setting.
 * @param string $value The value for the setting.
 * @return bool true if set successfully/false if not.
 */
function set_user_setting( $name, $value ) {

	if ( headers_sent() )
		return false;

	$all = get_all_user_settings();
	$name = preg_replace( '/[^A-Za-z0-9_]+/', '', $name );

	if ( empty($name) )
		return false;

	$all[$name] = $value;

	return wp_set_all_user_settings($all);
}

/**
 * Delete user interface settings.
 *
 * Deleting settings would reset them to the defaults.
 * This function has to be used before any output has started as it calls setcookie().
 *
 * @package WordPress
 * @subpackage Option
 * @since 2.7.0
 *
 * @param mixed $names The name or array of names of the setting to be deleted.
 * @return bool true if deleted successfully/false if not.
 */
function delete_user_setting( $names ) {

	if ( headers_sent() )
		return false;

	$all = get_all_user_settings();
	$names = (array) $names;

	foreach ( $names as $name ) {
		if ( isset($all[$name]) ) {
			unset($all[$name]);
			$deleted = true;
		}
	}

	if ( isset($deleted) )
		return wp_set_all_user_settings($all);

	return false;
}

/**
 * Retrieve all user interface settings.
 *
 * @package WordPress
 * @subpackage Option
 * @since 2.7.0
 *
 * @return array the last saved user settings or empty array.
 */
function get_all_user_settings() {
	global $_updated_user_settings;

	if ( ! $user = wp_get_current_user() )
		return array();

	if ( isset($_updated_user_settings) && is_array($_updated_user_settings) )
		return $_updated_user_settings;

	$all = array();
	if ( isset($_COOKIE['wp-settings-' . $user->ID]) ) {
		$cookie = preg_replace( '/[^A-Za-z0-9=&_]/', '', $_COOKIE['wp-settings-' . $user->ID] );

		if ( $cookie && strpos($cookie, '=') ) // the '=' cannot be 1st char
			parse_str($cookie, $all);

	} else {
		$option = get_user_option('user-settings', $user->ID);
		if ( $option && is_string($option) )
			parse_str( $option, $all );
	}

	return $all;
}

/**
 * Private. Set all user interface settings.
 *
 * @package WordPress
 * @subpackage Option
 * @since 2.8.0
 *
 * @param unknown $all
 * @return bool
 */
function wp_set_all_user_settings($all) {
	global $_updated_user_settings;

	if ( ! $user = wp_get_current_user() )
		return false;

	$_updated_user_settings = $all;
	$settings = '';
	foreach ( $all as $k => $v ) {
		$v = preg_replace( '/[^A-Za-z0-9_]+/', '', $v );
		$settings .= $k . '=' . $v . '&';
	}

	$settings = rtrim($settings, '&');

	update_user_option( $user->ID, 'user-settings', $settings, false );
	update_user_option( $user->ID, 'user-settings-time', time(), false );

	return true;
}

/**
 * Delete the user settings of the current user.
 *
 * @package WordPress
 * @subpackage Option
 * @since 2.7.0
 */
function delete_all_user_settings() {
	if ( ! $user = wp_get_current_user() )
		return;

	update_user_option( $user->ID, 'user-settings', '', false );
	setcookie('wp-settings-' . $user->ID, ' ', time() - 31536000, SITECOOKIEPATH);
}

/**
 * Serialize data, if needed.
 *
 * @since 2.0.5
 *
 * @param mixed $data Data that might be serialized.
 * @return mixed A scalar data
 */
function maybe_serialize( $data ) {
	if ( is_array( $data ) || is_object( $data ) )
		return serialize( $data );

	if ( is_serialized( $data ) )
		return serialize( $data );

	return $data;
}

/**
 * Retrieve post title from XMLRPC XML.
 *
 * If the title element is not part of the XML, then the default post title from
 * the $post_default_title will be used instead.
 *
 * @package WordPress
 * @subpackage XMLRPC
 * @since 0.71
 *
 * @global string $post_default_title Default XMLRPC post title.
 *
 * @param string $content XMLRPC XML Request content
 * @return string Post title
 */
function xmlrpc_getposttitle( $content ) {
	global $post_default_title;
	if ( preg_match( '/<title>(.+?)<\/title>/is', $content, $matchtitle ) ) {
		$post_title = $matchtitle[1];
	} else {
		$post_title = $post_default_title;
	}
	return $post_title;
}

/**
 * Retrieve the post category or categories from XMLRPC XML.
 *
 * If the category element is not found, then the default post category will be
 * used. The return type then would be what $post_default_category. If the
 * category is found, then it will always be an array.
 *
 * @package WordPress
 * @subpackage XMLRPC
 * @since 0.71
 *
 * @global string $post_default_category Default XMLRPC post category.
 *
 * @param string $content XMLRPC XML Request content
 * @return string|array List of categories or category name.
 */
function xmlrpc_getpostcategory( $content ) {
	global $post_default_category;
	if ( preg_match( '/<category>(.+?)<\/category>/is', $content, $matchcat ) ) {
		$post_category = trim( $matchcat[1], ',' );
		$post_category = explode( ',', $post_category );
	} else {
		$post_category = $post_default_category;
	}
	return $post_category;
}

/**
 * XMLRPC XML content without title and category elements.
 *
 * @package WordPress
 * @subpackage XMLRPC
 * @since 0.71
 *
 * @param string $content XMLRPC XML Request content
 * @return string XMLRPC XML Request content without title and category elements.
 */
function xmlrpc_removepostdata( $content ) {
	$content = preg_replace( '/<title>(.+?)<\/title>/si', '', $content );
	$content = preg_replace( '/<category>(.+?)<\/category>/si', '', $content );
	$content = trim( $content );
	return $content;
}

/**
 * Open the file handle for debugging.
 *
 * This function is used for XMLRPC feature, but it is general purpose enough
 * to be used in anywhere.
 *
 * @see fopen() for mode options.
 * @package WordPress
 * @subpackage Debug
 * @since 0.71
 * @uses $debug Used for whether debugging is enabled.
 *
 * @param string $filename File path to debug file.
 * @param string $mode Same as fopen() mode parameter.
 * @return bool|resource File handle. False on failure.
 */
function debug_fopen( $filename, $mode ) {
	global $debug;
	if ( 1 == $debug ) {
		$fp = fopen( $filename, $mode );
		return $fp;
	} else {
		return false;
	}
}

/**
 * Write contents to the file used for debugging.
 *
 * Technically, this can be used to write to any file handle when the global
 * $debug is set to 1 or true.
 *
 * @package WordPress
 * @subpackage Debug
 * @since 0.71
 * @uses $debug Used for whether debugging is enabled.
 *
 * @param resource $fp File handle for debugging file.
 * @param string $string Content to write to debug file.
 */
function debug_fwrite( $fp, $string ) {
	global $debug;
	if ( 1 == $debug )
		fwrite( $fp, $string );
}

/**
 * Close the debugging file handle.
 *
 * Technically, this can be used to close any file handle when the global $debug
 * is set to 1 or true.
 *
 * @package WordPress
 * @subpackage Debug
 * @since 0.71
 * @uses $debug Used for whether debugging is enabled.
 *
 * @param resource $fp Debug File handle.
 */
function debug_fclose( $fp ) {
	global $debug;
	if ( 1 == $debug )
		fclose( $fp );
}

/**
 * Check content for video and audio links to add as enclosures.
 *
 * Will not add enclosures that have already been added and will
 * remove enclosures that are no longer in the post. This is called as
 * pingbacks and trackbacks.
 *
 * @package WordPress
 * @since 1.5.0
 *
 * @uses $wpdb
 *
 * @param string $content Post Content
 * @param int $post_ID Post ID
 */
function do_enclose( $content, $post_ID ) {
	global $wpdb;

	//TODO: Tidy this ghetto code up and make the debug code optional
	include_once( ABSPATH . WPINC . '/class-IXR.php' );

	$log = debug_fopen( ABSPATH . 'enclosures.log', 'a' );
	$post_links = array();
	debug_fwrite( $log, 'BEGIN ' . date( 'YmdHis', time() ) . "\n" );

	$pung = get_enclosed( $post_ID );

	$ltrs = '\w';
	$gunk = '/#~:.?+=&%@!\-';
	$punc = '.:?\-';
	$any = $ltrs . $gunk . $punc;

	preg_match_all( "{\b http : [$any] +? (?= [$punc] * [^$any] | $)}x", $content, $post_links_temp );

	debug_fwrite( $log, 'Post contents:' );
	debug_fwrite( $log, $content . "\n" );

	foreach ( $pung as $link_test ) {
		if ( !in_array( $link_test, $post_links_temp[0] ) ) { // link no longer in post
			$mid = $wpdb->get_col( $wpdb->prepare("SELECT meta_id FROM $wpdb->postmeta WHERE post_id = %d AND meta_key = 'enclosure' AND meta_value LIKE (%s)", $post_ID, like_escape( $link_test ) . '%') );
			do_action( 'delete_postmeta', $mid );
			$wpdb->query( $wpdb->prepare("DELETE FROM $wpdb->postmeta WHERE meta_id IN(%s)", implode( ',', $mid ) ) );
			do_action( 'deleted_postmeta', $mid );
		}
	}

	foreach ( (array) $post_links_temp[0] as $link_test ) {
		if ( !in_array( $link_test, $pung ) ) { // If we haven't pung it already
			$test = @parse_url( $link_test );
			if ( false === $test )
				continue;
			if ( isset( $test['query'] ) )
				$post_links[] = $link_test;
			elseif ( isset($test['path']) && ( $test['path'] != '/' ) &&  ($test['path'] != '' ) )
				$post_links[] = $link_test;
		}
	}

	foreach ( (array) $post_links as $url ) {
		if ( $url != '' && !$wpdb->get_var( $wpdb->prepare( "SELECT post_id FROM $wpdb->postmeta WHERE post_id = %d AND meta_key = 'enclosure' AND meta_value LIKE (%s)", $post_ID, like_escape( $url ) . '%' ) ) ) {

			if ( $headers = wp_get_http_headers( $url) ) {
				$len = (int) $headers['content-length'];
				$type = $headers['content-type'];
				$allowed_types = array( 'video', 'audio' );

				// Check to see if we can figure out the mime type from
				// the extension
				$url_parts = @parse_url( $url );
				if ( false !== $url_parts ) {
					$extension = pathinfo( $url_parts['path'], PATHINFO_EXTENSION );
					if ( !empty( $extension ) ) {
						foreach ( get_allowed_mime_types( ) as $exts => $mime ) {
							if ( preg_match( '!^(' . $exts . ')$!i', $extension ) ) {
								$type = $mime;
								break;
							}
						}
					}
				}

				if ( in_array( substr( $type, 0, strpos( $type, "/" ) ), $allowed_types ) ) {
					$meta_value = "$url\n$len\n$type\n";
					$wpdb->insert($wpdb->postmeta, array('post_id' => $post_ID, 'meta_key' => 'enclosure', 'meta_value' => $meta_value) );
					do_action( 'added_postmeta', $wpdb->insert_id, $post_ID, 'enclosure', $meta_value );
				}
			}
		}
	}
}

/**
 * Perform a HTTP HEAD or GET request.
 *
 * If $file_path is a writable filename, this will do a GET request and write
 * the file to that path.
 *
 * @since 2.5.0
 *
 * @param string $url URL to fetch.
 * @param string|bool $file_path Optional. File path to write request to.
 * @param int $red (private) The number of Redirects followed, Upon 5 being hit, returns false.
 * @return bool|string False on failure and string of headers if HEAD request.
 */
function wp_get_http( $url, $file_path = false, $red = 1 ) {
	@set_time_limit( 60 );

	if ( $red > 5 )
		return false;

	$options = array();
	$options['redirection'] = 5;

	if ( false == $file_path )
		$options['method'] = 'HEAD';
	else
		$options['method'] = 'GET';

	$response = wp_remote_request($url, $options);

	if ( is_wp_error( $response ) )
		return false;

	$headers = wp_remote_retrieve_headers( $response );
	$headers['response'] = wp_remote_retrieve_response_code( $response );

	// WP_HTTP no longer follows redirects for HEAD requests.
	if ( 'HEAD' == $options['method'] && in_array($headers['response'], array(301, 302)) && isset( $headers['location'] ) ) {
		return wp_get_http( $headers['location'], $file_path, ++$red );
	}

	if ( false == $file_path )
		return $headers;

	// GET request - write it to the supplied filename
	$out_fp = fopen($file_path, 'w');
	if ( !$out_fp )
		return $headers;

	fwrite( $out_fp,  wp_remote_retrieve_body( $response ) );
	fclose($out_fp);
	clearstatcache();

	return $headers;
}

/**
 * Retrieve HTTP Headers from URL.
 *
 * @since 1.5.1
 *
 * @param string $url
 * @param bool $deprecated Not Used.
 * @return bool|string False on failure, headers on success.
 */
function wp_get_http_headers( $url, $deprecated = false ) {
	if ( !empty( $deprecated ) )
		_deprecated_argument( __FUNCTION__, '2.7' );

	$response = wp_remote_head( $url );

	if ( is_wp_error( $response ) )
		return false;

	return wp_remote_retrieve_headers( $response );
}

/**
 * Whether today is a new day.
 *
 * @since 0.71
 * @uses $day Today
 * @uses $previousday Previous day
 *
 * @return int 1 when new day, 0 if not a new day.
 */
function is_new_day() {
	global $currentday, $previousday;
	if ( $currentday != $previousday )
		return 1;
	else
		return 0;
}

/**
 * Build URL query based on an associative and, or indexed array.
 *
 * This is a convenient function for easily building url queries. It sets the
 * separator to '&' and uses _http_build_query() function.
 *
 * @see _http_build_query() Used to build the query
 * @link http://us2.php.net/manual/en/function.http-build-query.php more on what
 *		http_build_query() does.
 *
 * @since 2.3.0
 *
 * @param array $data URL-encode key/value pairs.
 * @return string URL encoded string
 */
function build_query( $data ) {
	return _http_build_query( $data, null, '&', '', false );
}

// from php.net (modified by Mark Jaquith to behave like the native PHP5 function)
function _http_build_query($data, $prefix=null, $sep=null, $key='', $urlencode=true) {
	$ret = array();

	foreach ( (array) $data as $k => $v ) {
		if ( $urlencode)
			$k = urlencode($k);
		if ( is_int($k) && $prefix != null )
			$k = $prefix.$k;
		if ( !empty($key) )
			$k = $key . '%5B' . $k . '%5D';
		if ( $v === NULL )
			continue;
		elseif ( $v === FALSE )
			$v = '0';

		if ( is_array($v) || is_object($v) )
			array_push($ret,_http_build_query($v, '', $sep, $k, $urlencode));
		elseif ( $urlencode )
			array_push($ret, $k.'='.urlencode($v));
		else
			array_push($ret, $k.'='.$v);
	}

	if ( NULL === $sep )
		$sep = ini_get('arg_separator.output');

	return implode($sep, $ret);
}

/**
 * Retrieve a modified URL query string.
 *
 * You can rebuild the URL and append a new query variable to the URL query by
 * using this function. You can also retrieve the full URL with query data.
 *
 * Adding a single key & value or an associative array. Setting a key value to
 * emptystring removes the key. Omitting oldquery_or_uri uses the $_SERVER
 * value.
 *
 * @since 1.5.0
 *
 * @param mixed $param1 Either newkey or an associative_array
 * @param mixed $param2 Either newvalue or oldquery or uri
 * @param mixed $param3 Optional. Old query or uri
 * @return string New URL query string.
 */
function add_query_arg() {
	$ret = '';
	if ( is_array( func_get_arg(0) ) ) {
		if ( @func_num_args() < 2 || false === @func_get_arg( 1 ) )
			$uri = $_SERVER['REQUEST_URI'];
		else
			$uri = @func_get_arg( 1 );
	} else {
		if ( @func_num_args() < 3 || false === @func_get_arg( 2 ) )
			$uri = $_SERVER['REQUEST_URI'];
		else
			$uri = @func_get_arg( 2 );
	}

	if ( $frag = strstr( $uri, '#' ) )
		$uri = substr( $uri, 0, -strlen( $frag ) );
	else
		$frag = '';

	if ( preg_match( '|^https?://|i', $uri, $matches ) ) {
		$protocol = $matches[0];
		$uri = substr( $uri, strlen( $protocol ) );
	} else {
		$protocol = '';
	}

	if ( strpos( $uri, '?' ) !== false ) {
		$parts = explode( '?', $uri, 2 );
		if ( 1 == count( $parts ) ) {
			$base = '?';
			$query = $parts[0];
		} else {
			$base = $parts[0] . '?';
			$query = $parts[1];
		}
	} elseif ( !empty( $protocol ) || strpos( $uri, '=' ) === false ) {
		$base = $uri . '?';
		$query = '';
	} else {
		$base = '';
		$query = $uri;
	}

	wp_parse_str( $query, $qs );
	$qs = urlencode_deep( $qs ); // this re-URL-encodes things that were already in the query string
	if ( is_array( func_get_arg( 0 ) ) ) {
		$kayvees = func_get_arg( 0 );
		$qs = array_merge( $qs, $kayvees );
	} else {
		$qs[func_get_arg( 0 )] = func_get_arg( 1 );
	}

	foreach ( (array) $qs as $k => $v ) {
		if ( $v === false )
			unset( $qs[$k] );
	}

	$ret = build_query( $qs );
	$ret = trim( $ret, '?' );
	$ret = preg_replace( '#=(&|$)#', '$1', $ret );
	$ret = $protocol . $base . $ret . $frag;
	$ret = rtrim( $ret, '?' );
	return $ret;
}

/**
 * Removes an item or list from the query string.
 *
 * @since 1.5.0
 *
 * @param string|array $key Query key or keys to remove.
 * @param bool $query When false uses the $_SERVER value.
 * @return string New URL query string.
 */
function remove_query_arg( $key, $query=false ) {
	if ( is_array( $key ) ) { // removing multiple keys
		foreach ( $key as $k )
			$query = add_query_arg( $k, false, $query );
		return $query;
	}
	return add_query_arg( $key, false, $query );
}

/**
 * Walks the array while sanitizing the contents.
 *
 * @since 0.71
 *
 * @param array $array Array to used to walk while sanitizing contents.
 * @return array Sanitized $array.
 */
function add_magic_quotes( $array ) {
	foreach ( (array) $array as $k => $v ) {
		if ( is_array( $v ) ) {
			$array[$k] = add_magic_quotes( $v );
		} else {
			$array[$k] = addslashes( $v );
		}
	}
	return $array;
}

/**
 * HTTP request for URI to retrieve content.
 *
 * @since 1.5.1
 * @uses wp_remote_get()
 *
 * @param string $uri URI/URL of web page to retrieve.
 * @return bool|string HTTP content. False on failure.
 */
function wp_remote_fopen( $uri ) {
	$parsed_url = @parse_url( $uri );

	if ( !$parsed_url || !is_array( $parsed_url ) )
		return false;

	$options = array();
	$options['timeout'] = 10;

	$response = wp_remote_get( $uri, $options );

	if ( is_wp_error( $response ) )
		return false;

	return wp_remote_retrieve_body( $response );
}

/**
 * Set up the WordPress query.
 *
 * @since 2.0.0
 *
 * @param string $query_vars Default WP_Query arguments.
 */
function wp( $query_vars = '' ) {
	global $wp, $wp_query, $wp_the_query;
	$wp->main( $query_vars );

	if ( !isset($wp_the_query) )
		$wp_the_query = $wp_query;
}

/**
 * Retrieve the description for the HTTP status.
 *
 * @since 2.3.0
 *
 * @param int $code HTTP status code.
 * @return string Empty string if not found, or description if found.
 */
function get_status_header_desc( $code ) {
	global $wp_header_to_desc;

	$code = absint( $code );

	if ( !isset( $wp_header_to_desc ) ) {
		$wp_header_to_desc = array(
			100 => 'Continue',
			101 => 'Switching Protocols',
			102 => 'Processing',

			200 => 'OK',
			201 => 'Created',
			202 => 'Accepted',
			203 => 'Non-Authoritative Information',
			204 => 'No Content',
			205 => 'Reset Content',
			206 => 'Partial Content',
			207 => 'Multi-Status',
			226 => 'IM Used',

			300 => 'Multiple Choices',
			301 => 'Moved Permanently',
			302 => 'Found',
			303 => 'See Other',
			304 => 'Not Modified',
			305 => 'Use Proxy',
			306 => 'Reserved',
			307 => 'Temporary Redirect',

			400 => 'Bad Request',
			401 => 'Unauthorized',
			402 => 'Payment Required',
			403 => 'Forbidden',
			404 => 'Not Found',
			405 => 'Method Not Allowed',
			406 => 'Not Acceptable',
			407 => 'Proxy Authentication Required',
			408 => 'Request Timeout',
			409 => 'Conflict',
			410 => 'Gone',
			411 => 'Length Required',
			412 => 'Precondition Failed',
			413 => 'Request Entity Too Large',
			414 => 'Request-URI Too Long',
			415 => 'Unsupported Media Type',
			416 => 'Requested Range Not Satisfiable',
			417 => 'Expectation Failed',
			422 => 'Unprocessable Entity',
			423 => 'Locked',
			424 => 'Failed Dependency',
			426 => 'Upgrade Required',

			500 => 'Internal Server Error',
			501 => 'Not Implemented',
			502 => 'Bad Gateway',
			503 => 'Service Unavailable',
			504 => 'Gateway Timeout',
			505 => 'HTTP Version Not Supported',
			506 => 'Variant Also Negotiates',
			507 => 'Insufficient Storage',
			510 => 'Not Extended'
		);
	}

	if ( isset( $wp_header_to_desc[$code] ) )
		return $wp_header_to_desc[$code];
	else
		return '';
}

/**
 * Set HTTP status header.
 *
 * @since 2.0.0
 * @uses apply_filters() Calls 'status_header' on status header string, HTTP
 *		HTTP code, HTTP code description, and protocol string as separate
 *		parameters.
 *
 * @param int $header HTTP status code
 * @return unknown
 */
function status_header( $header ) {
	$text = get_status_header_desc( $header );

	if ( empty( $text ) )
		return false;

	$protocol = $_SERVER["SERVER_PROTOCOL"];
	if ( 'HTTP/1.1' != $protocol && 'HTTP/1.0' != $protocol )
		$protocol = 'HTTP/1.0';
	$status_header = "$protocol $header $text";
	if ( function_exists( 'apply_filters' ) )
		$status_header = apply_filters( 'status_header', $status_header, $header, $text, $protocol );

	return @header( $status_header, true, $header );
}

/**
 * Gets the header information to prevent caching.
 *
 * The several different headers cover the different ways cache prevention is handled
 * by different browsers
 *
 * @since 2.8.0
 *
 * @uses apply_filters()
 * @return array The associative array of header names and field values.
 */
function wp_get_nocache_headers() {
	$headers = array(
		'Expires' => 'Wed, 11 Jan 1984 05:00:00 GMT',
		'Last-Modified' => gmdate( 'D, d M Y H:i:s' ) . ' GMT',
		'Cache-Control' => 'no-cache, must-revalidate, max-age=0',
		'Pragma' => 'no-cache',
	);

	if ( function_exists('apply_filters') ) {
		$headers = (array) apply_filters('nocache_headers', $headers);
	}
	return $headers;
}

/**
 * Sets the headers to prevent caching for the different browsers.
 *
 * Different browsers support different nocache headers, so several headers must
 * be sent so that all of them get the point that no caching should occur.
 *
 * @since 2.0.0
 * @uses wp_get_nocache_headers()
 */
function nocache_headers() {
	$headers = wp_get_nocache_headers();
	foreach( $headers as $name => $field_value )
		@header("{$name}: {$field_value}");
}

/**
 * Set the headers for caching for 10 days with JavaScript content type.
 *
 * @since 2.1.0
 */
function cache_javascript_headers() {
	$expiresOffset = 864000; // 10 days
	header( "Content-Type: text/javascript; charset=" . get_bloginfo( 'charset' ) );
	header( "Vary: Accept-Encoding" ); // Handle proxies
	header( "Expires: " . gmdate( "D, d M Y H:i:s", time() + $expiresOffset ) . " GMT" );
}

/**
 * Retrieve the number of database queries during the WordPress execution.
 *
 * @since 2.0.0
 *
 * @return int Number of database queries
 */
function get_num_queries() {
	global $wpdb;
	return $wpdb->num_queries;
}

/**
 * Whether input is yes or no. Must be 'y' to be true.
 *
 * @since 1.0.0
 *
 * @param string $yn Character string containing either 'y' or 'n'
 * @return bool True if yes, false on anything else
 */
function bool_from_yn( $yn ) {
	return ( strtolower( $yn ) == 'y' );
}

/**
 * Loads the feed template from the use of an action hook.
 *
 * If the feed action does not have a hook, then the function will die with a
 * message telling the visitor that the feed is not valid.
 *
 * It is better to only have one hook for each feed.
 *
 * @since 2.1.0
 * @uses $wp_query Used to tell if the use a comment feed.
 * @uses do_action() Calls 'do_feed_$feed' hook, if a hook exists for the feed.
 */
function do_feed() {
	global $wp_query;

	$feed = get_query_var( 'feed' );

	// Remove the pad, if present.
	$feed = preg_replace( '/^_+/', '', $feed );

	if ( $feed == '' || $feed == 'feed' )
		$feed = get_default_feed();

	$hook = 'do_feed_' . $feed;
	if ( !has_action($hook) ) {
		$message = sprintf( __( 'ERROR: %s is not a valid feed template.' ), esc_html($feed));
		wp_die( $message, '', array( 'response' => 404 ) );
	}

	do_action( $hook, $wp_query->is_comment_feed );
}

/**
 * Load the RDF RSS 0.91 Feed template.
 *
 * @since 2.1.0
 */
function do_feed_rdf() {
	load_template( ABSPATH . WPINC . '/feed-rdf.php' );
}

/**
 * Load the RSS 1.0 Feed Template
 *
 * @since 2.1.0
 */
function do_feed_rss() {
	load_template( ABSPATH . WPINC . '/feed-rss.php' );
}

/**
 * Load either the RSS2 comment feed or the RSS2 posts feed.
 *
 * @since 2.1.0
 *
 * @param bool $for_comments True for the comment feed, false for normal feed.
 */
function do_feed_rss2( $for_comments ) {
	if ( $for_comments )
		load_template( ABSPATH . WPINC . '/feed-rss2-comments.php' );
	else
		load_template( ABSPATH . WPINC . '/feed-rss2.php' );
}

/**
 * Load either Atom comment feed or Atom posts feed.
 *
 * @since 2.1.0
 *
 * @param bool $for_comments True for the comment feed, false for normal feed.
 */
function do_feed_atom( $for_comments ) {
	if ($for_comments)
		load_template( ABSPATH . WPINC . '/feed-atom-comments.php');
	else
		load_template( ABSPATH . WPINC . '/feed-atom.php' );
}

/**
 * Display the robot.txt file content.
 *
 * The echo content should be with usage of the permalinks or for creating the
 * robot.txt file.
 *
 * @since 2.1.0
 * @uses do_action() Calls 'do_robotstxt' hook for displaying robot.txt rules.
 */
function do_robots() {
	header( 'Content-Type: text/plain; charset=utf-8' );

	do_action( 'do_robotstxt' );

	$output = '';
	$public = get_option( 'blog_public' );
	if ( '0' ==  $public ) {
		$output .= "User-agent: *\n";
		$output .= "Disallow: /\n";
	} else {
		$output .= "User-agent: *\n";
		$output .= "Disallow:\n";
	}

	echo apply_filters('robots_txt', $output, $public);
}

/**
 * Test whether blog is already installed.
 *
 * The cache will be checked first. If you have a cache plugin, which saves the
 * cache values, then this will work. If you use the default WordPress cache,
 * and the database goes away, then you might have problems.
 *
 * Checks for the option siteurl for whether WordPress is installed.
 *
 * @since 2.1.0
 * @uses $wpdb
 *
 * @return bool Whether blog is already installed.
 */
function is_blog_installed() {
	global $wpdb;

	// Check cache first. If options table goes away and we have true cached, oh well.
	if ( wp_cache_get( 'is_blog_installed' ) )
		return true;

	$suppress = $wpdb->suppress_errors();
	if ( ! defined( 'WP_INSTALLING' ) ) {
		$alloptions = wp_load_alloptions();
	}
	// If siteurl is not set to autoload, check it specifically
	if ( !isset( $alloptions['siteurl'] ) )
		$installed = $wpdb->get_var( "SELECT option_value FROM $wpdb->options WHERE option_name = 'siteurl'" );
	else
		$installed = $alloptions['siteurl'];
	$wpdb->suppress_errors( $suppress );

	$installed = !empty( $installed );
	wp_cache_set( 'is_blog_installed', $installed );

	if ( $installed )
		return true;

	$suppress = $wpdb->suppress_errors();
	$tables = $wpdb->get_col('SHOW TABLES');
	$wpdb->suppress_errors( $suppress );

	$wp_tables = $wpdb->tables();
	// Loop over the WP tables.  If none exist, then scratch install is allowed.
	// If one or more exist, suggest table repair since we got here because the options
	// table could not be accessed.
	foreach ( $wp_tables as $table ) {
		// If one of the WP tables exist, then we are in an insane state.
		if ( in_array( $table, $tables ) ) {
			// The existence of custom user tables shouldn't suggest an insane state or prevent a clean install.
			if ( defined( 'CUSTOM_USER_TABLE' ) && CUSTOM_USER_TABLE == $table )
				continue;
			if ( defined( 'CUSTOM_USER_META_TABLE' ) && CUSTOM_USER_META_TABLE == $table )
				continue;

			// If visiting repair.php, return true and let it take over.
			if ( defined('WP_REPAIRING') )
				return true;
			// Die with a DB error.
			$wpdb->error = sprintf( /*WP_I18N_NO_TABLES*/'One or more database tables are unavailable.  The database may need to be <a href="%s">repaired</a>.'/*/WP_I18N_NO_TABLES*/, 'maint/repair.php?referrer=is_blog_installed' );
			dead_db();
		}
	}

	wp_cache_set( 'is_blog_installed', false );

	return false;
}

/**
 * Retrieve URL with nonce added to URL query.
 *
 * @package WordPress
 * @subpackage Security
 * @since 2.0.4
 *
 * @param string $actionurl URL to add nonce action
 * @param string $action Optional. Nonce action name
 * @return string URL with nonce action added.
 */
function wp_nonce_url( $actionurl, $action = -1 ) {
	$actionurl = str_replace( '&amp;', '&', $actionurl );
	return esc_html( add_query_arg( '_wpnonce', wp_create_nonce( $action ), $actionurl ) );
}

/**
 * Retrieve or display nonce hidden field for forms.
 *
 * The nonce field is used to validate that the contents of the form came from
 * the location on the current site and not somewhere else. The nonce does not
 * offer absolute protection, but should protect against most cases. It is very
 * important to use nonce field in forms.
 *
 * The $action and $name are optional, but if you want to have better security,
 * it is strongly suggested to set those two parameters. It is easier to just
 * call the function without any parameters, because validation of the nonce
 * doesn't require any parameters, but since crackers know what the default is
 * it won't be difficult for them to find a way around your nonce and cause
 * damage.
 *
 * The input name will be whatever $name value you gave. The input value will be
 * the nonce creation value.
 *
 * @package WordPress
 * @subpackage Security
 * @since 2.0.4
 *
 * @param string $action Optional. Action name.
 * @param string $name Optional. Nonce name.
 * @param bool $referer Optional, default true. Whether to set the referer field for validation.
 * @param bool $echo Optional, default true. Whether to display or return hidden form field.
 * @return string Nonce field.
 */
function wp_nonce_field( $action = -1, $name = "_wpnonce", $referer = true , $echo = true ) {
	$name = esc_attr( $name );
	$nonce_field = '<input type="hidden" id="' . $name . '" name="' . $name . '" value="' . wp_create_nonce( $action ) . '" />';

	if ( $referer )
		$nonce_field .= wp_referer_field( false );

	if ( $echo )
		echo $nonce_field;

	return $nonce_field;
}

/**
 * Retrieve or display referer hidden field for forms.
 *
 * The referer link is the current Request URI from the server super global. The
 * input name is '_wp_http_referer', in case you wanted to check manually.
 *
 * @package WordPress
 * @subpackage Security
 * @since 2.0.4
 *
 * @param bool $echo Whether to echo or return the referer field.
 * @return string Referer field.
 */
function wp_referer_field( $echo = true ) {
	$ref = esc_attr( $_SERVER['REQUEST_URI'] );
	$referer_field = '<input type="hidden" name="_wp_http_referer" value="'. $ref . '" />';

	if ( $echo )
		echo $referer_field;
	return $referer_field;
}

/**
 * Retrieve or display original referer hidden field for forms.
 *
 * The input name is '_wp_original_http_referer' and will be either the same
 * value of {@link wp_referer_field()}, if that was posted already or it will
 * be the current page, if it doesn't exist.
 *
 * @package WordPress
 * @subpackage Security
 * @since 2.0.4
 *
 * @param bool $echo Whether to echo the original http referer
 * @param string $jump_back_to Optional, default is 'current'. Can be 'previous' or page you want to jump back to.
 * @return string Original referer field.
 */
function wp_original_referer_field( $echo = true, $jump_back_to = 'current' ) {
	$jump_back_to = ( 'previous' == $jump_back_to ) ? wp_get_referer() : $_SERVER['REQUEST_URI'];
	$ref = ( wp_get_original_referer() ) ? wp_get_original_referer() : $jump_back_to;
	$orig_referer_field = '<input type="hidden" name="_wp_original_http_referer" value="' . esc_attr( stripslashes( $ref ) ) . '" />';
	if ( $echo )
		echo $orig_referer_field;
	return $orig_referer_field;
}

/**
 * Retrieve referer from '_wp_http_referer', HTTP referer, or current page respectively.
 *
 * @package WordPress
 * @subpackage Security
 * @since 2.0.4
 *
 * @return string|bool False on failure. Referer URL on success.
 */
function wp_get_referer() {
	$ref = '';
	if ( ! empty( $_REQUEST['_wp_http_referer'] ) )
		$ref = $_REQUEST['_wp_http_referer'];
	else if ( ! empty( $_SERVER['HTTP_REFERER'] ) )
		$ref = $_SERVER['HTTP_REFERER'];

	if ( $ref !== $_SERVER['REQUEST_URI'] )
		return $ref;
	return false;
}

/**
 * Retrieve original referer that was posted, if it exists.
 *
 * @package WordPress
 * @subpackage Security
 * @since 2.0.4
 *
 * @return string|bool False if no original referer or original referer if set.
 */
function wp_get_original_referer() {
	if ( !empty( $_REQUEST['_wp_original_http_referer'] ) )
		return $_REQUEST['_wp_original_http_referer'];
	return false;
}

/**
 * Recursive directory creation based on full path.
 *
 * Will attempt to set permissions on folders.
 *
 * @since 2.0.1
 *
 * @param string $target Full path to attempt to create.
 * @return bool Whether the path was created. True if path already exists.
 */
function wp_mkdir_p( $target ) {
	// from php.net/mkdir user contributed notes
	$target = str_replace( '//', '/', $target );

	// safe mode fails with a trailing slash under certain PHP versions.
	$target = rtrim($target, '/'); // Use rtrim() instead of untrailingslashit to avoid formatting.php dependency.
	if ( empty($target) )
		$target = '/';

	if ( file_exists( $target ) )
		return @is_dir( $target );

	// Attempting to create the directory may clutter up our display.
	if ( @mkdir( $target ) ) {
		$stat = @stat( dirname( $target ) );
		$dir_perms = $stat['mode'] & 0007777;  // Get the permission bits.
		@chmod( $target, $dir_perms );
		return true;
	} elseif ( is_dir( dirname( $target ) ) ) {
			return false;
	}

	// If the above failed, attempt to create the parent node, then try again.
	if ( ( $target != '/' ) && ( wp_mkdir_p( dirname( $target ) ) ) )
		return wp_mkdir_p( $target );

	return false;
}

/**
 * Test if a give filesystem path is absolute ('/foo/bar', 'c:\windows').
 *
 * @since 2.5.0
 *
 * @param string $path File path
 * @return bool True if path is absolute, false is not absolute.
 */
function path_is_absolute( $path ) {
	// this is definitive if true but fails if $path does not exist or contains a symbolic link
	if ( realpath($path) == $path )
		return true;

	if ( strlen($path) == 0 || $path[0] == '.' )
		return false;

	// windows allows absolute paths like this
	if ( preg_match('#^[a-zA-Z]:\\\\#', $path) )
		return true;

	// a path starting with / or \ is absolute; anything else is relative
	return (bool) preg_match('#^[/\\\\]#', $path);
}

/**
 * Join two filesystem paths together (e.g. 'give me $path relative to $base').
 *
 * If the $path is absolute, then it the full path is returned.
 *
 * @since 2.5.0
 *
 * @param string $base
 * @param string $path
 * @return string The path with the base or absolute path.
 */
function path_join( $base, $path ) {
	if ( path_is_absolute($path) )
		return $path;

	return rtrim($base, '/') . '/' . ltrim($path, '/');
}

/**
 * Determines a writable directory for temporary files.
 * Function's preference is to WP_CONTENT_DIR followed by the return value of <code>sys_get_temp_dir()</code>, before finally defaulting to /tmp/
 *
 * In the event that this function does not find a writable location, It may be overridden by the <code>WP_TEMP_DIR</code> constant in your <code>wp-config.php</code> file.
 *
 * @since 2.5.0
 *
 * @return string Writable temporary directory
 */
function get_temp_dir() {
	static $temp;
	if ( defined('WP_TEMP_DIR') )
		return trailingslashit(WP_TEMP_DIR);

	if ( $temp )
		return trailingslashit($temp);

	$temp = WP_CONTENT_DIR . '/';
	if ( is_dir($temp) && @is_writable($temp) )
		return $temp;

	if  ( function_exists('sys_get_temp_dir') ) {
		$temp = sys_get_temp_dir();
		if ( @is_writable($temp) )
			return trailingslashit($temp);
	}

	$temp = ini_get('upload_tmp_dir');
	if ( is_dir($temp) && @is_writable($temp) )
		return trailingslashit($temp);

	$temp = '/tmp/';
	return $temp;
}

/**
 * Get an array containing the current upload directory's path and url.
 *
 * Checks the 'upload_path' option, which should be from the web root folder,
 * and if it isn't empty it will be used. If it is empty, then the path will be
 * 'WP_CONTENT_DIR/uploads'. If the 'UPLOADS' constant is defined, then it will
 * override the 'upload_path' option and 'WP_CONTENT_DIR/uploads' path.
 *
 * The upload URL path is set either by the 'upload_url_path' option or by using
 * the 'WP_CONTENT_URL' constant and appending '/uploads' to the path.
 *
 * If the 'uploads_use_yearmonth_folders' is set to true (checkbox if checked in
 * the administration settings panel), then the time will be used. The format
 * will be year first and then month.
 *
 * If the path couldn't be created, then an error will be returned with the key
 * 'error' containing the error message. The error suggests that the parent
 * directory is not writable by the server.
 *
 * On success, the returned array will have many indices:
 * 'path' - base directory and sub directory or full path to upload directory.
 * 'url' - base url and sub directory or absolute URL to upload directory.
 * 'subdir' - sub directory if uploads use year/month folders option is on.
 * 'basedir' - path without subdir.
 * 'baseurl' - URL path without subdir.
 * 'error' - set to false.
 *
 * @since 2.0.0
 * @uses apply_filters() Calls 'upload_dir' on returned array.
 *
 * @param string $time Optional. Time formatted in 'yyyy/mm'.
 * @return array See above for description.
 */
function wp_upload_dir( $time = null ) {
	global $switched;
	$siteurl = get_option( 'siteurl' );
	$upload_path = get_option( 'upload_path' );
	$upload_path = trim($upload_path);
	$main_override = is_multisite() && defined( 'MULTISITE' ) && is_main_site();
	if ( empty($upload_path) ) {
		$dir = WP_CONTENT_DIR . '/uploads';
	} else {
		$dir = $upload_path;
		if ( 'wp-content/uploads' == $upload_path ) {
			$dir = WP_CONTENT_DIR . '/uploads';
		} elseif ( 0 !== strpos($dir, ABSPATH) ) {
			// $dir is absolute, $upload_path is (maybe) relative to ABSPATH
			$dir = path_join( ABSPATH, $dir );
		}
	}

	if ( !$url = get_option( 'upload_url_path' ) ) {
		if ( empty($upload_path) || ( 'wp-content/uploads' == $upload_path ) || ( $upload_path == $dir ) )
			$url = WP_CONTENT_URL . '/uploads';
		else
			$url = trailingslashit( $siteurl ) . $upload_path;
	}

	if ( defined('UPLOADS') && !$main_override && ( !isset( $switched ) || $switched === false ) ) {
		$dir = ABSPATH . UPLOADS;
		$url = trailingslashit( $siteurl ) . UPLOADS;
	}

	if ( is_multisite() && !$main_override && ( !isset( $switched ) || $switched === false ) ) {
		if ( defined( 'BLOGUPLOADDIR' ) )
			$dir = untrailingslashit(BLOGUPLOADDIR);
		$url = str_replace( UPLOADS, 'files', $url );
	}

	$bdir = $dir;
	$burl = $url;

	$subdir = '';
	if ( get_option( 'uploads_use_yearmonth_folders' ) ) {
		// Generate the yearly and monthly dirs
		if ( !$time )
			$time = current_time( 'mysql' );
		$y = substr( $time, 0, 4 );
		$m = substr( $time, 5, 2 );
		$subdir = "/$y/$m";
	}

	$dir .= $subdir;
	$url .= $subdir;

	$uploads = apply_filters( 'upload_dir', array( 'path' => $dir, 'url' => $url, 'subdir' => $subdir, 'basedir' => $bdir, 'baseurl' => $burl, 'error' => false ) );

	// Make sure we have an uploads dir
	if ( ! wp_mkdir_p( $uploads['path'] ) ) {
		$message = sprintf( __( 'Unable to create directory %s. Is its parent directory writable by the server?' ), $uploads['path'] );
		return array( 'error' => $message );
	}

	return $uploads;
}

/**
 * Get a filename that is sanitized and unique for the given directory.
 *
 * If the filename is not unique, then a number will be added to the filename
 * before the extension, and will continue adding numbers until the filename is
 * unique.
 *
 * The callback is passed three parameters, the first one is the directory, the
 * second is the filename, and the third is the extension.
 *
 * @since 2.5.0
 *
 * @param string $dir
 * @param string $filename
 * @param mixed $unique_filename_callback Callback.
 * @return string New filename, if given wasn't unique.
 */
function wp_unique_filename( $dir, $filename, $unique_filename_callback = null ) {
	// sanitize the file name before we begin processing
	$filename = sanitize_file_name($filename);

	// separate the filename into a name and extension
	$info = pathinfo($filename);
	$ext = !empty($info['extension']) ? '.' . $info['extension'] : '';
	$name = basename($filename, $ext);

	// edge case: if file is named '.ext', treat as an empty name
	if ( $name === $ext )
		$name = '';

	// Increment the file number until we have a unique file to save in $dir. Use callback if supplied.
	if ( $unique_filename_callback && is_callable( $unique_filename_callback ) ) {
		$filename = call_user_func( $unique_filename_callback, $dir, $name, $ext );
	} else {
		$number = '';

		// change '.ext' to lower case
		if ( $ext && strtolower($ext) != $ext ) {
			$ext2 = strtolower($ext);
			$filename2 = preg_replace( '|' . preg_quote($ext) . '$|', $ext2, $filename );

			// check for both lower and upper case extension or image sub-sizes may be overwritten
			while ( file_exists($dir . "/$filename") || file_exists($dir . "/$filename2") ) {
				$new_number = $number + 1;
				$filename = str_replace( "$number$ext", "$new_number$ext", $filename );
				$filename2 = str_replace( "$number$ext2", "$new_number$ext2", $filename2 );
				$number = $new_number;
			}
			return $filename2;
		}

		while ( file_exists( $dir . "/$filename" ) ) {
			if ( '' == "$number$ext" )
				$filename = $filename . ++$number . $ext;
			else
				$filename = str_replace( "$number$ext", ++$number . $ext, $filename );
		}
	}

	return $filename;
}

/**
 * Create a file in the upload folder with given content.
 *
 * If there is an error, then the key 'error' will exist with the error message.
 * If success, then the key 'file' will have the unique file path, the 'url' key
 * will have the link to the new file. and the 'error' key will be set to false.
 *
 * This function will not move an uploaded file to the upload folder. It will
 * create a new file with the content in $bits parameter. If you move the upload
 * file, read the content of the uploaded file, and then you can give the
 * filename and content to this function, which will add it to the upload
 * folder.
 *
 * The permissions will be set on the new file automatically by this function.
 *
 * @since 2.0.0
 *
 * @param string $name
 * @param null $deprecated Never used. Set to null.
 * @param mixed $bits File content
 * @param string $time Optional. Time formatted in 'yyyy/mm'.
 * @return array
 */
function wp_upload_bits( $name, $deprecated, $bits, $time = null ) {
	if ( !empty( $deprecated ) )
		_deprecated_argument( __FUNCTION__, '2.0' );

	if ( empty( $name ) )
		return array( 'error' => __( 'Empty filename' ) );

	$wp_filetype = wp_check_filetype( $name );
	if ( !$wp_filetype['ext'] )
		return array( 'error' => __( 'Invalid file type' ) );

	$upload = wp_upload_dir( $time );

	if ( $upload['error'] !== false )
		return $upload;

	$upload_bits_error = apply_filters( 'wp_upload_bits', array( 'name' => $name, 'bits' => $bits, 'time' => $time ) );
	if ( !is_array( $upload_bits_error ) ) {
		$upload[ 'error' ] = $upload_bits_error;
		return $upload;
	}

	$filename = wp_unique_filename( $upload['path'], $name );

	$new_file = $upload['path'] . "/$filename";
	if ( ! wp_mkdir_p( dirname( $new_file ) ) ) {
		$message = sprintf( __( 'Unable to create directory %s. Is its parent directory writable by the server?' ), dirname( $new_file ) );
		return array( 'error' => $message );
	}

	$ifp = @ fopen( $new_file, 'wb' );
	if ( ! $ifp )
		return array( 'error' => sprintf( __( 'Could not write file %s' ), $new_file ) );

	@fwrite( $ifp, $bits );
	fclose( $ifp );
	clearstatcache();

	// Set correct file permissions
	$stat = @ stat( dirname( $new_file ) );
	$perms = $stat['mode'] & 0007777;
	$perms = $perms & 0000666;
	@ chmod( $new_file, $perms );
	clearstatcache();

	// Compute the URL
	$url = $upload['url'] . "/$filename";

	return array( 'file' => $new_file, 'url' => $url, 'error' => false );
}

/**
 * Retrieve the file type based on the extension name.
 *
 * @package WordPress
 * @since 2.5.0
 * @uses apply_filters() Calls 'ext2type' hook on default supported types.
 *
 * @param string $ext The extension to search.
 * @return string|null The file type, example: audio, video, document, spreadsheet, etc. Null if not found.
 */
function wp_ext2type( $ext ) {
	$ext2type = apply_filters( 'ext2type', array(
		'audio'       => array( 'aac', 'ac3',  'aif',  'aiff', 'm3a',  'm4a',   'm4b', 'mka', 'mp1', 'mp2',  'mp3', 'ogg', 'oga', 'ram', 'wav', 'wma' ),
		'video'       => array( 'asf', 'avi',  'divx', 'dv',   'flv',  'm4v',   'mkv', 'mov', 'mp4', 'mpeg', 'mpg', 'mpv', 'ogm', 'ogv', 'qt',  'rm', 'vob', 'wmv' ),
		'document'    => array( 'doc', 'docx', 'docm', 'dotm', 'odt',  'pages', 'pdf', 'rtf', 'wp',  'wpd' ),
		'spreadsheet' => array( 'numbers',     'ods',  'xls',  'xlsx', 'xlsb',  'xlsm' ),
		'interactive' => array( 'key', 'ppt',  'pptx', 'pptm', 'odp',  'swf' ),
		'text'        => array( 'asc', 'csv',  'tsv',  'txt' ),
		'archive'     => array( 'bz2', 'cab',  'dmg',  'gz',   'rar',  'sea',   'sit', 'sqx', 'tar', 'tgz',  'zip' ),
		'code'        => array( 'css', 'htm',  'html', 'php',  'js' ),
	));
	foreach ( $ext2type as $type => $exts )
		if ( in_array( $ext, $exts ) )
			return $type;
}

/**
 * Retrieve the file type from the file name.
 *
 * You can optionally define the mime array, if needed.
 *
 * @since 2.0.4
 *
 * @param string $filename File name or path.
 * @param array $mimes Optional. Key is the file extension with value as the mime type.
 * @return array Values with extension first and mime type.
 */
function wp_check_filetype( $filename, $mimes = null ) {
	if ( empty($mimes) )
		$mimes = get_allowed_mime_types();
	$type = false;
	$ext = false;

	foreach ( $mimes as $ext_preg => $mime_match ) {
		$ext_preg = '!\.(' . $ext_preg . ')$!i';
		if ( preg_match( $ext_preg, $filename, $ext_matches ) ) {
			$type = $mime_match;
			$ext = $ext_matches[1];
			break;
		}
	}

	return compact( 'ext', 'type' );
}

/**
 * Attempt to determine the real file type of a file.
 * If unable to, the file name extension will be used to determine type.
 *
 * If it's determined that the extension does not match the file's real type,
 * then the "proper_filename" value will be set with a proper filename and extension.
 *
 * Currently this function only supports validating images known to getimagesize().
 *
 * @since 3.0.0
 *
 * @param string $file Full path to the image.
 * @param string $filename The filename of the image (may differ from $file due to $file being in a tmp directory)
 * @param array $mimes Optional. Key is the file extension with value as the mime type.
 * @return array Values for the extension, MIME, and either a corrected filename or false if original $filename is valid
 */
function wp_check_filetype_and_ext( $file, $filename, $mimes = null ) {

	$proper_filename = false;

	// Do basic extension validation and MIME mapping
	$wp_filetype = wp_check_filetype( $filename, $mimes );
	extract( $wp_filetype );

	// We can't do any further validation without a file to work with
	if ( ! file_exists( $file ) )
		return compact( 'ext', 'type', 'proper_filename' );

	// We're able to validate images using GD
	if ( $type && 0 === strpos( $type, 'image/' ) && function_exists('getimagesize') ) {

		// Attempt to figure out what type of image it actually is
		$imgstats = @getimagesize( $file );

		// If getimagesize() knows what kind of image it really is and if the real MIME doesn't match the claimed MIME
		if ( !empty($imgstats['mime']) && $imgstats['mime'] != $type ) {
			// This is a simplified array of MIMEs that getimagesize() can detect and their extensions
			// You shouldn't need to use this filter, but it's here just in case
			$mime_to_ext = apply_filters( 'getimagesize_mimes_to_exts', array(
				'image/jpeg' => 'jpg',
				'image/png'  => 'png',
				'image/gif'  => 'gif',
				'image/bmp'  => 'bmp',
				'image/tiff' => 'tif',
			) );

			// Replace whatever is after the last period in the filename with the correct extension
			if ( ! empty( $mime_to_ext[ $imgstats['mime'] ] ) ) {
				$filename_parts = explode( '.', $filename );
				array_pop( $filename_parts );
				$filename_parts[] = $mime_to_ext[ $imgstats['mime'] ];
				$new_filename = implode( '.', $filename_parts );

				if ( $new_filename != $filename )
					$proper_filename = $new_filename; // Mark that it changed

				// Redefine the extension / MIME
				$wp_filetype = wp_check_filetype( $new_filename, $mimes );
				extract( $wp_filetype );
			}
		}
	}

	// Let plugins try and validate other types of files
	// Should return an array in the style of array( 'ext' => $ext, 'type' => $type, 'proper_filename' => $proper_filename )
	return apply_filters( 'wp_check_filetype_and_ext', compact( 'ext', 'type', 'proper_filename' ), $file, $filename, $mimes );
}

/**
 * Retrieve list of allowed mime types and file extensions.
 *
 * @since 2.8.6
 *
 * @return array Array of mime types keyed by the file extension regex corresponding to those types.
 */
function get_allowed_mime_types() {
	static $mimes = false;

	if ( !$mimes ) {
		// Accepted MIME types are set here as PCRE unless provided.
		$mimes = apply_filters( 'upload_mimes', array(
		'jpg|jpeg|jpe' => 'image/jpeg',
		'gif' => 'image/gif',
		'png' => 'image/png',
		'bmp' => 'image/bmp',
		'tif|tiff' => 'image/tiff',
		'ico' => 'image/x-icon',
		'asf|asx|wax|wmv|wmx' => 'video/asf',
		'avi' => 'video/avi',
		'divx' => 'video/divx',
		'flv' => 'video/x-flv',
		'mov|qt' => 'video/quicktime',
		'mpeg|mpg|mpe' => 'video/mpeg',
		'txt|asc|c|cc|h' => 'text/plain',
		'csv' => 'text/csv',
		'tsv' => 'text/tab-separated-values',
		'ics' => 'text/calendar',
		'rtx' => 'text/richtext',
		'css' => 'text/css',
		'htm|html' => 'text/html',
		'mp3|m4a|m4b' => 'audio/mpeg',
		'mp4|m4v' => 'video/mp4',
		'ra|ram' => 'audio/x-realaudio',
		'wav' => 'audio/wav',
		'ogg|oga' => 'audio/ogg',
		'ogv' => 'video/ogg',
		'mid|midi' => 'audio/midi',
		'wma' => 'audio/wma',
		'mka' => 'audio/x-matroska',
		'mkv' => 'video/x-matroska',
		'rtf' => 'application/rtf',
		'js' => 'application/javascript',
		'pdf' => 'application/pdf',
		'doc|docx' => 'application/msword',
		'pot|pps|ppt|pptx|ppam|pptm|sldm|ppsm|potm' => 'application/vnd.ms-powerpoint',
		'wri' => 'application/vnd.ms-write',
		'xla|xls|xlsx|xlt|xlw|xlam|xlsb|xlsm|xltm' => 'application/vnd.ms-excel',
		'mdb' => 'application/vnd.ms-access',
		'mpp' => 'application/vnd.ms-project',
		'docm|dotm' => 'application/vnd.ms-word',
		'pptx|sldx|ppsx|potx' => 'application/vnd.openxmlformats-officedocument.presentationml',
		'xlsx|xltx' => 'application/vnd.openxmlformats-officedocument.spreadsheetml',
		'docx|dotx' => 'application/vnd.openxmlformats-officedocument.wordprocessingml',
		'onetoc|onetoc2|onetmp|onepkg' => 'application/onenote',
		'swf' => 'application/x-shockwave-flash',
		'class' => 'application/java',
		'tar' => 'application/x-tar',
		'zip' => 'application/zip',
		'gz|gzip' => 'application/x-gzip',
		'exe' => 'application/x-msdownload',
		// openoffice formats
		'odt' => 'application/vnd.oasis.opendocument.text',
		'odp' => 'application/vnd.oasis.opendocument.presentation',
		'ods' => 'application/vnd.oasis.opendocument.spreadsheet',
		'odg' => 'application/vnd.oasis.opendocument.graphics',
		'odc' => 'application/vnd.oasis.opendocument.chart',
		'odb' => 'application/vnd.oasis.opendocument.database',
		'odf' => 'application/vnd.oasis.opendocument.formula',
		// wordperfect formats
		'wp|wpd' => 'application/wordperfect',
		) );
	}

	return $mimes;
}

/**
 * Retrieve nonce action "Are you sure" message.
 *
 * The action is split by verb and noun. The action format is as follows:
 * verb-action_extra. The verb is before the first dash and has the format of
 * letters and no spaces and numbers. The noun is after the dash and before the
 * underscore, if an underscore exists. The noun is also only letters.
 *
 * The filter will be called for any action, which is not defined by WordPress.
 * You may use the filter for your plugin to explain nonce actions to the user,
 * when they get the "Are you sure?" message. The filter is in the format of
 * 'explain_nonce_$verb-$noun' with the $verb replaced by the found verb and the
 * $noun replaced by the found noun. The two parameters that are given to the
 * hook are the localized "Are you sure you want to do this?" message with the
 * extra text (the text after the underscore).
 *
 * @package WordPress
 * @subpackage Security
 * @since 2.0.4
 *
 * @param string $action Nonce action.
 * @return string Are you sure message.
 */
function wp_explain_nonce( $action ) {
	if ( $action !== -1 && preg_match( '/([a-z]+)-([a-z]+)(_(.+))?/', $action, $matches ) ) {
		$verb = $matches[1];
		$noun = $matches[2];

		$trans = array();
		$trans['update']['attachment'] = array( __( 'Your attempt to edit this attachment: &#8220;%s&#8221; has failed.' ), 'get_the_title' );

		$trans['add']['category']      = array( __( 'Your attempt to add this category has failed.' ), false );
		$trans['delete']['category']   = array( __( 'Your attempt to delete this category: &#8220;%s&#8221; has failed.' ), 'get_cat_name' );
		$trans['update']['category']   = array( __( 'Your attempt to edit this category: &#8220;%s&#8221; has failed.' ), 'get_cat_name' );

		$trans['delete']['comment']    = array( __( 'Your attempt to delete this comment: &#8220;%s&#8221; has failed.' ), 'use_id' );
		$trans['unapprove']['comment'] = array( __( 'Your attempt to unapprove this comment: &#8220;%s&#8221; has failed.' ), 'use_id' );
		$trans['approve']['comment']   = array( __( 'Your attempt to approve this comment: &#8220;%s&#8221; has failed.' ), 'use_id' );
		$trans['update']['comment']    = array( __( 'Your attempt to edit this comment: &#8220;%s&#8221; has failed.' ), 'use_id' );
		$trans['bulk']['comments']     = array( __( 'Your attempt to bulk modify comments has failed.' ), false );
		$trans['moderate']['comments'] = array( __( 'Your attempt to moderate comments has failed.' ), false );

		$trans['add']['bookmark']      = array( __( 'Your attempt to add this link has failed.' ), false );
		$trans['delete']['bookmark']   = array( __( 'Your attempt to delete this link: &#8220;%s&#8221; has failed.' ), 'use_id' );
		$trans['update']['bookmark']   = array( __( 'Your attempt to edit this link: &#8220;%s&#8221; has failed.' ), 'use_id' );
		$trans['bulk']['bookmarks']    = array( __( 'Your attempt to bulk modify links has failed.' ), false );

		$trans['add']['page']          = array( __( 'Your attempt to add this page has failed.' ), false );
		$trans['delete']['page']       = array( __( 'Your attempt to delete this page: &#8220;%s&#8221; has failed.' ), 'get_the_title' );
		$trans['update']['page']       = array( __( 'Your attempt to edit this page: &#8220;%s&#8221; has failed.' ), 'get_the_title' );

		$trans['edit']['plugin']       = array( __( 'Your attempt to edit this plugin file: &#8220;%s&#8221; has failed.' ), 'use_id' );
		$trans['activate']['plugin']   = array( __( 'Your attempt to activate this plugin: &#8220;%s&#8221; has failed.' ), 'use_id' );
		$trans['deactivate']['plugin'] = array( __( 'Your attempt to deactivate this plugin: &#8220;%s&#8221; has failed.' ), 'use_id' );
		$trans['upgrade']['plugin']    = array( __( 'Your attempt to update this plugin: &#8220;%s&#8221; has failed.' ), 'use_id' );

		$trans['add']['post']          = array( __( 'Your attempt to add this post has failed.' ), false );
		$trans['delete']['post']       = array( __( 'Your attempt to delete this post: &#8220;%s&#8221; has failed.' ), 'get_the_title' );
		$trans['update']['post']       = array( __( 'Your attempt to edit this post: &#8220;%s&#8221; has failed.' ), 'get_the_title' );

		$trans['add']['user']          = array( __( 'Your attempt to add this user has failed.' ), false );
		$trans['delete']['users']      = array( __( 'Your attempt to delete users has failed.' ), false );
		$trans['bulk']['users']        = array( __( 'Your attempt to bulk modify users has failed.' ), false );
		$trans['update']['user']       = array( __( 'Your attempt to edit this user: &#8220;%s&#8221; has failed.' ), 'get_the_author_meta', 'display_name' );
		$trans['update']['profile']    = array( __( 'Your attempt to modify the profile for: &#8220;%s&#8221; has failed.' ), 'get_the_author_meta', 'display_name' );

		$trans['update']['options']    = array( __( 'Your attempt to edit your settings has failed.' ), false );
		$trans['update']['permalink']  = array( __( 'Your attempt to change your permalink structure to: %s has failed.' ), 'use_id' );
		$trans['edit']['file']         = array( __( 'Your attempt to edit this file: &#8220;%s&#8221; has failed.' ), 'use_id' );
		$trans['edit']['theme']        = array( __( 'Your attempt to edit this theme file: &#8220;%s&#8221; has failed.' ), 'use_id' );
		$trans['switch']['theme']      = array( __( 'Your attempt to switch to this theme: &#8220;%s&#8221; has failed.' ), 'use_id' );

		$trans['log']['out']           = array( sprintf( __( 'You are attempting to log out of %s' ), get_bloginfo( 'sitename' ) ), false );

		if ( isset( $trans[$verb][$noun] ) ) {
			if ( !empty( $trans[$verb][$noun][1] ) ) {
				$lookup = $trans[$verb][$noun][1];
				if ( isset($trans[$verb][$noun][2]) )
					$lookup_value = $trans[$verb][$noun][2];
				$object = $matches[4];
				if ( 'use_id' != $lookup ) {
					if ( isset( $lookup_value ) )
						$object = call_user_func( $lookup, $lookup_value, $object );
					else
						$object = call_user_func( $lookup, $object );
				}
				return sprintf( $trans[$verb][$noun][0], esc_html($object) );
			} else {
				return $trans[$verb][$noun][0];
			}
		}

		return apply_filters( 'explain_nonce_' . $verb . '-' . $noun, __( 'Are you sure you want to do this?' ), isset($matches[4]) ? $matches[4] : '' );
	} else {
		return apply_filters( 'explain_nonce_' . $action, __( 'Are you sure you want to do this?' ) );
	}
}

/**
 * Display "Are You Sure" message to confirm the action being taken.
 *
 * If the action has the nonce explain message, then it will be displayed along
 * with the "Are you sure?" message.
 *
 * @package WordPress
 * @subpackage Security
 * @since 2.0.4
 *
 * @param string $action The nonce action.
 */
function wp_nonce_ays( $action ) {
	$title = __( 'WordPress Failure Notice' );
	$html = esc_html( wp_explain_nonce( $action ) );
	if ( 'log-out' == $action )
		$html .= "</p><p>" . sprintf( __( "Do you really want to <a href='%s'>log out</a>?"), wp_logout_url() );
	elseif ( wp_get_referer() )
		$html .= "</p><p><a href='" . esc_url( remove_query_arg( 'updated', wp_get_referer() ) ) . "'>" . __( 'Please try again.' ) . "</a>";

	wp_die( $html, $title, array('response' => 403) );
}


/**
 * Kill WordPress execution and display HTML message with error message.
 *
 * This function complements the die() PHP function. The difference is that
 * HTML will be displayed to the user. It is recommended to use this function
 * only, when the execution should not continue any further. It is not
 * recommended to call this function very often and try to handle as many errors
 * as possible siliently.
 *
 * @since 2.0.4
 *
 * @param string $message Error message.
 * @param string $title Error title.
 * @param string|array $args Optional arguements to control behaviour.
 */
function wp_die( $message, $title = '', $args = array() ) {
	if ( defined( 'DOING_AJAX' ) && DOING_AJAX )
		die('-1');

	if ( function_exists( 'apply_filters' ) ) {
		$function = apply_filters( 'wp_die_handler', '_default_wp_die_handler');
	} else {
		$function = '_default_wp_die_handler';
	}

	call_user_func( $function, $message, $title, $args );
}

/**
 * Kill WordPress execution and display HTML message with error message.
 *
 * This is the default handler for wp_die if you want a custom one for your
 * site then you can overload using the wp_die_handler filter in wp_die
 *
 * @since 3.0.0
 * @access private
 *
 * @param string $message Error message.
 * @param string $title Error title.
 * @param string|array $args Optional arguements to control behaviour.
 */
function _default_wp_die_handler( $message, $title = '', $args = array() ) {
	$defaults = array( 'response' => 500 );
	$r = wp_parse_args($args, $defaults);

	$have_gettext = function_exists('__');

	if ( function_exists( 'is_wp_error' ) && is_wp_error( $message ) ) {
		if ( empty( $title ) ) {
			$error_data = $message->get_error_data();
			if ( is_array( $error_data ) && isset( $error_data['title'] ) )
				$title = $error_data['title'];
		}
		$errors = $message->get_error_messages();
		switch ( count( $errors ) ) :
		case 0 :
			$message = '';
			break;
		case 1 :
			$message = "<p>{$errors[0]}</p>";
			break;
		default :
			$message = "<ul>\n\t\t<li>" . join( "</li>\n\t\t<li>", $errors ) . "</li>\n\t</ul>";
			break;
		endswitch;
	} elseif ( is_string( $message ) ) {
		$message = "<p>$message</p>";
	}

	if ( isset( $r['back_link'] ) && $r['back_link'] ) {
		$back_text = $have_gettext? __('&laquo; Back') : '&laquo; Back';
		$message .= "\n<p><a href='javascript:history.back()'>$back_text</p>";
	}

	if ( defined( 'WP_SITEURL' ) && '' != WP_SITEURL )
		$admin_dir = WP_SITEURL . '/wp-admin/';
	elseif ( function_exists( 'get_bloginfo' ) && '' != get_bloginfo( 'wpurl' ) )
		$admin_dir = get_bloginfo( 'wpurl' ) . '/wp-admin/';
	elseif ( strpos( $_SERVER['PHP_SELF'], 'wp-admin' ) !== false )
		$admin_dir = '';
	else
		$admin_dir = 'wp-admin/';

	if ( !function_exists( 'did_action' ) || !did_action( 'admin_head' ) ) :
	if ( !headers_sent() ) {
		status_header( $r['response'] );
		nocache_headers();
		header( 'Content-Type: text/html; charset=utf-8' );
	}

	if ( empty($title) )
		$title = $have_gettext ? __('WordPress &rsaquo; Error') : 'WordPress &rsaquo; Error';

	$text_direction = 'ltr';
	if ( isset($r['text_direction']) && 'rtl' == $r['text_direction'] )
		$text_direction = 'rtl';
	elseif ( function_exists( 'is_rtl' ) && is_rtl() )
		$text_direction = 'rtl';
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<!-- Ticket #11289, IE bug fix: always pad the error page with enough characters such that it is greater than 512 bytes, even after gzip compression abcdefghijklmnopqrstuvwxyz1234567890aabbccddeeffgghhiijjkkllmmnnooppqqrrssttuuvvwwxxyyzz11223344556677889900abacbcbdcdcededfefegfgfhghgihihjijikjkjlklkmlmlnmnmononpopoqpqprqrqsrsrtstsubcbcdcdedefefgfabcadefbghicjkldmnoepqrfstugvwxhyz1i234j567k890laabmbccnddeoeffpgghqhiirjjksklltmmnunoovppqwqrrxsstytuuzvvw0wxx1yyz2z113223434455666777889890091abc2def3ghi4jkl5mno6pqr7stu8vwx9yz11aab2bcc3dd4ee5ff6gg7hh8ii9j0jk1kl2lmm3nnoo4p5pq6qrr7ss8tt9uuvv0wwx1x2yyzz13aba4cbcb5dcdc6dedfef8egf9gfh0ghg1ihi2hji3jik4jkj5lkl6kml7mln8mnm9ono -->
<html xmlns="http://www.w3.org/1999/xhtml" <?php if ( function_exists( 'language_attributes' ) && function_exists( 'is_rtl' ) ) language_attributes(); else echo "dir='$text_direction'"; ?>>
<head>
	<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
	<title><?php echo $title ?></title>
	<link rel="stylesheet" href="<?php echo $admin_dir; ?>css/install.css" type="text/css" />
<?php
if ( 'rtl' == $text_direction ) : ?>
	<link rel="stylesheet" href="<?php echo $admin_dir; ?>css/install-rtl.css" type="text/css" />
<?php endif; ?>
</head>
<body id="error-page">
<?php endif; ?>
	<?php echo $message; ?>
</body>
</html>
<?php
	die();
}

/**
 * Kill WordPress execution and display XML message with error message.
 *
 * This is the handler for wp_die when processing XMLRPC requests.
 *
 * @since 3.2.0
 * @access private
 *
 * @param string $message Error message.
 * @param string $title Error title.
 * @param string|array $args Optional arguements to control behaviour.
 */
function _xmlrpc_wp_die_handler( $message, $title = '', $args = array() ) {
	global $wp_xmlrpc_server;
	$defaults = array( 'response' => 500 );

	$r = wp_parse_args($args, $defaults);

	if ( $wp_xmlrpc_server ) {
		$error = new IXR_Error( $r['response'] , $message);
		$wp_xmlrpc_server->output( $error->getXml() );
	}
	die();
}

/**
 * Filter to enable special wp_die handler for xmlrpc requests.
 *
 * @since 3.2.0
 * @access private
 */
function _xmlrpc_wp_die_filter() {
	return '_xmlrpc_wp_die_handler';
}


/**
 * Retrieve the WordPress home page URL.
 *
 * If the constant named 'WP_HOME' exists, then it willl be used and returned by
 * the function. This can be used to counter the redirection on your local
 * development environment.
 *
 * @access private
 * @package WordPress
 * @since 2.2.0
 *
 * @param string $url URL for the home location
 * @return string Homepage location.
 */
function _config_wp_home( $url = '' ) {
	if ( defined( 'WP_HOME' ) )
		return WP_HOME;
	return $url;
}

/**
 * Retrieve the WordPress site URL.
 *
 * If the constant named 'WP_SITEURL' is defined, then the value in that
 * constant will always be returned. This can be used for debugging a site on
 * your localhost while not having to change the database to your URL.
 *
 * @access private
 * @package WordPress
 * @since 2.2.0
 *
 * @param string $url URL to set the WordPress site location.
 * @return string The WordPress Site URL
 */
function _config_wp_siteurl( $url = '' ) {
	if ( defined( 'WP_SITEURL' ) )
		return WP_SITEURL;
	return $url;
}

/**
 * Set the localized direction for MCE plugin.
 *
 * Will only set the direction to 'rtl', if the WordPress locale has the text
 * direction set to 'rtl'.
 *
 * Fills in the 'directionality', 'plugins', and 'theme_advanced_button1' array
 * keys. These keys are then returned in the $input array.
 *
 * @access private
 * @package WordPress
 * @subpackage MCE
 * @since 2.1.0
 *
 * @param array $input MCE plugin array.
 * @return array Direction set for 'rtl', if needed by locale.
 */
function _mce_set_direction( $input ) {
	if ( is_rtl() ) {
		$input['directionality'] = 'rtl';
		$input['plugins'] .= ',directionality';
		$input['theme_advanced_buttons1'] .= ',ltr';
	}

	return $input;
}


/**
 * Convert smiley code to the icon graphic file equivalent.
 *
 * You can turn off smilies, by going to the write setting screen and unchecking
 * the box, or by setting 'use_smilies' option to false or removing the option.
 *
 * Plugins may override the default smiley list by setting the $wpsmiliestrans
 * to an array, with the key the code the blogger types in and the value the
 * image file.
 *
 * The $wp_smiliessearch global is for the regular expression and is set each
 * time the function is called.
 *
 * The full list of smilies can be found in the function and won't be listed in
 * the description. Probably should create a Codex page for it, so that it is
 * available.
 *
 * @global array $wpsmiliestrans
 * @global array $wp_smiliessearch
 * @since 2.2.0
 */
function smilies_init() {
	global $wpsmiliestrans, $wp_smiliessearch;

	// don't bother setting up smilies if they are disabled
	if ( !get_option( 'use_smilies' ) )
		return;

	if ( !isset( $wpsmiliestrans ) ) {
		$wpsmiliestrans = array(
		':mrgreen:' => 'icon_mrgreen.gif',
		':neutral:' => 'icon_neutral.gif',
		':twisted:' => 'icon_twisted.gif',
		  ':arrow:' => 'icon_arrow.gif',
		  ':shock:' => 'icon_eek.gif',
		  ':smile:' => 'icon_smile.gif',
		    ':???:' => 'icon_confused.gif',
		   ':cool:' => 'icon_cool.gif',
		   ':evil:' => 'icon_evil.gif',
		   ':grin:' => 'icon_biggrin.gif',
		   ':idea:' => 'icon_idea.gif',
		   ':oops:' => 'icon_redface.gif',
		   ':razz:' => 'icon_razz.gif',
		   ':roll:' => 'icon_rolleyes.gif',
		   ':wink:' => 'icon_wink.gif',
		    ':cry:' => 'icon_cry.gif',
		    ':eek:' => 'icon_surprised.gif',
		    ':lol:' => 'icon_lol.gif',
		    ':mad:' => 'icon_mad.gif',
		    ':sad:' => 'icon_sad.gif',
		      '8-)' => 'icon_cool.gif',
		      '8-O' => 'icon_eek.gif',
		      ':-(' => 'icon_sad.gif',
		      ':-)' => 'icon_smile.gif',
		      ':-?' => 'icon_confused.gif',
		      ':-D' => 'icon_biggrin.gif',
		      ':-P' => 'icon_razz.gif',
		      ':-o' => 'icon_surprised.gif',
		      ':-x' => 'icon_mad.gif',
		      ':-|' => 'icon_neutral.gif',
		      ';-)' => 'icon_wink.gif',
		       '8)' => 'icon_cool.gif',
		       '8O' => 'icon_eek.gif',
		       ':(' => 'icon_sad.gif',
		       ':)' => 'icon_smile.gif',
		       ':?' => 'icon_confused.gif',
		       ':D' => 'icon_biggrin.gif',
		       ':P' => 'icon_razz.gif',
		       ':o' => 'icon_surprised.gif',
		       ':x' => 'icon_mad.gif',
		       ':|' => 'icon_neutral.gif',
		       ';)' => 'icon_wink.gif',
		      ':!:' => 'icon_exclaim.gif',
		      ':?:' => 'icon_question.gif',
		);
	}

	if (count($wpsmiliestrans) == 0) {
		return;
	}

	/*
	 * NOTE: we sort the smilies in reverse key order. This is to make sure
	 * we match the longest possible smilie (:???: vs :?) as the regular
	 * expression used below is first-match
	 */
	krsort($wpsmiliestrans);

	$wp_smiliessearch = '/(?:\s|^)';

	$subchar = '';
	foreach ( (array) $wpsmiliestrans as $smiley => $img ) {
		$firstchar = substr($smiley, 0, 1);
		$rest = substr($smiley, 1);

		// new subpattern?
		if ($firstchar != $subchar) {
			if ($subchar != '') {
				$wp_smiliessearch .= ')|(?:\s|^)';
			}
			$subchar = $firstchar;
			$wp_smiliessearch .= preg_quote($firstchar, '/') . '(?:';
		} else {
			$wp_smiliessearch .= '|';
		}
		$wp_smiliessearch .= preg_quote($rest, '/');
	}

	$wp_smiliessearch .= ')(?:\s|$)/m';
}

/**
 * Merge user defined arguments into defaults array.
 *
 * This function is used throughout WordPress to allow for both string or array
 * to be merged into another array.
 *
 * @since 2.2.0
 *
 * @param string|array $args Value to merge with $defaults
 * @param array $defaults Array that serves as the defaults.
 * @return array Merged user defined values with defaults.
 */
function wp_parse_args( $args, $defaults = '' ) {
	if ( is_object( $args ) )
		$r = get_object_vars( $args );
	elseif ( is_array( $args ) )
		$r =& $args;
	else
		wp_parse_str( $args, $r );

	if ( is_array( $defaults ) )
		return array_merge( $defaults, $r );
	return $r;
}

/**
 * Clean up an array, comma- or space-separated list of IDs
 *
 * @since 3.0.0
 *
 * @param array|string $list
 * @return array Sanitized array of IDs
 */
function wp_parse_id_list( $list ) {
	if ( !is_array($list) )
		$list = preg_split('/[\s,]+/', $list);

	return array_unique(array_map('absint', $list));
}

/**
 * Extract a slice of an array, given a list of keys
 *
 * @since 3.1.0
 *
 * @param array $array The original array
 * @param array $keys The list of keys
 * @return array The array slice
 */
function wp_array_slice_assoc( $array, $keys ) {
	$slice = array();
	foreach ( $keys as $key )
		if ( isset( $array[ $key ] ) )
			$slice[ $key ] = $array[ $key ];

	return $slice;
}

/**
 * Filters a list of objects, based on a set of key => value arguments
 *
 * @since 3.0.0
 *
 * @param array $list An array of objects to filter
 * @param array $args An array of key => value arguments to match against each object
 * @param string $operator The logical operation to perform. 'or' means only one element
 *	from the array needs to match; 'and' means all elements must match. The default is 'and'.
 * @param bool|string $field A field from the object to place instead of the entire object
 * @return array A list of objects or object fields
 */
function wp_filter_object_list( $list, $args = array(), $operator = 'and', $field = false ) {
	if ( ! is_array( $list ) )
		return array();

	$list = wp_list_filter( $list, $args, $operator );

	if ( $field )
		$list = wp_list_pluck( $list, $field );

	return $list;
}

/**
 * Filters a list of objects, based on a set of key => value arguments
 *
 * @since 3.1.0
 *
 * @param array $list An array of objects to filter
 * @param array $args An array of key => value arguments to match against each object
 * @param string $operator The logical operation to perform:
 *    'AND' means all elements from the array must match;
 *    'OR' means only one element needs to match;
 *    'NOT' means no elements may match.
 *   The default is 'AND'.
 * @return array
 */
function wp_list_filter( $list, $args = array(), $operator = 'AND' ) {
	if ( ! is_array( $list ) )
		return array();

	if ( empty( $args ) )
		return $list;

	$operator = strtoupper( $operator );
	$count = count( $args );
	$filtered = array();

	foreach ( $list as $key => $obj ) {
		$matched = count( array_intersect_assoc( (array) $obj, $args ) );
		if ( ( 'AND' == $operator && $matched == $count )
		  || ( 'OR' == $operator && $matched <= $count )
		  || ( 'NOT' == $operator && 0 == $matched ) ) {
			$filtered[$key] = $obj;
		}
	}

	return $filtered;
}

/**
 * Pluck a certain field out of each object in a list
 *
 * @since 3.1.0
 *
 * @param array $list A list of objects or arrays
 * @param int|string $field A field from the object to place instead of the entire object
 * @return array
 */
function wp_list_pluck( $list, $field ) {
	foreach ( $list as $key => $value ) {
		$value = (array) $value;
		$list[ $key ] = $value[ $field ];
	}

	return $list;
}

/**
 * Determines if Widgets library should be loaded.
 *
 * Checks to make sure that the widgets library hasn't already been loaded. If
 * it hasn't, then it will load the widgets library and run an action hook.
 *
 * @since 2.2.0
 * @uses add_action() Calls '_admin_menu' hook with 'wp_widgets_add_menu' value.
 */
function wp_maybe_load_widgets() {
	if ( ! apply_filters('load_default_widgets', true) )
		return;
	require_once( ABSPATH . WPINC . '/default-widgets.php' );
	add_action( '_admin_menu', 'wp_widgets_add_menu' );
}

/**
 * Append the Widgets menu to the themes main menu.
 *
 * @since 2.2.0
 * @uses $submenu The administration submenu list.
 */
function wp_widgets_add_menu() {
	global $submenu;
	$submenu['themes.php'][7] = array( __( 'Widgets' ), 'edit_theme_options', 'widgets.php' );
	ksort( $submenu['themes.php'], SORT_NUMERIC );
}

/**
 * Flush all output buffers for PHP 5.2.
 *
 * Make sure all output buffers are flushed before our singletons our destroyed.
 *
 * @since 2.2.0
 */
function wp_ob_end_flush_all() {
	$levels = ob_get_level();
	for ($i=0; $i<$levels; $i++)
		ob_end_flush();
}

/**
 * Load custom DB error or display WordPress DB error.
 *
 * If a file exists in the wp-content directory named db-error.php, then it will
 * be loaded instead of displaying the WordPress DB error. If it is not found,
 * then the WordPress DB error will be displayed instead.
 *
 * The WordPress DB error sets the HTTP status header to 500 to try to prevent
 * search engines from caching the message. Custom DB messages should do the
 * same.
 *
 * This function was backported to the the WordPress 2.3.2, but originally was
 * added in WordPress 2.5.0.
 *
 * @since 2.3.2
 * @uses $wpdb
 */
function dead_db() {
	global $wpdb;

	// Load custom DB error template, if present.
	if ( file_exists( WP_CONTENT_DIR . '/db-error.php' ) ) {
		require_once( WP_CONTENT_DIR . '/db-error.php' );
		die();
	}

	// If installing or in the admin, provide the verbose message.
	if ( defined('WP_INSTALLING') || defined('WP_ADMIN') )
		wp_die($wpdb->error);

	// Otherwise, be terse.
	status_header( 500 );
	nocache_headers();
	header( 'Content-Type: text/html; charset=utf-8' );
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" <?php if ( function_exists( 'language_attributes' ) ) language_attributes(); ?>>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
	<title>Database Error</title>

</head>
<body>
	<h1>Error establishing a database connection</h1>
</body>
</html>
<?php
	die();
}

/**
 * Converts value to nonnegative integer.
 *
 * @since 2.5.0
 *
 * @param mixed $maybeint Data you wish to have convered to an nonnegative integer
 * @return int An nonnegative integer
 */
function absint( $maybeint ) {
	return abs( intval( $maybeint ) );
}

/**
 * Determines if the blog can be accessed over SSL.
 *
 * Determines if blog can be accessed over SSL by using cURL to access the site
 * using the https in the siteurl. Requires cURL extension to work correctly.
 *
 * @since 2.5.0
 *
 * @param string $url
 * @return bool Whether SSL access is available
 */
function url_is_accessable_via_ssl($url)
{
	if (in_array('curl', get_loaded_extensions())) {
		$ssl = preg_replace( '/^http:\/\//', 'https://',  $url );

		$ch = curl_init();
		curl_setopt($ch, CURLOPT_URL, $ssl);
		curl_setopt($ch, CURLOPT_FAILONERROR, true);
		curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
		curl_setopt($ch, CURLOPT_SSL_VERIFYPEER, false);
		curl_setopt($ch, CURLOPT_CONNECTTIMEOUT, 5);

		curl_exec($ch);

		$status = curl_getinfo($ch, CURLINFO_HTTP_CODE);
		curl_close ($ch);

		if ($status == 200 || $status == 401) {
			return true;
		}
	}
	return false;
}

/**
 * Marks a function as deprecated and informs when it has been used.
 *
 * There is a hook deprecated_function_run that will be called that can be used
 * to get the backtrace up to what file and function called the deprecated
 * function.
 *
 * The current behavior is to trigger a user error if WP_DEBUG is true.
 *
 * This function is to be used in every function that is deprecated.
 *
 * @package WordPress
 * @subpackage Debug
 * @since 2.5.0
 * @access private
 *
 * @uses do_action() Calls 'deprecated_function_run' and passes the function name, what to use instead,
 *   and the version the function was deprecated in.
 * @uses apply_filters() Calls 'deprecated_function_trigger_error' and expects boolean value of true to do
 *   trigger or false to not trigger error.
 *
 * @param string $function The function that was called
 * @param string $version The version of WordPress that deprecated the function
 * @param string $replacement Optional. The function that should have been called
 */
function _deprecated_function( $function, $version, $replacement=null ) {

	do_action( 'deprecated_function_run', $function, $replacement, $version );

	// Allow plugin to filter the output error trigger
	if ( WP_DEBUG && apply_filters( 'deprecated_function_trigger_error', true ) ) {
		if ( ! is_null($replacement) )
			trigger_error( sprintf( __('%1$s is <strong>deprecated</strong> since version %2$s! Use %3$s instead.'), $function, $version, $replacement ) );
		else
			trigger_error( sprintf( __('%1$s is <strong>deprecated</strong> since version %2$s with no alternative available.'), $function, $version ) );
	}
}

/**
 * Marks a file as deprecated and informs when it has been used.
 *
 * There is a hook deprecated_file_included that will be called that can be used
 * to get the backtrace up to what file and function included the deprecated
 * file.
 *
 * The current behavior is to trigger a user error if WP_DEBUG is true.
 *
 * This function is to be used in every file that is deprecated.
 *
 * @package WordPress
 * @subpackage Debug
 * @since 2.5.0
 * @access private
 *
 * @uses do_action() Calls 'deprecated_file_included' and passes the file name, what to use instead,
 *   the version in which the file was deprecated, and any message regarding the change.
 * @uses apply_filters() Calls 'deprecated_file_trigger_error' and expects boolean value of true to do
 *   trigger or false to not trigger error.
 *
 * @param string $file The file that was included
 * @param string $version The version of WordPress that deprecated the file
 * @param string $replacement Optional. The file that should have been included based on ABSPATH
 * @param string $message Optional. A message regarding the change
 */
function _deprecated_file( $file, $version, $replacement = null, $message = '' ) {

	do_action( 'deprecated_file_included', $file, $replacement, $version, $message );

	// Allow plugin to filter the output error trigger
	if ( WP_DEBUG && apply_filters( 'deprecated_file_trigger_error', true ) ) {
		$message = empty( $message ) ? '' : ' ' . $message;
		if ( ! is_null( $replacement ) )
			trigger_error( sprintf( __('%1$s is <strong>deprecated</strong> since version %2$s! Use %3$s instead.'), $file, $version, $replacement ) . $message );
		else
			trigger_error( sprintf( __('%1$s is <strong>deprecated</strong> since version %2$s with no alternative available.'), $file, $version ) . $message );
	}
}
/**
 * Marks a function argument as deprecated and informs when it has been used.
 *
 * This function is to be used whenever a deprecated function argument is used.
 * Before this function is called, the argument must be checked for whether it was
 * used by comparing it to its default value or evaluating whether it is empty.
 * For example:
 * <code>
 * if ( !empty($deprecated) )
 * 	_deprecated_argument( __FUNCTION__, '3.0' );
 * </code>
 *
 * There is a hook deprecated_argument_run that will be called that can be used
 * to get the backtrace up to what file and function used the deprecated
 * argument.
 *
 * The current behavior is to trigger a user error if WP_DEBUG is true.
 *
 * @package WordPress
 * @subpackage Debug
 * @since 3.0.0
 * @access private
 *
 * @uses do_action() Calls 'deprecated_argument_run' and passes the function name, a message on the change,
 *   and the version in which the argument was deprecated.
 * @uses apply_filters() Calls 'deprecated_argument_trigger_error' and expects boolean value of true to do
 *   trigger or false to not trigger error.
 *
 * @param string $function The function that was called
 * @param string $version The version of WordPress that deprecated the argument used
 * @param string $message Optional. A message regarding the change.
 */
function _deprecated_argument( $function, $version, $message = null ) {

	do_action( 'deprecated_argument_run', $function, $message, $version );

	// Allow plugin to filter the output error trigger
	if ( WP_DEBUG && apply_filters( 'deprecated_argument_trigger_error', true ) ) {
		if ( ! is_null( $message ) )
			trigger_error( sprintf( __('%1$s was called with an argument that is <strong>deprecated</strong> since version %2$s! %3$s'), $function, $version, $message ) );
		else
			trigger_error( sprintf( __('%1$s was called with an argument that is <strong>deprecated</strong> since version %2$s with no alternative available.'), $function, $version ) );
	}
}

/**
 * Marks something as being incorrectly called.
 *
 * There is a hook doing_it_wrong_run that will be called that can be used
 * to get the backtrace up to what file and function called the deprecated
 * function.
 *
 * The current behavior is to trigger a user error if WP_DEBUG is true.
 *
 * @package WordPress
 * @subpackage Debug
 * @since 3.1.0
 * @access private
 *
 * @uses do_action() Calls 'doing_it_wrong_run' and passes the function arguments.
 * @uses apply_filters() Calls 'doing_it_wrong_trigger_error' and expects boolean value of true to do
 *   trigger or false to not trigger error.
 *
 * @param string $function The function that was called.
 * @param string $message A message explaining what has been done incorrectly.
 * @param string $version The version of WordPress where the message was added.
 */
function _doing_it_wrong( $function, $message, $version ) {

	do_action( 'doing_it_wrong_run', $function, $message, $version );

	// Allow plugin to filter the output error trigger
	if ( WP_DEBUG && apply_filters( 'doing_it_wrong_trigger_error', true ) ) {
		$version = is_null( $version ) ? '' : sprintf( __( '(This message was added in version %s.)' ), $version );
		trigger_error( sprintf( __( '%1$s was called <strong>incorrectly</strong>. %2$s %3$s' ), $function, $message, $version ) );
	}
}

/**
 * Is the server running earlier than 1.5.0 version of lighttpd
 *
 * @since 2.5.0
 *
 * @return bool Whether the server is running lighttpd < 1.5.0
 */
function is_lighttpd_before_150() {
	$server_parts = explode( '/', isset( $_SERVER['SERVER_SOFTWARE'] )? $_SERVER['SERVER_SOFTWARE'] : '' );
	$server_parts[1] = isset( $server_parts[1] )? $server_parts[1] : '';
	return  'lighttpd' == $server_parts[0] && -1 == version_compare( $server_parts[1], '1.5.0' );
}

/**
 * Does the specified module exist in the apache config?
 *
 * @since 2.5.0
 *
 * @param string $mod e.g. mod_rewrite
 * @param bool $default The default return value if the module is not found
 * @return bool
 */
function apache_mod_loaded($mod, $default = false) {
	global $is_apache;

	if ( !$is_apache )
		return false;

	if ( function_exists('apache_get_modules') ) {
		$mods = apache_get_modules();
		if ( in_array($mod, $mods) )
			return true;
	} elseif ( function_exists('phpinfo') ) {
			ob_start();
			phpinfo(8);
			$phpinfo = ob_get_clean();
			if ( false !== strpos($phpinfo, $mod) )
				return true;
	}
	return $default;
}

/**
 * Check if IIS 7 supports pretty permalinks
 *
 * @since 2.8.0
 *
 * @return bool
 */
function iis7_supports_permalinks() {
	global $is_iis7;

	$supports_permalinks = false;
	if ( $is_iis7 ) {
		/* First we check if the DOMDocument class exists. If it does not exist,
		 * which is the case for PHP 4.X, then we cannot easily update the xml configuration file,
		 * hence we just bail out and tell user that pretty permalinks cannot be used.
		 * This is not a big issue because PHP 4.X is going to be depricated and for IIS it
		 * is recommended to use PHP 5.X NTS.
		 * Next we check if the URL Rewrite Module 1.1 is loaded and enabled for the web site. When
		 * URL Rewrite 1.1 is loaded it always sets a server variable called 'IIS_UrlRewriteModule'.
		 * Lastly we make sure that PHP is running via FastCGI. This is important because if it runs
		 * via ISAPI then pretty permalinks will not work.
		 */
		$supports_permalinks = class_exists('DOMDocument') && isset($_SERVER['IIS_UrlRewriteModule']) && ( php_sapi_name() == 'cgi-fcgi' );
	}

	return apply_filters('iis7_supports_permalinks', $supports_permalinks);
}

/**
 * File validates against allowed set of defined rules.
 *
 * A return value of '1' means that the $file contains either '..' or './'. A
 * return value of '2' means that the $file contains ':' after the first
 * character. A return value of '3' means that the file is not in the allowed
 * files list.
 *
 * @since 1.2.0
 *
 * @param string $file File path.
 * @param array $allowed_files List of allowed files.
 * @return int 0 means nothing is wrong, greater than 0 means something was wrong.
 */
function validate_file( $file, $allowed_files = '' ) {
	if ( false !== strpos( $file, '..' ))
		return 1;

	if ( false !== strpos( $file, './' ))
		return 1;

	if (!empty ( $allowed_files ) && (!in_array( $file, $allowed_files ) ) )
		return 3;

	if (':' == substr( $file, 1, 1 ))
		return 2;

	return 0;
}

/**
 * Determine if SSL is used.
 *
 * @since 2.6.0
 *
 * @return bool True if SSL, false if not used.
 */
function is_ssl() {
	if ( isset($_SERVER['HTTPS']) ) {
		if ( 'on' == strtolower($_SERVER['HTTPS']) )
			return true;
		if ( '1' == $_SERVER['HTTPS'] )
			return true;
	} elseif ( isset($_SERVER['SERVER_PORT']) && ( '443' == $_SERVER['SERVER_PORT'] ) ) {
		return true;
	}
	return false;
}

/**
 * Whether SSL login should be forced.
 *
 * @since 2.6.0
 *
 * @param string|bool $force Optional.
 * @return bool True if forced, false if not forced.
 */
function force_ssl_login( $force = null ) {
	static $forced = false;

	if ( !is_null( $force ) ) {
		$old_forced = $forced;
		$forced = $force;
		return $old_forced;
	}

	return $forced;
}

/**
 * Whether to force SSL used for the Administration Screens.
 *
 * @since 2.6.0
 *
 * @param string|bool $force
 * @return bool True if forced, false if not forced.
 */
function force_ssl_admin( $force = null ) {
	static $forced = false;

	if ( !is_null( $force ) ) {
		$old_forced = $forced;
		$forced = $force;
		return $old_forced;
	}

	return $forced;
}

/**
 * Guess the URL for the site.
 *
 * Will remove wp-admin links to retrieve only return URLs not in the wp-admin
 * directory.
 *
 * @since 2.6.0
 *
 * @return string
 */
function wp_guess_url() {
	if ( defined('WP_SITEURL') && '' != WP_SITEURL ) {
		$url = WP_SITEURL;
	} else {
		$schema = is_ssl() ? 'https://' : 'http://';
		$url = preg_replace('|/wp-admin/.*|i', '', $schema . $_SERVER['HTTP_HOST'] . $_SERVER['REQUEST_URI']);
	}
	return rtrim($url, '/');
}

/**
 * Suspend cache invalidation.
 *
 * Turns cache invalidation on and off.  Useful during imports where you don't wont to do invalidations
 * every time a post is inserted.  Callers must be sure that what they are doing won't lead to an inconsistent
 * cache when invalidation is suspended.
 *
 * @since 2.7.0
 *
 * @param bool $suspend Whether to suspend or enable cache invalidation
 * @return bool The current suspend setting
 */
function wp_suspend_cache_invalidation($suspend = true) {
	global $_wp_suspend_cache_invalidation;

	$current_suspend = $_wp_suspend_cache_invalidation;
	$_wp_suspend_cache_invalidation = $suspend;
	return $current_suspend;
}

/**
 * Retrieve site option value based on name of option.
 *
 * @see get_option()
 * @package WordPress
 * @subpackage Option
 * @since 2.8.0
 *
 * @uses apply_filters() Calls 'pre_site_option_$option' before checking the option.
 * 	Any value other than false will "short-circuit" the retrieval of the option
 *	and return the returned value.
 * @uses apply_filters() Calls 'site_option_$option', after checking the  option, with
 * 	the option value.
 *
 * @param string $option Name of option to retrieve. Expected to not be SQL-escaped.
 * @param mixed $default Optional value to return if option doesn't exist. Default false.
 * @param bool $use_cache Whether to use cache. Multisite only. Default true.
 * @return mixed Value set for the option.
 */
function get_site_option( $option, $default = false, $use_cache = true ) {
	global $wpdb;

	// Allow plugins to short-circuit site options.
 	$pre = apply_filters( 'pre_site_option_' . $option, false );
 	if ( false !== $pre )
 		return $pre;

	if ( !is_multisite() ) {
		$value = get_option($option, $default);
	} else {
		$cache_key = "{$wpdb->siteid}:$option";
		if ( $use_cache )
			$value = wp_cache_get($cache_key, 'site-options');

		if ( !isset($value) || (false === $value) ) {
			$row = $wpdb->get_row( $wpdb->prepare("SELECT meta_value FROM $wpdb->sitemeta WHERE meta_key = %s AND site_id = %d", $option, $wpdb->siteid ) );

			// Has to be get_row instead of get_var because of funkiness with 0, false, null values
			if ( is_object( $row ) )
				$value = $row->meta_value;
			else
				$value = $default;

			$value = maybe_unserialize( $value );

			wp_cache_set( $cache_key, $value, 'site-options' );
		}
	}

 	return apply_filters( 'site_option_' . $option, $value );
}

/**
 * Add a new site option.
 *
 * @see add_option()
 * @package WordPress
 * @subpackage Option
 * @since 2.8.0
 *
 * @uses apply_filters() Calls 'pre_add_site_option_$option' hook to allow overwriting the
 * 	option value to be stored.
 * @uses do_action() Calls 'add_site_option_$option' and 'add_site_option' hooks on success.
 *
 * @param string $option Name of option to add. Expected to not be SQL-escaped.
 * @param mixed $value Optional. Option value, can be anything. Expected to not be SQL-escaped.
 * @return bool False if option was not added and true if option was added.
 */
function add_site_option( $option, $value ) {
	global $wpdb;

	$value = apply_filters( 'pre_add_site_option_' . $option, $value );

	if ( !is_multisite() ) {
		$result = add_option( $option, $value );
	} else {
		$cache_key = "{$wpdb->siteid}:$option";

		if ( $wpdb->get_row( $wpdb->prepare( "SELECT meta_value FROM $wpdb->sitemeta WHERE meta_key = %s AND site_id = %d", $option, $wpdb->siteid ) ) )
			return update_site_option( $option, $value );

		$value = sanitize_option( $option, $value );
		wp_cache_set( $cache_key, $value, 'site-options' );

		$_value = $value;
		$value = maybe_serialize($value);
		$result = $wpdb->insert( $wpdb->sitemeta, array('site_id' => $wpdb->siteid, 'meta_key' => $option, 'meta_value' => $value ) );
		$value = $_value;
	}

	do_action( "add_site_option_{$option}", $option, $value );
	do_action( "add_site_option", $option, $value );

	return $result;
}

/**
 * Removes site option by name.
 *
 * @see delete_option()
 * @package WordPress
 * @subpackage Option
 * @since 2.8.0
 *
 * @uses do_action() Calls 'pre_delete_site_option_$option' hook before option is deleted.
 * @uses do_action() Calls 'delete_site_option' and 'delete_site_option_$option'
 * 	hooks on success.
 *
 * @param string $option Name of option to remove. Expected to not be SQL-escaped.
 * @return bool True, if succeed. False, if failure.
 */
function delete_site_option( $option ) {
	global $wpdb;

	// ms_protect_special_option( $option ); @todo

	do_action( 'pre_delete_site_option_' . $option );

	if ( !is_multisite() ) {
		$result = delete_option( $option );
	} else {
		$row = $wpdb->get_row( $wpdb->prepare( "SELECT meta_id FROM {$wpdb->sitemeta} WHERE meta_key = %s AND site_id = %d", $option, $wpdb->siteid ) );
		if ( is_null( $row ) || !$row->meta_id )
			return false;
		$cache_key = "{$wpdb->siteid}:$option";
		wp_cache_delete( $cache_key, 'site-options' );

		$result = $wpdb->query( $wpdb->prepare( "DELETE FROM {$wpdb->sitemeta} WHERE meta_key = %s AND site_id = %d", $option, $wpdb->siteid ) );
	}

	if ( $result ) {
		do_action( "delete_site_option_{$option}", $option );
		do_action( "delete_site_option", $option );
		return true;
	}
	return false;
}

/**
 * Update the value of a site option that was already added.
 *
 * @see update_option()
 * @since 2.8.0
 * @package WordPress
 * @subpackage Option
 *
 * @uses apply_filters() Calls 'pre_update_site_option_$option' hook to allow overwriting the
 * 	option value to be stored.
 * @uses do_action() Calls 'update_site_option_$option' and 'update_site_option' hooks on success.
 *
 * @param string $option Name of option. Expected to not be SQL-escaped.
 * @param mixed $value Option value. Expected to not be SQL-escaped.
 * @return bool False if value was not updated and true if value was updated.
 */
function update_site_option( $option, $value ) {
	global $wpdb;

	$oldvalue = get_site_option( $option );
	$value = apply_filters( 'pre_update_site_option_' . $option, $value, $oldvalue );

	if ( $value === $oldvalue )
		return false;

	if ( !is_multisite() ) {
		$result = update_option( $option, $value );
	} else {
		$cache_key = "{$wpdb->siteid}:$option";

		if ( $value && !$wpdb->get_row( $wpdb->prepare( "SELECT meta_value FROM $wpdb->sitemeta WHERE meta_key = %s AND site_id = %d", $option, $wpdb->siteid ) ) )
			return add_site_option( $option, $value );
		$value = sanitize_option( $option, $value );
		wp_cache_set( $cache_key, $value, 'site-options' );

		$_value = $value;
		$value = maybe_serialize( $value );
		$result = $wpdb->update( $wpdb->sitemeta, array( 'meta_value' => $value ), array( 'site_id' => $wpdb->siteid, 'meta_key' => $option ) );
		$value = $_value;
	}

	if ( $result ) {
		do_action( "update_site_option_{$option}", $option, $value );
		do_action( "update_site_option", $option, $value );
		return true;
	}
	return false;
}

/**
 * Delete a site transient
 *
 * @since 2.9.0
 * @package WordPress
 * @subpackage Transient
 *
 * @uses do_action() Calls 'delete_site_transient_$transient' hook before transient is deleted.
 * @uses do_action() Calls 'deleted_site_transient' hook on success.
 *
 * @param string $transient Transient name. Expected to not be SQL-escaped.
 * @return bool True if successful, false otherwise
 */
function delete_site_transient( $transient ) {
	global $_wp_using_ext_object_cache;

	do_action( 'delete_site_transient_' . $transient, $transient );
	if ( $_wp_using_ext_object_cache ) {
		$result = wp_cache_delete( $transient, 'site-transient' );
	} else {
		$option_timeout = '_site_transient_timeout_' . $transient;
		$option = '_site_transient_' . $transient;
		$result = delete_site_option( $option );
		if ( $result )
			delete_site_option( $option_timeout );
	}
	if ( $result )
		do_action( 'deleted_site_transient', $transient );
	return $result;
}

/**
 * Get the value of a site transient
 *
 * If the transient does not exist or does not have a value, then the return value
 * will be false.
 *
 * @see get_transient()
 * @since 2.9.0
 * @package WordPress
 * @subpackage Transient
 *
 * @uses apply_filters() Calls 'pre_site_transient_$transient' hook before checking the transient.
 * 	Any value other than false will "short-circuit" the retrieval of the transient
 *	and return the returned value.
 * @uses apply_filters() Calls 'site_transient_$option' hook, after checking the transient, with
 * 	the transient value.
 *
 * @param string $transient Transient name. Expected to not be SQL-escaped.
 * @return mixed Value of transient
 */
function get_site_transient( $transient ) {
	global $_wp_using_ext_object_cache;

	$pre = apply_filters( 'pre_site_transient_' . $transient, false );
	if ( false !== $pre )
		return $pre;

	if ( $_wp_using_ext_object_cache ) {
		$value = wp_cache_get( $transient, 'site-transient' );
	} else {
		// Core transients that do not have a timeout. Listed here so querying timeouts can be avoided.
		$no_timeout = array('update_core', 'update_plugins', 'update_themes');
		$transient_option = '_site_transient_' . $transient;
		if ( ! in_array( $transient, $no_timeout ) ) {
			$transient_timeout = '_site_transient_timeout_' . $transient;
			$timeout = get_site_option( $transient_timeout );
			if ( false !== $timeout && $timeout < time() ) {
				delete_site_option( $transient_option  );
				delete_site_option( $transient_timeout );
				return false;
			}
		}

		$value = get_site_option( $transient_option );
	}

	return apply_filters( 'site_transient_' . $transient, $value );
}

/**
 * Set/update the value of a site transient
 *
 * You do not need to serialize values, if the value needs to be serialize, then
 * it will be serialized before it is set.
 *
 * @see set_transient()
 * @since 2.9.0
 * @package WordPress
 * @subpackage Transient
 *
 * @uses apply_filters() Calls 'pre_set_site_transient_$transient' hook to allow overwriting the
 * 	transient value to be stored.
 * @uses do_action() Calls 'set_site_transient_$transient' and 'setted_site_transient' hooks on success.
 *
 * @param string $transient Transient name. Expected to not be SQL-escaped.
 * @param mixed $value Transient value. Expected to not be SQL-escaped.
 * @param int $expiration Time until expiration in seconds, default 0
 * @return bool False if value was not set and true if value was set.
 */
function set_site_transient( $transient, $value, $expiration = 0 ) {
	global $_wp_using_ext_object_cache;

	$value = apply_filters( 'pre_set_site_transient_' . $transient, $value );

	if ( $_wp_using_ext_object_cache ) {
		$result = wp_cache_set( $transient, $value, 'site-transient', $expiration );
	} else {
		$transient_timeout = '_site_transient_timeout_' . $transient;
		$transient = '_site_transient_' . $transient;
		if ( false === get_site_option( $transient ) ) {
			if ( $expiration )
				add_site_option( $transient_timeout, time() + $expiration );
			$result = add_site_option( $transient, $value );
		} else {
			if ( $expiration )
				update_site_option( $transient_timeout, time() + $expiration );
			$result = update_site_option( $transient, $value );
		}
	}
	if ( $result ) {
		do_action( 'set_site_transient_' . $transient );
		do_action( 'setted_site_transient', $transient );
	}
	return $result;
}

/**
 * is main site
 *
 *
 * @since 3.0.0
 * @package WordPress
 *
 * @param int $blog_id optional blog id to test (default current blog)
 * @return bool True if not multisite or $blog_id is main site
 */
function is_main_site( $blog_id = '' ) {
	global $current_site, $current_blog;

	if ( !is_multisite() )
		return true;

	if ( !$blog_id )
		$blog_id = $current_blog->blog_id;

	return $blog_id == $current_site->blog_id;
}

/**
 * Whether global terms are enabled.
 *
 *
 * @since 3.0.0
 * @package WordPress
 *
 * @return bool True if multisite and global terms enabled
 */
function global_terms_enabled() {
	if ( ! is_multisite() )
		return false;

	static $global_terms = null;
	if ( is_null( $global_terms ) ) {
		$filter = apply_filters( 'global_terms_enabled', null );
		if ( ! is_null( $filter ) )
			$global_terms = (bool) $filter;
		else
			$global_terms = (bool) get_site_option( 'global_terms_enabled', false );
	}
	return $global_terms;
}

/**
 * gmt_offset modification for smart timezone handling
 *
 * Overrides the gmt_offset option if we have a timezone_string available
 *
 * @since 2.8.0
 *
 * @return float|bool
 */
function wp_timezone_override_offset() {
	if ( !$timezone_string = get_option( 'timezone_string' ) ) {
		return false;
	}

	$timezone_object = timezone_open( $timezone_string );
	$datetime_object = date_create();
	if ( false === $timezone_object || false === $datetime_object ) {
		return false;
	}
	return round( timezone_offset_get( $timezone_object, $datetime_object ) / 3600, 2 );
}

/**
 * {@internal Missing Short Description}}
 *
 * @since 2.9.0
 *
 * @param unknown_type $a
 * @param unknown_type $b
 * @return int
 */
function _wp_timezone_choice_usort_callback( $a, $b ) {
	// Don't use translated versions of Etc
	if ( 'Etc' === $a['continent'] && 'Etc' === $b['continent'] ) {
		// Make the order of these more like the old dropdown
		if ( 'GMT+' === substr( $a['city'], 0, 4 ) && 'GMT+' === substr( $b['city'], 0, 4 ) ) {
			return -1 * ( strnatcasecmp( $a['city'], $b['city'] ) );
		}
		if ( 'UTC' === $a['city'] ) {
			if ( 'GMT+' === substr( $b['city'], 0, 4 ) ) {
				return 1;
			}
			return -1;
		}
		if ( 'UTC' === $b['city'] ) {
			if ( 'GMT+' === substr( $a['city'], 0, 4 ) ) {
				return -1;
			}
			return 1;
		}
		return strnatcasecmp( $a['city'], $b['city'] );
	}
	if ( $a['t_continent'] == $b['t_continent'] ) {
		if ( $a['t_city'] == $b['t_city'] ) {
			return strnatcasecmp( $a['t_subcity'], $b['t_subcity'] );
		}
		return strnatcasecmp( $a['t_city'], $b['t_city'] );
	} else {
		// Force Etc to the bottom of the list
		if ( 'Etc' === $a['continent'] ) {
			return 1;
		}
		if ( 'Etc' === $b['continent'] ) {
			return -1;
		}
		return strnatcasecmp( $a['t_continent'], $b['t_continent'] );
	}
}

/**
 * Gives a nicely formatted list of timezone strings // temporary! Not in final
 *
 * @since 2.9.0
 *
 * @param string $selected_zone Selected Zone
 * @return string
 */
function wp_timezone_choice( $selected_zone ) {
	static $mo_loaded = false;

	$continents = array( 'Africa', 'America', 'Antarctica', 'Arctic', 'Asia', 'Atlantic', 'Australia', 'Europe', 'Indian', 'Pacific');

	// Load translations for continents and cities
	if ( !$mo_loaded ) {
		$locale = get_locale();
		$mofile = WP_LANG_DIR . '/continents-cities-' . $locale . '.mo';
		load_textdomain( 'continents-cities', $mofile );
		$mo_loaded = true;
	}

	$zonen = array();
	foreach ( timezone_identifiers_list() as $zone ) {
		$zone = explode( '/', $zone );
		if ( !in_array( $zone[0], $continents ) ) {
			continue;
		}

		// This determines what gets set and translated - we don't translate Etc/* strings here, they are done later
		$exists = array(
			0 => ( isset( $zone[0] ) && $zone[0] ),
			1 => ( isset( $zone[1] ) && $zone[1] ),
			2 => ( isset( $zone[2] ) && $zone[2] ),
		);
		$exists[3] = ( $exists[0] && 'Etc' !== $zone[0] );
		$exists[4] = ( $exists[1] && $exists[3] );
		$exists[5] = ( $exists[2] && $exists[3] );

		$zonen[] = array(
			'continent'   => ( $exists[0] ? $zone[0] : '' ),
			'city'        => ( $exists[1] ? $zone[1] : '' ),
			'subcity'     => ( $exists[2] ? $zone[2] : '' ),
			't_continent' => ( $exists[3] ? translate( str_replace( '_', ' ', $zone[0] ), 'continents-cities' ) : '' ),
			't_city'      => ( $exists[4] ? translate( str_replace( '_', ' ', $zone[1] ), 'continents-cities' ) : '' ),
			't_subcity'   => ( $exists[5] ? translate( str_replace( '_', ' ', $zone[2] ), 'continents-cities' ) : '' )
		);
	}
	usort( $zonen, '_wp_timezone_choice_usort_callback' );

	$structure = array();

	if ( empty( $selected_zone ) ) {
		$structure[] = '<option selected="selected" value="">' . __( 'Select a city' ) . '</option>';
	}

	foreach ( $zonen as $key => $zone ) {
		// Build value in an array to join later
		$value = array( $zone['continent'] );

		if ( empty( $zone['city'] ) ) {
			// It's at the continent level (generally won't happen)
			$display = $zone['t_continent'];
		} else {
			// It's inside a continent group

			// Continent optgroup
			if ( !isset( $zonen[$key - 1] ) || $zonen[$key - 1]['continent'] !== $zone['continent'] ) {
				$label = $zone['t_continent'];
				$structure[] = '<optgroup label="'. esc_attr( $label ) .'">';
			}

			// Add the city to the value
			$value[] = $zone['city'];

			$display = $zone['t_city'];
			if ( !empty( $zone['subcity'] ) ) {
				// Add the subcity to the value
				$value[] = $zone['subcity'];
				$display .= ' - ' . $zone['t_subcity'];
			}
		}

		// Build the value
		$value = join( '/', $value );
		$selected = '';
		if ( $value === $selected_zone ) {
			$selected = 'selected="selected" ';
		}
		$structure[] = '<option ' . $selected . 'value="' . esc_attr( $value ) . '">' . esc_html( $display ) . "</option>";

		// Close continent optgroup
		if ( !empty( $zone['city'] ) && ( !isset($zonen[$key + 1]) || (isset( $zonen[$key + 1] ) && $zonen[$key + 1]['continent'] !== $zone['continent']) ) ) {
			$structure[] = '</optgroup>';
		}
	}

	// Do UTC
	$structure[] = '<optgroup label="'. esc_attr__( 'UTC' ) .'">';
	$selected = '';
	if ( 'UTC' === $selected_zone )
		$selected = 'selected="selected" ';
	$structure[] = '<option ' . $selected . 'value="' . esc_attr( 'UTC' ) . '">' . __('UTC') . '</option>';
	$structure[] = '</optgroup>';

	// Do manual UTC offsets
	$structure[] = '<optgroup label="'. esc_attr__( 'Manual Offsets' ) .'">';
	$offset_range = array (-12, -11.5, -11, -10.5, -10, -9.5, -9, -8.5, -8, -7.5, -7, -6.5, -6, -5.5, -5, -4.5, -4, -3.5, -3, -2.5, -2, -1.5, -1, -0.5,
		0, 0.5, 1, 1.5, 2, 2.5, 3, 3.5, 4, 4.5, 5, 5.5, 5.75, 6, 6.5, 7, 7.5, 8, 8.5, 8.75, 9, 9.5, 10, 10.5, 11, 11.5, 12, 12.75, 13, 13.75, 14);
	foreach ( $offset_range as $offset ) {
		if ( 0 <= $offset )
			$offset_name = '+' . $offset;
		else
			$offset_name = (string) $offset;

		$offset_value = $offset_name;
		$offset_name = str_replace(array('.25','.5','.75'), array(':15',':30',':45'), $offset_name);
		$offset_name = 'UTC' . $offset_name;
		$offset_value = 'UTC' . $offset_value;
		$selected = '';
		if ( $offset_value === $selected_zone )
			$selected = 'selected="selected" ';
		$structure[] = '<option ' . $selected . 'value="' . esc_attr( $offset_value ) . '">' . esc_html( $offset_name ) . "</option>";

	}
	$structure[] = '</optgroup>';

	return join( "\n", $structure );
}

/**
 * Strip close comment and close php tags from file headers used by WP
 * See http://core.trac.wordpress.org/ticket/8497
 *
 * @since 2.8.0
 *
 * @param string $str
 * @return string
 */
function _cleanup_header_comment($str) {
	return trim(preg_replace("/\s*(?:\*\/|\?>).*/", '', $str));
}

/**
 * Permanently deletes posts, pages, attachments, and comments which have been in the trash for EMPTY_TRASH_DAYS.
 *
 * @since 2.9.0
 */
function wp_scheduled_delete() {
	global $wpdb;

	$delete_timestamp = time() - (60*60*24*EMPTY_TRASH_DAYS);

	$posts_to_delete = $wpdb->get_results($wpdb->prepare("SELECT post_id FROM $wpdb->postmeta WHERE meta_key = '_wp_trash_meta_time' AND meta_value < '%d'", $delete_timestamp), ARRAY_A);

	foreach ( (array) $posts_to_delete as $post ) {
		$post_id = (int) $post['post_id'];
		if ( !$post_id )
			continue;

		$del_post = get_post($post_id);

		if ( !$del_post || 'trash' != $del_post->post_status ) {
			delete_post_meta($post_id, '_wp_trash_meta_status');
			delete_post_meta($post_id, '_wp_trash_meta_time');
		} else {
			wp_delete_post($post_id);
		}
	}

	$comments_to_delete = $wpdb->get_results($wpdb->prepare("SELECT comment_id FROM $wpdb->commentmeta WHERE meta_key = '_wp_trash_meta_time' AND meta_value < '%d'", $delete_timestamp), ARRAY_A);

	foreach ( (array) $comments_to_delete as $comment ) {
		$comment_id = (int) $comment['comment_id'];
		if ( !$comment_id )
			continue;

		$del_comment = get_comment($comment_id);

		if ( !$del_comment || 'trash' != $del_comment->comment_approved ) {
			delete_comment_meta($comment_id, '_wp_trash_meta_time');
			delete_comment_meta($comment_id, '_wp_trash_meta_status');
		} else {
			wp_delete_comment($comment_id);
		}
	}
}

/**
 * Retrieve metadata from a file.
 *
 * Searches for metadata in the first 8kiB of a file, such as a plugin or theme.
 * Each piece of metadata must be on its own line. Fields can not span multple
 * lines, the value will get cut at the end of the first line.
 *
 * If the file data is not within that first 8kiB, then the author should correct
 * their plugin file and move the data headers to the top.
 *
 * @see http://codex.wordpress.org/File_Header
 *
 * @since 2.9.0
 * @param string $file Path to the file
 * @param array $default_headers List of headers, in the format array('HeaderKey' => 'Header Name')
 * @param string $context If specified adds filter hook "extra_{$context}_headers"
 */
function get_file_data( $file, $default_headers, $context = '' ) {
	// We don't need to write to the file, so just open for reading.
	$fp = fopen( $file, 'r' );

	// Pull only the first 8kiB of the file in.
	$file_data = fread( $fp, 8192 );

	// PHP will close file handle, but we are good citizens.
	fclose( $fp );

	if ( $context != '' ) {
		$extra_headers = apply_filters( "extra_{$context}_headers", array() );

		$extra_headers = array_flip( $extra_headers );
		foreach( $extra_headers as $key=>$value ) {
			$extra_headers[$key] = $key;
		}
		$all_headers = array_merge( $extra_headers, (array) $default_headers );
	} else {
		$all_headers = $default_headers;
	}

	foreach ( $all_headers as $field => $regex ) {
		preg_match( '/^[ \t\/*#@]*' . preg_quote( $regex, '/' ) . ':(.*)$/mi', $file_data, ${$field});
		if ( !empty( ${$field} ) )
			${$field} = _cleanup_header_comment( ${$field}[1] );
		else
			${$field} = '';
	}

	$file_data = compact( array_keys( $all_headers ) );

	return $file_data;
}

/**
 * Used internally to tidy up the search terms
 *
 * @access private
 * @since 2.9.0
 *
 * @param string $t
 * @return string
 */
function _search_terms_tidy($t) {
	return trim($t, "\"'\n\r ");
}

/**
 * Returns true
 *
 * Useful for returning true to filters easily
 *
 * @since 3.0.0
 * @see __return_false()
 * @return bool true
 */
function __return_true() {
	return true;
}

/**
 * Returns false
 *
 * Useful for returning false to filters easily
 *
 * @since 3.0.0
 * @see __return_true()
 * @return bool false
 */
function __return_false() {
	return false;
}

/**
 * Returns 0
 *
 * Useful for returning 0 to filters easily
 *
 * @since 3.0.0
 * @see __return_zero()
 * @return int 0
 */
function __return_zero() {
	return 0;
}

/**
 * Returns an empty array
 *
 * Useful for returning an empty array to filters easily
 *
 * @since 3.0.0
 * @see __return_zero()
 * @return array Empty array
 */
function __return_empty_array() {
	return array();
}

/**
 * Send a HTTP header to disable content type sniffing in browsers which support it.
 *
 * @link http://blogs.msdn.com/ie/archive/2008/07/02/ie8-security-part-v-comprehensive-protection.aspx
 * @link http://src.chromium.org/viewvc/chrome?view=rev&revision=6985
 *
 * @since 3.0.0
 * @return none
 */
function send_nosniff_header() {
	@header( 'X-Content-Type-Options: nosniff' );
}

/**
 * Returns a MySQL expression for selecting the week number based on the start_of_week option.
 *
 * @internal
 * @since 3.0.0
 * @param string $column
 * @return string
 */
function _wp_mysql_week( $column ) {
	switch ( $start_of_week = (int) get_option( 'start_of_week' ) ) {
	default :
	case 0 :
		return "WEEK( $column, 0 )";
	case 1 :
		return "WEEK( $column, 1 )";
	case 2 :
	case 3 :
	case 4 :
	case 5 :
	case 6 :
		return "WEEK( DATE_SUB( $column, INTERVAL $start_of_week DAY ), 0 )";
	}
}

/**
 * Finds hierarchy loops using a callback function that maps object IDs to parent IDs.
 *
 * @since 3.1.0
 * @access private
 *
 * @param callback $callback function that accepts ( ID, $callback_args ) and outputs parent_ID
 * @param int $start The ID to start the loop check at
 * @param int $start_parent the parent_ID of $start to use instead of calling $callback( $start ). Use null to always use $callback
 * @param array $callback_args optional additional arguments to send to $callback
 * @return array IDs of all members of loop
 */
function wp_find_hierarchy_loop( $callback, $start, $start_parent, $callback_args = array() ) {
	$override = is_null( $start_parent ) ? array() : array( $start => $start_parent );

	if ( !$arbitrary_loop_member = wp_find_hierarchy_loop_tortoise_hare( $callback, $start, $override, $callback_args ) )
		return array();

	return wp_find_hierarchy_loop_tortoise_hare( $callback, $arbitrary_loop_member, $override, $callback_args, true );
}

/**
 * Uses the "The Tortoise and the Hare" algorithm to detect loops.
 *
 * For every step of the algorithm, the hare takes two steps and the tortoise one.
 * If the hare ever laps the tortoise, there must be a loop.
 *
 * @since 3.1.0
 * @access private
 *
 * @param callback $callback function that accupts ( ID, callback_arg, ... ) and outputs parent_ID
 * @param int $start The ID to start the loop check at
 * @param array $override an array of ( ID => parent_ID, ... ) to use instead of $callback
 * @param array $callback_args optional additional arguments to send to $callback
 * @param bool $_return_loop Return loop members or just detect presence of loop?
 *             Only set to true if you already know the given $start is part of a loop
 *             (otherwise the returned array might include branches)
 * @return mixed scalar ID of some arbitrary member of the loop, or array of IDs of all members of loop if $_return_loop
 */
function wp_find_hierarchy_loop_tortoise_hare( $callback, $start, $override = array(), $callback_args = array(), $_return_loop = false ) {
	$tortoise = $hare = $evanescent_hare = $start;
	$return = array();

	// Set evanescent_hare to one past hare
	// Increment hare two steps
	while (
		$tortoise
	&&
		( $evanescent_hare = isset( $override[$hare] ) ? $override[$hare] : call_user_func_array( $callback, array_merge( array( $hare ), $callback_args ) ) )
	&&
		( $hare = isset( $override[$evanescent_hare] ) ? $override[$evanescent_hare] : call_user_func_array( $callback, array_merge( array( $evanescent_hare ), $callback_args ) ) )
	) {
		if ( $_return_loop )
			$return[$tortoise] = $return[$evanescent_hare] = $return[$hare] = true;

		// tortoise got lapped - must be a loop
		if ( $tortoise == $evanescent_hare || $tortoise == $hare )
			return $_return_loop ? $return : $tortoise;

		// Increment tortoise by one step
		$tortoise = isset( $override[$tortoise] ) ? $override[$tortoise] : call_user_func_array( $callback, array_merge( array( $tortoise ), $callback_args ) );
	}

	return false;
}

/**
 * Send a HTTP header to limit rendering of pages to same origin iframes.
 *
 * @link https://developer.mozilla.org/en/the_x-frame-options_response_header
 *
 * @since 3.1.3
 * @return none
 */
function send_frame_options_header() {
	@header( 'X-Frame-Options: SAMEORIGIN' );
}

?>
