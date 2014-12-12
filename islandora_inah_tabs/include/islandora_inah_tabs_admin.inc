<?php
function islandora_inah_tabs_admin($form,&$form_state){

 	$form['intro'] = array(
 		"#type"		=> 'markup',
 		'#markup'	=> '<div>Forma para importar discos</div>' 
 	);
 	
	$form['islandora_inah_tabs_fieldset'] = array(
		'#type' => 'fieldset',
		'#title' => t('INAH Tabs config'),
		'#description' => t('Settings for INAH Tabs View'),
		);

	$form['islandora_inah_tabs_fieldset']['islandora_inah_tabs_show'] = array(
		'#type' => 'checkbox',
		'#title' => t('Mostrar u ocultar'),
		'#description' => t('Si el campo está marcado se mostrarán los datos.'),
		'#default_value' => variable_get('islandora_inah_tabs_show', FALSE),
		);

	$form['islandora_inah_tabs_fieldset']['islandora_inah_tabs_pdf'] = array(
		'#type' => 'checkbox',
		'#title' => t('Mostrar u ocultar'),
		'#description' => t('Si el campo está marcado se mostrará la pestaña PDF en los objetos que tengan ese Datastream.'),
		'#default_value' => variable_get('islandora_inah_tabs_pdf', FALSE),
		);

 	$form['islandora_inah_tabs_do_save'] = array(
 		"#type"		=> 'submit',
 		'#value'	=> 'Guardar los cambios' 
 	);
 	
 	return $form;
}

function islandora_inah_tabs_admin_submit(array $form, array &$form_state) {
	variable_set('islandora_inah_tabs_show', $form_state['values']['islandora_inah_tabs_show']);
	variable_set('islandora_inah_tabs_pdf', $form_state['values']['islandora_inah_tabs_pdf']);
}