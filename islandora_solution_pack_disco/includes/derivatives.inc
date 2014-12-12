<?php

/**
 * @file
 * This file contains functions used to generate derivatives of disco (audio/pdf) objects.
 */

/**
 * Gets the source of all derivatives as an unmanaged file.
 *
 * @param AbstractObject $object
 *   The object to generate/add the deriviative datastreams to.
 *
 * @return string
 *   The file path to the unmanaged derivative source file.
 */
function islandora_disco_get_derivative_source_as_unmanaged_file_pdf(AbstractObject $object) {
  $mime_detect = new MimeDetect();
  $ext = $mime_detect->getExtension($object['PDF']->mimeType);
  $file_name = str_replace(':', '-', $object->id);
  $file_uri = file_create_filename("{$file_name}.PDF.{$ext}", 'temporary://');
  $object['PDF']->getContent($file_uri);
  return $file_uri;
}

/**
 * Gets the source of all derivatives as an unmanaged file.
 *
 * @param AbstractObject $object
 *   The object to generate/add the deriviative datastreams to.
 *
 * @return string
 *   The file path to the unmanaged derivative source file.
 */
function islandora_disco_get_derivative_source_as_unmanaged_file_img(AbstractObject $object) {
  $mime_detect = new MimeDetect();
  $ext = $mime_detect->getExtension($object['IMG']->mimeType);
  $file_name = str_replace(':', '-', $object->id);
  $file_uri = file_create_filename("{$file_name}.IMG.{$ext}", 'temporary://');
  $object['IMG']->getContent($file_uri);
  return $file_uri;
}

/**
 * Creates a TN deriviative for the IMG datastream.
 *
 * @param AbstractObject $object
 *   The object to add the deriviative datastreams to.
 * @param bool $force
 *   Whether the derivative generation is being forced or not.
 *
 * @return array
 *   An array describing the outcome of the thumbnail creation.
 *
 * @see hook_islandora_derivative()
 */
function islandora_disco_add_tn_derivative(AbstractObject $object, $force = FALSE) {
  $width = variable_get('islandora_disco_thumbnail_width', 200);
  $height = variable_get('islandora_disco_thumbnail_height', 200);
  $file_uri = islandora_disco_get_derivative_source_as_unmanaged_file_img($object);
  $results = islandora_disco_add_jpg_derivative($object, $file_uri, 'TN', $width, $height, $force);
  file_unmanaged_delete($file_uri);
  return $results;
}

/**
 * Creates a textual derivative for the FULL_TEXT datastream.
 *
 * @param AbstractObject $object
 *   The object to add the deriviative datastreams to.
 * @param bool $force
 *   Whether the derivative generation is being forced or not.
 *
 * @return array
 *   An array describing the outcome of the FULL_TEXT datastream creation.
 *
 * @see hook_islandora_derivative()
 */
function islandora_disco_add_fulltext_derivative(AbstractObject $object, $force = FALSE) {
  module_load_include('inc', 'islandora', 'includes/utilities');
  if (!isset($object['FULL_TEXT']) || (isset($object['FULL_TEXT']) && $force === TRUE)) {
    if (!isset($object['PDF'])) {
      return islandora_disco_missing_obj_datastream($object->id);
    }
    $file_uri = islandora_disco_get_derivative_source_as_unmanaged_file_pdf($object);
    $dsid = "FULL_TEXT";
    $source = $source = drupal_realpath($file_uri);
    $executable = variable_get('islandora_pdftotext_url', '/usr/bin/pdftotext');
    $temp = uniqid();
    $temp = file_create_filename($temp.'.fulltext.txt', 'temporary://');
    $derivative_file_uri = drupal_realpath($temp);
    $command = "$executable \"$source\" \"$derivative_file_uri\"";
    exec($command, $execout, $returncode);
    file_unmanaged_delete($file_uri);
    $success = ($returncode === 0);

    if (!$success) {
      $success = array(
        'success' => FALSE,
        'messages' => array(
          array(
            'message' => t('`%command` returned an error when creating %dsid derivative of PID %pid: %error'),
            'message_sub' => array(
              '%command' => $command,
              '%dsid' => $dsid,
              '%pid' => $object->id,
              '%error' => $execout,
            ),
            'type' => 'dsm',
            'severity' => WATCHDOG_ERROR,
          ),
        ),
      );
    }
    else {
      // Before we add this datastream let's rip out non valid XML characters.
      $full_text = file_get_contents($derivative_file_uri);
      $sanitized_full_text = islandora_sanitize_input_for_valid_xml($full_text);
      file_put_contents($derivative_file_uri, $sanitized_full_text);
      $success = islandora_disco_add_datastream($object, $dsid, $derivative_file_uri);
      file_unmanaged_delete($derivative_file_uri);
      if ($success === TRUE) {
        $success = array(
          'success' => TRUE,
          'messages' => array(
            array(
              'message' => t('Created FULL_TEXT derivative.'),
              'type' => 'dsm',
            ),
          ),
        );
        return $success;
      }
      else {
        $success = array(
          'success' => FALSE,
          'messages' => array(
            array(
              'message' => t('Failed to create FULL_TEXT derivative for @pid. Error message: @error'),
              'message_sub' => array(
                '@pid' => $object->id,
                '@error' => $success,
              ),
              'type' => 'watchdog',
              'severity' => WATCHDOG_ERROR,
            ),
          ),
        );
        return $success;
      }
    }
    return $success;
  }
}

/*
 * Creates a IMG deriviative for the PDF datastream.
 *
 * @param AbstractObject $object
 *   The object to add the deriviative datastreams to.
 * @param bool $force
 *   Whether the derivative generation is being forced or not.
 *
 * @return array
 *   An array describing the outcome of the IMG datastream creation.
 *
 * @see hook_islandora_derivative()
 */
/*function islandora_disco_add_img_derivative(AbstractObject $object, $force = FALSE) {
  $width = variable_get('islandora_disco_img_width', 500);
  $height = variable_get('islandora_disco_img_height', 700);
  $file_uri = islandora_disco_get_derivative_source_as_unmanaged_file_pdf($object);
  $results = islandora_disco_add_jpg_derivative($object, $file_uri, 'IMG', $width, $height, $force);
  file_unmanaged_delete($file_uri);
  return $results;
}*/

/**
 * Creates a JPG deriviative of the given file.
 *
 * @param AbstractObject $object
 *   The object to add the deriviative datastreams to.
 * @param string $file_uri
 *   The URI to the PDF file the derivative will be generated from.
 * @param string $dsid
 *   The datastream ID to use for the derived datastream.
 * @param int $width
 *   The width to make the derived datastream.
 * @param int $height
 *   The height to make the derived datastream.
 * @param bool $force
 *   Whether the derivative generation is being forced or not.
 *
 * @return array
 *   An array describing the outcome of adding the JPG derivative.
 *
 * @see hook_islandora_derivative()
 */
function islandora_disco_add_jpg_derivative(AbstractObject $object, $file_uri, $dsid, $width, $height, $force) {
  if (!isset($object[$dsid]) || (isset($object[$dsid]) && $force === TRUE)) {
    if (!isset($object['IMG'])) {
      return islandora_disco_missing_obj_datastream($object->id);
    }
    $derivative_file_uri = islandora_disco_create_jpg_derivative($file_uri, $dsid, $width, $height);

    file_unmanaged_delete($file_uri);
    // Receive a valid file URI to add or an error message otherwise.
    if (!is_array($derivative_file_uri) && file_valid_uri($derivative_file_uri)) {
      $success = islandora_disco_add_datastream($object, $dsid, $derivative_file_uri);
      file_unmanaged_delete($derivative_file_uri);
      if ($success === TRUE) {
        return array(
          'success' => TRUE,
          'messages' => array(
            array(
              'message' => t('Created JPG derivative for @dsid (@pid).'),
              'message_sub' => array(
                '@dsid' => $dsid,
                '@pid' => $object->id,
              ),
              'type' => 'dsm',
            ),
          ),
        );
      }
      else {
        return array(
          'success' => FALSE,
          'messages' => array(
            array(
              'message' => t('Failed to create JPG derivative for @dsid (@pid). Error message: @message'),
              'message_sub' => array(
                '@dsid' => $dsid,
                '@pid' => $object->id,
                '@message' => $success,
              ),
              'type' => 'watchdog',
              'severity' => WATCHDOG_ERROR,
            ),
          ),
        );
      }
    }
    else {
      return $derivative_file_uri;
    }
  }
}

/**
 * Creates a TN derivative from the given IMG file.
 *
 * @param string $file_uri
 *   The URI to the IMG file the derivative will be generated from.
 * @param string $dsid
 *   Used to identify the generated file.
 * @param int $width
 *   The width to make the derived datastream.
 * @param int $height
 *   The height to make the derived datastream.
 *
 * @return string|array
 *   A URI to the generated derivative if successful, error message otherwise.
 *
 * @see hook_islandora_derivative()
 */
function islandora_disco_create_jpg_derivative($file_uri, $dsid, $width, $height) {
  $source = drupal_realpath($file_uri);//. '[0]';

  //DEBUG
  file_put_contents('/home/santiago/002_FTMM/testing.log', print_r(array('islandora_disco_create_jpg_derivative',"source->IMG",$source),true), FILE_APPEND);

  $matches = array();
  // Get the base name of the source file.
  //$base = pathinfo($source, PATHINFO_FILENAME);
  $base = uniqid();
  $temp = file_create_filename($base.".".$dsid.".jpg", 'temporary://');
  $dest = drupal_realpath($temp);
  $args['quality'] = '-quality ' . escapeshellarg(variable_get('imagemagick_quality', 75));
  $args['imgsize'] = '-resize ' . escapeshellarg("{$width}x{$height}");
  $args['colors'] = '-colorspace RGB';
  $args['flatten'] = '-flatten';
  $context = array(
    'source' => $source,
    'destination' => $dest,
  );
  //DEBUG
  file_put_contents('/home/santiago/002_FTMM/testing.log', print_r(array('islandora_disco_create_jpg_derivative',"dest->IMG",$dest),true), FILE_APPEND);
  drupal_alter('imagemagick_arguments', $args, $context);
  // To make use of ImageMagick 6's parenthetical command grouping we need to
  // make the $source image the first parameter and $dest the last.
  // See http://www.imagemagick.org/Usage/basics/#cmdline
  $command = escapeshellarg($source) . ' ' . implode(' ', $args) . ' ' . escapeshellarg("jpg:$dest");
  $output = '';
  $ret = -1;
  if (_imagemagick_convert_exec($command, $output, $ret) !== TRUE) {
    return array(
      'success' => FALSE,
      'messages' => array(
        array(
          'message' => t('ImageMagick failed to create derivatives for @pid<br/>Error: @ret<br/>Command: @command<br/>Output: @output'),
          'message_sub' => array(
            '@ret' => $ret,
            '@command' => $command,
            '@output' => $output,
          ),
          'type' => 'watchdog',
          'severity' => WATCHDOG_ERROR,
        ),
      ),
    );
  }
  return $temp;
}

/**
 * Adds the given file as a datastream to the given object.
 *
 * @param AbstractObject $object
 *   An AbstractObject representing an object within Fedora.
 * @param string $datastream_id
 *   The datastream id of the added datastream.
 * @param string $file_uri
 *   A URI to the file containing the content for the datastream.
 *
 * @return array
 *   An array describing the outcome of the datastream addition.
 */
function islandora_disco_add_datastream(AbstractObject $object, $datastream_id, $file_uri, $dslabel='') {
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
    $ds->mimetype = $mime_detector->getMimetype($file_uri);
    $ds->setContentFromFile(drupal_realpath($file_uri));
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

/**
 * Helper function declaring that no OBJ datastream is present.
 *
 * @param string $pid
 *   The pid of the ingested object.
 *
 * @return array
 *   An array describing the failure of the operation..
 *
 * @see hook_islandora_derivative()
 */
function islandora_disco_no_obj_datastream($pid) {
  return array(
    'success' => FALSE,
    'messages' => array(
      array(
        'message' => t('Could not create image derivatives for %s. No image file was uploaded.'),
        'message_sub' => array('%s' => $pid),
        'type' => 'watchdog',
        'severity' => WATCHDOG_ERROR,
      ),
    ),
  );
}

/**
 * Creates an RSS file for the playlist FWPlayer
 *
 * @param array $form
 *   The drupal form.
 * @param array $form_state
 *   The drupal form state.
 *
 * @return array
 *   The drupal form definition.
 */
  //Gets empty rss playlist file
function islandora_disco_playlist_create_rss(array $playlist_files, $objectid) {
  $ofile = drupal_get_path('module', 'islandora_disco') . '/includes/playlist2.rss';

  $xml = new SimpleXMLElement($ofile, NULL, TRUE);
  foreach ($playlist_files as $track) {
    $item = $xml->channel->addChild('item');

    $item->addChild('title', (string)$track['track_label']);
    $item->addChild('description', (string)$track['track_desc']);
    
    $mediaGroup = $item->addChild('media:group', '', 'http://search.yahoo.com/mrss/');

    $mediaCredit = $mediaGroup->addChild('media:credit', $track['track_desc'], 'http://search.yahoo.com/mrss/');
    $mediaCredit->addAttribute('role', 'author');

    $mediaContent = $mediaGroup->addChild('media:content', '', 'http://search.yahoo.com/mrss/');
    $mediaContent->addAttribute('url', $track['track_url']);

    $mediaThumbnail = $mediaGroup->addChild('media:thumbnail', '', 'http://search.yahoo.com/mrss/');
    $mediaThumbnail->addAttribute('url', $track['thumbnail']);

    /*$item->addChild('description', (string)$track['track_desc'], TRUE);
    $item->addChild('title', (string)$track['track_label'], TRUE);

    $item->addChild('image', $track['thumbnail'],'http://rss.jwpcdn.com/');

    $source = $item->addChild('source', FALSE,'http://rss.jwpcdn.com/');
    $source->addAttribute('file', $track['track_url']);*/
  }

  //Creates temporary rss file
    //ordena la playlist por título
    $xsltPaht = drupal_get_path('module','islandora_disco')."/includes/sort_rss.xsl";
    $xsl = new DOMDocument();
    $xsl->load($xsltPaht);
    $processor = new XSLTProcessor();
    $processor->importStylesheet($xsl);
    $sortedstring  = $processor->transformToXML($xml);

  $file_name = str_replace(":", "-", $objectid) . ".rss";
  $file_uri = file_create_filename($file_name, 'temporary://');
  $file_out = drupal_realpath($file_uri);
  file_put_contents($file_out,$sortedstring);
  return $file_out;

}
