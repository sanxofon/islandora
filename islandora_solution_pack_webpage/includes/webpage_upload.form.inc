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
function islandora_webpage_upload_form(array $form, array &$form_state) {
  $upload_size = min((int) ini_get('post_max_size'), (int) ini_get('upload_max_filesize'));
  $extensions = array('gif png jpg jpeg');

  return array(
    'url' => array(
      '#title' => t('Website url.'),
      '#type' => 'textfield',
      '#required' => TRUE,
      '#description' => t('Include http:// at the begining of the URL'),
      '#default_value' => '',
    ),
    'img' => array(
      '#title' => t('Image'),
      '#type' => 'managed_file',
      '#required' => FALSE,
      '#description' => t('Select image to upload.<br/>Files must be less than <b>@size MB.</b><br/>Allowed file types: <b>@ext.</b>', array('@size' => $upload_size, '@ext' => $extensions[0])),
      '#default_value' => isset($form_state['values']['files']) ? $form_state['values']['files'] : NULL,
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
 * Adds the uploaded file into the ingestable objects 'OBJ' datastream.
 *
 * @param array $form
 *   The Drupal form.
 * @param array $form_state
 *   The Drupal form state.
 */
function islandora_webpage_upload_form_submit(array $form, array &$form_state) {

  global $user;
  //Load needed modules
  module_load_include('inc', 'islandora', 'includes/utilities');
  module_load_include('inc', 'islandora_webpage', 'includes/derivatives');

  $connection = islandora_get_tuque_connection();
  $repository = $connection->repository;

  $object = $form_state['islandora']['objects'][0];
  $obj = $object->constructDatastream('OBJ', 'M');
  $object->ingestDatastream($obj);

  $webpage_url = trim($form_state['values']['url']);
  //Adds OBJ datastream
  $obj->setContentFromString($webpage_url);
  $obj->label = $webpage_url;
  $obj->mimetype = 'text/plain';

  //Get TN from parent disco object for child track objects
  $width = variable_get('islandora_webpage_thumbnail_width', 200);
  $height = variable_get('islandora_webpage_thumbnail_height', 200);
  $iwidth = variable_get('islandora_webpage_img_width', 500);
  $iheight = variable_get('islandora_webpage_img_height', 700);

  //Adds default IMG and TN if they dont exist
  if ($img_file = file_load($form_state['values']['img'])) {
    $image_path = drupal_realpath($img_file->uri);
    $img_path = islandora_webpage_create_jpg_derivative($image_path, 'IMG', $iwidth, $iheight);
    islandora_webpage_add_datastream($object, 'IMG', $img_path, 'Medium size image');
    $tn_path = islandora_webpage_create_jpg_derivative($image_path, 'TN', $width, $height);
    islandora_webpage_add_datastream($object, 'TN', $tn_path, 'Thumbnail size image');
  }

}