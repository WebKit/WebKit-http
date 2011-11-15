<?php
/**
 * Multisite sites administration panel.
 *
 * @package WordPress
 * @subpackage Multisite
 * @since 3.0.0
 */

/** Load WordPress Administration Bootstrap */
require_once( './admin.php' );

if ( ! is_multisite() )
	wp_die( __( 'Multisite support is not enabled.' ) );

if ( ! current_user_can( 'manage_sites' ) )
	wp_die( __( 'You do not have permission to access this page.' ) );

$wp_list_table = _get_list_table('WP_MS_Sites_List_Table');
$pagenum = $wp_list_table->get_pagenum();

$title = __( 'Sites' );
$parent_file = 'sites.php';

add_screen_option( 'per_page', array('label' => _x( 'Sites', 'sites per page (screen options)' )) );

add_contextual_help($current_screen,
	'<p>' . __('Add New takes you to the Add New Site screen. You can search for a site by Name, ID number, or IP address. Screen Options allows you to choose how many sites to display on one page.') . '</p>' .
	'<p>' . __('This is the main table of all sites on this network. Switch between list and excerpt views by using the icons above the right side of the table.') . '</p>' .
	'<p>' . __('Hovering over each site reveals seven options (three for the primary site):') . '</p>' .
	'<ul><li>' . __('An Edit link to a separate Edit Site screen.') . '</li>' .
	'<li>' . __('Dashboard leads to the Dashboard for that site.') . '</li>' .
	'<li>' . __('Deactivate, Archive, and Spam which lead to confirmation screens. These actions can be reversed later.') . '</li>' .
	'<li>' . __('Delete which is a permanent action after the confirmation screens.') . '</li>' .
	'<li>' . __('Visit to go to the frontend site live.') . '</li></ul>' .
	'<p>' . __('The site ID is used internally, and is not shown on the front end of the site or to users/viewers.') . '</p>' .
	'<p>' . __('Clicking on bold headings can re-sort this table.') . '</p>' .
	'<p><strong>' . __('For more information:') . '</strong></p>' .
	'<p>' . __('<a href="http://codex.wordpress.org/Network_Admin_Sites_Screens" target="_blank">Documentation on Site Management</a>') . '</p>' .
	'<p>' . __('<a href="http://wordpress.org/support/forum/multisite/" target="_blank">Support Forums</a>') . '</p>'
);

$id = isset( $_REQUEST['id'] ) ? intval( $_REQUEST['id'] ) : 0;

$msg = '';
if ( isset( $_REQUEST['updated'] ) && $_REQUEST['updated'] == 'true' && ! empty( $_REQUEST['action'] ) ) {
	switch ( $_REQUEST['action'] ) {
		case 'all_notspam':
			$msg = __( 'Sites removed from spam.' );
		break;
		case 'all_spam':
			$msg = __( 'Sites marked as spam.' );
		break;
		case 'all_delete':
			$msg = __( 'Sites deleted.' );
		break;
		case 'delete':
			$msg = __( 'Site deleted.' );
		break;
		case 'not_deleted':
			$msg = __( 'You do not have permission to delete that site.' );
		break;
		case 'archive':
			$msg = __( 'Site archived.' );
		break;
		case 'unarchive':
			$msg = __( 'Site unarchived.' );
		break;
		case 'activate':
			$msg = __( 'Site activated.' );
		break;
		case 'deactivate':
			$msg = __( 'Site deactivated.' );
		break;
		case 'unspam':
			$msg = __( 'Site removed from spam.' );
		break;
		case 'spam':
			$msg = __( 'Site marked as spam.' );
		break;
		default:
			$msg = apply_filters( 'network_sites_updated_message_' . $_REQUEST['action'] , __( 'Settings saved.' ) );
		break;
	}
	if ( $msg )
		$msg = '<div class="updated" id="message"><p>' . $msg . '</p></div>';
}

$wp_list_table->prepare_items();

require_once( '../admin-header.php' );
?>

<div class="wrap">
<?php screen_icon('ms-admin'); ?>
<h2><?php _e('Sites') ?>
<?php echo $msg; ?>
<?php if ( current_user_can( 'create_sites') ) : ?>
        <a href="<?php echo network_admin_url('site-new.php'); ?>" class="add-new-h2"><?php echo esc_html_x( 'Add New', 'site' ); ?></a>
<?php endif; ?>

<?php if ( isset( $_REQUEST['s'] ) && $_REQUEST['s'] ) {
	printf( '<span class="subtitle">' . __( 'Search results for &#8220;%s&#8221;' ) . '</span>', esc_html( $s ) );
} ?>
</h2>

<form action="" method="get" id="ms-search">
<?php $wp_list_table->search_box( __( 'Search Sites' ), 'site' ); ?>
<input type="hidden" name="action" value="blogs" />
</form>

<form id="form-site-list" action="edit.php?action=allblogs" method="post">
	<?php $wp_list_table->display(); ?>
</form>
</div>
<?php

require_once( '../admin-footer.php' ); ?>
