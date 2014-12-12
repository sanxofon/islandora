jQuery( document ).ready(function() {
	if (jQuery("#edit-inah-import-disco-redo-max").val()>jQuery("#edit-inah-import-disco-curr").val()+jQuery("#edit-inah-import-disco-max").val()) {
		setTimeout('jQuery("#edit-inah-import-disco-do-import").click();',30000);
		console.log('Re-enviando el formulario en 30 segundos');
	}
});