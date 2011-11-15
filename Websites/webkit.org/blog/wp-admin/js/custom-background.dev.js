var farbtastic;

function pickColor(color) {
	farbtastic.setColor(color);
	jQuery('#background-color').val(color);
	jQuery('#custom-background-image').css('background-color', color);
	if ( color && color !== '#' )
		jQuery('#clearcolor').show();
	else
		jQuery('#clearcolor').hide();
}

jQuery(document).ready(function() {
	jQuery('#pickcolor').click(function() {
		jQuery('#colorPickerDiv').show();
		return false;
	});

	jQuery('#clearcolor a').click( function(e) {
		pickColor('');
		e.preventDefault();
	});

	jQuery('#background-color').keyup(function() {
		var _hex = jQuery('#background-color').val(), hex = _hex;
		if ( hex.charAt(0) != '#' )
			hex = '#' + hex;
		hex = hex.replace(/[^#a-fA-F0-9]+/, '');
		if ( hex != _hex )
			jQuery('#background-color').val(hex);
		if ( hex.length == 4 || hex.length == 7 )
			pickColor( hex );
	});

	jQuery('input[name="background-position-x"]').change(function() {
		jQuery('#custom-background-image').css('background-position', jQuery(this).val() + ' top');
	});

	jQuery('input[name="background-repeat"]').change(function() {
		jQuery('#custom-background-image').css('background-repeat', jQuery(this).val());
	});

	farbtastic = jQuery.farbtastic('#colorPickerDiv', function(color) {
		pickColor(color);
	});
	pickColor(jQuery('#background-color').val());

	jQuery(document).mousedown(function(){
		jQuery('#colorPickerDiv').each(function(){
			var display = jQuery(this).css('display');
			if ( display == 'block' )
				jQuery(this).fadeOut(2);
		});
	});
});
