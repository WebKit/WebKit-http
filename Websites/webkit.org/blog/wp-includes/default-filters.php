<?php
/**
 * Sets up the default filters and actions for most
 * of the WordPress hooks.
 *
 * If you need to remove a default hook, this file will
 * give you the priority for which to use to remove the
 * hook.
 *
 * Not all of the default hooks are found in default-filters.php
 *
 * @package WordPress
 */

// Strip, trim, kses, special chars for string saves
foreach ( array( 'pre_term_name', 'pre_comment_author_name', 'pre_link_name', 'pre_link_target', 'pre_link_rel', 'pre_user_display_name', 'pre_user_first_name', 'pre_user_last_name', 'pre_user_nickname' ) as $filter ) {
	add_filter( $filter, 'sanitize_text_field'  );
	add_filter( $filter, 'wp_filter_kses'       );
	add_filter( $filter, '_wp_specialchars', 30 );
}

// Strip, kses, special chars for string display
foreach ( array( 'term_name', 'comment_author_name', 'link_name', 'link_target', 'link_rel', 'user_display_name', 'user_first_name', 'user_last_name', 'user_nickname' ) as $filter ) {
	if ( is_admin() ) {
		// These are expensive. Run only on admin pages for defense in depth.
		add_filter( $filter, 'sanitize_text_field'  );
		add_filter( $filter, 'wp_kses_data'       );
	}
	add_filter( $filter, '_wp_specialchars', 30 );
}

// Kses only for textarea saves
foreach ( array( 'pre_term_description', 'pre_link_description', 'pre_link_notes', 'pre_user_description' ) as $filter ) {
	add_filter( $filter, 'wp_filter_kses' );
}

// Kses only for textarea admin displays
if ( is_admin() ) {
	foreach ( array( 'term_description', 'link_description', 'link_notes', 'user_description' ) as $filter ) {
		add_filter( $filter, 'wp_kses_data' );
	}
	add_filter( 'comment_text', 'wp_kses_post' );
}

// Email saves
foreach ( array( 'pre_comment_author_email', 'pre_user_email' ) as $filter ) {
	add_filter( $filter, 'trim'           );
	add_filter( $filter, 'sanitize_email' );
	add_filter( $filter, 'wp_filter_kses' );
}

// Email admin display
foreach ( array( 'comment_author_email', 'user_email' ) as $filter ) {
	add_filter( $filter, 'sanitize_email' );
	if ( is_admin() )
		add_filter( $filter, 'wp_kses_data' );
}

// Save URL
foreach ( array( 'pre_comment_author_url', 'pre_user_url', 'pre_link_url', 'pre_link_image',
	'pre_link_rss', 'pre_post_guid' ) as $filter ) {
	add_filter( $filter, 'wp_strip_all_tags' );
	add_filter( $filter, 'esc_url_raw'       );
	add_filter( $filter, 'wp_filter_kses'    );
}

// Display URL
foreach ( array( 'user_url', 'link_url', 'link_image', 'link_rss', 'comment_url', 'post_guid' ) as $filter ) {
	if ( is_admin() )
		add_filter( $filter, 'wp_strip_all_tags' );
	add_filter( $filter, 'esc_url'           );
	if ( is_admin() )
		add_filter( $filter, 'wp_kses_data'    );
}

// Slugs
foreach ( array( 'pre_term_slug' ) as $filter ) {
	add_filter( $filter, 'sanitize_title' );
}

// Keys
foreach ( array( 'pre_post_type' ) as $filter ) {
	add_filter( $filter, 'sanitize_user' );
}
foreach ( array( 'pre_post_status', 'pre_post_comment_status', 'pre_post_ping_status' ) as $filter ) {
	add_filter( $filter, 'sanitize_key' );
}

// Mime types
add_filter( 'pre_post_mime_type', 'sanitize_mime_type' );
add_filter( 'post_mime_type', 'sanitize_mime_type' );

// Places to balance tags on input
foreach ( array( 'content_save_pre', 'excerpt_save_pre', 'comment_save_pre', 'pre_comment_content' ) as $filter ) {
	add_filter( $filter, 'balanceTags', 50 );
}

// Format strings for display.
foreach ( array( 'comment_author', 'term_name', 'link_name', 'link_description', 'link_notes', 'bloginfo', 'wp_title', 'widget_title' ) as $filter ) {
	add_filter( $filter, 'wptexturize'   );
	add_filter( $filter, 'convert_chars' );
	add_filter( $filter, 'esc_html'      );
}

// Format WordPress
foreach ( array( 'the_content', 'the_title' ) as $filter )
	add_filter( $filter, 'capital_P_dangit', 11 );
add_filter( 'comment_text', 'capital_P_dangit', 31 );

// Format titles
foreach ( array( 'single_post_title', 'single_cat_title', 'single_tag_title', 'single_month_title', 'nav_menu_attr_title', 'nav_menu_description' ) as $filter ) {
	add_filter( $filter, 'wptexturize' );
	add_filter( $filter, 'strip_tags'  );
}

// Format text area for display.
foreach ( array( 'term_description' ) as $filter ) {
	add_filter( $filter, 'wptexturize'      );
	add_filter( $filter, 'convert_chars'    );
	add_filter( $filter, 'wpautop'          );
	add_filter( $filter, 'shortcode_unautop');
}

// Format for RSS
foreach ( array( 'term_name_rss' ) as $filter ) {
	add_filter( $filter, 'convert_chars' );
}

// Pre save hierarchy
add_filter( 'wp_insert_post_parent', 'wp_check_post_hierarchy_for_loops', 10, 2 );
add_filter( 'wp_update_term_parent', 'wp_check_term_hierarchy_for_loops', 10, 3 );

// Display filters
add_filter( 'the_title', 'wptexturize'   );
add_filter( 'the_title', 'convert_chars' );
add_filter( 'the_title', 'trim'          );

add_filter( 'the_content', 'wptexturize'        );
add_filter( 'the_content', 'convert_smilies'    );
add_filter( 'the_content', 'convert_chars'      );
add_filter( 'the_content', 'wpautop'            );
add_filter( 'the_content', 'shortcode_unautop'  );
add_filter( 'the_content', 'prepend_attachment' );

add_filter( 'the_excerpt',     'wptexturize'      );
add_filter( 'the_excerpt',     'convert_smilies'  );
add_filter( 'the_excerpt',     'convert_chars'    );
add_filter( 'the_excerpt',     'wpautop'          );
add_filter( 'the_excerpt',     'shortcode_unautop');
add_filter( 'get_the_excerpt', 'wp_trim_excerpt'  );

add_filter( 'comment_text', 'wptexturize'            );
add_filter( 'comment_text', 'convert_chars'          );
add_filter( 'comment_text', 'make_clickable',      9 );
add_filter( 'comment_text', 'force_balance_tags', 25 );
add_filter( 'comment_text', 'convert_smilies',    20 );
add_filter( 'comment_text', 'wpautop',            30 );

add_filter( 'comment_excerpt', 'convert_chars' );

add_filter( 'list_cats',         'wptexturize' );

add_filter( 'wp_sprintf', 'wp_sprintf_l', 10, 2 );

// RSS filters
add_filter( 'the_title_rss',      'strip_tags'      );
add_filter( 'the_title_rss',      'ent2ncr',      8 );
add_filter( 'the_title_rss',      'esc_html'        );
add_filter( 'the_content_rss',    'ent2ncr',      8 );
add_filter( 'the_excerpt_rss',    'convert_chars'   );
add_filter( 'the_excerpt_rss',    'ent2ncr',      8 );
add_filter( 'comment_author_rss', 'ent2ncr',      8 );
add_filter( 'comment_text_rss',   'ent2ncr',      8 );
add_filter( 'comment_text_rss',   'esc_html'        );
add_filter( 'bloginfo_rss',       'ent2ncr',      8 );
add_filter( 'the_author',         'ent2ncr',      8 );

// Misc filters
add_filter( 'option_ping_sites',    'privacy_ping_filter'                 );
add_filter( 'option_blog_charset',  '_wp_specialchars'                    ); // IMPORTANT: This must not be wp_specialchars() or esc_html() or it'll cause an infinite loop
add_filter( 'option_home',          '_config_wp_home'                     );
add_filter( 'option_siteurl',       '_config_wp_siteurl'                  );
add_filter( 'tiny_mce_before_init', '_mce_set_direction'                  );
add_filter( 'pre_kses',             'wp_pre_kses_less_than'               );
add_filter( 'sanitize_title',       'sanitize_title_with_dashes'          );
add_action( 'check_comment_flood',  'check_comment_flood_db',       10, 3 );
add_filter( 'comment_flood_filter', 'wp_throttle_comment_flood',    10, 3 );
add_filter( 'pre_comment_content',  'wp_rel_nofollow',              15    );
add_filter( 'comment_email',        'antispambot'                         );
add_filter( 'option_tag_base',      '_wp_filter_taxonomy_base'            );
add_filter( 'option_category_base', '_wp_filter_taxonomy_base'            );
add_filter( 'the_posts',            '_close_comments_for_old_posts'       );
add_filter( 'comments_open',        '_close_comments_for_old_post', 10, 2 );
add_filter( 'pings_open',           '_close_comments_for_old_post', 10, 2 );
add_filter( 'editable_slug',        'urldecode'                           );
add_filter( 'nav_menu_meta_box_object', '_wp_nav_menu_meta_box_object'    );

// Actions
add_action( 'wp_head',             'wp_enqueue_scripts',            1     );
add_action( 'wp_head',             'feed_links',                    2     );
add_action( 'wp_head',             'feed_links_extra',              3     );
add_action( 'wp_head',             'rsd_link'                             );
add_action( 'wp_head',             'wlwmanifest_link'                     );
add_action( 'wp_head',             'index_rel_link'                       );
add_action( 'wp_head',             'parent_post_rel_link',          10, 0 );
add_action( 'wp_head',             'start_post_rel_link',           10, 0 );
add_action( 'wp_head',             'adjacent_posts_rel_link_wp_head', 10, 0 );
add_action( 'wp_head',             'locale_stylesheet'                    );
add_action( 'publish_future_post', 'check_and_publish_future_post', 10, 1 );
add_action( 'wp_head',             'noindex',                       1     );
add_action( 'wp_head',             'wp_print_styles',               8     );
add_action( 'wp_head',             'wp_print_head_scripts',         9     );
add_action( 'wp_head',             'wp_generator'                         );
add_action( 'wp_head',             'rel_canonical'                        );
add_action( 'wp_footer',           'wp_print_footer_scripts'              );
add_action( 'wp_head',             'wp_shortlink_wp_head',          10, 0 );
add_action( 'template_redirect',   'wp_shortlink_header',           11, 0 );

if ( isset( $_GET['replytocom'] ) )
    add_filter( 'pre_option_blog_public', '__return_zero' );

// Login actions
add_action( 'login_head',          'wp_print_head_scripts',         9     );
add_action( 'login_footer',        'wp_print_footer_scripts'              );
add_action( 'login_init',          'send_frame_options_header',     10, 0 );

// Feed Generator Tags
foreach ( array( 'rss2_head', 'commentsrss2_head', 'rss_head', 'rdf_header', 'atom_head', 'comments_atom_head', 'opml_head', 'app_head' ) as $action ) {
	add_action( $action, 'the_generator' );
}

// WP Cron
if ( !defined( 'DOING_CRON' ) )
	add_action( 'sanitize_comment_cookies', 'wp_cron' );

// 2 Actions 2 Furious
add_action( 'do_feed_rdf',                'do_feed_rdf',             10, 1 );
add_action( 'do_feed_rss',                'do_feed_rss',             10, 1 );
add_action( 'do_feed_rss2',               'do_feed_rss2',            10, 1 );
add_action( 'do_feed_atom',               'do_feed_atom',            10, 1 );
add_action( 'do_pings',                   'do_all_pings',            10, 1 );
add_action( 'do_robots',                  'do_robots'                      );
add_action( 'sanitize_comment_cookies',   'sanitize_comment_cookies'       );
add_action( 'admin_print_scripts',        'print_head_scripts',      20    );
add_action( 'admin_print_footer_scripts', 'print_footer_scripts',    20    );
add_action( 'admin_print_styles',         'print_admin_styles',      20    );
add_action( 'init',                       'smilies_init',             5    );
add_action( 'plugins_loaded',             'wp_maybe_load_widgets',    0    );
add_action( 'plugins_loaded',             'wp_maybe_load_embeds',     0    );
add_action( 'shutdown',                   'wp_ob_end_flush_all',      1    );
add_action( 'pre_post_update',            'wp_save_post_revision'          );
add_action( 'publish_post',               '_publish_post_hook',       5, 1 );
add_action( 'save_post',                  '_save_post_hook',          5, 2 );
add_action( 'transition_post_status',     '_transition_post_status',  5, 3 );
add_action( 'comment_form', 'wp_comment_form_unfiltered_html_nonce'        );
add_action( 'wp_scheduled_delete',        'wp_scheduled_delete'            );
add_action( 'before_wp_tiny_mce',         'wp_print_editor_js'             );
add_action( 'after_wp_tiny_mce',          'wp_preload_dialogs',      10, 1 );
add_action( 'admin_init',                 'send_frame_options_header', 10, 0 );
add_action( 'importer_scheduled_cleanup', 'wp_delete_attachment'           );

// Navigation menu actions
add_action( 'delete_post',                '_wp_delete_post_menu_item'      );
add_action( 'delete_term',                '_wp_delete_tax_menu_item'       );
add_action( 'transition_post_status', '_wp_auto_add_pages_to_menu',  10, 3 );

// Post Thumbnail CSS class filtering
add_action( 'begin_fetch_post_thumbnail_html', '_wp_post_thumbnail_class_filter_add'    );
add_action( 'end_fetch_post_thumbnail_html',   '_wp_post_thumbnail_class_filter_remove' );

// Redirect Old Slugs
add_action( 'template_redirect', 'wp_old_slug_redirect'              );
add_action( 'post_updated',      'wp_check_for_changed_slugs', 12, 3 );

// Nonce check for Post Previews
add_action( 'init', '_show_post_preview' );

// Timezone
add_filter( 'pre_option_gmt_offset','wp_timezone_override_offset' );

// Admin Color Schemes
add_action( 'admin_init', 'register_admin_color_schemes', 1);
add_action( 'admin_color_scheme_picker', 'admin_color_scheme_picker' );

unset($filter, $action);

?>
