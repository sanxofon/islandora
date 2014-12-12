<?php
 
function inah_import_manuscrito_admin($form,&$form_state){
 	$form['intro'] = array(
 		"#type"		=> 'markup',
 		'#markup'	=> '<div>Forma para importar manuscritos</div>' 
 	);
 	
	$form['inah_import_manuscrito_fieldset'] = array(
		'#type' => 'fieldset',
		'#title' => t('Manuscrito batch import config'),
		'#description' => t('Settings for Manuscrito Batch Import'),
		);

	$form['inah_import_manuscrito_fieldset']['inah_import_manuscrito_dir_img'] = array(
		'#type' => 'textfield',
		'#title' => t('Image folder uri'),
		'#description' => t('Directorio de subcarpetas de imágenes.'),
		'#default_value' => variable_get('inah_import_manuscrito_dir_img', '/opt/Acervos/001_Archivo histórico Casa de Morelos'),
		'#size' => 20,
		);

	$form['inah_import_manuscrito_fieldset']['inah_import_manuscrito_dir_mods'] = array(
		'#type' => 'textfield',
		'#title' => t('MODS folder uri'),
		'#description' => t('Directorio de subcarpetas de MODS.'),
		'#default_value' => variable_get('inah_import_manuscrito_dir_mods', '/home/santiago/001_AHCM/mods'),
		'#size' => 20,
		);

	$form['inah_import_manuscrito_fieldset']['inah_import_manuscrito_col'] = array(
		'#type' => 'textfield',
		'#title' => t('Collection to ingest to.'),
		'#description' => t('Colección a donde se ingestarán los manuscritos.'),
		'#default_value' => variable_get('inah_import_manuscrito_col', 'islandora:bookCollection'),
		'#size' => 20,
		);

	$form['inah_import_manuscrito_fieldset']['inah_import_manuscrito_cmodel'] = array(
		'#type' => 'textfield',
		'#title' => t('CModel of parent objects.'),
		'#description' => t('CModel en que se ingestarán los manuscritos.'),
		'#default_value' => variable_get('inah_import_manuscrito_cmodel', 'islandora:bookCModel'),
		'#size' => 20,
		);

	$form['inah_import_manuscrito_fieldset']['inah_import_manuscrito_pcmodel'] = array(
		'#type' => 'textfield',
		'#title' => t('CModel of child (page) objects.'),
		'#description' => t('CModel en que se ingestarán las páginas de los manuscritos.'),
		'#default_value' => variable_get('inah_import_manuscrito_pcmodel', 'islandora:pageCModel'),
		'#size' => 20,
		);

	$form['inah_import_manuscrito_fieldset']['inah_import_manuscrito_language'] = array(
		'#type' => 'textfield',
		'#title' => t('CModel of child (page) objects.'),
		'#description' => t('CModel en que se ingestarán las páginas de los manuscritos.'),
		'#default_value' => variable_get('inah_import_manuscrito_language', 'spa_old'),
		'#size' => 20,
		);

	$form['inah_import_manuscrito_fieldset']['inah_import_manuscrito_max'] = array(
		'#type' => 'textfield',
		'#title' => t('Max imports'),
		'#description' => t('Máximo de manuscritos a importar.'),
		'#element_validate' => array('element_validate_number'),
		'#default_value' => variable_get('inah_import_manuscrito_max', 1),
		'#size' => 5,
		);

	$form['inah_import_manuscrito_fieldset']['inah_import_manuscrito_redo_max'] = array(
		'#type' => 'textfield',
		'#title' => t('Max imports REDO'),
		'#description' => t('Máximo de carpetas a importar (REDO).'),
		'#element_validate' => array('element_validate_number'),
		'#default_value' => variable_get('inah_import_manuscrito_redo_max', 0),
		'#size' => 5,
		);

	$form['inah_import_manuscrito_fieldset']['inah_import_manuscrito_curr'] = array(
		'#type' => 'textfield',
		'#title' => t('Current offset'),
		'#description' => t('Current offset already imported. Starts at <i>0</i>'),
		'#element_validate' => array('element_validate_number'),
		'#default_value' => variable_get('inah_import_manuscrito_curr', -1)+1,
		'#size' => 5,
		);

 	$form['inah_import_manuscrito_do_import'] = array(
 		"#type"		=> 'submit',
 		'#value'	=> 'Importar los manuscritos' 
 	);

    drupal_add_js(drupal_get_path('module','inah_import_manuscrito') . "/include/inah_import_manuscrito_redoform.js");
 	
 	return $form;
}

function inah_import_manuscrito_admin_validate($form,&$form_state){
	
}

function inah_import_manuscrito_connectToFedora(){
 	global $user;
 	$connection = islandora_get_tuque_connection($user);
 	$connection->reuseConnection = TRUE;
 	$repository = $connection->repository;
 	return $repository;
}

function inah_import_manuscrito_listFolders($dir,$isfile=false) {
    $ffs = scandir($dir);
    $r = array();
    foreach ($ffs as $ff) {
        if ($ff != '.' && $ff != '..') {
            if ( ($isfile || is_dir($dir.'/'.$ff)) ) {
            	//Grabs first number(s) to appear in string
            	$x = preg_replace('/^[^0-9]*([0-9]+).*$/i', '$1', $ff);
            	$r[] = array('num'=>$x,'dir'=>$ff);
            }
        }
    }
    return $r;
}

function inah_import_manuscrito_admin_submit(array $form, array &$form_state) {

	//DEBUG
	//file_put_contents('/home/ljorda/arquelogiczone.log', "-----------------------------------------------\nINICIANDO IMPORTACIÓN DESDE ".$form_state['values']['inah_import_manuscrito_curr']." HASTA ".($form_state['values']['inah_import_manuscrito_curr']+$form_state['values']['inah_import_manuscrito_max'])."\n-----------------------------------------------\n",FILE_APPEND);

	$repo = inah_import_manuscrito_connectToFedora();
	$batch = array(
			'init_message'		=> t('INICIANDO LA IMPORTACION'),
		'progress_message' 	=> t('Importando @current de un total de @total.'),
		'error_message' 	=> t('El proceso de importación a fallado.'),

			'title' => t('Importando manuscritos'),
		'operations' => array(),
			'finished' => 'inah_import_manuscrito_importFinish',
		'file' => drupal_get_path('module','inah_import_manuscrito').'/include/inah_import_manuscrito_admin.inc', //Need to be import from this file
	);

	variable_set('inah_import_manuscrito_dir_img', $form_state['values']['inah_import_manuscrito_dir_img']);
	variable_set('inah_import_manuscrito_dir_mods', $form_state['values']['inah_import_manuscrito_dir_mods']);
	variable_set('inah_import_manuscrito_cmodel', $form_state['values']['inah_import_manuscrito_cmodel']);
	variable_set('inah_import_manuscrito_pcmodel', $form_state['values']['inah_import_manuscrito_pcmodel']);
	variable_set('inah_import_manuscrito_language', $form_state['values']['inah_import_manuscrito_language']);
	variable_set('inah_import_manuscrito_col', $form_state['values']['inah_import_manuscrito_col']);

	//Máximo de importaciones REDO (guarda el dato)
	variable_set('inah_import_manuscrito_redo_max', $form_state['values']['inah_import_manuscrito_redo_max']);

	//WINDOWS//$img_folder = utf8_decode(variable_get('inah_import_manuscrito_dir_img', '/opt/Acervos/001_Archivo histórico Casa de Morelos'));
	$img_folder = variable_get('inah_import_manuscrito_dir_img', '/home/ljorda/zonas_arqueologicas/imagenes_a_ingestar');
	//WINDOWS//$mods_folder = utf8_decode(variable_get('inah_import_manuscrito_dir_mods', '/home/santiago/001_AHCM/mods'));
	$mods_folder = variable_get('inah_import_manuscrito_dir_mods', '/home/ljorda/zonas_arqueologicas/mods_zonas_arqueologicas');

	$imgdirs = inah_import_manuscrito_listFolders($img_folder,false);

	$allowed_extensions = array('jpg', 'jpeg', 'tif', 'tiff');
	$callback = function ($element) use ($allowed_extensions) {
		$ext = pathinfo($element, PATHINFO_EXTENSION);
		$ext = strtolower($ext);
		//$ext = drupal_strtolower($ext);

	// An allowed extension and does /not/ contain __MACOSX.
		return in_array($ext, $allowed_extensions) && preg_match('/__MACOSX/', $element) === 0;
	};

	$manuscritos = array();
	for ($i=0;$i<count($imgdirs);$i++) {

		$destination_dir = $img_folder.DIRECTORY_SEPARATOR.$imgdirs[$i]['dir'];
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
		$imgdirs[$i]['pages'] = $files_to_add;
		$fondo_caja = preg_replace('/^.*\/([^\/]+)\/[^\/0-9]*([0-9]+)[^\/0-9]*$/i','\1_\2_',$img_folder);
		//$mods_file = $fondo_caja.DIRECTORY_SEPARATOR.$imgdirs[$i]['dir'].".MODS.xml";
		$mods_file = $imgdirs[$i]['dir'].".MODS.xml";
		error_log('Mods a importar: '.$mods_file);
		if (file_exists($mods_folder.DIRECTORY_SEPARATOR.$mods_file)) {
			$imgdirs[$i]['mods'] = $mods_file;
		}

		$manuscritos[] = $imgdirs[$i];
	}

	//Máximo de importaciones
	$max_imports = $form_state['values']['inah_import_manuscrito_max'];
	if ($max_imports<=0) $max_imports = 1;
	variable_set('inah_import_manuscrito_max', $max_imports);

	//Número actual en memoria
	$curr_imports = $form_state['values']['inah_import_manuscrito_curr']-1;
	if ($curr_imports>=count($manuscritos)) $curr_imports = -1;
	variable_set('inah_import_manuscrito_curr', $curr_imports);

	$operations = array();
	$i=-1;
	$ii=0;
	foreach ($manuscritos as $manuscrito) {
		$i++;
		if ($i<=$curr_imports || $i>$curr_imports+$max_imports) continue;
		$operations[] = array('inah_import_manuscrito_createIslandoraObject'	, array($img_folder,$manuscrito,$repo));
		$e = 0;
		foreach ($manuscrito['pages'] as $pages) {
			$e++;
			$operations[] = array('inah_import_manuscrito_ingestPages'	, array($e,$img_folder,$manuscrito,$repo));
		}

		$ii++;
		//Create PDF's of books
		//$operations[] = array('inah_import_manuscrito_aggregate_pdf_derivative'	, array($ii,$repo));

	}

	$batch['operations'] = $operations;
	batch_set($batch);

	//$form_state['rebuild'] = TRUE;
}

function inah_import_manuscrito_createIslandoraObject($img_folder,$manuscrito,$repository,&$context){
 	
	$collectionid	= variable_get('inah_import_manuscrito_col', 'inah:arqueologiczone');
 	$context['message'] = "Creando objeto manuscrito : <i>".$manuscrito['dir']."</i> " ;
 	//WINDOWS//inah_import_manuscrito_ingestFedoraObject(utf8_encode($img_folder),$manuscrito,$collectionid,$repository,$context);
 	inah_import_manuscrito_ingestFedoraObject($img_folder,$manuscrito,$collectionid,$repository,$context);
 	
}

function inah_import_manuscrito_ingestFedoraObject($img_folder, $manuscrito,$collectionUUID,$repository,&$context){
	global $user;

	$contentModel	= variable_get('inah_import_manuscrito_cmodel', 'islandora:codexbookCModel');
	$namespace		= substr($contentModel, 0, strpos($contentModel, ":"));

	$object = $repository->constructObject('inaharqueologicas'); // allow fedora to generate a PID $namespace
	$object->models = $contentModel;
	$object->relationships->add(FEDORA_RELS_EXT_URI, 'isMemberOfCollection', $collectionUUID);
	//$object->relationships->add(FEDORA_MODEL_URI, 'hasModel', $contentModel);
	$object->owner = $user->name;
	 
	//WINDOWS//$mods_folder = utf8_decode(variable_get('inah_import_manuscrito_dir_mods', '/home/santiago/001_AHCM/mods'));
	$mods_folder = variable_get('inah_import_manuscrito_dir_mods', '/home/ljorda/zonas_arqueologicas/mods_zonas_arqueologicas');

	if (!empty($manuscrito['mods'])) {
		$xmlModsAct = simplexml_load_file($mods_folder.DIRECTORY_SEPARATOR.$manuscrito['mods']);
		//WINDOWS//$xmlModsAct = simplexml_load_file($img_folder."/".$manuscrito['mods']);
		if ($xmlModsAct->titleInfo->title!='') {
			//WINDOWS//$object->label = utf8_encode($xmlModsAct->titleInfo->title);
			$object->label = $xmlModsAct->titleInfo->title;
		} else {
			//WINDOWS//$object->label = utf8_encode($manuscrito['dir']);
			$object->label = $manuscrito['dir'];
		}
	} else {
		//WINDOWS//$object->label = utf8_encode($manuscrito['dir']);
		$object->label = $manuscrito['dir'];
	}


	// Extract file vars.
	//WINDOWS//$destination_dir = utf8_decode($img_folder)."/".$manuscrito['dir'];
	$destination_dir = $img_folder.DIRECTORY_SEPARATOR.$manuscrito['dir'];

	$allowed_extensions = array('jpg', 'jpeg', 'tif', 'tiff');
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
		$file_a = pathinfo($a, PATHINFO_FILENAME);
		$file_b = pathinfo($b, PATHINFO_FILENAME);
		return ($file_a < $file_b) ? -1 : 1;
	};
	usort($files_to_add, $comparator);

	$object_url = url("islandora/object/{$object->id}", array('absolute' => TRUE));
	$insertion_point = 0;

	//Array of page objects for ingesting
	$pages = array();

	foreach ($files_to_add as $imagen) {
		$config['page_number'] = ++$insertion_point;
		$config['page_name'] = pathinfo(basename($imagen), PATHINFO_FILENAME);
		$config['page_str'] = "Page_".$config['page_number'];

		//Programs creation PAGE child object as pageCModel
		$contentModelpage	= variable_get('inah_import_manuscrito_pcmodel', 'islandora:pageCModel');
		$languagepage	= variable_get('inah_import_manuscrito_language', 'spa_old');

      	$page = $repository->constructObject('inaharqueologicas');
      	$page->owner = $user->name;
		//WINDOWS//$page->label = utf8_encode($config['page_name']);
		$page->label = $config['page_name'];

      	//Adds relationships of pages
		$page->models = $contentModelpage;
		//$page->relationships->add(FEDORA_MODEL_URI, 'hasModel', $contentModelpage);
		$page->relationships->add(ISLANDORA_RELS_EXT_URI, 'hasLanguage', $languagepage, TRUE);
		$page->relationships->add(ISLANDORA_RELS_EXT_URI, 'isPageOf', $object->id);
		$page->relationships->add(ISLANDORA_RELS_EXT_URI, 'isSequenceNumber', (string) $config['page_number'], TRUE);
		$page->relationships->add(ISLANDORA_RELS_EXT_URI, 'isPageNumber', (string) $config['page_number'], TRUE);
		$page->relationships->add(ISLANDORA_RELS_EXT_URI, 'isSection', (string) $config['page_number'], TRUE);
		$page->relationships->add(FEDORA_RELS_EXT_URI, 'isMemberOf', $object->id);

		$pagez = array($page,$object->id,$imagen, $config);

		$pages[] = $pagez;
	}

	/**
	 * GUARDA EN CONTEXT LOS OBJETOS PAGE DE MANUSCRITO ACTUAL (pageCModel)
	 */

	$i=0;
	foreach ($pages as $page) {
		$i++;
		$context['results']["page_{$manuscrito['num']}_$i"] = serialize($page);
	}

	//DEBUG
	//file_put_contents('/home/ljorda/arqueologiczone.log', print_r(array('Manuscrito',$manuscrito),true),FILE_APPEND);

	/**
	 * INGESTA EL OBJETO MANUSCRITO (bookCModel)
	 */
	$mods = $manuscrito['mods'];
	if ($mods!='') {
		$mods = $mods_folder."/".$mods;

		if (file_exists($mods)) {
			$dcString  = inah_import_manuscrito_mods_to_dc($mods);

			$DC = $object->constructDatastream('DC','M');
			$DC->label    = 'Dublin Core';
			$DC->mimetype = 'text/xml';
			$DC->setContentFromString($dcString); 
			$object->ingestDatastream($DC);

			inah_import_manuscrito_content_update_datastream($object, $mods, 'MODS', 'MODS Metadata', NULL, 'M', FALSE);
		} else {
			//DEBUG
			//file_put_contents('/home/ljorda/arqueologiczone.log', print_r(array('NO MODS FOUND!',$mods),true),FILE_APPEND);
			drupal_set_message('No MODS file found for '.$manuscrito['dir'],'error');
		}
	} else {
		drupal_set_message('No MODS file for '.$manuscrito['dir'],'error');
	}
 	$repository->ingestObject($object);


	//DEBUG
	//file_put_contents('/home/ljorda/arqueologiczone.log', $object->id.'|'.$img_folder.'/'.$manuscrito['dir']."\n",FILE_APPEND);
	if (!isset($context['results']['total_books'])) $context['results']['total_books'] = 0;
 	$context['results']['total_books'] = $context['results']['total_books']+1;
 	$context['results']['ingested_book_'.$context['results']['total_books']] = serialize($object);

    //Ingest manuscrito object
	$context['message'] = "Ingestando objeto manuscrito  : <i>".$manuscrito['dir']."</i> " ;

	//Update current manuscrito for later imports
	variable_set('inah_import_manuscrito_curr', variable_get('inah_import_manuscrito_curr', -1)+1);
}

function inah_import_manuscrito_ingestPages($i,$img_folder,$manuscrito,$repository,&$context){

	$temp_array = unserialize($context['results']["page_{$manuscrito['num']}_$i"]);
	$page = $temp_array[0];
	$object_id = $temp_array[1];
	$image = $temp_array[2];
	$config = $temp_array[3];

 	//Adds DS's to page object
 	
 	//Adds OBJ datastream with original image file of the page
    $mime_detector = new MimeDetect();
 	$obj = $page->constructDatastream('OBJ', 'M');
 	$obj->mimetype = $mime_detector->getMimetype($image);
 	$obj->label = $config['page_name'];
	$obj->setContentFromFile($image, FALSE);
    $page->ingestDatastream($obj);

    //Adds large_image default datastreams
	module_load_include('inc', 'islandora_large_image', 'includes/derivatives');
	if (islandora_large_image_create_all_derivatives($page)) {
	 	$repository->ingestObject($page);
		//DEBUG
		//file_put_contents('/home/ljorda/arqueologiczone.log', $page->id.'|'.$image."\n",FILE_APPEND);
	}

	$context['message'] = "Ingestando objeto page <i>".$i."</i> del manuscrito <i>".$manuscrito['dir']."</i> " ;

 	
}

/**
 * MODS to DC converter
 * @param  String $mods uri to MODS file
 * @return String $dcString DC XML String (not file)
 */
function inah_import_manuscrito_mods_to_dc($mods) {
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
function inah_import_manuscrito_content_update_datastream(AbstractObject $object, $file, $dsid, $label = NULL, $mimetype = NULL, $control_group = 'M', $copy = TRUE) {
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

function inah_import_manuscrito_importFinish($success, $results, $operations, &$context){
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


function inah_import_manuscrito_get_pages(AbstractObject $object) {
  $query = <<<EOQ
PREFIX islandora-rels-ext: <http://islandora.ca/ontology/relsext#>
SELECT ?pid ?page ?label ?width ?height
FROM <#ri>
WHERE {
  ?pid <fedora-rels-ext:isMemberOf> <info:fedora/{$object->id}> ;
       <fedora-model:label> ?label ;
       islandora-rels-ext:isSequenceNumber ?page ;
       <fedora-model:state> <fedora-model:Active> .
  OPTIONAL {
    ?pid <fedora-view:disseminates> ?dss .
    ?dss <fedora-view:disseminationType> <info:fedora/*/JP2> ;
         islandora-rels-ext:width ?width ;
         islandora-rels-ext:height ?height .
 }
}
ORDER BY ?page
EOQ;

  $results = $object->repository->ri->sparqlQuery($query);

  // Get rid of the "extra" info...
  $map = function($o) {
    foreach ($o as $key => &$info) {
      $info = $info['value'];
    }

    $o = array_filter($o);

    return $o;
  };
  $pages = array_map($map, $results);

  // Grab the PIDs...
  $get_pid = function($o) {
    return $o['pid'];
  };
  $pids = array_map($get_pid, $pages);

  // If we have some pages, combine our remapped results to produce an array
  // mapping pids to the values for that pid.
  $pages = count($pids) ? array_combine($pids, $pages) : array();

  // Sort the pages into their proper order.
  $sort = function($a, $b) {
    $a = (is_array($a) && isset($a['page'])) ? $a['page'] : 0;
    $b = (is_array($b) && isset($b['page'])) ? $b['page'] : 0;
    if ($a == $b) {
      return 0;
    }
    return ($a < $b) ? -1 : 1;
  };
  uasort($pages, $sort);

  return $pages;
}

function inah_import_manuscrito_aggregate_pdf_derivative($i, $repo, &$context) {
	
 	$object = unserialize($context['results']['ingested_book_'.$i]);
 	$pages = array_keys(inah_import_manuscrito_get_pages($object));
 	$pdf_files = array();
 	foreach($pages as $pagid) {
 		$page = islandora_object_load($pagid);
 		$pdf = $page['PDF'];
		$base_name = "book_".$i."_".$pagid.".pdf";
		$base_name = str_replace(':', '-', $base_name);
		$source_file = file_create_filename($base_name, 'temporary://'); 
		
		drupal_set_message( $source_file .' - '.  file_exists(drupal_realpath($source_file)) . ' '.drupal_realpath($source_file));

		$pdf->getContent($source_file);

		$pdf_files[] = drupal_realpath($source_file);
	}
	//DEBUG
	//file_put_contents('/home/ljorda/arqueologiczone.log', print_r(array('pdf_files',$pdf_files),true),FILE_APPEND);

	$pdf_out = file_create_filename("book_out_".$i.".pdf", 'temporary://');
	drupal_set_message( "book_out_".$i.".pdf" .' - '.  file_exists(drupal_realpath($source_file)) .' '.drupal_realpath($source_file));
	$pdf_out = drupal_realpath($pdf_out);
	if (inah_import_manuscrito_pdf_combine($pdf_files,$pdf_out)) {
		inah_import_manuscrito_content_update_datastream($object, $pdf_out, 'PDF', 'PDF file', NULL, 'M', FALSE);
	}

	//DEBUG
	//file_put_contents('/home/ljorda/arqueologiczone.log', print_r(array('pdf_out',$pdf_out),true),FILE_APPEND);

}

function inah_import_manuscrito_pdf_combine(array $files, $out) {
  $gs = variable_get('islandora_paged_content_gs', '/usr/bin/gs');
  $files = implode(' ', $files);
  $command = "{$gs} -dBATCH -dNOPAUSE -q -sDEVICE=pdfwrite -sOutputFile={$out} {$files}";
  $output = array(); $ret = 0;
  exec($command, $output, $ret);
  if ($ret != 0) {
    $variables = array(
      '@ret' => $ret,
      '@command' => $command,
      '!output' => implode('<br/>', $output),
    );
    watchdog('islandora_paged_content', 'GhostScript failed to combine PDFs.<br/>Error: @ret<br/>Command: @command <br/>Output !output', $variables, WATCHDOG_ERROR);
    return FALSE;
  }
  return TRUE;
}
