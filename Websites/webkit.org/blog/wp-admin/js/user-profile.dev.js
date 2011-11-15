(function($){

	function check_pass_strength() {
		var pass1 = $('#pass1').val(), user = $('#user_login').val(), pass2 = $('#pass2').val(), strength;

		$('#pass-strength-result').removeClass('short bad good strong');
		if ( ! pass1 ) {
			$('#pass-strength-result').html( pwsL10n.empty );
			return;
		}

		strength = passwordStrength(pass1, user, pass2);

		switch ( strength ) {
			case 2:
				$('#pass-strength-result').addClass('bad').html( pwsL10n['bad'] );
				break;
			case 3:
				$('#pass-strength-result').addClass('good').html( pwsL10n['good'] );
				break;
			case 4:
				$('#pass-strength-result').addClass('strong').html( pwsL10n['strong'] );
				break;
			case 5:
				$('#pass-strength-result').addClass('short').html( pwsL10n['mismatch'] );
				break;
			default:
				$('#pass-strength-result').addClass('short').html( pwsL10n['short'] );
		}
	}

	$(document).ready(function() {
		$('#pass1').val('').keyup( check_pass_strength );
		$('#pass2').val('').keyup( check_pass_strength );
		$('#pass-strength-result').show();
		$('.color-palette').click(function(){$(this).siblings('input[name="admin_color"]').prop('checked', true)});
		$('#first_name, #last_name, #nickname').blur(function(){
			var select = $('#display_name'), current = select.find('option:selected').attr('id'), dub = [],
				inputs = {
					display_nickname : $('#nickname').val(),
					display_username : $('#user_login').val(),
					display_firstname : $('#first_name').val(),
					display_lastname : $('#last_name').val()
				};

			if ( inputs.display_firstname && inputs.display_lastname ) {
				inputs['display_firstlast'] = inputs.display_firstname + ' ' + inputs.display_lastname;
				inputs['display_lastfirst'] = inputs.display_lastname + ' ' + inputs.display_firstname;
			}

			$('option', select).remove();
			$.each(inputs, function( id, value ) {
				var val = value.replace(/<\/?[a-z][^>]*>/gi, '');

				if ( inputs[id].length && $.inArray( val, dub ) == -1 ) {
					dub.push(val);
					$('<option />', {
					  	'id': id,
						'text': val,
						'selected': (id == current)
					}).appendTo( select );
				}
			});
		});
	});

})(jQuery);
