<?php
 
function inah_import_batch_purge($form,&$form_state){
 	$form['intro'] = array(
 		"#type"		=> 'markup',
 		'#markup'	=> '<div>Forma para eliminar objetos por PID</div>' 
 	);

	$form['inah_import_batch_purge_ini'] = array(
		'#type' => 'textfield',
		'#title' => t('Ini PID'),
		'#description' => t('Pid inicial.'),
		'#element_validate' => array('element_validate_number'),
		'#default_value' => variable_get('inah_import_batch_purge_ini', 0),
		'#size' => 5,
		);

	$form['inah_import_batch_purge_fin'] = array(
		'#type' => 'textfield',
		'#title' => t('Fin PID'),
		'#description' => t('Pid final.'),
		'#element_validate' => array('element_validate_number'),
		'#default_value' => variable_get('inah_import_batch_purge_fin', 0),
		'#size' => 5,
		);

	$form['inah_import_batch_purge_oneperline'] = array(
      '#type' => 'text_format',
      '#title' => t('Add one pid per line'),
      '#size' => 60,
      '#description' => t('This will purge all object of theese pids.'),
      '#default_value' => '',
      '#format' => 'plain_text',
      );

 	$form['inah_import_batch_do_purge'] = array(
 		"#type"		=> 'submit',
 		'#value'	=> 'Eliminar los objetos' 
 	);
 	
 	return $form;
}

function inah_import_batch_purge_validate($form,&$form_state){
	
}

function inah_import_batch_connectToFedora(){
 	global $user;
 	$connection = islandora_get_tuque_connection($user);
 	$connection->reuseConnection = TRUE;
 	$repository = $connection->repository;
 	return $repository;
}

function inah_import_batch_purge_submit(array $form, array &$form_state) {
	$repo = inah_import_batch_connectToFedora();
	$ini = $form_state['values']['inah_import_batch_purge_ini'];
	$fin = $form_state['values']['inah_import_batch_purge_fin'];
	$pids = $form_state['values']['inah_import_batch_purge_oneperline']['value'];
	$pids = trim($pids);
	if (!empty($pids)) {
		$pids = str_replace("\r","",$pids);
		$pids = explode("\n",$pids);
		foreach ($pids as $pid) {
			$pid = trim($pid);
			$pid = str_replace("info:fedora/","",$pid);
			try {
				$obj = $repo->getObject($pid);
			} catch(Exception $e) {
				drupal_set_message(t("Error loading object").": '".$pid."'");
				continue;
			}
			$repo->purgeObject($obj);
		}

	} else {
		$namespace = 'islandora';
		for ($i=$ini;$i<=$fin;$i++) {
			$pid = $namespace.":".$i;
			try {
				$obj = $repo->getObject($pid);
			} catch(Exception $e) {
				drupal_set_message("Error loading object: '".$pid."'");
				continue;
			}
			$repo->purgeObject($obj);
		}
	}
}