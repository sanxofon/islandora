<?php

/**
 * @file
 * Handles the creation of child audio objects for a disco cmodel 
 */


/**
 * Adds the uploaded file into a new object child to disco one
 *
 * @param AbstractObject $object
 *   An AbstractObject representing an object within Fedora.
 * @param string $repository
 *   Tuque repository
 * @param string $audio_uri
 *   A URI to the file containing the audio file
 * @param array $config
 *   Config variables.
 */
function islandora_disco_ingest_track(AbstractObject $track, $object_id, $repository, $audio_uri, $config,$mods='') {
  global $user;
  //module_load_include('inc', 'islandora', 'includes/utilities');
  //module_load_include('inc', 'islandora', 'includes/ingest.form');

  //Moved to disco_upload.form.inc -> islandora_disco_audios_upload_form_submit
  /*$namespace = substr($object_id, 0, strpos($object_id, ":"));
  $track = $repository->constructObject($namespace);
  $track->owner = $user->name;*/

  $contentModel = 'islandora:sp-audioCModel';
  $collection = variable_get('islandora_disco_add_to_collection', 'islandora:disco_collection');

  $track->label = $config['track_name'];
  $track->models = $contentModel;

  $relationships = $track->relationships;
  $escaped_pid = str_replace(':', '_', $object_id);
  //Use compound relationship string
  $rels_predicate = variable_get('islandora_compound_object_relationship', 'isConstituentOf');

  $relationships->add(FEDORA_RELS_EXT_URI, 'isMemberOfCollection', 'islandora:audio_collection'); //$collection);
  $relationships->add(FEDORA_RELS_EXT_URI, 'isMemberOfCollection', $object_id); //$collection);
  $relationships->add(FEDORA_MODEL_URI, 'hasModel', $contentModel);
  /*$relationships->add(FEDORA_RELS_EXT_URI, $rels_predicate, $object_id);*/
  $relationships->add(ISLANDORA_RELS_EXT_URI, "isSequenceNumberOf$escaped_pid", $config['track_number'], TRUE);

  //Add audio datastream to track object
  islandora_disco_content_update_datastream($track, $audio_uri, 'OBJ', 'Original audio file', NULL, 'M', FALSE);
  //Get Mime Type of audio file
  $mime_detector = new MimeDetect();
  $mimetype = $mime_detector->getMimetype($audio_uri);
  $ext = $mime_detector->getExtension($mimetype);

  //Add TN datastream to track object -> Recived from parent
  islandora_disco_content_update_datastream($track, $config['tn'], 'TN', 'Thumbnail', NULL, 'M', FALSE);

  //Add PROXY_MP3 datastream directly if mime type is MP3
  $dont_redo_mp3_file = variable_get('islandora_disco_dont_mp3_redo', FALSE);
  if ($dont_redo_mp3_file && strtolower($mimetype)=='audio/mpeg' && strtolower($ext)=='mp3') {
    $mp3_audio_uri = $audio_uri;
    $set_file_for_delete = FALSE;
  } else {
    $mp3_audio_uri = inslandora_disco_create_mp3($audio_uri);
    $set_file_for_delete = TRUE;
  }
  if ($mp3_audio_uri) {
    islandora_disco_content_update_datastream($track, $mp3_audio_uri, 'PROXY_MP3', 'MP3 file', NULL, 'M', FALSE);
    //if ($set_file_for_delete) file_unmanaged_delete($mp3_audio_uri);
  }

  //Add PROXY_OGG derivative of audio track
  $do_ogg_derivative = variable_get('islandora_disco_allow_ogg_derivative', FALSE);
  if ($do_ogg_derivative) {
    $ogg_audio_uri = inslandora_disco_create_ogg($audio_uri);
    if ($ogg_audio_uri) {
      islandora_disco_content_update_datastream($track, $ogg_audio_uri, 'PROXY_OGG', 'OGG file', NULL, 'M', FALSE);
      //file_unmanaged_delete($ogg_audio_uri);
    }
  }
  
  //Try to create FITS TECHMD derivative
  module_load_include('inc', 'islandora_fits', 'includes/derivatives');
  $fits_file = islandora_fits_create_fits($audio_uri);
  if ($fits_file) {
    islandora_disco_content_update_datastream($track, $fits_file, 'TECHMD', 'Technical metadata', NULL, 'M', FALSE);
    file_unmanaged_delete($fits_file);
  }

  //Add MODS and DC ds
  if ($mods!='') {
    $xsltPaht = drupal_get_path('module','islandora_importer')."/xsl/mods_to_dc.xsl";
    $xsl = new DOMDocument();
    $xsl->load($xsltPaht);

    $xml = new SimpleXMLElement($mods, NULL, TRUE);

    $processor = new XSLTProcessor();
    $processor->importStylesheet($xsl);
    $dcString  = $processor->transformToXML($xml);
    
    $DC = $track->constructDatastream('DC','M');
    $DC->label    = 'Dublin Core';
    $DC->mimetype = 'text/xml';
    $DC->setContentFromString($dcString); 
    $track->ingestDatastream($DC);

    islandora_disco_content_update_datastream($track, $mods, 'MODS', 'MODS Metadata', NULL, 'M', FALSE);
  }

  return $track;

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
function islandora_disco_content_update_datastream(AbstractObject $object, $file, $dsid, $label = NULL, $mimetype = NULL, $control_group = 'M', $copy = TRUE) {
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

/**
 * Create mp3 file from audio
 *
 * @param String $audio_uri
 *   Uri of the original audio
 * @return  String
 *   Uri of generated mp3 file
 */
function inslandora_disco_create_mp3($audio_uri) {
  $output = array();
  $mp3_audio_uri = drupal_realpath("temporary://").basename($audio_uri) . '.mp3';
  $lame_url = variable_get('islandora_lame_url', '/usr/bin/lame');
  $command = "$lame_url -V5 --vbr-new \"$audio_uri\" \"$mp3_audio_uri\"";

  //DEBUG
  file_put_contents('/home/santiago/002_FTMM/testing.log', print_r(array('inslandora_disco_create_mp3','audio_uri',$audio_uri,'mp3_audio_uri',$mp3_audio_uri),true),FILE_APPEND);

  $ret = FALSE;
  exec($command, $output, $ret);

  //DEBUG
  file_put_contents('/home/santiago/002_FTMM/testing.log', print_r(array('inslandora_disco_create_mp3','ret',$ret),true),FILE_APPEND);

  // Error creating the derivative.
  if (!file_exists(drupal_realpath($mp3_audio_uri))) {
    drupal_set_message("Error creating MP3 file for ".$audio_uri.": ".print_r($ret,true).": ".print_r($output,true), 'error');
    return FALSE;
  } else {
    return $mp3_audio_uri;
  }
}

/**
 * Create ogg file from audio
 *
 * @param String $audio_uri
 *   Uri of the original audio
 * @return  String
 *   Uri of generated ogg file
 */
function inslandora_disco_create_ogg($audio_uri) {
  $output = array();
  $ogg_audio_uri = drupal_realpath("temporary://").basename($audio_uri) . '.ogg';
  $theora = variable_get('islandora_theora_url', 'usr/bin/ffmpeg/ffmpeg2theora');
  $command = "$theora \"$audio_uri\" -o \"$ogg_audio_uri\"";
  $ret = FALSE;
  exec($command, $output, $ret);
  // Error creating the derivative.
  if (!file_exists(drupal_realpath($ogg_audio_uri))) {
    drupal_set_message("Error creating OGG file for ".$audio_uri.": ".print_r($ret,true).": ".print_r($output,true), 'error');
    return FALSE;
  } else {
    return $ogg_audio_uri;
  }
}