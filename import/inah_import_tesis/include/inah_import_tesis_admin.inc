<?php
 
function inah_import_tesis_admin($form,&$form_state){
 	$form['intro'] = array(
 		"#type"		=> 'markup',
 		'#markup'	=> '<div>Forma para importar tesis</div>' 
 	);
 	
	$form['inah_import_tesis_fieldset'] = array(
		'#type' => 'fieldset',
		'#title' => t('Tesis batch import config'),
		'#description' => t('Settings for Tesis Batch Import'),
		);

	$form['inah_import_tesis_fieldset']['inah_import_tesis_dir_img'] = array(
		'#type' => 'textfield',
		'#title' => t('PDF folder uri'),
		'#description' => t('Directorio de subcarpetas de imágenes.'),
		'#default_value' => variable_get('inah_import_tesis_dir_img', '/opt/Acervos/006_Tesis ENAH/Tesis Digitales A. Social/Tesis A. Social. Lic/Archivos Digitales A. Social Licenciatura'),
		'#size' => 20,
		);

	$form['inah_import_tesis_fieldset']['inah_import_tesis_dir_mods'] = array(
		'#type' => 'textfield',
		'#title' => t('MODS folder uri'),
		'#description' => t('Directorio de MODS.'),
		'#default_value' => variable_get('inah_import_tesis_dir_mods', '/home/eduardocs/mods'),
		'#size' => 20,
		);

	$form['inah_import_tesis_fieldset']['inah_import_tesis_col'] = array(
		'#type' => 'textfield',
		'#title' => t('Collection ID to ingest'),
		'#description' => t('PID de la coleccion a donde se agregara el objeto.'),
		'#default_value' => variable_get('inah_import_tesis_col', 'islandora:sp_pdf_collection'),
		'#size' => 20,
		);

	$form['inah_import_tesis_fieldset']['inah_import_tesis_max'] = array(
		'#type' => 'textfield',
		'#title' => t('Max imports'),
		'#description' => t('Máximo de tesiss a importar.'),
		'#element_validate' => array('element_validate_number'),
		'#default_value' => variable_get('inah_import_tesis_max', 1),
		'#size' => 5,
		);

	$form['inah_import_tesis_fieldset']['inah_import_tesis_curr'] = array(
		'#type' => 'textfield',
		'#title' => t('Current offset'),
		'#description' => t('Current offset already imported. Starts at <i>0</i>'),
		'#element_validate' => array('element_validate_number'),
		'#default_value' => variable_get('inah_import_tesis_curr', -1)+1,
		'#size' => 5,
		);

 	$form['inah_import_tesis_do_import'] = array(
 		"#type"		=> 'submit',
 		'#value'	=> 'Importar las tesis' 
 	);
 	
 	return $form;
}

function inah_import_tesis_admin_validate($form,&$form_state){
	
}


function inah_import_tesis_admin_submit(array $form, array &$form_state) {

	//DEBUG
	file_put_contents('/home/eduardocs/testing.log', print_r(array('inah_import_tesis_admin_submit'),true));

	$repo = inah_import_tesis_connectToFedora();
	$batch = array(
			'init_message'	=> t('INICIANDO LA IMPORTACION'),
		'progress_message' 	=> t('Importando @current de un total de @total.'),
		'error_message' 	=> t('El proceso de importación ha fallado.'),

			'title' => t('Importando tesis'),
	   'operations' => array(),
		 'finished' => 'inah_import_tesis_importFinish',
		     'file' => drupal_get_path('module','inah_import_tesis').'/include/inah_import_tesis_admin.inc', //Need to be import from this file
	);

	variable_set('inah_import_tesis_dir_img', $form_state['values']['inah_import_tesis_dir_img']);
	variable_set('inah_import_tesis_dir_mods', $form_state['values']['inah_import_tesis_dir_mods']);
	variable_set('inah_import_tesis_col', $form_state['values']['inah_import_tesis_col']);


	$img_folder = /*utf8_decode*/(variable_get('inah_import_tesis_dir_img', '/opt/Acervos/006_Tesis ENAH/Tesis Digitales A. Social/Tesis A. Social. Lic/Archivos Digitales A. Social Licenciatura'));
	$mods_folder = /*utf8_decode*/(variable_get('inah_import_tesis_dir_mods', '/home/eduardocs/mods'));
	$collectionid = variable_get('inah_import_tesis_col', 'islandora:sp_pdf_collection');

	$imgdirs = inah_import_tesis_listFolders($img_folder,false);

	$allowed_extensions = array('pdf');
	$callback = function ($element) use ($allowed_extensions) {
		$ext = pathinfo($element, PATHINFO_EXTENSION);
		$ext = strtolower($ext);
		//$ext = drupal_strtolower($ext);

	// An allowed extension and does /not/ contain __MACOSX.
		return in_array($ext, $allowed_extensions) && preg_match('/__MACOSX/', $element) === 0;
	};

	$tesiss = array();
	for ($i=0;$i<count($imgdirs);$i++) {

		//$imgdirs[$i]['pages'] = $files_to_add;
		$mods_file = substr($imgdirs[$i]['fil'],0,-4).".MODS.xml";
		if (file_exists($mods_folder."/".$mods_file)) {
			$imgdirs[$i]['mods'] = $mods_file;
		}

		$tesiss[] = $imgdirs[$i];
	}

	file_put_contents('/home/eduardocs/testing.log',print_r($tesiss,true),FILE_APPEND);
	//die();

	//Máximo de importaciones
	$max_imports = $form_state['values']['inah_import_tesis_max'];
	if ($max_imports<=0) $max_imports = 1;
	variable_set('inah_import_tesis_max', $max_imports);

	//Número actual en memoria
	$curr_imports = $form_state['values']['inah_import_tesis_curr']-1;
	if ($curr_imports>=count($tesiss)) $curr_imports = -1;
	variable_set('inah_import_tesis_curr', $curr_imports);

	$operations = array();
	$i=-1;
	$ii=0;
	foreach ($tesiss as $tesis) {
		$i++;
		if ($i<=$curr_imports || $i>$curr_imports+$max_imports) continue;

		$operations[] = array('inah_import_tesis_createIslandoraObject'	, array($img_folder,$mods_folder,$tesis,$collectionid,$repo));

	}

	$batch['operations'] = $operations;
	batch_set($batch);
} 

function inah_import_tesis_createIslandoraObject($img_folder,$mods_folder,$tesis,$collectionid,$repository,&$context){
 	
 	$context['message'] = "Creando objeto tesis : <i>".$tesis['fil']."</i> " ;
 	inah_import_tesis_ingestFedoraObject(/*utf8_encode*/($img_folder),/*utf8_encode*/($mods_folder),$tesis,$collectionid,$repository,$context);
 	
}

function inah_import_tesis_ingestFedoraObject($img_folder,$mods_folder,$tesis,$collectionUUID,$repository,&$context){
	global $user;

	$namespace		= 'islandora';
	$contentModel	= 'islandora:sp_pdf';

	$object = $repository->constructObject($namespace); // allow fedora to generate a PID
	$object->models = $contentModel;
	$object->relationships->add(FEDORA_RELS_EXT_URI, 'isMemberOfCollection', $collectionUUID);
	//$object->relationships->add(FEDORA_MODEL_URI, 'hasModel', $contentModel);
	$object->owner = $user->name;

	if (!empty($tesis['mods'])) {
		$xmlModsAct = simplexml_load_file($mods_folder."/".$tesis['mods']);
		$object->label = /*utf8_encode*/($xmlModsAct->titleInfo->title);
	} else {
		$object->label = /*utf8_encode*/($tesis['fil']);
	}


	// Extract file vars.
	$pdf_file = /*utf8_decode*/($img_folder)."/".$tesis['fil'];
	$mods_file = /*utf8_decode*/($mods_folder)."/".$tesis['mods'];

	$object_url = url("islandora/object/".$object->id, array('absolute' => TRUE));

	/**
	 * INGESTA EL OBJETO PDF
	 */
	//DEBUG
	file_put_contents('/home/eduardocs/testing.log',print_r(array('PDF',$pdf_file,'MODS',$mods_file),true),FILE_APPEND);

	if (file_exists($mods_file)) {
		$dcString  = inah_import_tesis_mods_to_dc($mods_file);

		$DC = $object->constructDatastream('DC','M');
		$DC->label    = 'Dublin Core';
		$DC->mimetype = 'text/xml';
		$DC->setContentFromString($dcString); 
		$object->ingestDatastream($DC);

		inah_import_tesis_content_update_datastream($object, $mods_file, 'MODS', 'MODS Metadata', NULL, 'M', FALSE);
	}
	inah_import_tesis_content_update_datastream($object, $pdf_file, 'OBJ', 'PDF File', NULL, 'M', FALSE);

 	$repository->ingestObject($object);
	module_load_include('inc', 'islandora_pdf', 'includes/derivatives');
	islandora_pdf_add_fulltext_derivative($object, TRUE);
	islandora_pdf_add_preview_derivative($object, TRUE);
	islandora_pdf_add_tn_derivative($object, TRUE);


	//DEBUG
	file_put_contents('/home/eduardocs/ingested.log',$object->id.'|'.$pdf_file."\n",FILE_APPEND);

    //Ingest tesis object
	$context['message'] = "Ingestando objeto PDF  : <i>".$pdf_file."</i> " ;

	//Update current tesis for later imports
	variable_set('inah_import_tesis_curr', variable_get('inah_import_tesis_curr', -1)+1);
}

/*
 * MODS to DC converter
 * @param  String $mods uri to MODS file
 * @return String $dcString DC XML String (not file)
 */
function inah_import_tesis_mods_to_dc($mods) {
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
function inah_import_tesis_content_update_datastream(AbstractObject $object, $file, $dsid, $label = NULL, $mimetype = NULL, $control_group = 'M', $copy = TRUE) {
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

function inah_import_tesis_importFinish($success, $results, $operations, &$context){
// The 'success' parameter means no fatal PHP errors were detected. All
// other error management should be handled using 'results'.
	if ($success) {
		$message = format_plural(count($results), 'One post processed.', '@count posts processed.');
	}  else {
		$message = t('Finished with an error.');
	}
	drupal_set_message($message);
	$items = array();
// Providing data for the redirected page is done through $_SESSION.
	foreach ($results as $result) {
		$items[] = t('Loaded node %title.', array('%title' => $result));
	}
	$_SESSION['my_batch_results'] = $items;
}


/////////////////////////////////////////////////////

function inah_import_tesis_connectToFedora(){
 	global $user;
 	$connection = islandora_get_tuque_connection($user);
 	$connection->reuseConnection = TRUE;
 	$repository = $connection->repository;
 	return $repository;
}

function inah_import_tesis_listFolders($dir,$isfile=false) {
    $ffs = scandir($dir);
    $r = array();
    foreach ($ffs as $ff) {
        if ($ff != '.' && $ff != '..') {
            $r[] = array('fil'=>$ff);
        }
    }
    return $r;
}