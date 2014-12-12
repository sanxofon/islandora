<?php
 
function inah_import_youtube_admin($form,&$form_state){
 	$form['intro'] = array(
 		"#type"		=> 'markup',
 		'#markup'	=> '<div>Forma para importar videos_youtube</div>' 
 	);
 	
	$form['inah_import_youtube_fieldset'] = array(
		'#type' => 'fieldset',
		'#title' => t('Video Youtube batch import config'),
		'#description' => t('Settings for Video Youtube Batch Import'),
		);

	$form['inah_import_youtube_fieldset']['inah_import_youtube_dir'] = array(
		'#type' => 'textfield',
		'#title' => t('Youtube videos folder uri'),
		'#description' => t('Directorio de subcarpetas de videos youtube.'),
		'#default_value' => variable_get('inah_import_youtube_dir', '/home/santiago/114_RADIOINAH'),
		'#size' => 20,
		);

	$form['inah_import_youtube_fieldset']['inah_import_youtube_cmodel'] = array(
		'#type' => 'textfield',
		'#title' => t('CModel of parent objects.'),
		'#description' => t('CModel en que se ingestarán los videos youtube.'),
		'#default_value' => variable_get('inah_import_youtube_cmodel', 'islandora:sp-youtubeCModel'),
		'#size' => 20,
		);

	$form['inah_import_youtube_fieldset']['inah_import_youtube_col'] = array(
		'#type' => 'textfield',
		'#title' => t('Collection to ingest to.'),
		'#description' => t('Colección a donde se ingestarán los videos_youtube.'),
		'#default_value' => variable_get('inah_import_youtube_col', 'islandora:youtube_collection'),
		'#size' => 20,
		);

	$form['inah_import_youtube_fieldset']['inah_import_youtube_max'] = array(
		'#type' => 'textfield',
		'#title' => t('Max imports'),
		'#description' => t('Máximo de videos_youtube a importar.'),
		'#element_validate' => array('element_validate_number'),
		'#default_value' => variable_get('inah_import_youtube_max', 1),
		'#size' => 5,
		);

	$form['inah_import_youtube_fieldset']['inah_import_youtube_curr'] = array(
		'#type' => 'textfield',
		'#title' => t('Current offset'),
		'#description' => t('Current offset already imported. Starts at <i>0</i>'),
		'#element_validate' => array('element_validate_number'),
		'#default_value' => variable_get('inah_import_youtube_curr', -1)+1,
		'#size' => 5,
		);

 	$form['inah_import_youtube_do_import'] = array(
 		"#type"		=> 'submit',
 		'#value'	=> 'Importar los videos_youtube' 
 	);
 	
 	return $form;
}

function inah_import_youtube_admin_validate($form,&$form_state){
	
}

function inah_import_youtube_connectToFedora(){
 	global $user;
 	$connection = islandora_get_tuque_connection($user);
 	$connection->reuseConnection = TRUE;
 	$repository = $connection->repository;
 	return $repository;
}

function inah_import_youtube_listFolders($dir,$isfile=false) {
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

function inah_import_youtube_admin_submit(array $form, array &$form_state) {

	//DEBUG
	file_put_contents('/home/santiago/114_RADIOINAH/testing.log', print_r(array('inah_import_youtube_admin_submit'),true));

	$repo = inah_import_youtube_connectToFedora();
	$batch = array(
			'init_message'		=> t('INICIANDO LA IMPORTACION'),
		'progress_message' 	=> t('Importando @current de un total de @total.'),
		'error_message' 	=> t('El proceso de importación a fallado.'),

			'title' => t('Importando videos_youtube'),
		'operations' => array(),
			'finished' => 'inah_import_youtube_importFinish',
		'file' => drupal_get_path('module','inah_import_youtube').'/include/inah_import_youtube_admin.inc', //Need to be import from this file
	);

	variable_set('inah_import_youtube_dir', $form_state['values']['inah_import_youtube_dir']);
	variable_set('inah_import_youtube_cmodel', $form_state['values']['inah_import_youtube_cmodel']);
	variable_set('inah_import_youtube_col', $form_state['values']['inah_import_youtube_col']);

	//Máximo de importaciones
	$max_imports = $form_state['values']['inah_import_youtube_max'];
	if ($max_imports<=0) $max_imports = 1;
	variable_set('inah_import_youtube_max', $max_imports);

	//Número actual en memoria
	$curr_imports = $form_state['values']['inah_import_youtube_curr']-1;
	variable_set('inah_import_youtube_curr', $curr_imports);

	$vid_folder = variable_get('inah_import_youtube_dir', '/home/santiago/114_RADIOINAH');

	$urls = inah_import_youtube_listFolders($vid_folder.'/urls',false);
	$mp3s = inah_import_youtube_listFolders($vid_folder.'/audio',false);
	$mods = inah_import_youtube_listFolders($vid_folder.'/mods',false);
	$datas = inah_import_youtube_listFolders($vid_folder.'/data',false);

	//DEBUG
	file_put_contents('/home/santiago/114_RADIOINAH/testing.log', print_r(array('URLS',$urls),true),FILE_APPEND);
	file_put_contents('/home/santiago/114_RADIOINAH/testing.log', print_r(array('MP3S',$mp3s),true),FILE_APPEND);
	file_put_contents('/home/santiago/114_RADIOINAH/testing.log', print_r(array('MODS',$mods),true),FILE_APPEND);

	$videos_youtube = array();
	for ($i=0;$i<count($urls);$i++) {

		if (file_exists($vid_folder.'/mods/'.$mods[$i])) {
			$arrayut = array(
				'url' => $vid_folder.'/urls/'.$urls[$i],
				'mods' => $vid_folder.'/mods/'.$mods[$i],
				'data' => $vid_folder.'/data/'.$datas[$i],
			);
			if (file_exists($vid_folder.'/audio/'.$mp3s[$i])) $arrayut['mp3'] = $vid_folder.'/audio/'.$mp3s[$i];
			$videos_youtube[] = $arrayut;
		}
	}

	$operations = array();
	$i=-1;
	foreach ($videos_youtube as $youtube) {
		$i++;
		if ($i<=$curr_imports || $i>$curr_imports+$max_imports) continue;
		$operations[] = array('inah_import_youtube_createIslandoraObject'	, array($vid_folder,$youtube,$repo));
	}
	//DEBUG
	//file_put_contents('/home/santiago/114_RADIOINAH/testing.log', print_r(array('operations',$operations),true),FILE_APPEND);

	$batch['operations'] = $operations;
	batch_set($batch);
}

function inah_import_youtube_createIslandoraObject($vid_folder,$youtube,$repository,&$context){
 	
	$collectionid	= variable_get('inah_import_youtube_col', 'islandora:youtube_collection');
 	$context['message'] = "Creando objeto video youtube : <i>".$youtube['url']."</i> in ".$collectionid ;
 	inah_import_youtube_ingestFedoraObject($vid_folder,$youtube,$collectionid,$repository,$context);
 	
}

function inah_import_youtube_ingestFedoraObject($vid_folder, $youtube,$collectionUUID,$repository,&$context){
	global $user;

	module_load_include('inc', 'islandora_youtube', 'includes/youtube_upload.form');
	module_load_include('inc', 'islandora_youtube', 'includes/derivatives');

	$contentModel	= variable_get('inah_import_youtube_cmodel', 'islandora:sp-youtubeCModel');
	$namespace		= substr($contentModel, 0, strpos($contentModel, ":"));



	$object = $repository->constructObject($namespace); // allow fedora to generate a PID
	$object->models = $contentModel;
	$object->relationships->add(FEDORA_RELS_EXT_URI, 'isMemberOfCollection', $collectionUUID);
	//$object->relationships->add(FEDORA_MODEL_URI, 'hasModel', $contentModel);
	 
	$object->owner = $user->name;

	$xmlModsAct = simplexml_load_file($youtube['mods']);
	$object->label = $xmlModsAct->titleInfo->title;

	//DEBUG
	file_put_contents('/home/santiago/114_RADIOINAH/testing.log', print_r(array('Create object',$xmlModsAct->titleInfo->title),true),FILE_APPEND);

	//Read video urls
	$videos_txt = explode("\n", trim(str_replace("\r","",file_get_contents($youtube['url']))));
	$files_to_add = array();
	for ($i=0; $i < count($videos_txt) ; $i++) { 
		$files_to_add[$i] = unserialize(trim(file_get_contents($youtube['data'])));
	}

	$playlist_files = array();
	$object_url = url("islandora/object/{$object->id}", array('absolute' => TRUE));
	$insertion_point = 0;

	//Adds default IMG and TN if they dont exist
	islandora_youtube_add_datastream($object, 'IMG', $files_to_add[0]['img'], 'Medium size image');
	islandora_youtube_add_datastream($object, 'TN', $files_to_add[0]['tn'], 'Thumbnail');
	//Default URL ds
	islandora_youtube_add_datastream($object, 'URL', $files_to_add[0]['url'], 'URL of the video in youtube');
	if (!empty($youtube['mp3'])) islandora_youtube_add_datastream($object, 'PROXY_MP3', $youtube['mp3'], 'MP3 Audio file');

	//DEBUG
	file_put_contents('/home/santiago/114_RADIOINAH/testing.log', print_r(array('files_to_add',$files_to_add),true),FILE_APPEND);


	foreach ($files_to_add as $video) {
		$video['url'] = trim(file_get_contents($youtube['url']));
		$video['num'] = ++$insertion_point;
		$video['str'] = "VIDEO_".$video['num'];
		$playlist_files[$insertion_point]=$video;
		$playlist_files[$insertion_point]['tn']=$object_url.'/datastream/TN_'.$video['num'].'/view';

		//Adds VIDEO datastreams (IMG, TN, PLAYLIST)
		islandora_youtube_add_datastream($object, 'IMG_'.$video['num'], $video['img'], 'Medium size image');
		islandora_youtube_add_datastream($object, 'TN_'.$video['num'], $video['tn'], 'Thumbnail size image');
		islandora_youtube_add_datastream($object, 'URL_'.$video['num'], $video['url'], 'URL of the video in youtube', TRUE);
	}

	//Crea e ingesta el playlist DS, luego bora el archivo temporal
	$playlist = islandora_youtube_playlist_create_rss($playlist_files, $object->id);
	islandora_youtube_add_datastream($object, 'PLAYLIST', $playlist);
	islandora_youtube_add_datastream($object, 'OBJ', $youtube['url'], 'URL of video in Youtube');

	/**
	 * INGESTA EL OBJETO Video Youtube (sp-youtubeCModel)
	 */
	$mods = $youtube['mods'];

	if (file_exists($mods)) {
		$dcString  = inah_import_youtube_mods_to_dc($mods);

		$DC = $object->constructDatastream('DC','M');
		$DC->label    = 'Dublin Core';
		$DC->mimetype = 'text/xml';
		$DC->setContentFromString($dcString); 
		$object->ingestDatastream($DC);

		inah_import_youtube_content_update_datastream($object, $mods, 'MODS', 'MODS Metadata', NULL, 'M', FALSE);
	} else {
		drupal_set_message('No MODS file found for '.$youtube['mods'],'error');
	}

 	$repository->ingestObject($object);

    //Ingest video youtube object
	$context['message'] = "Ingestando objeto video youtube  : <i>".$youtube['url']."</i>";
	//$context['results']['ingestado_'.$youtube['num']] = 'Ingestado '.$youtube['num'];

	//Update current video youtube for later imports
	variable_set('inah_import_youtube_curr', variable_get('inah_import_youtube_curr', -1)+1);
}

/**
 * MODS to DC converter
 * @param  String $mods uri to MODS file
 * @return String $dcString DC XML String (not file)
 */
function inah_import_youtube_mods_to_dc($mods) {
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
function inah_import_youtube_content_update_datastream(AbstractObject $object, $file, $dsid, $label = NULL, $mimetype = NULL, $control_group = 'M', $copy = TRUE) {
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

function inah_import_youtube_importFinish($success, $results, $operations, &$context){
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
	
