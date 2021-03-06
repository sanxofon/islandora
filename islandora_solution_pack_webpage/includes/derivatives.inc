<?php

/**
 * @file
 * This file contains functions used to generate derivatives of webpage (audio/pdf) objects.
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
function islandora_webpage_get_derivative_source_as_unmanaged_file_img(AbstractObject $object) {
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
function islandora_webpage_add_tn_derivative(AbstractObject $object, $force = FALSE) {
  $width = variable_get('islandora_webpage_thumbnail_width', 200);
  $height = variable_get('islandora_webpage_thumbnail_height', 200);
  $file_uri = islandora_webpage_get_derivative_source_as_unmanaged_file_img($object);
  $results = islandora_webpage_add_jpg_derivative($object, $file_uri, 'TN', $width, $height, $force);
  file_unmanaged_delete($file_uri);
  return $results;
}

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
function islandora_webpage_add_jpg_derivative(AbstractObject $object, $file_uri, $dsid, $width, $height, $force) {
  if (!isset($object[$dsid]) || (isset($object[$dsid]) && $force === TRUE)) {
    if (!isset($object['IMG'])) {
      return islandora_webpage_missing_obj_datastream($object->id);
    }
    $derivative_file_uri = islandora_webpage_create_jpg_derivative($file_uri, $dsid, $width, $height);

    file_unmanaged_delete($file_uri);
    // Receive a valid file URI to add or an error message otherwise.
    if (!is_array($derivative_file_uri) && file_valid_uri($derivative_file_uri)) {
      $success = islandora_webpage_add_datastream($object, $dsid, $derivative_file_uri);
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
function islandora_webpage_create_jpg_derivative($file_uri, $dsid, $width, $height) {
  $source = drupal_realpath($file_uri);//. '[0]';

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
  return $dest;
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
function islandora_webpage_add_datastream(AbstractObject $object, $datastream_id, $file_uri, $dslabel='') {
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
function islandora_webpage_no_obj_datastream($pid) {
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