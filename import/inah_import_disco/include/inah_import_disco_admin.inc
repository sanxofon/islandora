<?php
 
function inah_import_disco_admin($form,&$form_state){

 	$form['intro'] = array(
 		"#type"		=> 'markup',
 		'#markup'	=> '<div>Forma para importar discos</div>' 
 	);
 	
	$form['inah_import_disco_fieldset'] = array(
		'#type' => 'fieldset',
		'#title' => t('Disco batch import config'),
		'#description' => t('Settings for Disco Batch Import'),
		);

	$form['inah_import_disco_fieldset']['inah_import_disco_namespace'] = array(
		'#type' => 'textfield',
		'#title' => t('Namespace'),
		'#description' => t('Namespace for ingesting objects.'),
		'#default_value' => variable_get('inah_import_disco_namespace', 'islandora'),
		'#size' => 20,
		);

	$form['inah_import_disco_fieldset']['inah_import_disco_dir_audio'] = array(
		'#type' => 'textfield',
		'#title' => t('Audios folder uri'),
		'#description' => t('Directorio de subcarpetas de audios.'),
		'#default_value' => variable_get('inah_import_disco_dir_audio', '/home/santiago/002_FTMM/audio'),
		'#size' => 20,
		);

	$form['inah_import_disco_fieldset']['inah_import_disco_dir_audio_filter'] = array(
		'#type' => 'textfield',
		'#title' => t('Audios folder filter'),
		'#description' => t('Filtro para determinar carpetas de audi válidas.'),
		'#default_value' => variable_get('inah_import_disco_dir_audio_filter', 'Disco'),
		'#size' => 20,
		);

	$form['inah_import_disco_fieldset']['inah_import_disco_dir_pdf'] = array(
		'#type' => 'textfield',
		'#title' => t('PDFs folder uri'),
		'#description' => t('Directorio de PDFs.'),
		'#default_value' => variable_get('inah_import_disco_dir_pdf', '/home/santiago/002_FTMM/pdf'),
		'#size' => 20,
		);

	$form['inah_import_disco_fieldset']['inah_import_disco_dir_img'] = array(
		'#type' => 'textfield',
		'#title' => t('Image folder uri'),
		'#description' => t('Directorio de subcarpetas de imágenes de portada.'),
		'#default_value' => variable_get('inah_import_disco_dir_img', '/home/santiago/002_FTMM/jpg'),
		'#size' => 20,
		);

	$form['inah_import_disco_fieldset']['inah_import_disco_dir_mods'] = array(
		'#type' => 'textfield',
		'#title' => t('MODS folder uri'),
		'#description' => t('Directorio de archivos MODS de los discos y tracks.'),
		'#default_value' => variable_get('inah_import_disco_dir_mods', '/home/santiago/002_FTMM/mods'),
		'#size' => 20,
		);

	$form['inah_import_disco_fieldset']['inah_import_disco_col'] = array(
		'#type' => 'textfield',
		'#title' => t('Collection to ingest to.'),
		'#description' => t('Colección donde se agregarán los objectos disco.'),
		'#default_value' => variable_get('inah_import_disco_col', 'islandora:disco_collection'),
		'#size' => 20,
		);

	$form['inah_import_disco_fieldset']['inah_import_disco_max'] = array(
		'#type' => 'textfield',
		'#title' => t('Max imports'),
		'#description' => t('Máximo de discos a importar.'),
		'#element_validate' => array('element_validate_number'),
		'#default_value' => variable_get('inah_import_disco_max', 1),
		'#size' => 5,
		);

	$form['inah_import_disco_fieldset']['inah_import_disco_redo_max'] = array(
		'#type' => 'textfield',
		'#title' => t('Max imports REDO'),
		'#description' => t('Máximo de discos a importar (REDO).'),
		'#element_validate' => array('element_validate_number'),
		'#default_value' => variable_get('inah_import_disco_redo_max', 0),
		'#size' => 5,
		);

	$form['inah_import_disco_fieldset']['inah_import_disco_curr'] = array(
		'#type' => 'textfield',
		'#title' => t('Current offset'),
		'#description' => t('Current offset already imported. Starts at <i>0</i>'),
		'#element_validate' => array('element_validate_number'),
		'#default_value' => variable_get('inah_import_disco_curr', -1)+1,
		'#size' => 5,
		);

 	$form['inah_import_disco_do_import'] = array(
 		"#type"		=> 'submit',
 		'#value'	=> 'Importar los discos' 
 	);

    drupal_add_js(drupal_get_path('module','inah_import_disco') . "/include/inah_import_disco_redoform.js");
 	
 	return $form;
}

function inah_import_disco_admin_validate($form,&$form_state){
	
}

function inah_import_disco_connectToFedora(){
 	global $user;
 	$connection = islandora_get_tuque_connection($user);
 	$connection->reuseConnection = TRUE;
 	$repository = $connection->repository;
 	return $repository;
}

function inah_import_disco_listFolders($dir,$isfile=false,$filter='') {
    $ffs = @scandir($dir);
	//DEBUG
	file_put_contents('/home/santiago/002_FTMM/testing.log', print_r(array('dir',$dir),true),FILE_APPEND);
    $r = array();
    foreach ($ffs as $ff) {
        if ($ff != '.' && $ff != '..') {//AQUI
            if ( ($isfile || is_dir($dir.'/'.$ff)) && ($filter=='' || substr($ff,0,strlen($filter))==$filter) ) {
            	//Grabs first number(s) to appear in string
            	$x = preg_replace('/^[^0-9]*([0-9]+).+$/i', '$1', $ff);
            	$r[] = array('num'=>$x,'dir'=>$ff);
            }
        }
    }
    return $r;
}

function inah_import_disco_admin_submit(array $form, array &$form_state) {

	//DEBUG
	file_put_contents('/home/santiago/002_FTMM/testing.log', "-----------------------------------------------\nINICIANDO IMPORTACIÓN DESDE ".$form_state['values']['inah_import_disco_curr']." HASTA ".($form_state['values']['inah_import_disco_curr']+$form_state['values']['inah_import_disco_max'])."\n-----------------------------------------------\n",FILE_APPEND);

	$repo = inah_import_disco_connectToFedora();
	$batch = array(
			'init_message'		=> t('INICIANDO LA IMPORTACION'),
		'progress_message' 	=> t('Importando @current de un total de @total.'),
		'error_message' 	=> t('El proceso de importación a fallado.'),

			'title' => t('Importando discos de la discoteca'),
		'operations' => array(),
			'finished' => 'inah_import_disco_importFinish',
		'file' => drupal_get_path('module','inah_import_disco').'/include/inah_import_disco_admin.inc', //Need to be import from this file
	);

	variable_set('inah_import_disco_namespace', $form_state['values']['inah_import_disco_namespace']);

	variable_set('inah_import_disco_dir_audio', $form_state['values']['inah_import_disco_dir_audio']);
	variable_set('inah_import_disco_dir_audio_filter', $form_state['values']['inah_import_disco_dir_audio_filter']);
	variable_set('inah_import_disco_dir_pdf', $form_state['values']['inah_import_disco_dir_pdf']);
	variable_set('inah_import_disco_dir_img', $form_state['values']['inah_import_disco_dir_img']);
	variable_set('inah_import_disco_dir_mods', $form_state['values']['inah_import_disco_dir_mods']);

	variable_set('inah_import_disco_col', $form_state['values']['inah_import_disco_col']);


	$audio_folder = variable_get('inah_import_disco_dir_audio', '/home/santiago/002_FTMM/audio');
	$filter_audio_folder = variable_get('inah_import_disco_dir_audio_filter', 'Disco');

	$pdf_folder = variable_get('inah_import_disco_dir_pdf', '/home/santiago/002_FTMM/pdf');
	$img_folder = variable_get('inah_import_disco_dir_img', '/home/santiago/002_FTMM/jpg');
	$mods_folder = variable_get('inah_import_disco_dir_mods', '/home/santiago/002_FTMM/mods');

	$audirs = inah_import_disco_listFolders($audio_folder,false,$filter_audio_folder);
	$imgs = inah_import_disco_listFolders($img_folder,true);
	$pdfs = inah_import_disco_listFolders($pdf_folder,true);
	
	$allowed_extensions = array('wav', 'flac', 'mp3');
	$callback = function ($element) use ($allowed_extensions) {
		$ext = pathinfo($element, PATHINFO_EXTENSION);
		$ext = strtolower($ext);
		//$ext = drupal_strtolower($ext);

	// An allowed extension and does /not/ contain __MACOSX.
		return in_array($ext, $allowed_extensions) && preg_match('/__MACOSX/', $element) === 0;
	};

	$discos = array();
	for ($i=0;$i<count($audirs);$i++) {
		foreach ($imgs as $x) {
			if ($x['num']==$audirs[$i]['num']) {
				$audirs[$i]['img'] = $x['dir'];
			}
		}
		foreach ($pdfs as $x) {
			if ($x['num']==$audirs[$i]['num']) {
				$audirs[$i]['pdf'] = $x['dir'];
			}
		}

		$destination_dir = $audio_folder."/".$audirs[$i]['dir'];
		$unfiltered = array();
		$dobjects = new RecursiveIteratorIterator(new RecursiveDirectoryIterator($destination_dir), RecursiveIteratorIterator::SELF_FIRST);
		foreach ($dobjects as $dfile => $dobject) {
			$unfiltered[] = $dfile;
		}
		$files_to_add = array_values(array_filter($unfiltered, $callback));

		// Sort files based on name.
		$comparator = function ($a, $b) {
			$file_a = $a; //pathinfo($a, PATHINFO_FILENAME);
			$file_b = $b; //pathinfo($b, PATHINFO_FILENAME);
			return ($file_a < $file_b) ? -1 : 1;
		};
		usort($files_to_add, $comparator);
		$audirs[$i]['tracks'] = $files_to_add;

		$discos[] = $audirs[$i];
	}

	//Máximo de importaciones REDO (guarda el dato)
	variable_set('inah_import_disco_redo_max', $form_state['values']['inah_import_disco_redo_max']);

	//Máximo de importaciones
	$max_imports = $form_state['values']['inah_import_disco_max'];
	if ($max_imports<=0) $max_imports = 1;
	variable_set('inah_import_disco_max', $max_imports);

	//Número actual en memoria
	$curr_imports = $form_state['values']['inah_import_disco_curr']-1;
	if ($curr_imports>=count($discos)) $curr_imports = -1;
	variable_set('inah_import_disco_curr', $curr_imports);

	$operations = array();
	$i=-1;
	foreach ($discos as $disco) {
		$i++;
		if ($i<=$curr_imports || $i>$curr_imports+$max_imports) continue;
		$operations[] = array('inah_import_disco_createIslandoraObject'	, array($audio_folder,$img_folder,$pdf_folder,$mods_folder,$disco,$repo));
		$e = 0;
		foreach ($disco['tracks'] as $tracks) {
			$e++;
			$operations[] = array('inah_import_disco_ingestTracks'	, array($e,$audio_folder,$img_folder,$pdf_folder,$mods_folder,$disco,$repo));
		}
	}

	//DEBUG
	file_put_contents('/home/santiago/002_FTMM/testing.log', print_r(array('operations',count($operations)),true), FILE_APPEND);

	$batch['operations'] = $operations;
	batch_set($batch);

	//$form_state['rebuild'] = TRUE;
}

function inah_import_disco_createIslandoraObject($audio_folder,$img_folder,$pdf_folder,$mods_folder,$disco,$repository,&$context){
 	//$xmlModsAct = $context['results']["mods_$idInv"];
 	
	$collectionid	= variable_get('inah_import_disco_col', 'islandora:disco_collection');

 	$context['message'] = "Creando objeto disco : <i>".$disco['dir']."</i> " ;
 	inah_import_disco_ingestFedoraObject($audio_folder,$img_folder,$pdf_folder,$mods_folder,$disco,$collectionid,$repository,$context);
 	
}

function inah_import_disco_sanitize_input_for_valid_xml($input, $replacement = '') {
  $input = preg_replace('/[^\x9\xA\xD\x20-\x{D7FF}\x{E000}-\x{FFFD}\x{10000}-\x{10FFFF}]/u', $replacement, $input);
  return $input;
}

function inah_import_disco_ingestFedoraObject($audio_folder,$img_folder,$pdf_folder,$mods_folder, $disco,$collectionUUID,$repository,&$context){
	//DEBUG
	file_put_contents('/home/santiago/002_FTMM/testing.log', print_r(array('inah_import_disco_ingestFedoraObject',"Disco ".$disco['num']." ".$disco['dir']),true), FILE_APPEND);
	global $user;
	//Load needed modules
	//module_load_include('inc', 'islandora', 'includes/utilities');
	//module_load_include('inc', 'islandora_disco', 'includes/disco_upload.form');
	module_load_include('inc', 'islandora_disco', 'includes/derivatives');
	//module_load_include('inc', 'islandora_disco', 'includes/create_tracks');

	$contentModel	= 'islandora:sp-discoCModel';
	$namespace		= variable_get('inah_import_disco_namespace', 'islandora');

	$object = $repository->constructObject($namespace); // allow fedora to generate a PID
	$object->models = $contentModel;
	$object->relationships->add(FEDORA_RELS_EXT_URI, 'isMemberOfCollection', $collectionUUID);
	$object->relationships->add(FEDORA_MODEL_URI, 'hasModel', $contentModel);
	 
	$object->owner = $user->name;

	//OBJ Datastream (empty?)
	//$obj = $object->constructDatastream('OBJ', 'M');
	//$object->ingestDatastream($obj);

	//Adds OBJ datastream
	/*$obj->setContentFromFile(drupal_realpath($audios_zip->uri), FALSE);
	if ($obj->label != $audios_zip->filename) {
		$obj->label = $audios_zip->filename;
	}
	if ($obj->mimetype != $audios_zip->filemime) {
		$obj->mimetype = $audios_zip->filemime;
	}*/

	//Add MODS and DC ds
	$mods = $mods_folder."/Disco_".$disco['num'].".MODS.xml";
	if (!file_exists($mods)) {
		$mods='';
		drupal_set_message("No se encontró el archivo MODS correspondiente al Disco ".$disco['num'],"warning");
		die("No se encontró el archivo MODS correspondiente al Disco ".$disco['num']);
	} else {
		$xsltPaht = drupal_get_path('module','islandora_importer')."/xsl/mods_to_dc.xsl";
		$xsl = new DOMDocument();
		$xsl->load($xsltPaht);

		$xml = new SimpleXMLElement($mods, NULL, TRUE);

		$processor = new XSLTProcessor();
		$processor->importStylesheet($xsl);
		$dcString  = $processor->transformToXML($xml);

		$DC = $object->constructDatastream('DC','M');
		$DC->label    = 'Dublin Core';
		$DC->mimetype = 'text/xml';
		$DC->setContentFromString($dcString); 
		$object->ingestDatastream($DC);


		islandora_disco_add_datastream($object, 'MODS', $mods, 'MODS Metadata');
	}
	if (!empty($xml->title)) {
		$object->label = $xml->title;
	} else {
		$object->label = $disco['dir'];
	}

	//Get TN from parent disco object to child track objects
	$width = variable_get('islandora_disco_thumbnail_width', 200);
	$height = variable_get('islandora_disco_thumbnail_height', 200);
	
	//IMG ds
	$mime_detector = new MimeDetect();
	$file = $img_folder."/".$disco['img'];
    $mimetype = $mime_detector->getMimetype($file);
	$ds = $object->constructDatastream('IMG', 'M');
    $ds->mimetype = $mimetype;
    $ds->label = 'Image';
    $ds->setContentFromFile($file);
    $object->ingestDatastream($ds);
	//DEBUG
	file_put_contents('/home/santiago/002_FTMM/testing.log', print_r(array('inah_import_disco_ingestFedoraObject',"file->IMG",$file),true), FILE_APPEND);

	//TN ds
	$mime_detector = new MimeDetect();
	$ttn_uri = islandora_disco_create_jpg_derivative($file, 'TN', $width, $height);
	$tn_uri = drupal_realpath($ttn_uri);
    $mimetype = $mime_detector->getMimetype($tn_uri);
	$ds = $object->constructDatastream('TN', 'M');
    $ds->mimetype = $mimetype;
    $ds->label = 'Thumbnail';
    $ds->setContentFromFile($tn_uri);
    $object->ingestDatastream($ds);

	// Extract file vars.
	$destination_dir = $audio_folder."/".$disco['dir'];

	$allowed_extensions = array('wav', 'flac', 'mp3');
	$callback = function ($element) use ($allowed_extensions) {
		$ext = pathinfo($element, PATHINFO_EXTENSION);
		$ext = drupal_strtolower($ext);

	// An allowed extension and does /not/ contain __MACOSX.
		return in_array($ext, $allowed_extensions) && preg_match('/__MACOSX/', $element) === 0;
	};
	$unfiltered = array();
	$dobjects = new RecursiveIteratorIterator(new RecursiveDirectoryIterator($destination_dir), RecursiveIteratorIterator::SELF_FIRST);
	foreach ($dobjects as $dfile => $dobject) {
		$unfiltered[] = $dfile;
	}
	$files_to_add = array_values(array_filter($unfiltered, $callback));

	// Sort files based on name.
	$comparator = function ($a, $b) {
		$file_a = preg_replace('/^.*((?:\/(?:Disco|tomo)[ -]?[0-9]+)?\/[^\/]+)$/i','\1',$a);//pathinfo($a, PATHINFO_FILENAME);
		$file_b = preg_replace('/^.*((?:\/(?:Disco|tomo)[ -]?[0-9]+)?\/[^\/]+)$/i','\1',$b);//pathinfo($b, PATHINFO_FILENAME);
		return ($file_a < $file_b) ? -1 : 1;
	};
	usort($files_to_add, $comparator);

	$playlist_files = array();
	$object_url = url("islandora/object/{$object->id}", array('absolute' => TRUE));
	$insertion_point = 0;

	//Array of track objects for ingesting
	$tracks = array();

	foreach ($files_to_add as $audio) {

		$config['track_number'] = ++$insertion_point;
		$config['tn'] = $tn_uri;
		$config['track_name'] = pathinfo(basename($audio), PATHINFO_FILENAME);
		$config['track_str'] = '';

		$tomo = preg_replace('/^.*(?:Disco|tomo) ([0-9])$/i', '\1', substr($audio,0,-(strlen(basename($audio))+1)));
		if (abs($tomo)>0) {
			$tomostr = abs($tomo).'.';
			$tomon = abs($tomo).'-';
			$tomo = '_'.abs($tomo);
		} else {
			$tomostr = '';
			$tomon = '';
			$tomo='';
		}
		$mods = $mods_folder."/Disco_".$disco['num'].$tomo."_".basename($audio).".MODS.xml";

		if (file_exists($mods)) {
			$xmlmods = new SimpleXMLElement($mods, NULL, TRUE);
			if (!empty($xmlmods->title)) {
				$config['track_name'] = (string)$xmlmods->title;
			}
			$track_str_temp = array();
			foreach ($xmlmods->note as $esto) {
				$track_str_temp[] = (string)$esto->namePart;
			}
			$track_str_temp = trim(implode('; ',$track_str_temp));
			if (!empty($track_str_temp) && $track_str_temp!=';') {
				$config['track_str'] = $track_str_temp;
			}
		}

		//Adds TRACK as datastream
		/*islandora_disco_add_datastream($object, $config['track_str'], $audio, $config['track_name']);
		$track_url = $object_url.'/datastream/'.$config['track_str'].'/file_name_spoof.mp3';*/

		//Programs creation TRACK child object as sp-audioCModel
		$namespace	= variable_get('inah_import_disco_namespace', 'islandora');
      	$track 		= $repository->constructObject($namespace);
      	$track->owner = $user->name;
		$trackz 	= array($track,$object->id,$audio, $config);

		$tracks[] = $trackz;

		$track_url = url("islandora/object/".$track->id, array('absolute' => TRUE)).'/datastream/PROXY_MP3/'.preg_replace('/\.[a-z]{3,4}$/i','.mp3',basename($audio));

		$playlist_files[] = array(
			'track_url' => $track_url,
			'track_number' => $config['track_number'],
			'track_label' => $tomostr.$config['track_name'],
			'track_desc' => $config['track_str'],
			'thumbnail' => url("islandora/object/".$track->id, array('absolute' => TRUE)).'/datastream/TN/thumbnail.jpg',
			);
	}

	//PLAYLIST DS
	$playlist = islandora_disco_playlist_create_rss($playlist_files, $object->id);
	islandora_disco_add_datastream($object, 'PLAYLIST', $playlist, $disco['dir']);
	file_unmanaged_delete($playlist);

	//DEBUG
	file_put_contents('/home/santiago/002_FTMM/testing.log', print_r(array('islandora_disco_playlist',$playlist),true), FILE_APPEND);

	//PDF ds
	$mime_detector = new MimeDetect();
	$file = $pdf_folder."/".$disco['pdf'];
    $mimetype = $mime_detector->getMimetype($file);
	$ds = $object->constructDatastream('PDF', 'M');
    $ds->mimetype = $mimetype;
    $ds->label = 'PDF File';
    $ds->setContentFromFile($file);
    $object->ingestDatastream($ds);
	//DEBUG
	file_put_contents('/home/santiago/002_FTMM/testing.log', print_r(array('islandora_disco_pdf',$file),true), FILE_APPEND);

	//FULL_TEXT ds
	$extract_full_text_from_pdf = variable_get('islandora_disco_create_fulltext', FALSE);
	if ($extract_full_text_from_pdf) {
		$mime_detector = new MimeDetect();
	    $executable = variable_get('islandora_pdftotext_url', '/usr/bin/pdftotext');
	    $temp = uniqid();
	    $temp = file_create_filename($temp.'.fulltext.txt', 'temporary://');
	    $derivative_file_uri = drupal_realpath($temp);
	    $command = "$executable \"$file\" \"$derivative_file_uri\"";

		//DEBUG
		file_put_contents('/home/santiago/002_FTMM/testing.log', print_r(array('extract_full_text_from_pdf',$command),true), FILE_APPEND);

	    exec($command, $execout, $returncode);

	    //Sanitize fulltext
		$full_text = file_get_contents($derivative_file_uri);
	    $sanitized_full_text = inah_import_disco_sanitize_input_for_valid_xml($full_text);
	    file_put_contents($derivative_file_uri, $sanitized_full_text);

	    $mimetype = $mime_detector->getMimetype($derivative_file_uri);
		$ds = $object->constructDatastream('FULL_TEXT', 'M');
	    $ds->mimetype = $mimetype;
	    $ds->label = 'Full Text';
	    $ds->setContentFromFile($derivative_file_uri);
	    $object->ingestDatastream($ds);
	    file_unmanaged_delete($derivative_file_uri);
	}

    //Ingest disco object
	$context['message'] = "Ingestando objeto disco  : <i>".$disco['dir']."</i> " ;
 	
	$repository->ingestObject($object);

	//DEBUG
	file_put_contents('/home/santiago/002_FTMM/ingest.log', $object->id.'|'.$disco['dir']."\n",FILE_APPEND);

	//DEBUG
	file_put_contents('/home/santiago/002_FTMM/testing.log', print_r(array('Order: disco',$disco),true), FILE_APPEND);

	$i=0;
	foreach ($tracks as $track) {
		$i++;
		$context['results']["track_{$disco['num']}_$i"] = serialize($track);
	}

	//Update current disco for later imports
	variable_set('inah_import_disco_curr', variable_get('inah_import_disco_curr', -1)+1);
}

function inah_import_disco_ingestTracks($i,$audio_folder,$img_folder,$pdf_folder,$mods_folder,$disco,$repository,&$context){

	
 	module_load_include('inc', 'islandora_disco', 'includes/create_tracks');

	$temp_array = unserialize($context['results']["track_{$disco['num']}_$i"]);
	$track = $temp_array[0];
	$object_id = $temp_array[1];
	$audio = $temp_array[2];
	$config = $temp_array[3];
	//DEBUG
	file_put_contents('/home/santiago/002_FTMM/testing.log', print_r(array('inah_import_disco_ingestTracks',"Disco ".$disco['num']." ".$disco['dir'],$i." - ".$audio),true), FILE_APPEND);


	$tomo = preg_replace('/^.*(?:Disco|tomo) ([0-9])$/i', '\1', substr($audio,0,-(strlen(basename($audio))+1)));
	if (abs($tomo)>0) $tomo = '_'.abs($tomo);
	else $tomo='';
	$mods = $mods_folder."/Disco_".$disco['num'].$tomo."_".basename($audio).".MODS.xml";
	if (!file_exists($mods)) {
		drupal_set_message("No se encontró el archivo MODS: ".$mods,"warning");
		$mods='';
	}

 	$track = islandora_disco_ingest_track($track, $object_id, $repository, $audio, $config, $mods);
 	
 	$context['message'] = "Ingestando objeto track #<i>".$i."</i>: ".$audio."</i> " ;
 	$repository->ingestObject($track);

	//DEBUG
	file_put_contents('/home/santiago/002_FTMM/ingest.log', $track->id.'|'.$audio."\n",FILE_APPEND);
 	
}

/**
 * MODS to DC converter
 * @param  String $mods uri to MODS file
 * @return String $dcString DC XML String (not file)
 */
function inah_import_disco_mods_to_dc($mods) {
	$xsltPaht = drupal_get_path('module','islandora_importer')."/xsl/mods_to_dc.xsl";
    $xsl = new DOMDocument();
    $xsl->load($xsltPaht);

    $xml = new SimpleXMLElement($mods, NULL, TRUE);

    $processor = new XSLTProcessor();
    $processor->importStylesheet($xsl);
    $dcString  = $processor->transformToXML($xml);

    return $dcString;
}

function inah_import_disco_importFinish($success, $results, $operations, &$context){
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