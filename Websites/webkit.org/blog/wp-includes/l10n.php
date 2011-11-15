<?php
/**
 * WordPress Translation API
 *
 * @package WordPress
 * @subpackage i18n
 */

/**
 * Gets the current locale.
 *
 * If the locale is set, then it will filter the locale in the 'locale' filter
 * hook and return the value.
 *
 * If the locale is not set already, then the WPLANG constant is used if it is
 * defined. Then it is filtered through the 'locale' filter hook and the value
 * for the locale global set and the locale is returned.
 *
 * The process to get the locale should only be done once but the locale will
 * always be filtered using the 'locale' hook.
 *
 * @since 1.5.0
 * @uses apply_filters() Calls 'locale' hook on locale value.
 * @uses $locale Gets the locale stored in the global.
 *
 * @return string The locale of the blog or from the 'locale' hook.
 */
function get_locale() {
	global $locale;

	if ( isset( $locale ) )
		return apply_filters( 'locale', $locale );

	// WPLANG is defined in wp-config.
	if ( defined( 'WPLANG' ) )
		$locale = WPLANG;

	// If multisite, check options.
	if ( is_multisite() && !defined('WP_INSTALLING') ) {
		$ms_locale = get_option('WPLANG');
		if ( $ms_locale === false )
			$ms_locale = get_site_option('WPLANG');

		if ( $ms_locale !== false )
			$locale = $ms_locale;
	}

	if ( empty( $locale ) )
		$locale = 'en_US';

	return apply_filters( 'locale', $locale );
}

/**
 * Retrieves the translation of $text. If there is no translation, or
 * the domain isn't loaded the original text is returned.
 *
 * @see __() Don't use translate() directly, use __()
 * @since 2.2.0
 * @uses apply_filters() Calls 'gettext' on domain translated text
 *		with the untranslated text as second parameter.
 *
 * @param string $text Text to translate.
 * @param string $domain Domain to retrieve the translated text.
 * @return string Translated text
 */
function translate( $text, $domain = 'default' ) {
	$translations = &get_translations_for_domain( $domain );
	return apply_filters( 'gettext', $translations->translate( $text ), $text, $domain );
}

function before_last_bar( $string ) {
	$last_bar = strrpos( $string, '|' );
	if ( false == $last_bar )
		return $string;
	else
		return substr( $string, 0, $last_bar );
}

function translate_with_gettext_context( $text, $context, $domain = 'default' ) {
	$translations = &get_translations_for_domain( $domain );
	return apply_filters( 'gettext_with_context', $translations->translate( $text, $context ), $text, $context, $domain );
}

/**
 * Retrieves the translation of $text. If there is no translation, or
 * the domain isn't loaded the original text is returned.
 *
 * @see translate() An alias of translate()
 * @since 2.1.0
 *
 * @param string $text Text to translate
 * @param string $domain Optional. Domain to retrieve the translated text
 * @return string Translated text
 */
function __( $text, $domain = 'default' ) {
	return translate( $text, $domain );
}

/**
 * Retrieves the translation of $text and escapes it for safe use in an attribute.
 * If there is no translation, or the domain isn't loaded the original text is returned.
 *
 * @see translate() An alias of translate()
 * @see esc_attr()
 * @since 2.8.0
 *
 * @param string $text Text to translate
 * @param string $domain Optional. Domain to retrieve the translated text
 * @return string Translated text
 */
function esc_attr__( $text, $domain = 'default' ) {
	return esc_attr( translate( $text, $domain ) );
}

/**
 * Retrieves the translation of $text and escapes it for safe use in HTML output.
 * If there is no translation, or the domain isn't loaded the original text is returned.
 *
 * @see translate() An alias of translate()
 * @see esc_html()
 * @since 2.8.0
 *
 * @param string $text Text to translate
 * @param string $domain Optional. Domain to retrieve the translated text
 * @return string Translated text
 */
function esc_html__( $text, $domain = 'default' ) {
	return esc_html( translate( $text, $domain ) );
}

/**
 * Displays the returned translated text from translate().
 *
 * @see translate() Echoes returned translate() string
 * @since 1.2.0
 *
 * @param string $text Text to translate
 * @param string $domain Optional. Domain to retrieve the translated text
 */
function _e( $text, $domain = 'default' ) {
	echo translate( $text, $domain );
}

/**
 * Displays translated text that has been escaped for safe use in an attribute.
 *
 * @see translate() Echoes returned translate() string
 * @see esc_attr()
 * @since 2.8.0
 *
 * @param string $text Text to translate
 * @param string $domain Optional. Domain to retrieve the translated text
 */
function esc_attr_e( $text, $domain = 'default' ) {
	echo esc_attr( translate( $text, $domain ) );
}

/**
 * Displays translated text that has been escaped for safe use in HTML output.
 *
 * @see translate() Echoes returned translate() string
 * @see esc_html()
 * @since 2.8.0
 *
 * @param string $text Text to translate
 * @param string $domain Optional. Domain to retrieve the translated text
 */
function esc_html_e( $text, $domain = 'default' ) {
	echo esc_html( translate( $text, $domain ) );
}

/**
 * Retrieve translated string with gettext context
 *
 * Quite a few times, there will be collisions with similar translatable text
 * found in more than two places but with different translated context.
 *
 * By including the context in the pot file translators can translate the two
 * string differently.
 *
 * @since 2.8.0
 *
 * @param string $text Text to translate
 * @param string $context Context information for the translators
 * @param string $domain Optional. Domain to retrieve the translated text
 * @return string Translated context string without pipe
 */
function _x( $text, $context, $domain = 'default' ) {
	return translate_with_gettext_context( $text, $context, $domain );
}

/**
 * Displays translated string with gettext context
 *
 * @see _x
 * @since 3.0.0
 *
 * @param string $text Text to translate
 * @param string $context Context information for the translators
 * @param string $domain Optional. Domain to retrieve the translated text
 * @return string Translated context string without pipe
 */
function _ex( $text, $context, $domain = 'default' ) {
	echo _x( $text, $context, $domain );
}

function esc_attr_x( $single, $context, $domain = 'default' ) {
	return esc_attr( translate_with_gettext_context( $single, $context, $domain ) );
}

function esc_html_x( $single, $context, $domain = 'default' ) {
	return esc_html( translate_with_gettext_context( $single, $context, $domain ) );
}

/**
 * Retrieve the plural or single form based on the amount.
 *
 * If the domain is not set in the $l10n list, then a comparison will be made
 * and either $plural or $single parameters returned.
 *
 * If the domain does exist, then the parameters $single, $plural, and $number
 * will first be passed to the domain's ngettext method. Then it will be passed
 * to the 'ngettext' filter hook along with the same parameters. The expected
 * type will be a string.
 *
 * @since 2.8.0
 * @uses $l10n Gets list of domain translated string (gettext_reader) objects
 * @uses apply_filters() Calls 'ngettext' hook on domains text returned,
 *		along with $single, $plural, and $number parameters. Expected to return string.
 *
 * @param string $single The text that will be used if $number is 1
 * @param string $plural The text that will be used if $number is not 1
 * @param int $number The number to compare against to use either $single or $plural
 * @param string $domain Optional. The domain identifier the text should be retrieved in
 * @return string Either $single or $plural translated text
 */
function _n( $single, $plural, $number, $domain = 'default' ) {
	$translations = &get_translations_for_domain( $domain );
	$translation = $translations->translate_plural( $single, $plural, $number );
	return apply_filters( 'ngettext', $translation, $single, $plural, $number, $domain );
}

/**
 * A hybrid of _n() and _x(). It supports contexts and plurals.
 *
 * @see _n()
 * @see _x()
 *
 */
function _nx($single, $plural, $number, $context, $domain = 'default') {
	$translations = &get_translations_for_domain( $domain );
	$translation = $translations->translate_plural( $single, $plural, $number, $context );
	return apply_filters( 'ngettext_with_context', $translation, $single, $plural, $number, $context, $domain );
}

/**
 * Register plural strings in POT file, but don't translate them.
 *
 * Used when you want do keep structures with translatable plural strings and
 * use them later.
 *
 * Example:
 *  $messages = array(
 *  	'post' => _n_noop('%s post', '%s posts'),
 *  	'page' => _n_noop('%s pages', '%s pages')
 *  );
 *  ...
 *  $message = $messages[$type];
 *  $usable_text = sprintf( translate_nooped_plural( $message, $count ), $count );
 *
 * @since 2.5
 * @param string $singular Single form to be i18ned
 * @param string $plural Plural form to be i18ned
 * @return array array($singular, $plural)
 */
function _n_noop( $singular, $plural ) {
	return array( 0 => $singular, 1 => $plural, 'singular' => $singular, 'plural' => $plural, 'context' => null );
}

/**
 * Register plural strings with context in POT file, but don't translate them.
 *
 * @see _n_noop()
 */
function _nx_noop( $singular, $plural, $context ) {
	return array( 0 => $singular, 1 => $plural, 2 => $context, 'singular' => $singular, 'plural' => $plural, 'context' => $context );
}

/**
 * Translate the result of _n_noop() or _nx_noop()
 *
 * @since 3.1
 * @param array $nooped_plural array with singular, plural and context keys, usually the result of _n_noop() or _nx_noop()
 * @param int $count number of objects
 * @param string $domain Optional. The domain identifier the text should be retrieved in
 */
function translate_nooped_plural( $nooped_plural, $count, $domain = 'default' ) {
	if ( $nooped_plural['context'] )
		return _nx( $nooped_plural['singular'], $nooped_plural['plural'], $count, $nooped_plural['context'], $domain );
	else
		return _n( $nooped_plural['singular'], $nooped_plural['plural'], $count, $domain );
}

/**
 * Loads a MO file into the domain $domain.
 *
 * If the domain already exists, the translations will be merged. If both
 * sets have the same string, the translation from the original value will be taken.
 *
 * On success, the .mo file will be placed in the $l10n global by $domain
 * and will be a MO object.
 *
 * @since 1.5.0
 * @uses $l10n Gets list of domain translated string objects
 *
 * @param string $domain Unique identifier for retrieving translated strings
 * @param string $mofile Path to the .mo file
 * @return bool true on success, false on failure
 */
function load_textdomain( $domain, $mofile ) {
	global $l10n;

	$plugin_override = apply_filters( 'override_load_textdomain', false, $domain, $mofile );

	if ( true == $plugin_override ) {
		return true;
	}

	do_action( 'load_textdomain', $domain, $mofile );

	$mofile = apply_filters( 'load_textdomain_mofile', $mofile, $domain );

	if ( !is_readable( $mofile ) ) return false;

	$mo = new MO();
	if ( !$mo->import_from_file( $mofile ) ) return false;

	if ( isset( $l10n[$domain] ) )
		$mo->merge_with( $l10n[$domain] );

	$l10n[$domain] = &$mo;

	return true;
}

/**
 * Unloads translations for a domain
 *
 * @since 3.0.0
 * @param string $domain Textdomain to be unloaded
 * @return bool Whether textdomain was unloaded
 */
function unload_textdomain( $domain ) {
	global $l10n;

	$plugin_override = apply_filters( 'override_unload_textdomain', false, $domain );

	if ( $plugin_override )
		return true;

	do_action( 'unload_textdomain', $domain );

	if ( isset( $l10n[$domain] ) ) {
		unset( $l10n[$domain] );
		return true;
	}

	return false;
}

/**
 * Loads default translated strings based on locale.
 *
 * Loads the .mo file in WP_LANG_DIR constant path from WordPress root. The
 * translated (.mo) file is named based off of the locale.
 *
 * @since 1.5.0
 */
function load_default_textdomain() {
	$locale = get_locale();

	load_textdomain( 'default', WP_LANG_DIR . "/$locale.mo" );

	if ( is_multisite() || ( defined( 'WP_NETWORK_ADMIN_PAGE' ) && WP_NETWORK_ADMIN_PAGE ) ) {
		load_textdomain( 'default', WP_LANG_DIR . "/ms-$locale.mo" );
	}
}

/**
 * Loads the plugin's translated strings.
 *
 * If the path is not given then it will be the root of the plugin directory.
 * The .mo file should be named based on the domain with a dash, and then the locale exactly.
 *
 * @since 1.5.0
 *
 * @param string $domain Unique identifier for retrieving translated strings
 * @param string $abs_rel_path Optional. Relative path to ABSPATH of a folder,
 * 	where the .mo file resides. Deprecated, but still functional until 2.7
 * @param string $plugin_rel_path Optional. Relative path to WP_PLUGIN_DIR. This is the preferred argument to use. It takes precendence over $abs_rel_path
 */
function load_plugin_textdomain( $domain, $abs_rel_path = false, $plugin_rel_path = false ) {
	$locale = apply_filters( 'plugin_locale', get_locale(), $domain );

	if ( false !== $plugin_rel_path	) {
		$path = WP_PLUGIN_DIR . '/' . trim( $plugin_rel_path, '/' );
	} else if ( false !== $abs_rel_path ) {
		_deprecated_argument( __FUNCTION__, '2.7' );
		$path = ABSPATH . trim( $abs_rel_path, '/' );
	} else {
		$path = WP_PLUGIN_DIR;
	}

	$mofile = $path . '/'. $domain . '-' . $locale . '.mo';
	return load_textdomain( $domain, $mofile );
}

/**
 * Load the translated strings for a plugin residing in the mu-plugins dir.
 *
 * @since 3.0.0
 *
 * @param string $domain Unique identifier for retrieving translated strings
 * @param strings $mu_plugin_rel_path Relative to WPMU_PLUGIN_DIR directory in which
 * the MO file resides. Defaults is empty string.
 */
function load_muplugin_textdomain( $domain, $mu_plugin_rel_path = '' ) {
	$locale = apply_filters( 'plugin_locale', get_locale(), $domain );
	$path = WPMU_PLUGIN_DIR . '/' . ltrim( $mu_plugin_rel_path, '/' );
	load_textdomain( $domain, trailingslashit( $path ) . "$domain-$locale.mo" );
}

/**
 * Loads the theme's translated strings.
 *
 * If the current locale exists as a .mo file in the theme's root directory, it
 * will be included in the translated strings by the $domain.
 *
 * The .mo files must be named based on the locale exactly.
 *
 * @since 1.5.0
 *
 * @param string $domain Unique identifier for retrieving translated strings
 */
function load_theme_textdomain( $domain, $path = false ) {
	$locale = apply_filters( 'theme_locale', get_locale(), $domain );

	$path = ( empty( $path ) ) ? get_template_directory() : $path;

	$mofile = "$path/$locale.mo";
	return load_textdomain($domain, $mofile);
}

/**
 * Loads the child themes translated strings.
 *
 * If the current locale exists as a .mo file in the child themes root directory, it
 * will be included in the translated strings by the $domain.
 *
 * The .mo files must be named based on the locale exactly.
 *
 * @since 2.9.0
 *
 * @param string $domain Unique identifier for retrieving translated strings
 */
function load_child_theme_textdomain( $domain, $path = false ) {
	$locale = apply_filters( 'theme_locale', get_locale(), $domain );

	$path = ( empty( $path ) ) ? get_stylesheet_directory() : $path;

	$mofile = "$path/$locale.mo";
	return load_textdomain($domain, $mofile);
}

/**
 * Returns the Translations instance for a domain. If there isn't one,
 * returns empty Translations instance.
 *
 * @param string $domain
 * @return object A Translation instance
 */
function &get_translations_for_domain( $domain ) {
	global $l10n;
	if ( !isset( $l10n[$domain] ) ) {
		$l10n[$domain] = &new NOOP_Translations;
	}
	return $l10n[$domain];
}

/**
 * Whether there are translations for the domain
 *
 * @since 3.0.0
 * @param string $domain
 * @return bool Whether there are translations
 */
function is_textdomain_loaded( $domain ) {
	global $l10n;
	return isset( $l10n[$domain] );
}

/**
 * Translates role name. Since the role names are in the database and
 * not in the source there are dummy gettext calls to get them into the POT
 * file and this function properly translates them back.
 *
 * The before_last_bar() call is needed, because older installs keep the roles
 * using the old context format: 'Role name|User role' and just skipping the
 * content after the last bar is easier than fixing them in the DB. New installs
 * won't suffer from that problem.
 */
function translate_user_role( $name ) {
	return translate_with_gettext_context( before_last_bar($name), 'User role' );
}

/**
 * Get all available languages based on the presence of *.mo files in a given directory. The default directory is WP_LANG_DIR.
 *
 * @since 3.0.0
 *
 * @param string $dir A directory in which to search for language files. The default directory is WP_LANG_DIR.
 * @return array Array of language codes or an empty array if no languages are present.  Language codes are formed by stripping the .mo extension from the language file names.
 */
function get_available_languages( $dir = null ) {
	$languages = array();

	foreach( (array)glob( ( is_null( $dir) ? WP_LANG_DIR : $dir ) . '/*.mo' ) as $lang_file ) {
		$lang_file = basename($lang_file, '.mo');
		if ( 0 !== strpos( $lang_file, 'continents-cities' ) && 0 !== strpos( $lang_file, 'ms-' ) )
			$languages[] = $lang_file;
	}

	return $languages;
}