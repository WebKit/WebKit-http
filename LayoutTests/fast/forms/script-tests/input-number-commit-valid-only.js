description('Type=number field should not accept invalid numbers though a user can type such strings');

var parent = document.createElement('div');
document.body.appendChild(parent);
parent.innerHTML = '<input type=number id=number value=256>';

var input = document.getElementById('number');
input.focus();
document.execCommand('SelectAll', false, null);
document.execCommand('InsertText', false, '512');
shouldBe('input.value', '"512"');

document.execCommand('SelectAll', false, null);
document.execCommand('InsertText', false, '+++');
input.blur();
shouldBe('input.value', '"512"');

input.focus();
document.execCommand('SelectAll', false, null);
document.execCommand('InsertText', false, '');
input.blur();
shouldBe('input.value', '""');

var successfullyParsed = true;
