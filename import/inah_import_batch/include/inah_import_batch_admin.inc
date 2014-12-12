<?php

function inah_import_batch_admin($form,&$form_state){
 	$form['intro'] = array(
 		"#type"		=> 'markup',
 		'#markup'	=> '<div>Importar datos del INAH</div>' 
 	);
 	
 	return $form;
}