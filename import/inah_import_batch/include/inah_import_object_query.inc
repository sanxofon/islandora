<?php

drupal_set_time_limit(1200);

function inah_import_object_query($form,&$form_state){
 	$form['intro'] = array(
 		"#type"		=> 'markup',
 		'#markup'	=> '<div>Forma para hacer querys al RI</div>' 
 	);

	$form['inah_import_object_query_col'] = array(
      '#type' => 'textfield',
      '#title' => t('Collection'),
      '#size' => 60,
      '#description' => t('Colección a recorrer.'),
      '#default_value' => '',
     );

	$form['inah_import_object_query_cad'] = array(
      '#type' => 'textfield',
      '#title' => t('String'),
      '#size' => 60,
      '#description' => t('Prefix for identifier: <em>PREFIX:PID</em>'),
      '#default_value' => variable_get('inah_import_object_query_cad','INAH:MEDIATECA:'),
     );

 	$form['inah_import_batch_do_query'] = array(
 		"#type"		=> 'submit',
 		'#value'	=> 'Enviar' 
 	);
 	
 	return $form;
}

function inah_import_object_query_validate($form,&$form_state){
	
}

function inah_import_batch_connectToFedora(){
 	global $user;
 	$connection = islandora_get_tuque_connection($user);
 	$connection->reuseConnection = TRUE;
 	$repository = $connection->repository;
 	return $repository;
}

function inah_import_object_query_submit(array $form, array &$form_state) {
	$repo = inah_import_batch_connectToFedora();
	$collection = $form_state['values']['inah_import_object_query_col'];
	$ret = array();
	$sparqlquery = '
	PREFIX islandora-rels-ext: <http://islandora.ca/ontology/relsext#>
	SELECT ?pid
	FROM <#ri>
	WHERE {
	  ?pid <fedora-rels-ext:isMemberOfCollection> <info:fedora/'.$collection.'> .
	}
	ORDER BY ?pid
	';
	echo chr(239).chr(187).chr(191);
	echo '<pre>';

	$cadena_identifier = $form_state['values']['inah_import_object_query_cad'];
	variable_set('inah_import_object_query_cad',$cadena_identifier);
	//$cadena_identifier = 'INAH:MEDIATECA:';

	$qu = $repo->ri->sparqlQuery($sparqlquery,'unlimited');

	/*
	//REDO DC from MODS
	$xsltPath = drupal_get_path('module','islandora_importer')."/xsl/mods_to_dc.xsl";
	$xsl = new DOMDocument();
	$xsl->load($xsltPath);
	*/

	foreach ($qu as $q) {
		$pid = $q['pid']['value'];
		$obj = islandora_object_load($pid);

		/*
		//REDO DC from MODS
		$mods_content = $obj['MODS']->content;
		$xml = new SimpleXMLElement($mods_content);
		$processor = new XSLTProcessor();
		$processor->importStylesheet($xsl);
		$dcString  = $processor->transformToXML($xml);

		inah_import_object_query_add_datastream($obj, 'DC', $dcString, 'Dublin Core');
		continue;
		*/

		$dc_content = $obj['DC']->content;
		$dc_object = DublinCore::importFromXMLString($dc_content);
		$stop = false;
		foreach ($dc_object->dc as $k=>$val) {
			if ($k=='dc:identifier') {
				foreach ($val as $v) {
					if (preg_match('/'.$cadena_identifier.'/i', $v)) {
						$stop = true;
						break;
					}
				}
			}
			if ($stop) {
				break;
			}
		}
		if ($stop) continue;
		//print_r($dc_object->dc);

		$dc = array();
		foreach ($dc_object->dc as $k => $val) {
			$k = substr($k,3);
			$v = implode(";", $val);
			//----------------------------
			//REEMPLAZOS EN EL ARRAY MISMO
			if ($k!='description' && $k!='title') {
				$v = str_replace("--", ";", $v);
				if ($k!='contributor') $v = str_replace(",", ";", $v);
				$v = explode(";", $v);
				for ($i=0; $i < count($v); $i++) { 
					$v[$i] = trim($v[$i],",.:- \t\n\r\0\x0B");
					if ( ( is_string($v[$i]) && strlen($v[$i])>128 ) || ( $k=='identifier' && ( $v[$i]==$pid || preg_match('/'.$cadena_identifier.'/', $v[$i] ) ) ) ) $v[$i]='';
				}
			} else {
				$v = array(trim($v,",;:- \t\n\r\0\x0B"));
			}
			$v = array_unique($v);
			$v = array_filter($v);
			if ($k=='identifier') {
				$v[]=$cadena_identifier.$pid;
			}
			if (!empty($v)) $dc[$k] = $v;

		}
		/*print_r($dc);
		continue;*/
		
		// creating object of SimpleXMLElement
		$module_path = drupal_get_path('module','inah_import_batch');
		$xml = new SimpleXMLElement($module_path.'/xml/dc.xml', NULL, TRUE);
		
		foreach ($dc as $k=>$d) {
			foreach ($d as $v) {
				$xml->addChild("dc:$k",htmlspecialchars($v),"http://purl.org/dc/elements/1.1/");
			}
		}

		//saving generated xml file
		$dom = new DOMDocument('1.0');
		$dom->preserveWhiteSpace = false;
		$dom->formatOutput = true;
		$dom->loadXML($xml->asXML());
		$dc_string = $dom->saveXML();
		//file_put_contents('/home/santiago/001_AHCM/dc/'.str_replace(":","-",$pid.'.xml'), $dc_string);
		//$xml->asXML('/home/santiago/001_AHCM/dc/'.str_replace(":","-",$pid.'.xml'));
		inah_import_object_query_add_datastream($obj, 'DC', $dc_string, 'Dublin Core');
		echo "Creado: ".$pid.".xml\n";

	}
	echo '</pre>';
	exit;
	//return true;
}

function inah_import_object_query_add_datastream(AbstractObject $object, $datastream_id, $file_str, $dslabel='') {
  try {
    $ingest = !isset($object[$datastream_id]);
    $mime_detector = new MimeDetect();

    if ($ingest) {
      $ds = $object->constructDatastream($datastream_id, "M");
      if (!empty($dslabel)) $ds->label = $dslabel;
      else $ds->label = $datastream_id;
    }
    else {
      $ds = $object[$datastream_id];
    }
    $ds->mimetype = 'text/xml';
    $ds->setContentFromString($file_str);
    if ($ingest) {
      $object->ingestDatastream($ds);
    }
    return array(
      'success' => TRUE,
      'messages' => array(
        array(
          'message' => t('Created @dsid derivative for OBJ (@pid).'),
          'message_sub' => array(
            '@dsid' => $datastream_id,
            '@pid' => $object->id,
          ),
          'type' => 'dsm',
        ),
      ),
    );
  }
  catch (exception $e) {
    return array(
      'success' => FALSE,
      'messages' => array(
        array(
          'message' => t('Disco solution pack failed to add @dsid datastream for @pid. Error message: @message<br/>Stack: @stack'),
          'message_sub' => array(
            '@dsid' => $datastream_id,
            '@pid' => $object->id,
            '@message' => $e->getmessage(),
            '@stack' => $e->getTraceAsString(),
          ),
          'type' => 'watchdog',
          'severity' => WATCHDOG_ERROR,
        ),
      ),
    );
  }
}
