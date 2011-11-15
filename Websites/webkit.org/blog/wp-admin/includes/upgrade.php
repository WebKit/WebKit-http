<?php
/**
 * WordPress Upgrade API
 *
 * Most of the functions are pluggable and can be overwritten
 *
 * @package WordPress
 * @subpackage Administration
 */

/** Include user install customize script. */
if ( file_exists(WP_CONTENT_DIR . '/install.php') )
	require (WP_CONTENT_DIR . '/install.php');

/** WordPress Administration API */
require_once(ABSPATH . 'wp-admin/includes/admin.php');

/** WordPress Schema API */
require_once(ABSPATH . 'wp-admin/includes/schema.php');

if ( !function_exists('wp_install') ) :
/**
 * Installs the blog
 *
 * {@internal Missing Long Description}}
 *
 * @since 2.1.0
 *
 * @param string $blog_title Blog title.
 * @param string $user_name User's username.
 * @param string $user_email User's email.
 * @param bool $public Whether blog is public.
 * @param null $deprecated Optional. Not used.
 * @param string $user_password Optional. User's chosen password. Will default to a random password.
 * @return array Array keys 'url', 'user_id', 'password', 'password_message'.
 */
function wp_install( $blog_title, $user_name, $user_email, $public, $deprecated = '', $user_password = '' ) {
	global $wp_rewrite;

	if ( !empty( $deprecated ) )
		_deprecated_argument( __FUNCTION__, '2.6' );

	wp_check_mysql_version();
	wp_cache_flush();
	make_db_current_silent();
	populate_options();
	populate_roles();

	update_option('blogname', $blog_title);
	update_option('admin_email', $user_email);
	update_option('blog_public', $public);

	$guessurl = wp_guess_url();

	update_option('siteurl', $guessurl);

	// If not a public blog, don't ping.
	if ( ! $public )
		update_option('default_pingback_flag', 0);

	// Create default user.  If the user already exists, the user tables are
	// being shared among blogs.  Just set the role in that case.
	$user_id = username_exists($user_name);
	$user_password = trim($user_password);
	$email_password = false;
	if ( !$user_id && empty($user_password) ) {
		$user_password = wp_generate_password( 12, false );
		$message = __('<strong><em>Note that password</em></strong> carefully! It is a <em>random</em> password that was generated just for you.');
		$user_id = wp_create_user($user_name, $user_password, $user_email);
		update_user_option($user_id, 'default_password_nag', true, true);
		$email_password = true;
	} else if ( !$user_id ) {
		// Password has been provided
		$message = '<em>'.__('Your chosen password.').'</em>';
		$user_id = wp_create_user($user_name, $user_password, $user_email);
	} else {
		$message =  __('User already exists. Password inherited.');
	}

	$user = new WP_User($user_id);
	$user->set_role('administrator');

	wp_install_defaults($user_id);

	$wp_rewrite->flush_rules();

	wp_new_blog_notification($blog_title, $guessurl, $user_id, ($email_password ? $user_password : __('The password you chose during the install.') ) );

	wp_cache_flush();

	return array('url' => $guessurl, 'user_id' => $user_id, 'password' => $user_password, 'password_message' => $message);
}
endif;

if ( !function_exists('wp_install_defaults') ) :
/**
 * {@internal Missing Short Description}}
 *
 * {@internal Missing Long Description}}
 *
 * @since 2.1.0
 *
 * @param int $user_id User ID.
 */
function wp_install_defaults($user_id) {
	global $wpdb, $wp_rewrite, $current_site, $table_prefix;

	// Default category
	$cat_name = __('Uncategorized');
	/* translators: Default category slug */
	$cat_slug = sanitize_title(_x('Uncategorized', 'Default category slug'));

	if ( global_terms_enabled() ) {
		$cat_id = $wpdb->get_var( $wpdb->prepare( "SELECT cat_ID FROM {$wpdb->sitecategories} WHERE category_nicename = %s", $cat_slug ) );
		if ( $cat_id == null ) {
			$wpdb->insert( $wpdb->sitecategories, array('cat_ID' => 0, 'cat_name' => $cat_name, 'category_nicename' => $cat_slug, 'last_updated' => current_time('mysql', true)) );
			$cat_id = $wpdb->insert_id;
		}
		update_option('default_category', $cat_id);
	} else {
		$cat_id = 1;
	}

	$wpdb->insert( $wpdb->terms, array('term_id' => $cat_id, 'name' => $cat_name, 'slug' => $cat_slug, 'term_group' => 0) );
	$wpdb->insert( $wpdb->term_taxonomy, array('term_id' => $cat_id, 'taxonomy' => 'category', 'description' => '', 'parent' => 0, 'count' => 1));
	$cat_tt_id = $wpdb->insert_id;

	// Default link category
	$cat_name = __('Blogroll');
	/* translators: Default link category slug */
	$cat_slug = sanitize_title(_x('Blogroll', 'Default link category slug'));

	if ( global_terms_enabled() ) {
		$blogroll_id = $wpdb->get_var( $wpdb->prepare( "SELECT cat_ID FROM {$wpdb->sitecategories} WHERE category_nicename = %s", $cat_slug ) );
		if ( $blogroll_id == null ) {
			$wpdb->insert( $wpdb->sitecategories, array('cat_ID' => 0, 'cat_name' => $cat_name, 'category_nicename' => $cat_slug, 'last_updated' => current_time('mysql', true)) );
			$blogroll_id = $wpdb->insert_id;
		}
		update_option('default_link_category', $blogroll_id);
	} else {
		$blogroll_id = 2;
	}

	$wpdb->insert( $wpdb->terms, array('term_id' => $blogroll_id, 'name' => $cat_name, 'slug' => $cat_slug, 'term_group' => 0) );
	$wpdb->insert( $wpdb->term_taxonomy, array('term_id' => $blogroll_id, 'taxonomy' => 'link_category', 'description' => '', 'parent' => 0, 'count' => 7));
	$blogroll_tt_id = $wpdb->insert_id;

	// Now drop in some default links
	$default_links = array();
	$default_links[] = array(	'link_url' => 'http://codex.wordpress.org/',
								'link_name' => 'Documentation',
								'link_rss' => '',
								'link_notes' => '');

	$default_links[] = array(	'link_url' => 'http://wordpress.org/news/',
								'link_name' => 'WordPress Blog',
								'link_rss' => 'http://wordpress.org/news/feed/',
								'link_notes' => '');

	$default_links[] = array(	'link_url' => 'http://wordpress.org/extend/ideas/',
								'link_name' => 'Suggest Ideas',
								'link_rss' => '',
								'link_notes' =>'');

	$default_links[] = array(	'link_url' => 'http://wordpress.org/support/',
								'link_name' => 'Support Forum',
								'link_rss' => '',
								'link_notes' =>'');

	$default_links[] = array(	'link_url' => 'http://wordpress.org/extend/plugins/',
								'link_name' => 'Plugins',
								'link_rss' => '',
								'link_notes' =>'');

	$default_links[] = array(	'link_url' => 'http://wordpress.org/extend/themes/',
								'link_name' => 'Themes',
								'link_rss' => '',
								'link_notes' =>'');

	$default_links[] = array(	'link_url' => 'http://planet.wordpress.org/',
								'link_name' => 'WordPress Planet',
								'link_rss' => '',
								'link_notes' =>'');

	foreach ( $default_links as $link ) {
		$wpdb->insert( $wpdb->links, $link);
		$wpdb->insert( $wpdb->term_relationships, array('term_taxonomy_id' => $blogroll_tt_id, 'object_id' => $wpdb->insert_id) );
	}

	// First post
	$now = date('Y-m-d H:i:s');
	$now_gmt = gmdate('Y-m-d H:i:s');
	$first_post_guid = get_option('home') . '/?p=1';

	if ( is_multisite() ) {
		$first_post = get_site_option( 'first_post' );

		if ( empty($first_post) )
			$first_post = stripslashes( __( 'Welcome to <a href="SITE_URL">SITE_NAME</a>. This is your first post. Edit or delete it, then start blogging!' ) );

		$first_post = str_replace( "SITE_URL", esc_url( network_home_url() ), $first_post );
		$first_post = str_replace( "SITE_NAME", $current_site->site_name, $first_post );
	} else {
		$first_post = __('Welcome to WordPress. This is your first post. Edit or delete it, then start blogging!');
	}

	$wpdb->insert( $wpdb->posts, array(
								'post_author' => $user_id,
								'post_date' => $now,
								'post_date_gmt' => $now_gmt,
								'post_content' => $first_post,
								'post_excerpt' => '',
								'post_title' => __('Hello world!'),
								/* translators: Default post slug */
								'post_name' => sanitize_title( _x('hello-world', 'Default post slug') ),
								'post_modified' => $now,
								'post_modified_gmt' => $now_gmt,
								'guid' => $first_post_guid,
								'comment_count' => 1,
								'to_ping' => '',
								'pinged' => '',
								'post_content_filtered' => ''
								));
	$wpdb->insert( $wpdb->term_relationships, array('term_taxonomy_id' => $cat_tt_id, 'object_id' => 1) );

	// Default comment
	$first_comment_author = __('Mr WordPress');
	$first_comment_url = 'http://wordpress.org/';
	$first_comment = __('Hi, this is a comment.<br />To delete a comment, just log in and view the post&#039;s comments. There you will have the option to edit or delete them.');
	if ( is_multisite() ) {
		$first_comment_author = get_site_option( 'first_comment_author', $first_comment_author );
		$first_comment_url = get_site_option( 'first_comment_url', network_home_url() );
		$first_comment = get_site_option( 'first_comment', $first_comment );
	}
	$wpdb->insert( $wpdb->comments, array(
								'comment_post_ID' => 1,
								'comment_author' => $first_comment_author,
								'comment_author_email' => '',
								'comment_author_url' => $first_comment_url,
								'comment_date' => $now,
								'comment_date_gmt' => $now_gmt,
								'comment_content' => $first_comment
								));

	// First Page
	$first_page = sprintf( __( "This is an example page. It's different from a blog post because it will stay in one place and will show up in your site navigation (in most themes). Most people start with an About page that introduces them to potential site visitors. It might say something like this:

<blockquote>Hi there! I'm a bike messenger by day, aspiring actor by night, and this is my blog. I live in Los Angeles, have a great dog named Jack, and I like pi&#241;a coladas. (And gettin' caught in the rain.)</blockquote>

...or something like this:

<blockquote>The XYZ Doohickey Company was founded in 1971, and has been providing quality doohickies to the public ever since. Located in Gotham City, XYZ employs over 2,000 people and does all kinds of awesome things for the Gotham community.</blockquote>

As a new WordPress user, you should go to <a href=\"%s\">your dashboard</a> to delete this page and create new pages for your content. Have fun!" ), admin_url() );
	if ( is_multisite() )
		$first_page = get_site_option( 'first_page', $first_page );
	$first_post_guid = get_option('home') . '/?page_id=2';
	$wpdb->insert( $wpdb->posts, array(
								'post_author' => $user_id,
								'post_date' => $now,
								'post_date_gmt' => $now_gmt,
								'post_content' => $first_page,
								'post_excerpt' => '',
								'post_title' => __( 'Sample Page' ),
								/* translators: Default page slug */
								'post_name' => __( 'sample-page' ),
								'post_modified' => $now,
								'post_modified_gmt' => $now_gmt,
								'guid' => $first_post_guid,
								'post_type' => 'page',
								'to_ping' => '',
								'pinged' => '',
								'post_content_filtered' => ''
								));
	$wpdb->insert( $wpdb->postmeta, array( 'post_id' => 2, 'meta_key' => '_wp_page_template', 'meta_value' => 'default' ) );

	// Set up default widgets for default theme.
	update_option( 'widget_search', array ( 2 => array ( 'title' => '' ), '_multiwidget' => 1 ) );
	update_option( 'widget_recent-posts', array ( 2 => array ( 'title' => '', 'number' => 5 ), '_multiwidget' => 1 ) );
	update_option( 'widget_recent-comments', array ( 2 => array ( 'title' => '', 'number' => 5 ), '_multiwidget' => 1 ) );
	update_option( 'widget_archives', array ( 2 => array ( 'title' => '', 'count' => 0, 'dropdown' => 0 ), '_multiwidget' => 1 ) );
	update_option( 'widget_categories', array ( 2 => array ( 'title' => '', 'count' => 0, 'hierarchical' => 0, 'dropdown' => 0 ), '_multiwidget' => 1 ) );
	update_option( 'widget_meta', array ( 2 => array ( 'title' => '' ), '_multiwidget' => 1 ) );
	update_option( 'sidebars_widgets', array ( 'wp_inactive_widgets' => array ( ), 'primary-widget-area' => array ( 0 => 'search-2', 1 => 'recent-posts-2', 2 => 'recent-comments-2', 3 => 'archives-2', 4 => 'categories-2', 5 => 'meta-2', ), 'secondary-widget-area' => array ( ), 'first-footer-widget-area' => array ( ), 'second-footer-widget-area' => array ( ), 'third-footer-widget-area' => array ( ), 'fourth-footer-widget-area' => array ( ), 'array_version' => 3 ) );

	if ( is_multisite() ) {
		// Flush rules to pick up the new page.
		$wp_rewrite->init();
		$wp_rewrite->flush_rules();

		$user = new WP_User($user_id);
		$wpdb->update( $wpdb->options, array('option_value' => $user->user_email), array('option_name' => 'admin_email') );

		// Remove all perms except for the login user.
		$wpdb->query( $wpdb->prepare("DELETE FROM $wpdb->usermeta WHERE user_id != %d AND meta_key = %s", $user_id, $table_prefix.'user_level') );
		$wpdb->query( $wpdb->prepare("DELETE FROM $wpdb->usermeta WHERE user_id != %d AND meta_key = %s", $user_id, $table_prefix.'capabilities') );

		// Delete any caps that snuck into the previously active blog. (Hardcoded to blog 1 for now.) TODO: Get previous_blog_id.
		if ( !is_super_admin( $user_id ) && $user_id != 1 )
			$wpdb->query( $wpdb->prepare("DELETE FROM $wpdb->usermeta WHERE user_id = %d AND meta_key = %s", $user_id, $wpdb->base_prefix.'1_capabilities') );
	}
}
endif;

if ( !function_exists('wp_new_blog_notification') ) :
/**
 * {@internal Missing Short Description}}
 *
 * {@internal Missing Long Description}}
 *
 * @since 2.1.0
 *
 * @param string $blog_title Blog title.
 * @param string $blog_url Blog url.
 * @param int $user_id User ID.
 * @param string $password User's Password.
 */
function wp_new_blog_notification($blog_title, $blog_url, $user_id, $password) {
	$user = new WP_User($user_id);
	$email = $user->user_email;
	$name = $user->user_login;
	$message = sprintf(__("Your new WordPress site has been successfully set up at:

%1\$s

You can log in to the administrator account with the following information:

Username: %2\$s
Password: %3\$s

We hope you enjoy your new site. Thanks!

--The WordPress Team
http://wordpress.org/
"), $blog_url, $name, $password);

	@wp_mail($email, __('New WordPress Site'), $message);
}
endif;

if ( !function_exists('wp_upgrade') ) :
/**
 * Run WordPress Upgrade functions.
 *
 * {@internal Missing Long Description}}
 *
 * @since 2.1.0
 *
 * @return null
 */
function wp_upgrade() {
	global $wp_current_db_version, $wp_db_version, $wpdb;

	$wp_current_db_version = __get_option('db_version');

	// We are up-to-date.  Nothing to do.
	if ( $wp_db_version == $wp_current_db_version )
		return;

	if ( ! is_blog_installed() )
		return;

	wp_check_mysql_version();
	wp_cache_flush();
	pre_schema_upgrade();
	make_db_current_silent();
	upgrade_all();
	if ( is_multisite() && is_main_site() )
		upgrade_network();
	wp_cache_flush();

	if ( is_multisite() ) {
		if ( $wpdb->get_row( "SELECT blog_id FROM {$wpdb->blog_versions} WHERE blog_id = '{$wpdb->blogid}'" ) )
			$wpdb->query( "UPDATE {$wpdb->blog_versions} SET db_version = '{$wp_db_version}' WHERE blog_id = '{$wpdb->blogid}'" );
		else
			$wpdb->query( "INSERT INTO {$wpdb->blog_versions} ( `blog_id` , `db_version` , `last_updated` ) VALUES ( '{$wpdb->blogid}', '{$wp_db_version}', NOW());" );
	}
}
endif;

/**
 * Functions to be called in install and upgrade scripts.
 *
 * {@internal Missing Long Description}}
 *
 * @since 1.0.1
 */
function upgrade_all() {
	global $wp_current_db_version, $wp_db_version, $wp_rewrite;
	$wp_current_db_version = __get_option('db_version');

	// We are up-to-date.  Nothing to do.
	if ( $wp_db_version == $wp_current_db_version )
		return;

	// If the version is not set in the DB, try to guess the version.
	if ( empty($wp_current_db_version) ) {
		$wp_current_db_version = 0;

		// If the template option exists, we have 1.5.
		$template = __get_option('template');
		if ( !empty($template) )
			$wp_current_db_version = 2541;
	}

	if ( $wp_current_db_version < 6039 )
		upgrade_230_options_table();

	populate_options();

	if ( $wp_current_db_version < 2541 ) {
		upgrade_100();
		upgrade_101();
		upgrade_110();
		upgrade_130();
	}

	if ( $wp_current_db_version < 3308 )
		upgrade_160();

	if ( $wp_current_db_version < 4772 )
		upgrade_210();

	if ( $wp_current_db_version < 4351 )
		upgrade_old_slugs();

	if ( $wp_current_db_version < 5539 )
		upgrade_230();

	if ( $wp_current_db_version < 6124 )
		upgrade_230_old_tables();

	if ( $wp_current_db_version < 7499 )
		upgrade_250();

	if ( $wp_current_db_version < 7935 )
		upgrade_252();

	if ( $wp_current_db_version < 8201 )
		upgrade_260();

	if ( $wp_current_db_version < 8989 )
		upgrade_270();

	if ( $wp_current_db_version < 10360 )
		upgrade_280();

	if ( $wp_current_db_version < 11958 )
		upgrade_290();

	if ( $wp_current_db_version < 15260 )
		upgrade_300();

	maybe_disable_automattic_widgets();

	update_option( 'db_version', $wp_db_version );
	update_option( 'db_upgraded', true );
}

/**
 * Execute changes made in WordPress 1.0.
 *
 * @since 1.0.0
 */
function upgrade_100() {
	global $wpdb;

	// Get the title and ID of every post, post_name to check if it already has a value
	$posts = $wpdb->get_results("SELECT ID, post_title, post_name FROM $wpdb->posts WHERE post_name = ''");
	if ($posts) {
		foreach($posts as $post) {
			if ('' == $post->post_name) {
				$newtitle = sanitize_title($post->post_title);
				$wpdb->query( $wpdb->prepare("UPDATE $wpdb->posts SET post_name = %s WHERE ID = %d", $newtitle, $post->ID) );
			}
		}
	}

	$categories = $wpdb->get_results("SELECT cat_ID, cat_name, category_nicename FROM $wpdb->categories");
	foreach ($categories as $category) {
		if ('' == $category->category_nicename) {
			$newtitle = sanitize_title($category->cat_name);
			$wpdb>update( $wpdb->categories, array('category_nicename' => $newtitle), array('cat_ID' => $category->cat_ID) );
		}
	}

	$wpdb->query("UPDATE $wpdb->options SET option_value = REPLACE(option_value, 'wp-links/links-images/', 'wp-images/links/')
	WHERE option_name LIKE 'links_rating_image%'
	AND option_value LIKE 'wp-links/links-images/%'");

	$done_ids = $wpdb->get_results("SELECT DISTINCT post_id FROM $wpdb->post2cat");
	if ($done_ids) :
		foreach ($done_ids as $done_id) :
			$done_posts[] = $done_id->post_id;
		endforeach;
		$catwhere = ' AND ID NOT IN (' . implode(',', $done_posts) . ')';
	else:
		$catwhere = '';
	endif;

	$allposts = $wpdb->get_results("SELECT ID, post_category FROM $wpdb->posts WHERE post_category != '0' $catwhere");
	if ($allposts) :
		foreach ($allposts as $post) {
			// Check to see if it's already been imported
			$cat = $wpdb->get_row( $wpdb->prepare("SELECT * FROM $wpdb->post2cat WHERE post_id = %d AND category_id = %d", $post->ID, $post->post_category) );
			if (!$cat && 0 != $post->post_category) { // If there's no result
				$wpdb->insert( $wpdb->post2cat, array('post_id' => $post->ID, 'category_id' => $post->post_category) );
			}
		}
	endif;
}

/**
 * Execute changes made in WordPress 1.0.1.
 *
 * @since 1.0.1
 */
function upgrade_101() {
	global $wpdb;

	// Clean up indices, add a few
	add_clean_index($wpdb->posts, 'post_name');
	add_clean_index($wpdb->posts, 'post_status');
	add_clean_index($wpdb->categories, 'category_nicename');
	add_clean_index($wpdb->comments, 'comment_approved');
	add_clean_index($wpdb->comments, 'comment_post_ID');
	add_clean_index($wpdb->links , 'link_category');
	add_clean_index($wpdb->links , 'link_visible');
}

/**
 * Execute changes made in WordPress 1.2.
 *
 * @since 1.2.0
 */
function upgrade_110() {
	global $wpdb;

	// Set user_nicename.
	$users = $wpdb->get_results("SELECT ID, user_nickname, user_nicename FROM $wpdb->users");
	foreach ($users as $user) {
		if ('' == $user->user_nicename) {
			$newname = sanitize_title($user->user_nickname);
			$wpdb->update( $wpdb->users, array('user_nicename' => $newname), array('ID' => $user->ID) );
		}
	}

	$users = $wpdb->get_results("SELECT ID, user_pass from $wpdb->users");
	foreach ($users as $row) {
		if (!preg_match('/^[A-Fa-f0-9]{32}$/', $row->user_pass)) {
			$wpdb->update( $wpdb->users, array('user_pass' => md5($row->user_pass)), array('ID' => $row->ID) );
		}
	}

	// Get the GMT offset, we'll use that later on
	$all_options = get_alloptions_110();

	$time_difference = $all_options->time_difference;

	$server_time = time()+date('Z');
	$weblogger_time = $server_time + $time_difference*3600;
	$gmt_time = time();

	$diff_gmt_server = ($gmt_time - $server_time) / 3600;
	$diff_weblogger_server = ($weblogger_time - $server_time) / 3600;
	$diff_gmt_weblogger = $diff_gmt_server - $diff_weblogger_server;
	$gmt_offset = -$diff_gmt_weblogger;

	// Add a gmt_offset option, with value $gmt_offset
	add_option('gmt_offset', $gmt_offset);

	// Check if we already set the GMT fields (if we did, then
	// MAX(post_date_gmt) can't be '0000-00-00 00:00:00'
	// <michel_v> I just slapped myself silly for not thinking about it earlier
	$got_gmt_fields = ! ($wpdb->get_var("SELECT MAX(post_date_gmt) FROM $wpdb->posts") == '0000-00-00 00:00:00');

	if (!$got_gmt_fields) {

		// Add or substract time to all dates, to get GMT dates
		$add_hours = intval($diff_gmt_weblogger);
		$add_minutes = intval(60 * ($diff_gmt_weblogger - $add_hours));
		$wpdb->query("UPDATE $wpdb->posts SET post_date_gmt = DATE_ADD(post_date, INTERVAL '$add_hours:$add_minutes' HOUR_MINUTE)");
		$wpdb->query("UPDATE $wpdb->posts SET post_modified = post_date");
		$wpdb->query("UPDATE $wpdb->posts SET post_modified_gmt = DATE_ADD(post_modified, INTERVAL '$add_hours:$add_minutes' HOUR_MINUTE) WHERE post_modified != '0000-00-00 00:00:00'");
		$wpdb->query("UPDATE $wpdb->comments SET comment_date_gmt = DATE_ADD(comment_date, INTERVAL '$add_hours:$add_minutes' HOUR_MINUTE)");
		$wpdb->query("UPDATE $wpdb->users SET user_registered = DATE_ADD(user_registered, INTERVAL '$add_hours:$add_minutes' HOUR_MINUTE)");
	}

}

/**
 * Execute changes made in WordPress 1.5.
 *
 * @since 1.5.0
 */
function upgrade_130() {
	global $wpdb;

	// Remove extraneous backslashes.
	$posts = $wpdb->get_results("SELECT ID, post_title, post_content, post_excerpt, guid, post_date, post_name, post_status, post_author FROM $wpdb->posts");
	if ($posts) {
		foreach($posts as $post) {
			$post_content = addslashes(deslash($post->post_content));
			$post_title = addslashes(deslash($post->post_title));
			$post_excerpt = addslashes(deslash($post->post_excerpt));
			if ( empty($post->guid) )
				$guid = get_permalink($post->ID);
			else
				$guid = $post->guid;

			$wpdb->update( $wpdb->posts, compact('post_title', 'post_content', 'post_excerpt', 'guid'), array('ID' => $post->ID) );

		}
	}

	// Remove extraneous backslashes.
	$comments = $wpdb->get_results("SELECT comment_ID, comment_author, comment_content FROM $wpdb->comments");
	if ($comments) {
		foreach($comments as $comment) {
			$comment_content = deslash($comment->comment_content);
			$comment_author = deslash($comment->comment_author);

			$wpdb->update($wpdb->comments, compact('comment_content', 'comment_author'), array('comment_ID' => $comment->comment_ID) );
		}
	}

	// Remove extraneous backslashes.
	$links = $wpdb->get_results("SELECT link_id, link_name, link_description FROM $wpdb->links");
	if ($links) {
		foreach($links as $link) {
			$link_name = deslash($link->link_name);
			$link_description = deslash($link->link_description);

			$wpdb->update( $wpdb->links, compact('link_name', 'link_description'), array('link_id' => $link->link_id) );
		}
	}

	$active_plugins = __get_option('active_plugins');

	// If plugins are not stored in an array, they're stored in the old
	// newline separated format.  Convert to new format.
	if ( !is_array( $active_plugins ) ) {
		$active_plugins = explode("\n", trim($active_plugins));
		update_option('active_plugins', $active_plugins);
	}

	// Obsolete tables
	$wpdb->query('DROP TABLE IF EXISTS ' . $wpdb->prefix . 'optionvalues');
	$wpdb->query('DROP TABLE IF EXISTS ' . $wpdb->prefix . 'optiontypes');
	$wpdb->query('DROP TABLE IF EXISTS ' . $wpdb->prefix . 'optiongroups');
	$wpdb->query('DROP TABLE IF EXISTS ' . $wpdb->prefix . 'optiongroup_options');

	// Update comments table to use comment_type
	$wpdb->query("UPDATE $wpdb->comments SET comment_type='trackback', comment_content = REPLACE(comment_content, '<trackback />', '') WHERE comment_content LIKE '<trackback />%'");
	$wpdb->query("UPDATE $wpdb->comments SET comment_type='pingback', comment_content = REPLACE(comment_content, '<pingback />', '') WHERE comment_content LIKE '<pingback />%'");

	// Some versions have multiple duplicate option_name rows with the same values
	$options = $wpdb->get_results("SELECT option_name, COUNT(option_name) AS dupes FROM `$wpdb->options` GROUP BY option_name");
	foreach ( $options as $option ) {
		if ( 1 != $option->dupes ) { // Could this be done in the query?
			$limit = $option->dupes - 1;
			$dupe_ids = $wpdb->get_col( $wpdb->prepare("SELECT option_id FROM $wpdb->options WHERE option_name = %s LIMIT %d", $option->option_name, $limit) );
			if ( $dupe_ids ) {
				$dupe_ids = join($dupe_ids, ',');
				$wpdb->query("DELETE FROM $wpdb->options WHERE option_id IN ($dupe_ids)");
			}
		}
	}

	make_site_theme();
}

/**
 * Execute changes made in WordPress 2.0.
 *
 * @since 2.0.0
 */
function upgrade_160() {
	global $wpdb, $wp_current_db_version;

	populate_roles_160();

	$users = $wpdb->get_results("SELECT * FROM $wpdb->users");
	foreach ( $users as $user ) :
		if ( !empty( $user->user_firstname ) )
			update_user_meta( $user->ID, 'first_name', $wpdb->escape($user->user_firstname) );
		if ( !empty( $user->user_lastname ) )
			update_user_meta( $user->ID, 'last_name', $wpdb->escape($user->user_lastname) );
		if ( !empty( $user->user_nickname ) )
			update_user_meta( $user->ID, 'nickname', $wpdb->escape($user->user_nickname) );
		if ( !empty( $user->user_level ) )
			update_user_meta( $user->ID, $wpdb->prefix . 'user_level', $user->user_level );
		if ( !empty( $user->user_icq ) )
			update_user_meta( $user->ID, 'icq', $wpdb->escape($user->user_icq) );
		if ( !empty( $user->user_aim ) )
			update_user_meta( $user->ID, 'aim', $wpdb->escape($user->user_aim) );
		if ( !empty( $user->user_msn ) )
			update_user_meta( $user->ID, 'msn', $wpdb->escape($user->user_msn) );
		if ( !empty( $user->user_yim ) )
			update_user_meta( $user->ID, 'yim', $wpdb->escape($user->user_icq) );
		if ( !empty( $user->user_description ) )
			update_user_meta( $user->ID, 'description', $wpdb->escape($user->user_description) );

		if ( isset( $user->user_idmode ) ):
			$idmode = $user->user_idmode;
			if ($idmode == 'nickname') $id = $user->user_nickname;
			if ($idmode == 'login') $id = $user->user_login;
			if ($idmode == 'firstname') $id = $user->user_firstname;
			if ($idmode == 'lastname') $id = $user->user_lastname;
			if ($idmode == 'namefl') $id = $user->user_firstname.' '.$user->user_lastname;
			if ($idmode == 'namelf') $id = $user->user_lastname.' '.$user->user_firstname;
			if (!$idmode) $id = $user->user_nickname;
			$wpdb->update( $wpdb->users, array('display_name' => $id), array('ID' => $user->ID) );
		endif;

		// FIXME: RESET_CAPS is temporary code to reset roles and caps if flag is set.
		$caps = get_user_meta( $user->ID, $wpdb->prefix . 'capabilities');
		if ( empty($caps) || defined('RESET_CAPS') ) {
			$level = get_user_meta($user->ID, $wpdb->prefix . 'user_level', true);
			$role = translate_level_to_role($level);
			update_user_meta( $user->ID, $wpdb->prefix . 'capabilities', array($role => true) );
		}

	endforeach;
	$old_user_fields = array( 'user_firstname', 'user_lastname', 'user_icq', 'user_aim', 'user_msn', 'user_yim', 'user_idmode', 'user_ip', 'user_domain', 'user_browser', 'user_description', 'user_nickname', 'user_level' );
	$wpdb->hide_errors();
	foreach ( $old_user_fields as $old )
		$wpdb->query("ALTER TABLE $wpdb->users DROP $old");
	$wpdb->show_errors();

	// populate comment_count field of posts table
	$comments = $wpdb->get_results( "SELECT comment_post_ID, COUNT(*) as c FROM $wpdb->comments WHERE comment_approved = '1' GROUP BY comment_post_ID" );
	if ( is_array( $comments ) )
		foreach ($comments as $comment)
			$wpdb->update( $wpdb->posts, array('comment_count' => $comment->c), array('ID' => $comment->comment_post_ID) );

	// Some alpha versions used a post status of object instead of attachment and put
	// the mime type in post_type instead of post_mime_type.
	if ( $wp_current_db_version > 2541 && $wp_current_db_version <= 3091 ) {
		$objects = $wpdb->get_results("SELECT ID, post_type FROM $wpdb->posts WHERE post_status = 'object'");
		foreach ($objects as $object) {
			$wpdb->update( $wpdb->posts, array(	'post_status' => 'attachment',
												'post_mime_type' => $object->post_type,
												'post_type' => ''),
										 array( 'ID' => $object->ID ) );

			$meta = get_post_meta($object->ID, 'imagedata', true);
			if ( ! empty($meta['file']) )
				update_attached_file( $object->ID, $meta['file'] );
		}
	}
}

/**
 * Execute changes made in WordPress 2.1.
 *
 * @since 2.1.0
 */
function upgrade_210() {
	global $wpdb, $wp_current_db_version;

	if ( $wp_current_db_version < 3506 ) {
		// Update status and type.
		$posts = $wpdb->get_results("SELECT ID, post_status FROM $wpdb->posts");

		if ( ! empty($posts) ) foreach ($posts as $post) {
			$status = $post->post_status;
			$type = 'post';

			if ( 'static' == $status ) {
				$status = 'publish';
				$type = 'page';
			} else if ( 'attachment' == $status ) {
				$status = 'inherit';
				$type = 'attachment';
			}

			$wpdb->query( $wpdb->prepare("UPDATE $wpdb->posts SET post_status = %s, post_type = %s WHERE ID = %d", $status, $type, $post->ID) );
		}
	}

	if ( $wp_current_db_version < 3845 ) {
		populate_roles_210();
	}

	if ( $wp_current_db_version < 3531 ) {
		// Give future posts a post_status of future.
		$now = gmdate('Y-m-d H:i:59');
		$wpdb->query ("UPDATE $wpdb->posts SET post_status = 'future' WHERE post_status = 'publish' AND post_date_gmt > '$now'");

		$posts = $wpdb->get_results("SELECT ID, post_date FROM $wpdb->posts WHERE post_status ='future'");
		if ( !empty($posts) )
			foreach ( $posts as $post )
				wp_schedule_single_event(mysql2date('U', $post->post_date, false), 'publish_future_post', array($post->ID));
	}
}

/**
 * Execute changes made in WordPress 2.3.
 *
 * @since 2.3.0
 */
function upgrade_230() {
	global $wp_current_db_version, $wpdb;

	if ( $wp_current_db_version < 5200 ) {
		populate_roles_230();
	}

	// Convert categories to terms.
	$tt_ids = array();
	$have_tags = false;
	$categories = $wpdb->get_results("SELECT * FROM $wpdb->categories ORDER BY cat_ID");
	foreach ($categories as $category) {
		$term_id = (int) $category->cat_ID;
		$name = $category->cat_name;
		$description = $category->category_description;
		$slug = $category->category_nicename;
		$parent = $category->category_parent;
		$term_group = 0;

		// Associate terms with the same slug in a term group and make slugs unique.
		if ( $exists = $wpdb->get_results( $wpdb->prepare("SELECT term_id, term_group FROM $wpdb->terms WHERE slug = %s", $slug) ) ) {
			$term_group = $exists[0]->term_group;
			$id = $exists[0]->term_id;
			$num = 2;
			do {
				$alt_slug = $slug . "-$num";
				$num++;
				$slug_check = $wpdb->get_var( $wpdb->prepare("SELECT slug FROM $wpdb->terms WHERE slug = %s", $alt_slug) );
			} while ( $slug_check );

			$slug = $alt_slug;

			if ( empty( $term_group ) ) {
				$term_group = $wpdb->get_var("SELECT MAX(term_group) FROM $wpdb->terms GROUP BY term_group") + 1;
				$wpdb->query( $wpdb->prepare("UPDATE $wpdb->terms SET term_group = %d WHERE term_id = %d", $term_group, $id) );
			}
		}

		$wpdb->query( $wpdb->prepare("INSERT INTO $wpdb->terms (term_id, name, slug, term_group) VALUES
		(%d, %s, %s, %d)", $term_id, $name, $slug, $term_group) );

		$count = 0;
		if ( !empty($category->category_count) ) {
			$count = (int) $category->category_count;
			$taxonomy = 'category';
			$wpdb->query( $wpdb->prepare("INSERT INTO $wpdb->term_taxonomy (term_id, taxonomy, description, parent, count) VALUES ( %d, %s, %s, %d, %d)", $term_id, $taxonomy, $description, $parent, $count) );
			$tt_ids[$term_id][$taxonomy] = (int) $wpdb->insert_id;
		}

		if ( !empty($category->link_count) ) {
			$count = (int) $category->link_count;
			$taxonomy = 'link_category';
			$wpdb->query( $wpdb->prepare("INSERT INTO $wpdb->term_taxonomy (term_id, taxonomy, description, parent, count) VALUES ( %d, %s, %s, %d, %d)", $term_id, $taxonomy, $description, $parent, $count) );
			$tt_ids[$term_id][$taxonomy] = (int) $wpdb->insert_id;
		}

		if ( !empty($category->tag_count) ) {
			$have_tags = true;
			$count = (int) $category->tag_count;
			$taxonomy = 'post_tag';
			$wpdb->insert( $wpdb->term_taxonomy, compact('term_id', 'taxonomy', 'description', 'parent', 'count') );
			$tt_ids[$term_id][$taxonomy] = (int) $wpdb->insert_id;
		}

		if ( empty($count) ) {
			$count = 0;
			$taxonomy = 'category';
			$wpdb->insert( $wpdb->term_taxonomy, compact('term_id', 'taxonomy', 'description', 'parent', 'count') );
			$tt_ids[$term_id][$taxonomy] = (int) $wpdb->insert_id;
		}
	}

	$select = 'post_id, category_id';
	if ( $have_tags )
		$select .= ', rel_type';

	$posts = $wpdb->get_results("SELECT $select FROM $wpdb->post2cat GROUP BY post_id, category_id");
	foreach ( $posts as $post ) {
		$post_id = (int) $post->post_id;
		$term_id = (int) $post->category_id;
		$taxonomy = 'category';
		if ( !empty($post->rel_type) && 'tag' == $post->rel_type)
			$taxonomy = 'tag';
		$tt_id = $tt_ids[$term_id][$taxonomy];
		if ( empty($tt_id) )
			continue;

		$wpdb->insert( $wpdb->term_relationships, array('object_id' => $post_id, 'term_taxonomy_id' => $tt_id) );
	}

	// < 3570 we used linkcategories.  >= 3570 we used categories and link2cat.
	if ( $wp_current_db_version < 3570 ) {
		// Create link_category terms for link categories.  Create a map of link cat IDs
		// to link_category terms.
		$link_cat_id_map = array();
		$default_link_cat = 0;
		$tt_ids = array();
		$link_cats = $wpdb->get_results("SELECT cat_id, cat_name FROM " . $wpdb->prefix . 'linkcategories');
		foreach ( $link_cats as $category) {
			$cat_id = (int) $category->cat_id;
			$term_id = 0;
			$name = $wpdb->escape($category->cat_name);
			$slug = sanitize_title($name);
			$term_group = 0;

			// Associate terms with the same slug in a term group and make slugs unique.
			if ( $exists = $wpdb->get_results( $wpdb->prepare("SELECT term_id, term_group FROM $wpdb->terms WHERE slug = %s", $slug) ) ) {
				$term_group = $exists[0]->term_group;
				$term_id = $exists[0]->term_id;
			}

			if ( empty($term_id) ) {
				$wpdb->insert( $wpdb->terms, compact('name', 'slug', 'term_group') );
				$term_id = (int) $wpdb->insert_id;
			}

			$link_cat_id_map[$cat_id] = $term_id;
			$default_link_cat = $term_id;

			$wpdb->insert( $wpdb->term_taxonomy, array('term_id' => $term_id, 'taxonomy' => 'link_category', 'description' => '', 'parent' => 0, 'count' => 0) );
			$tt_ids[$term_id] = (int) $wpdb->insert_id;
		}

		// Associate links to cats.
		$links = $wpdb->get_results("SELECT link_id, link_category FROM $wpdb->links");
		if ( !empty($links) ) foreach ( $links as $link ) {
			if ( 0 == $link->link_category )
				continue;
			if ( ! isset($link_cat_id_map[$link->link_category]) )
				continue;
			$term_id = $link_cat_id_map[$link->link_category];
			$tt_id = $tt_ids[$term_id];
			if ( empty($tt_id) )
				continue;

			$wpdb->insert( $wpdb->term_relationships, array('object_id' => $link->link_id, 'term_taxonomy_id' => $tt_id) );
		}

		// Set default to the last category we grabbed during the upgrade loop.
		update_option('default_link_category', $default_link_cat);
	} else {
		$links = $wpdb->get_results("SELECT link_id, category_id FROM $wpdb->link2cat GROUP BY link_id, category_id");
		foreach ( $links as $link ) {
			$link_id = (int) $link->link_id;
			$term_id = (int) $link->category_id;
			$taxonomy = 'link_category';
			$tt_id = $tt_ids[$term_id][$taxonomy];
			if ( empty($tt_id) )
				continue;
			$wpdb->insert( $wpdb->term_relationships, array('object_id' => $link_id, 'term_taxonomy_id' => $tt_id) );
		}
	}

	if ( $wp_current_db_version < 4772 ) {
		// Obsolete linkcategories table
		$wpdb->query('DROP TABLE IF EXISTS ' . $wpdb->prefix . 'linkcategories');
	}

	// Recalculate all counts
	$terms = $wpdb->get_results("SELECT term_taxonomy_id, taxonomy FROM $wpdb->term_taxonomy");
	foreach ( (array) $terms as $term ) {
		if ( ('post_tag' == $term->taxonomy) || ('category' == $term->taxonomy) )
			$count = $wpdb->get_var( $wpdb->prepare("SELECT COUNT(*) FROM $wpdb->term_relationships, $wpdb->posts WHERE $wpdb->posts.ID = $wpdb->term_relationships.object_id AND post_status = 'publish' AND post_type = 'post' AND term_taxonomy_id = %d", $term->term_taxonomy_id) );
		else
			$count = $wpdb->get_var( $wpdb->prepare("SELECT COUNT(*) FROM $wpdb->term_relationships WHERE term_taxonomy_id = %d", $term->term_taxonomy_id) );
		$wpdb->update( $wpdb->term_taxonomy, array('count' => $count), array('term_taxonomy_id' => $term->term_taxonomy_id) );
	}
}

/**
 * Remove old options from the database.
 *
 * @since 2.3.0
 */
function upgrade_230_options_table() {
	global $wpdb;
	$old_options_fields = array( 'option_can_override', 'option_type', 'option_width', 'option_height', 'option_description', 'option_admin_level' );
	$wpdb->hide_errors();
	foreach ( $old_options_fields as $old )
		$wpdb->query("ALTER TABLE $wpdb->options DROP $old");
	$wpdb->show_errors();
}

/**
 * Remove old categories, link2cat, and post2cat database tables.
 *
 * @since 2.3.0
 */
function upgrade_230_old_tables() {
	global $wpdb;
	$wpdb->query('DROP TABLE IF EXISTS ' . $wpdb->prefix . 'categories');
	$wpdb->query('DROP TABLE IF EXISTS ' . $wpdb->prefix . 'link2cat');
	$wpdb->query('DROP TABLE IF EXISTS ' . $wpdb->prefix . 'post2cat');
}

/**
 * Upgrade old slugs made in version 2.2.
 *
 * @since 2.2.0
 */
function upgrade_old_slugs() {
	// upgrade people who were using the Redirect Old Slugs plugin
	global $wpdb;
	$wpdb->query("UPDATE $wpdb->postmeta SET meta_key = '_wp_old_slug' WHERE meta_key = 'old_slug'");
}

/**
 * Execute changes made in WordPress 2.5.0.
 *
 * @since 2.5.0
 */
function upgrade_250() {
	global $wp_current_db_version;

	if ( $wp_current_db_version < 6689 ) {
		populate_roles_250();
	}

}

/**
 * Execute changes made in WordPress 2.5.2.
 *
 * @since 2.5.2
 */
function upgrade_252() {
	global $wpdb;

	$wpdb->query("UPDATE $wpdb->users SET user_activation_key = ''");
}

/**
 * Execute changes made in WordPress 2.6.
 *
 * @since 2.6.0
 */
function upgrade_260() {
	global $wp_current_db_version;

	if ( $wp_current_db_version < 8000 )
		populate_roles_260();

	if ( $wp_current_db_version < 8201 ) {
		update_option('enable_app', 1);
		update_option('enable_xmlrpc', 1);
	}
}

/**
 * Execute changes made in WordPress 2.7.
 *
 * @since 2.7.0
 */
function upgrade_270() {
	global $wpdb, $wp_current_db_version;

	if ( $wp_current_db_version < 8980 )
		populate_roles_270();

	// Update post_date for unpublished posts with empty timestamp
	if ( $wp_current_db_version < 8921 )
		$wpdb->query( "UPDATE $wpdb->posts SET post_date = post_modified WHERE post_date = '0000-00-00 00:00:00'" );
}

/**
 * Execute changes made in WordPress 2.8.
 *
 * @since 2.8.0
 */
function upgrade_280() {
	global $wp_current_db_version, $wpdb;

	if ( $wp_current_db_version < 10360 )
		populate_roles_280();
	if ( is_multisite() ) {
		$start = 0;
		while( $rows = $wpdb->get_results( "SELECT option_name, option_value FROM $wpdb->options ORDER BY option_id LIMIT $start, 20" ) ) {
			foreach( $rows as $row ) {
				$value = $row->option_value;
				if ( !@unserialize( $value ) )
					$value = stripslashes( $value );
				if ( $value !== $row->option_value ) {
					update_option( $row->option_name, $value );
				}
			}
			$start += 20;
		}
		refresh_blog_details( $wpdb->blogid );
	}
}

/**
 * Execute changes made in WordPress 2.9.
 *
 * @since 2.9.0
 */
function upgrade_290() {
	global $wp_current_db_version;

	if ( $wp_current_db_version < 11958 ) {
		// Previously, setting depth to 1 would redundantly disable threading, but now 2 is the minimum depth to avoid confusion
		if ( get_option( 'thread_comments_depth' ) == '1' ) {
			update_option( 'thread_comments_depth', 2 );
			update_option( 'thread_comments', 0 );
		}
	}
}

/**
 * Execute changes made in WordPress 3.0.
 *
 * @since 3.0.0
 */
function upgrade_300() {
	global $wp_current_db_version, $wpdb;

	if ( $wp_current_db_version < 15093 )
		populate_roles_300();

	if ( $wp_current_db_version < 14139 && is_multisite() && is_main_site() && ! defined( 'MULTISITE' ) && get_site_option( 'siteurl' ) === false )
		add_site_option( 'siteurl', '' );

	// 3.0-alpha nav menu postmeta changes. can be removed before release. // r13802
	if ( $wp_current_db_version >= 13226 && $wp_current_db_version < 13974 )
		$wpdb->query( "DELETE FROM $wpdb->postmeta WHERE meta_key IN( 'menu_type', 'object_id', 'menu_new_window', 'menu_link', '_menu_item_append', 'menu_item_append', 'menu_item_type', 'menu_item_object_id', 'menu_item_target', 'menu_item_classes', 'menu_item_xfn', 'menu_item_url' )" );

	// 3.0-beta1 remove_user primitive->meta cap. can be removed before release. r13956
	if ( $wp_current_db_version >= 12751 && $wp_current_db_version < 13974 ) {
		$role =& get_role( 'administrator' );
		if ( ! empty( $role ) )
			$role->remove_cap( 'remove_user' );
	}

	// 3.0-beta1 nav menu postmeta changes. can be removed before release. r13974
	if ( $wp_current_db_version >= 13802 && $wp_current_db_version < 13974 )
		$wpdb->update( $wpdb->postmeta, array( 'meta_value' => '' ), array( 'meta_key' => '_menu_item_target', 'meta_value' => '_self' ) );

	// 3.0 screen options key name changes.
	if ( is_main_site() && !defined('DO_NOT_UPGRADE_GLOBAL_TABLES') ) {
		$prefix = like_escape($wpdb->base_prefix);
		$wpdb->query( "DELETE FROM $wpdb->usermeta WHERE meta_key LIKE '{$prefix}%meta-box-hidden%' OR meta_key LIKE '{$prefix}%closedpostboxes%' OR meta_key LIKE '{$prefix}%manage-%-columns-hidden%' OR meta_key LIKE '{$prefix}%meta-box-order%' OR meta_key LIKE '{$prefix}%metaboxorder%' OR meta_key LIKE '{$prefix}%screen_layout%'
					 OR meta_key = 'manageedittagscolumnshidden' OR meta_key='managecategoriescolumnshidden' OR meta_key = 'manageedit-tagscolumnshidden' OR meta_key = 'manageeditcolumnshidden' OR meta_key = 'categories_per_page' OR meta_key = 'edit_tags_per_page'" );
	}

}

/**
 * Execute network level changes
 *
 * @since 3.0.0
 */
function upgrade_network() {
	global $wp_current_db_version, $wpdb;
	// 2.8
	if ( $wp_current_db_version < 11549 ) {
		$wpmu_sitewide_plugins = get_site_option( 'wpmu_sitewide_plugins' );
		$active_sitewide_plugins = get_site_option( 'active_sitewide_plugins' );
		if ( $wpmu_sitewide_plugins ) {
			if ( !$active_sitewide_plugins )
				$sitewide_plugins = (array) $wpmu_sitewide_plugins;
			else
				$sitewide_plugins = array_merge( (array) $active_sitewide_plugins, (array) $wpmu_sitewide_plugins );

			update_site_option( 'active_sitewide_plugins', $sitewide_plugins );
		}
		delete_site_option( 'wpmu_sitewide_plugins' );
		delete_site_option( 'deactivated_sitewide_plugins' );

		$start = 0;
		while( $rows = $wpdb->get_results( "SELECT meta_key, meta_value FROM {$wpdb->sitemeta} ORDER BY meta_id LIMIT $start, 20" ) ) {
			foreach( $rows as $row ) {
				$value = $row->meta_value;
				if ( !@unserialize( $value ) )
					$value = stripslashes( $value );
				if ( $value !== $row->meta_value ) {
					update_site_option( $row->meta_key, $value );
				}
			}
			$start += 20;
		}
	}
	// 3.0
	if ( $wp_current_db_version < 13576 )
		update_site_option( 'global_terms_enabled', '1' );
}

// The functions we use to actually do stuff

// General

/**
 * {@internal Missing Short Description}}
 *
 * {@internal Missing Long Description}}
 *
 * @since 1.0.0
 *
 * @param string $table_name Database table name to create.
 * @param string $create_ddl SQL statement to create table.
 * @return bool If table already exists or was created by function.
 */
function maybe_create_table($table_name, $create_ddl) {
	global $wpdb;
	if ( $wpdb->get_var("SHOW TABLES LIKE '$table_name'") == $table_name )
		return true;
	//didn't find it try to create it.
	$q = $wpdb->query($create_ddl);
	// we cannot directly tell that whether this succeeded!
	if ( $wpdb->get_var("SHOW TABLES LIKE '$table_name'") == $table_name )
		return true;
	return false;
}

/**
 * {@internal Missing Short Description}}
 *
 * {@internal Missing Long Description}}
 *
 * @since 1.0.1
 *
 * @param string $table Database table name.
 * @param string $index Index name to drop.
 * @return bool True, when finished.
 */
function drop_index($table, $index) {
	global $wpdb;
	$wpdb->hide_errors();
	$wpdb->query("ALTER TABLE `$table` DROP INDEX `$index`");
	// Now we need to take out all the extra ones we may have created
	for ($i = 0; $i < 25; $i++) {
		$wpdb->query("ALTER TABLE `$table` DROP INDEX `{$index}_$i`");
	}
	$wpdb->show_errors();
	return true;
}

/**
 * {@internal Missing Short Description}}
 *
 * {@internal Missing Long Description}}
 *
 * @since 1.0.1
 *
 * @param string $table Database table name.
 * @param string $index Database table index column.
 * @return bool True, when done with execution.
 */
function add_clean_index($table, $index) {
	global $wpdb;
	drop_index($table, $index);
	$wpdb->query("ALTER TABLE `$table` ADD INDEX ( `$index` )");
	return true;
}

/**
 ** maybe_add_column()
 ** Add column to db table if it doesn't exist.
 ** Returns:  true if already exists or on successful completion
 **           false on error
 */
function maybe_add_column($table_name, $column_name, $create_ddl) {
	global $wpdb, $debug;
	foreach ($wpdb->get_col("DESC $table_name", 0) as $column ) {
		if ($debug) echo("checking $column == $column_name<br />");
		if ($column == $column_name) {
			return true;
		}
	}
	//didn't find it try to create it.
	$q = $wpdb->query($create_ddl);
	// we cannot directly tell that whether this succeeded!
	foreach ($wpdb->get_col("DESC $table_name", 0) as $column ) {
		if ($column == $column_name) {
			return true;
		}
	}
	return false;
}

/**
 * Retrieve all options as it was for 1.2.
 *
 * @since 1.2.0
 *
 * @return array List of options.
 */
function get_alloptions_110() {
	global $wpdb;
	if ($options = $wpdb->get_results("SELECT option_name, option_value FROM $wpdb->options")) {
		foreach ($options as $option) {
			// "When trying to design a foolproof system,
			//  never underestimate the ingenuity of the fools :)" -- Dougal
			if ('siteurl' == $option->option_name) $option->option_value = preg_replace('|/+$|', '', $option->option_value);
			if ('home' == $option->option_name) $option->option_value = preg_replace('|/+$|', '', $option->option_value);
			if ('category_base' == $option->option_name) $option->option_value = preg_replace('|/+$|', '', $option->option_value);
			$all_options->{$option->option_name} = stripslashes($option->option_value);
		}
	}
	return $all_options;
}

/**
 * Version of get_option that is private to install/upgrade.
 *
 * @since 1.5.1
 * @access private
 *
 * @param string $setting Option name.
 * @return mixed
 */
function __get_option($setting) {
	global $wpdb;

	if ( $setting == 'home' && defined( 'WP_HOME' ) ) {
		return preg_replace( '|/+$|', '', WP_HOME );
	}

	if ( $setting == 'siteurl' && defined( 'WP_SITEURL' ) ) {
		return preg_replace( '|/+$|', '', WP_SITEURL );
	}

	$option = $wpdb->get_var( $wpdb->prepare("SELECT option_value FROM $wpdb->options WHERE option_name = %s", $setting) );

	if ( 'home' == $setting && '' == $option )
		return __get_option('siteurl');

	if ( 'siteurl' == $setting || 'home' == $setting || 'category_base' == $setting )
		$option = preg_replace('|/+$|', '', $option);

	@ $kellogs = unserialize($option);
	if ($kellogs !== FALSE)
		return $kellogs;
	else
		return $option;
}

/**
 * {@internal Missing Short Description}}
 *
 * {@internal Missing Long Description}}
 *
 * @since 1.5.0
 *
 * @param string $content
 * @return string
 */
function deslash($content) {
	// Note: \\\ inside a regex denotes a single backslash.

	// Replace one or more backslashes followed by a single quote with
	// a single quote.
	$content = preg_replace("/\\\+'/", "'", $content);

	// Replace one or more backslashes followed by a double quote with
	// a double quote.
	$content = preg_replace('/\\\+"/', '"', $content);

	// Replace one or more backslashes with one backslash.
	$content = preg_replace("/\\\+/", "\\", $content);

	return $content;
}

/**
 * {@internal Missing Short Description}}
 *
 * {@internal Missing Long Description}}
 *
 * @since 1.5.0
 *
 * @param unknown_type $queries
 * @param unknown_type $execute
 * @return unknown
 */
function dbDelta($queries, $execute = true) {
	global $wpdb;

	// Separate individual queries into an array
	if ( !is_array($queries) ) {
		$queries = explode( ';', $queries );
		if ('' == $queries[count($queries) - 1]) array_pop($queries);
	}

	$cqueries = array(); // Creation Queries
	$iqueries = array(); // Insertion Queries
	$for_update = array();

	// Create a tablename index for an array ($cqueries) of queries
	foreach($queries as $qry) {
		if (preg_match("|CREATE TABLE ([^ ]*)|", $qry, $matches)) {
			$cqueries[trim( strtolower($matches[1]), '`' )] = $qry;
			$for_update[$matches[1]] = 'Created table '.$matches[1];
		} else if (preg_match("|CREATE DATABASE ([^ ]*)|", $qry, $matches)) {
			array_unshift($cqueries, $qry);
		} else if (preg_match("|INSERT INTO ([^ ]*)|", $qry, $matches)) {
			$iqueries[] = $qry;
		} else if (preg_match("|UPDATE ([^ ]*)|", $qry, $matches)) {
			$iqueries[] = $qry;
		} else {
			// Unrecognized query type
		}
	}

	// Check to see which tables and fields exist
	if ($tables = $wpdb->get_col('SHOW TABLES;')) {
		// For every table in the database
		foreach ($tables as $table) {
			// Upgrade global tables only for the main site. Don't upgrade at all if DO_NOT_UPGRADE_GLOBAL_TABLES is defined.
			if ( in_array($table, $wpdb->tables('global')) && ( !is_main_site() || defined('DO_NOT_UPGRADE_GLOBAL_TABLES') ) )
				continue;

			// If a table query exists for the database table...
			if ( array_key_exists(strtolower($table), $cqueries) ) {
				// Clear the field and index arrays
				$cfields = $indices = array();
				// Get all of the field names in the query from between the parens
				preg_match("|\((.*)\)|ms", $cqueries[strtolower($table)], $match2);
				$qryline = trim($match2[1]);

				// Separate field lines into an array
				$flds = explode("\n", $qryline);

				//echo "<hr/><pre>\n".print_r(strtolower($table), true).":\n".print_r($cqueries, true)."</pre><hr/>";

				// For every field line specified in the query
				foreach ($flds as $fld) {
					// Extract the field name
					preg_match("|^([^ ]*)|", trim($fld), $fvals);
					$fieldname = trim( $fvals[1], '`' );

					// Verify the found field name
					$validfield = true;
					switch (strtolower($fieldname)) {
					case '':
					case 'primary':
					case 'index':
					case 'fulltext':
					case 'unique':
					case 'key':
						$validfield = false;
						$indices[] = trim(trim($fld), ", \n");
						break;
					}
					$fld = trim($fld);

					// If it's a valid field, add it to the field array
					if ($validfield) {
						$cfields[strtolower($fieldname)] = trim($fld, ", \n");
					}
				}

				// Fetch the table column structure from the database
				$tablefields = $wpdb->get_results("DESCRIBE {$table};");

				// For every field in the table
				foreach ($tablefields as $tablefield) {
					// If the table field exists in the field array...
					if (array_key_exists(strtolower($tablefield->Field), $cfields)) {
						// Get the field type from the query
						preg_match("|".$tablefield->Field." ([^ ]*( unsigned)?)|i", $cfields[strtolower($tablefield->Field)], $matches);
						$fieldtype = $matches[1];

						// Is actual field type different from the field type in query?
						if ($tablefield->Type != $fieldtype) {
							// Add a query to change the column type
							$cqueries[] = "ALTER TABLE {$table} CHANGE COLUMN {$tablefield->Field} " . $cfields[strtolower($tablefield->Field)];
							$for_update[$table.'.'.$tablefield->Field] = "Changed type of {$table}.{$tablefield->Field} from {$tablefield->Type} to {$fieldtype}";
						}

						// Get the default value from the array
							//echo "{$cfields[strtolower($tablefield->Field)]}<br>";
						if (preg_match("| DEFAULT '(.*)'|i", $cfields[strtolower($tablefield->Field)], $matches)) {
							$default_value = $matches[1];
							if ($tablefield->Default != $default_value) {
								// Add a query to change the column's default value
								$cqueries[] = "ALTER TABLE {$table} ALTER COLUMN {$tablefield->Field} SET DEFAULT '{$default_value}'";
								$for_update[$table.'.'.$tablefield->Field] = "Changed default value of {$table}.{$tablefield->Field} from {$tablefield->Default} to {$default_value}";
							}
						}

						// Remove the field from the array (so it's not added)
						unset($cfields[strtolower($tablefield->Field)]);
					} else {
						// This field exists in the table, but not in the creation queries?
					}
				}

				// For every remaining field specified for the table
				foreach ($cfields as $fieldname => $fielddef) {
					// Push a query line into $cqueries that adds the field to that table
					$cqueries[] = "ALTER TABLE {$table} ADD COLUMN $fielddef";
					$for_update[$table.'.'.$fieldname] = 'Added column '.$table.'.'.$fieldname;
				}

				// Index stuff goes here
				// Fetch the table index structure from the database
				$tableindices = $wpdb->get_results("SHOW INDEX FROM {$table};");

				if ($tableindices) {
					// Clear the index array
					unset($index_ary);

					// For every index in the table
					foreach ($tableindices as $tableindex) {
						// Add the index to the index data array
						$keyname = $tableindex->Key_name;
						$index_ary[$keyname]['columns'][] = array('fieldname' => $tableindex->Column_name, 'subpart' => $tableindex->Sub_part);
						$index_ary[$keyname]['unique'] = ($tableindex->Non_unique == 0)?true:false;
					}

					// For each actual index in the index array
					foreach ($index_ary as $index_name => $index_data) {
						// Build a create string to compare to the query
						$index_string = '';
						if ($index_name == 'PRIMARY') {
							$index_string .= 'PRIMARY ';
						} else if($index_data['unique']) {
							$index_string .= 'UNIQUE ';
						}
						$index_string .= 'KEY ';
						if ($index_name != 'PRIMARY') {
							$index_string .= $index_name;
						}
						$index_columns = '';
						// For each column in the index
						foreach ($index_data['columns'] as $column_data) {
							if ($index_columns != '') $index_columns .= ',';
							// Add the field to the column list string
							$index_columns .= $column_data['fieldname'];
							if ($column_data['subpart'] != '') {
								$index_columns .= '('.$column_data['subpart'].')';
							}
						}
						// Add the column list to the index create string
						$index_string .= ' ('.$index_columns.')';
						if (!(($aindex = array_search($index_string, $indices)) === false)) {
							unset($indices[$aindex]);
							//echo "<pre style=\"border:1px solid #ccc;margin-top:5px;\">{$table}:<br />Found index:".$index_string."</pre>\n";
						}
						//else echo "<pre style=\"border:1px solid #ccc;margin-top:5px;\">{$table}:<br /><b>Did not find index:</b>".$index_string."<br />".print_r($indices, true)."</pre>\n";
					}
				}

				// For every remaining index specified for the table
				foreach ( (array) $indices as $index ) {
					// Push a query line into $cqueries that adds the index to that table
					$cqueries[] = "ALTER TABLE {$table} ADD $index";
					$for_update[$table.'.'.$fieldname] = 'Added index '.$table.' '.$index;
				}

				// Remove the original table creation query from processing
				unset($cqueries[strtolower($table)]);
				unset($for_update[strtolower($table)]);
			} else {
				// This table exists in the database, but not in the creation queries?
			}
		}
	}

	$allqueries = array_merge($cqueries, $iqueries);
	if ($execute) {
		foreach ($allqueries as $query) {
			//echo "<pre style=\"border:1px solid #ccc;margin-top:5px;\">".print_r($query, true)."</pre>\n";
			$wpdb->query($query);
		}
	}

	return $for_update;
}

/**
 * {@internal Missing Short Description}}
 *
 * {@internal Missing Long Description}}
 *
 * @since 1.5.0
 */
function make_db_current() {
	global $wp_queries;

	$alterations = dbDelta($wp_queries);
	echo "<ol>\n";
	foreach($alterations as $alteration) echo "<li>$alteration</li>\n";
	echo "</ol>\n";
}

/**
 * {@internal Missing Short Description}}
 *
 * {@internal Missing Long Description}}
 *
 * @since 1.5.0
 */
function make_db_current_silent() {
	global $wp_queries;

	$alterations = dbDelta($wp_queries);
}

/**
 * {@internal Missing Short Description}}
 *
 * {@internal Missing Long Description}}
 *
 * @since 1.5.0
 *
 * @param unknown_type $theme_name
 * @param unknown_type $template
 * @return unknown
 */
function make_site_theme_from_oldschool($theme_name, $template) {
	$home_path = get_home_path();
	$site_dir = WP_CONTENT_DIR . "/themes/$template";

	if (! file_exists("$home_path/index.php"))
		return false;

	// Copy files from the old locations to the site theme.
	// TODO: This does not copy arbitarary include dependencies.  Only the
	// standard WP files are copied.
	$files = array('index.php' => 'index.php', 'wp-layout.css' => 'style.css', 'wp-comments.php' => 'comments.php', 'wp-comments-popup.php' => 'comments-popup.php');

	foreach ($files as $oldfile => $newfile) {
		if ($oldfile == 'index.php')
			$oldpath = $home_path;
		else
			$oldpath = ABSPATH;

		if ($oldfile == 'index.php') { // Check to make sure it's not a new index
			$index = implode('', file("$oldpath/$oldfile"));
			if (strpos($index, 'WP_USE_THEMES') !== false) {
				if (! @copy(WP_CONTENT_DIR . '/themes/' . WP_DEFAULT_THEME . '/index.php', "$site_dir/$newfile"))
					return false;
				continue; // Don't copy anything
				}
		}

		if (! @copy("$oldpath/$oldfile", "$site_dir/$newfile"))
			return false;

		chmod("$site_dir/$newfile", 0777);

		// Update the blog header include in each file.
		$lines = explode("\n", implode('', file("$site_dir/$newfile")));
		if ($lines) {
			$f = fopen("$site_dir/$newfile", 'w');

			foreach ($lines as $line) {
				if (preg_match('/require.*wp-blog-header/', $line))
					$line = '//' . $line;

				// Update stylesheet references.
				$line = str_replace("<?php echo __get_option('siteurl'); ?>/wp-layout.css", "<?php bloginfo('stylesheet_url'); ?>", $line);

				// Update comments template inclusion.
				$line = str_replace("<?php include(ABSPATH . 'wp-comments.php'); ?>", "<?php comments_template(); ?>", $line);

				fwrite($f, "{$line}\n");
			}
			fclose($f);
		}
	}

	// Add a theme header.
	$header = "/*\nTheme Name: $theme_name\nTheme URI: " . __get_option('siteurl') . "\nDescription: A theme automatically created by the update.\nVersion: 1.0\nAuthor: Moi\n*/\n";

	$stylelines = file_get_contents("$site_dir/style.css");
	if ($stylelines) {
		$f = fopen("$site_dir/style.css", 'w');

		fwrite($f, $header);
		fwrite($f, $stylelines);
		fclose($f);
	}

	return true;
}

/**
 * {@internal Missing Short Description}}
 *
 * {@internal Missing Long Description}}
 *
 * @since 1.5.0
 *
 * @param unknown_type $theme_name
 * @param unknown_type $template
 * @return unknown
 */
function make_site_theme_from_default($theme_name, $template) {
	$site_dir = WP_CONTENT_DIR . "/themes/$template";
	$default_dir = WP_CONTENT_DIR . '/themes/' . WP_DEFAULT_THEME;

	// Copy files from the default theme to the site theme.
	//$files = array('index.php', 'comments.php', 'comments-popup.php', 'footer.php', 'header.php', 'sidebar.php', 'style.css');

	$theme_dir = @ opendir($default_dir);
	if ($theme_dir) {
		while(($theme_file = readdir( $theme_dir )) !== false) {
			if (is_dir("$default_dir/$theme_file"))
				continue;
			if (! @copy("$default_dir/$theme_file", "$site_dir/$theme_file"))
				return;
			chmod("$site_dir/$theme_file", 0777);
		}
	}
	@closedir($theme_dir);

	// Rewrite the theme header.
	$stylelines = explode("\n", implode('', file("$site_dir/style.css")));
	if ($stylelines) {
		$f = fopen("$site_dir/style.css", 'w');

		foreach ($stylelines as $line) {
			if (strpos($line, 'Theme Name:') !== false) $line = 'Theme Name: ' . $theme_name;
			elseif (strpos($line, 'Theme URI:') !== false) $line = 'Theme URI: ' . __get_option('url');
			elseif (strpos($line, 'Description:') !== false) $line = 'Description: Your theme.';
			elseif (strpos($line, 'Version:') !== false) $line = 'Version: 1';
			elseif (strpos($line, 'Author:') !== false) $line = 'Author: You';
			fwrite($f, $line . "\n");
		}
		fclose($f);
	}

	// Copy the images.
	umask(0);
	if (! mkdir("$site_dir/images", 0777)) {
		return false;
	}

	$images_dir = @ opendir("$default_dir/images");
	if ($images_dir) {
		while(($image = readdir($images_dir)) !== false) {
			if (is_dir("$default_dir/images/$image"))
				continue;
			if (! @copy("$default_dir/images/$image", "$site_dir/images/$image"))
				return;
			chmod("$site_dir/images/$image", 0777);
		}
	}
	@closedir($images_dir);
}

// Create a site theme from the default theme.
/**
 * {@internal Missing Short Description}}
 *
 * {@internal Missing Long Description}}
 *
 * @since 1.5.0
 *
 * @return unknown
 */
function make_site_theme() {
	// Name the theme after the blog.
	$theme_name = __get_option('blogname');
	$template = sanitize_title($theme_name);
	$site_dir = WP_CONTENT_DIR . "/themes/$template";

	// If the theme already exists, nothing to do.
	if ( is_dir($site_dir)) {
		return false;
	}

	// We must be able to write to the themes dir.
	if (! is_writable(WP_CONTENT_DIR . "/themes")) {
		return false;
	}

	umask(0);
	if (! mkdir($site_dir, 0777)) {
		return false;
	}

	if (file_exists(ABSPATH . 'wp-layout.css')) {
		if (! make_site_theme_from_oldschool($theme_name, $template)) {
			// TODO:  rm -rf the site theme directory.
			return false;
		}
	} else {
		if (! make_site_theme_from_default($theme_name, $template))
			// TODO:  rm -rf the site theme directory.
			return false;
	}

	// Make the new site theme active.
	$current_template = __get_option('template');
	if ($current_template == WP_DEFAULT_THEME) {
		update_option('template', $template);
		update_option('stylesheet', $template);
	}
	return $template;
}

/**
 * Translate user level to user role name.
 *
 * @since 2.0.0
 *
 * @param int $level User level.
 * @return string User role name.
 */
function translate_level_to_role($level) {
	switch ($level) {
	case 10:
	case 9:
	case 8:
		return 'administrator';
	case 7:
	case 6:
	case 5:
		return 'editor';
	case 4:
	case 3:
	case 2:
		return 'author';
	case 1:
		return 'contributor';
	case 0:
		return 'subscriber';
	}
}

/**
 * {@internal Missing Short Description}}
 *
 * {@internal Missing Long Description}}
 *
 * @since 2.1.0
 */
function wp_check_mysql_version() {
	global $wpdb;
	$result = $wpdb->check_database_version();
	if ( is_wp_error( $result ) )
		die( $result->get_error_message() );
}

/**
 * {@internal Missing Short Description}}
 *
 * {@internal Missing Long Description}}
 *
 * @since 2.2.0
 */
function maybe_disable_automattic_widgets() {
	$plugins = __get_option( 'active_plugins' );

	foreach ( (array) $plugins as $plugin ) {
		if ( basename( $plugin ) == 'widgets.php' ) {
			array_splice( $plugins, array_search( $plugin, $plugins ), 1 );
			update_option( 'active_plugins', $plugins );
			break;
		}
	}
}

/**
 * Runs before the schema is upgraded.
 *
 * @since 2.9.0
 */
function pre_schema_upgrade() {
	global $wp_current_db_version, $wp_db_version, $wpdb;

	// Upgrade versions prior to 2.9
	if ( $wp_current_db_version < 11557 ) {
		// Delete duplicate options.  Keep the option with the highest option_id.
		$wpdb->query("DELETE o1 FROM $wpdb->options AS o1 JOIN $wpdb->options AS o2 USING (`option_name`) WHERE o2.option_id > o1.option_id");

		// Drop the old primary key and add the new.
		$wpdb->query("ALTER TABLE $wpdb->options DROP PRIMARY KEY, ADD PRIMARY KEY(option_id)");

		// Drop the old option_name index. dbDelta() doesn't do the drop.
		$wpdb->query("ALTER TABLE $wpdb->options DROP INDEX option_name");
	}

}

/**
 * Install Network.
 *
 * @since 3.0.0
 *
 */
if ( !function_exists( 'install_network' ) ) :
function install_network() {
	global $wpdb, $charset_collate;
	$ms_queries = "
CREATE TABLE $wpdb->users (
  ID bigint(20) unsigned NOT NULL auto_increment,
  user_login varchar(60) NOT NULL default '',
  user_pass varchar(64) NOT NULL default '',
  user_nicename varchar(50) NOT NULL default '',
  user_email varchar(100) NOT NULL default '',
  user_url varchar(100) NOT NULL default '',
  user_registered datetime NOT NULL default '0000-00-00 00:00:00',
  user_activation_key varchar(60) NOT NULL default '',
  user_status int(11) NOT NULL default '0',
  display_name varchar(250) NOT NULL default '',
  spam tinyint(2) NOT NULL default '0',
  deleted tinyint(2) NOT NULL default '0',
  PRIMARY KEY  (ID),
  KEY user_login_key (user_login),
  KEY user_nicename (user_nicename)
) $charset_collate;
CREATE TABLE $wpdb->blogs (
  blog_id bigint(20) NOT NULL auto_increment,
  site_id bigint(20) NOT NULL default '0',
  domain varchar(200) NOT NULL default '',
  path varchar(100) NOT NULL default '',
  registered datetime NOT NULL default '0000-00-00 00:00:00',
  last_updated datetime NOT NULL default '0000-00-00 00:00:00',
  public tinyint(2) NOT NULL default '1',
  archived enum('0','1') NOT NULL default '0',
  mature tinyint(2) NOT NULL default '0',
  spam tinyint(2) NOT NULL default '0',
  deleted tinyint(2) NOT NULL default '0',
  lang_id int(11) NOT NULL default '0',
  PRIMARY KEY  (blog_id),
  KEY domain (domain(50),path(5)),
  KEY lang_id (lang_id)
) $charset_collate;
CREATE TABLE $wpdb->blog_versions (
  blog_id bigint(20) NOT NULL default '0',
  db_version varchar(20) NOT NULL default '',
  last_updated datetime NOT NULL default '0000-00-00 00:00:00',
  PRIMARY KEY  (blog_id),
  KEY db_version (db_version)
) $charset_collate;
CREATE TABLE $wpdb->registration_log (
  ID bigint(20) NOT NULL auto_increment,
  email varchar(255) NOT NULL default '',
  IP varchar(30) NOT NULL default '',
  blog_id bigint(20) NOT NULL default '0',
  date_registered datetime NOT NULL default '0000-00-00 00:00:00',
  PRIMARY KEY  (ID),
  KEY IP (IP)
) $charset_collate;
CREATE TABLE $wpdb->site (
  id bigint(20) NOT NULL auto_increment,
  domain varchar(200) NOT NULL default '',
  path varchar(100) NOT NULL default '',
  PRIMARY KEY  (id),
  KEY domain (domain,path)
) $charset_collate;
CREATE TABLE $wpdb->sitemeta (
  meta_id bigint(20) NOT NULL auto_increment,
  site_id bigint(20) NOT NULL default '0',
  meta_key varchar(255) default NULL,
  meta_value longtext,
  PRIMARY KEY  (meta_id),
  KEY meta_key (meta_key),
  KEY site_id (site_id)
) $charset_collate;
CREATE TABLE $wpdb->signups (
  domain varchar(200) NOT NULL default '',
  path varchar(100) NOT NULL default '',
  title longtext NOT NULL,
  user_login varchar(60) NOT NULL default '',
  user_email varchar(100) NOT NULL default '',
  registered datetime NOT NULL default '0000-00-00 00:00:00',
  activated datetime NOT NULL default '0000-00-00 00:00:00',
  active tinyint(1) NOT NULL default '0',
  activation_key varchar(50) NOT NULL default '',
  meta longtext,
  KEY activation_key (activation_key),
  KEY domain (domain)
) $charset_collate;
";
// now create tables
	dbDelta( $ms_queries );
}
endif;

/**
 * Install global terms.
 *
 * @since 3.0.0
 *
 */
if ( !function_exists( 'install_global_terms' ) ) :
function install_global_terms() {
	global $wpdb, $charset_collate;
	$ms_queries = "
CREATE TABLE $wpdb->sitecategories (
  cat_ID bigint(20) NOT NULL auto_increment,
  cat_name varchar(55) NOT NULL default '',
  category_nicename varchar(200) NOT NULL default '',
  last_updated timestamp NOT NULL,
  PRIMARY KEY  (cat_ID),
  KEY category_nicename (category_nicename),
  KEY last_updated (last_updated)
) $charset_collate;
";
// now create tables
	dbDelta( $ms_queries );
}
endif;
?>
