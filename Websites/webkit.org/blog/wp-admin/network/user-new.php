<?php
/**
 * Add Site Administration Screen
 *
 * @package WordPress
 * @subpackage Multisite
 * @since 3.1.0
 */

/** Load WordPress Administration Bootstrap */
require_once( './admin.php' );

if ( ! is_multisite() )
	wp_die( __( 'Multisite support is not enabled.' ) );

if ( ! current_user_can('create_users') )
	wp_die(__('You do not have sufficient permissions to add users to this network.'));


add_contextual_help($current_screen,
	'<p>' . __('Add User will set up a new user account on the network and send that person an email with username and password.') . '</p>' .
	'<p>' . __('Users who are signed up to the network without a site are added as subscribers to the main or primary dashboard site, giving them profile pages to manage their accounts. These users will only see Dashboard and My Sites in the main navigation until a site is created for them.') . '</p>' .
	'<p><strong>' . __('For more information:') . '</strong></p>' .
	'<p>' . __('<a href="http://codex.wordpress.org/Network_Admin_Users_Screen" target="_blank">Documentation on Network Users</a>') . '</p>' .
	'<p>' . __('<a href="http://wordpress.org/support/forum/multisite/" target="_blank">Support Forums</a>') . '</p>'
);

if ( isset($_REQUEST['action']) && 'add-user' == $_REQUEST['action'] ) {
	check_admin_referer( 'add-user', '_wpnonce_add-user' );
	if ( ! current_user_can( 'manage_network_users' ) )
		wp_die( __( 'You do not have permission to access this page.' ) );

	if ( ! is_array( $_POST['user'] ) )
		wp_die( __( 'Cannot create an empty user.' ) );

	$user = $_POST['user'];

	$user_details = wpmu_validate_user_signup( $user['username'], $user['email'] ); 
	if ( is_wp_error( $user_details[ 'errors' ] ) && ! empty( $user_details[ 'errors' ]->errors ) ) {
		$add_user_errors = $user_details[ 'errors' ];
	} else {
		$password = wp_generate_password( 12, false);
		$user_id = wpmu_create_user( esc_html( strtolower( $user['username'] ) ), $password, esc_html( $user['email'] ) );

		if ( ! $user_id ) {
	 		$add_user_errors = new WP_Error( 'add_user_fail', __( 'Cannot add user.' ) );
		} else {
			wp_new_user_notification( $user_id, $password );
			wp_redirect( add_query_arg( array('update' => 'added'), 'user-new.php' ) );
			exit;
		}
	}
}

if ( isset($_GET['update']) ) {
	$messages = array();
	if ( 'added' == $_GET['update'] )
		$messages[] = __('User added.');
}

$title = __('Add New User');
$parent_file = 'users.php';

require('../admin-header.php'); ?>

<div class="wrap">
<?php screen_icon(); ?>
<h2 id="add-new-user"><?php _e('Add New User') ?></h2>
<?php
if ( ! empty( $messages ) ) {
	foreach ( $messages as $msg )
		echo '<div id="message" class="updated"><p>' . $msg . '</p></div>';
}

if ( isset( $add_user_errors ) && is_wp_error( $add_user_errors ) ) { ?>
	<div class="error">
		<?php
			foreach ( $add_user_errors->get_error_messages() as $message )
				echo "<p>$message</p>";
		?>
	</div>
<?php } ?>
	<form action="<?php echo network_admin_url('user-new.php?action=add-user'); ?>" id="adduser" method="post">	
	<table class="form-table">
		<tr class="form-field form-required">
			<th scope="row"><?php _e( 'Username' ) ?></th>
			<td><input type="text" class="regular-text" name="user[username]" /></td>
		</tr>
		<tr class="form-field form-required">
			<th scope="row"><?php _e( 'Email' ) ?></th>
			<td><input type="text" class="regular-text" name="user[email]" /></td>
		</tr>
		<tr class="form-field">
			<td colspan="2"><?php _e( 'Username and password will be mailed to the above email address.' ) ?></td>
		</tr>
	</table>
	<?php wp_nonce_field( 'add-user', '_wpnonce_add-user' ) ?>
	<?php submit_button( __('Add User'), 'primary', 'add-user' ); ?>
	</form>
</div>
<?php
require('../admin-footer.php');
?>
