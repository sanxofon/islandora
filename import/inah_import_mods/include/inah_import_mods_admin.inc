<?php
function inah_import_mods_admin($form,&$form_state){
 	$form['intro'] = array(
 		"#type"		=> 'markup',
 		'#markup'	=> '<div>Forma para actualizar MODS por PID</div>' 
 	);
 	
	$form['inah_import_mods_fieldset'] = array(
		'#type' => 'fieldset',
		'#title' => t('MODS batch update config'),
		'#description' => t('Settings for MODS Batch Update'),
		);

	$form['inah_import_youtube_fieldset']['inah_import_mods_dir'] = array(
		'#type' => 'textfield',
		'#title' => t('MODS folder uri'),
		'#description' => t('Directorio de MODS.'),
		'#default_value' => variable_get('inah_import_mods_dir', '/home/eduardocs/codexmods'),
		'#size' => 20,
		);

 	$form['inah_import_mods_do_import'] = array(
 		"#type"		=> 'submit',
 		'#value'	=> 'Actualizar los mods' 
 	);
 	
 	return $form;
}

function inah_import_mods_admin_validate($form,&$form_state){
	
}

function inah_import_mods_admin_submit(array $form, array &$form_state) {
	$repo = inah_import_mods_connectToFedora();
	$modsdir = $form_state['values']['inah_import_mods_dir'];
  variable_set('inah_import_mods_dir', $modsdir);

	$modsfiles = inah_import_mods_listFolders($modsdir,false);

	foreach($modsfiles as $mods) {
		if (file_exists($modsdir.'/'.$mods)) {
      //EJEMPLO: islandora_234.xml
			$pid = substr($mods,0,-strlen('.MODS.xml'));
      //EJEMPLO: islandora_234
      //$pid = str_replace('_', ':'.$pid;
      $pid = 'inah:'.$pid;
      //EJEMPLO: islandora:234
			unset($object);

			$object = islandora_object_load($pid);

			$dcString  = inah_import_mods_mods_to_dc($modsdir.'/'.$mods);
      //$object->purgeDatastream('DC');
			$DC = $object['DC'];
			//$DC->label    = 'Dublin Core';
			//$DC->mimetype = 'text/xml';
			$DC->setContentFromString($dcString); 
			//$object->ingestDatastream($DC);

			inah_import_mods_content_update_datastream($object, $modsdir.'/'.$mods, 'MODS', 'MODS Metadata', NULL, 'M', FALSE);
		} else {
			drupal_set_message('No MODS file found for '.$mods,'error');
		}
	}
}



function inah_import_mods_connectToFedora(){
 	global $user;
 	$connection = islandora_get_tuque_connection($user);
 	$connection->reuseConnection = TRUE;
 	$repository = $connection->repository;
 	return $repository;
}

/**
 * MODS to DC converter
 * @param  String $mods uri to MODS file
 * @return String $dcString DC XML String (not file)
 */
function inah_import_mods_mods_to_dc($mods) {
	$xsltPaht = drupal_get_path('module','islandora_importer')."/xsl/mods_to_dc.xsl";
    $xsl = new DOMDocument();
    $xsl->load($xsltPaht);

    $xml = simplexml_load_file($mods);

    $processor = new XSLTProcessor();
    $processor->importStylesheet($xsl);
    $dcString  = $processor->transformToXML($xml);

    return $dcString;
}
/**
 * Create/Update a datastream with the given parameters.
 *
 * @param AbstractObject $object
 *   The object to update.
 * @param string $file
 *   The absolute path to the file to use as content for the datastream.
 * @param string $dsid
 *   The datastream ID.
 * @param string $label
 *   The datastream label.
 * @param string $mimetype
 *   The datastream mimetype, if none given it will be guessed from the file
 *   extension.
 * @param string $control_group
 *   The datastream control group.
 *
 * @return bool
 *   TRUE if successful, FALSE otherwise.
 */
function inah_import_mods_content_update_datastream(AbstractObject $object, $file, $dsid, $label = NULL, $mimetype = NULL, $control_group = 'M', $copy = TRUE) {
  if (!isset($mimetype)) {
    $mime_detector = new MimeDetect();
    $mimetype = $mime_detector->getMimetype($file);
  }
  $label = isset($label) ? $label : $dsid;

  if (!isset($object[$dsid])) {
    $ds = $object->constructDatastream($dsid, $control_group);
    $ds->mimetype = $mimetype;
    $ds->label = $label;
    $ds->setContentFromFile($file, $copy);
    $object->ingestDatastream($ds);
  }
  else {
    $ds = $object[$dsid];
    $ds->setContentFromFile($file, $copy);
    if ($ds->label != $label) {
      $ds->label = $label;
    }
    if ($ds->mimetype != $mimetype) {
      $ds->mimetype = $mimetype;
    }
  }
  return TRUE;
}
function inah_import_mods_listFolders($dir,$isfile=false) {
    $ffs = scandir($dir);
    $r = array();
    foreach ($ffs as $ff) {
        if ($ff != '.' && $ff != '..') {
            if (!is_dir($dir.'/'.$ff) ) {
            	$r[] = $ff;
            }
        }
    }
    return $r;
}

