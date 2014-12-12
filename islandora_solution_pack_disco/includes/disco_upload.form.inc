<?php

/**
 * @file
 * Handles the uploading of the disco (audio/pdf/img) file as part of the ingest process.
 */

/**
 * Defines a file upload form for uploading the audios zipped file.
 *
 * @param array $form
 *   The Drupal form.
 * @param array $form_state
 *   The Drupal form state.
 *
 * @return array
 *   The drupal form definition.
 */
function islandora_disco_audios_upload_form(array $form, array &$form_state) {
  $upload_size = min((int) ini_get('post_max_size'), (int) ini_get('upload_max_filesize'));
  $audios_extensions = array('zip');

  return array(
    'audios_zip' => array(
      '#title' => t('Audios Zipped File'),
      '#type' => 'managed_file',
      '#required' => TRUE,
      '#description' => t('Select a file to upload.<br/>Files must be less than <b>@size MB.</b><br/>Allowed file types: <b>@ext.</b>', array('@size' => $upload_size, '@ext' => $audios_extensions[0])),
      '#default_value' => isset($form_state['values']['audios_zip']) ? $form_state['values']['audios_zip'] : NULL,
      '#upload_location' => 'temporary://',
      '#upload_validators' => array(
        'file_validate_extensions' => $audios_extensions,
         // Assume its specified in MB.
        'file_validate_size' => array($upload_size * 1024 * 1024),
      ),
    ),
  );
}

/**
 * Adds the uploaded file into the ingestable objects 'OBJ' datastream.
 *
 * @param array $form
 *   The Drupal form.
 * @param array $form_state
 *   The Drupal form state.
 */
function islandora_disco_audios_upload_form_submit(array $form, array &$form_state) {

  global $user;
  //Load needed modules
  module_load_include('inc', 'islandora', 'includes/utilities');
  module_load_include('inc', 'islandora_disco', 'includes/derivatives');
  module_load_include('inc', 'islandora_disco', 'includes/create_tracks');

  $connection = islandora_get_tuque_connection();
  $repository = $connection->repository;

  $object = $form_state['islandora']['objects'][0];

  if (empty($object['OBJ'])) {
    $obj = $object->constructDatastream('OBJ', 'M');
    $object->ingestDatastream($obj);
  }
  else {
    $obj = $object['OBJ'];
  }
  $audios_zip = file_load($form_state['values']['audios_zip']);

  //Get TN from parent disco object for child track objects
  $width = variable_get('islandora_disco_thumbnail_width', 200);
  $height = variable_get('islandora_disco_thumbnail_height', 200);
  $img_uri = islandora_disco_get_derivative_source_as_unmanaged_file_img($object);
  $tn_uri = drupal_realpath(islandora_disco_create_jpg_derivative($img_uri, 'TN', $width, $height));
  
  //Adds OBJ datastream
  $obj->setContentFromFile(drupal_realpath($audios_zip->uri), FALSE);
  if ($obj->label != $audios_zip->filename) {
    $obj->label = $audios_zip->filename;
  }
  if ($obj->mimetype != $audios_zip->filemime) {
    $obj->mimetype = $audios_zip->filemime;
  }

  // Extract file vars.
  $tmp_dir = uniqid();
  $zip_uri = drupal_realpath($audios_zip->uri);
  $destination_dir = drupal_realpath("temporary://$tmp_dir");

  //Use commandline to extract files for avoiding charset encoding errors
  if (strtoupper(substr(PHP_OS, 0, 3)) === 'WIN') {
    //7zip -> windows -> 7Zip must be installed to defined path
    $unzipcommand = "C:\\usr\\bin\\7zip\\7za.exe e \"$zip_uri\" -o$destination_dir";
  } else {
    //unzip -> linux
    $unzipcommand = "unzip $zip_uri -d $destination_dir";
  }
  exec($unzipcommand,$output, $return_value);

  if ($return_value!='0') {
    drupal_set_message(t('Ingest failed: Could not extract '.$zip_uri.' to '.$destination_dir.'<br>Error: '.print_r($output,true)), 'error');
    return FALSE;
  }

  $allowed_extensions = array('wav', 'flac', 'mp3');
  $callback = function ($element) use ($allowed_extensions) {
    $ext = pathinfo($element, PATHINFO_EXTENSION);
    $ext = drupal_strtolower($ext);

    // An allowed extension and does /not/ contain __MACOSX.
    return in_array($ext, $allowed_extensions) && preg_match('/__MACOSX/', $element) === 0;
  };

  $objects = new RecursiveIteratorIterator(new RecursiveDirectoryIterator($destination_dir), RecursiveIteratorIterator::SELF_FIRST);
  foreach ($objects as $file => $object) {
    $unfiltered[] = $file;
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
  unset($object);
  $object = $form_state['islandora']['objects'][0];
  $object_url = url("islandora/object/{$object->id}", array('absolute' => TRUE));
  $insertion_point = 0;

  //Array of track objects for ingesting
  $tracks = array();

  foreach ($files_to_add as $audio) {
    $config['track_number'] = ++$insertion_point;
    $config['track_name'] = pathinfo(basename($audio), PATHINFO_FILENAME);
    $config['track_str'] = "TRACK_".$config['track_number'];
    $config['tn'] = $tn_uri;

    //TODO -> Se puede eliminar esto?
    unset($object);
    $object = $form_state['islandora']['objects'][0];
    
    
    //Adds TRACK child object as sp-audioCModel
    
    //Moved from create_tracks.inc -> islandora_disco_ingest_track
    $namespace = substr($object->id, 0, strpos($object->id, ":"));
    $track = $repository->constructObject($namespace);
    $track->owner = $user->name;

    //Removed for better performance
    $track = islandora_disco_ingest_track($track, $object->id,$repository, $audio, $config);
    $tracks[] = $track;

    $track_url = url("islandora/object/".$track->id, array('absolute' => TRUE)).'/datastream/PROXY_MP3/'.preg_replace('/\.[a-z]{3,4}$/i','.mp3',basename($audio));

    $playlist_files[] = array(
        'track_url' => $track_url,
        'track_number' => $config['track_number'],
        'track_label' => $config['track_name'],
        'thumbnail' => $object_url.'/datastream/IMG/thumbnail.jpg',
      );
  }

  //Crea e ingesta el playlist DS, luego bora el archivo temporal
  $playlist = islandora_disco_playlist_create_rss($playlist_files, $object->id);
  islandora_disco_add_datastream($object, 'PLAYLIST', $playlist);

  //Ingest tracks
  foreach ($tracks as $track) {
    $repository->ingestObject($track);
  }

}

/**
 * Defines a file upload form for uploading the pdf file.
 *
 * @param array $form
 *   The drupal form.
 * @param array $form_state
 *   The drupal form state.
 *
 * @return array
 *   The drupal form definition.
 */
function islandora_disco_pdf_upload_form(array $form, array &$form_state) {
  $upload_size = min((int) ini_get('post_max_size'), (int) ini_get('upload_max_filesize'));
  $extensions = array('pdf');
  $form = array();
  $form['pdf_file'] = array(
    '#title' => t('PDF File'),
    '#type' => 'managed_file',
    '#required' => TRUE,
    '#description' => t('Select file to upload.<br/>Files must be less than <b>@size MB.</b><br/>Allowed file types: <b>@ext.</b>', array('@size' => $upload_size, '@ext' => $extensions[0])),
    '#default_value' => isset($form_state['values']['pdf_file']) ? $form_state['values']['pdf_file'] : NULL,
    '#upload_location' => 'temporary://',
    '#upload_validators' => array(
      'file_validate_extensions' => $extensions,
      // Assume it's specified in MB.
      'file_validate_size' => array($upload_size * 1024 * 1024),
    ),
  );

  if (variable_get('islandora_disco_allow_text_upload', FALSE)) {
    $form['islandora_disco_text_upload'] = array(
      '#type' => 'checkbox',
      '#title' => t("Add text file to this upload?"),
      '#default_value' => FALSE,
    );
    $form['pdf_text'] = array(
      '#title' => t('PDF text'),
      '#type' => 'managed_file',
      '#required' => FALSE,
      '#description' => t('Select text file to upload.<br/>Files must be less than <b>@size MB.</b><br/>Allowed file types: <b>@ext.</b><br />This file is optional.', array('@size' => $upload_size, '@ext' => 'txt')),
      '#default_value' => isset($form_state['values']['pdf_text']) ? $form_state['values']['pdf_text'] : NULL,
      '#upload_location' => 'temporary://',
      '#upload_validators' => array(
        'file_validate_extensions' => array('txt'),
        // Assume it's specified in MB.
        'file_validate_size' => array($upload_size * 1024 * 1024),
      ),
      '#states' => array(
        'visible' => array(
          ':input[name="islandora_disco_text_upload"]' => array('checked' => TRUE),
        ),
      ),
    );
  }
  return $form;
}

/**
 * Addes the uploaded file into the ingestable objects 'PDF' datastream.
 *
 * @param array $form
 *   The drupal form.
 * @param array $form_state
 *   The drupal form state.
 */
function islandora_disco_pdf_upload_form_submit(array $form, array &$form_state) {
  $object = $form_state['islandora']['objects'][0];
  if (empty($object['PDF'])) {
    $ds = $object->constructDatastream('PDF', 'M');
    $object->ingestDatastream($ds);
  }
  else {
    $ds = $object['PDF'];
  }
  $pdf_file = file_load($form_state['values']['pdf_file']);
  $pdf_path = drupal_realpath($pdf_file->uri);
  $ds->setContentFromFile($pdf_path, FALSE);
  $ds->label = $pdf_file->filename;
  $ds->mimetype = $pdf_file->filemime;

  if (isset($form_state['values']['pdf_text']) && ($form_state['values']['pdf_text'] > 0)) {
    if (empty($object['FULL_TEXT'])) {
      $ds = $object->constructDatastream('FULL_TEXT', 'M');
      $object->ingestDatastream($ds);
    }
    else {
      $ds = $object['FULL_TEXT'];
    }
    $text_file = file_load($form_state['values']['pdf_text']);
    $text_path = drupal_realpath($text_file->uri);
    $ds->setContentFromFile($text_path, FALSE);
    $ds->label = $text_file->filename;
    $ds->mimetype = $text_file->filemime;
  }
}

/**
 * Defines a file upload form for uploading the CD cover image.
 *
 * @param array $form
 *   The Drupal form.
 * @param array $form_state
 *   The Drupal form state.
 *
 * @return array
 *   The Drupal form definition.
 */
function islandora_disco_img_upload_form(array $form, array &$form_state) {
  $upload_size = min((int) ini_get('post_max_size'), (int) ini_get('upload_max_filesize'));
  $extensions = array('jpg jpeg png');
  return array(
    'img_file' => array(
      '#title' => t('CD cover image'),
      '#type' => 'managed_file',
      '#required' => TRUE,
      '#description' => t('Select image to upload.<br/>Files must be less than <b>@size MB.</b><br/>Allowed file types: <b>@ext.</b>', array('@size' => $upload_size, '@ext' => $extensions[0])),
      '#default_value' => isset($form_state['values']['img_file']) ? $form_state['values']['img_file'] : NULL,
      '#upload_location' => 'temporary://',
      '#upload_validators' => array(
        'file_validate_extensions' => $extensions,
        // Assume its specified in MB.
        'file_validate_size' => array($upload_size * 1024 * 1024),
      ),
    ),
  );
}

/**
 * Submit handler, adds uploaded image to ingest object IMG.
 *
 * @param array $form
 *   The Drupal form.
 * @param array $form_state
 *   The Drupal form state.
 */
function islandora_disco_img_upload_form_submit(array $form, array &$form_state) {
  $object = $form_state['islandora']['objects'][0];
  if (empty($object['IMG'])) {
    $ds = $object->constructDatastream('IMG', 'M');
    $object->ingestDatastream($ds);
  } else {
    $ds = $object['IMG'];
  }
  $img_file = file_load($form_state['values']['img_file']);
  $path = drupal_realpath($img_file->uri);
  $ds->setContentFromFile($path, FALSE);
  $ds->label = $img_file->filename;
  $ds->mimetype = $img_file->filemime;
}