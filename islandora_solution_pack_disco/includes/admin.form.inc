<?php

/**
 * @file
 * Handles the display/submission of the admin settings form for this module.
 */

/**
 * Defines the admin settings form.
 *
 * @param array $form
 *   The Drupal form definition.
 * @param array $form_state
 *   The Drupal form state.
 *
 * @return array
 *   The Drupal form definition.
 */
function islandora_disco_admin(array $form, array &$form_state) {

  //AUDIO
  module_load_include('inc', 'islandora', 'includes/utilities');
  $get_default_value = function($name, $default) use(&$form_state) {
    return isset($form_state['values'][$name]) ? $form_state['values'][$name] : variable_get($name, $default);
  };

  $lame = $get_default_value('islandora_lame_url', '/usr/bin/lame');
  $theora = $get_default_value('islandora_theora_url', '/usr/bin/ffmpeg/ffmpegtheora');
  $pdftotext = $get_default_value('islandora_pdftotext_url', '/usr/bin/pdftotext');

  $form = array();

  //AUDIO
  
  // A que collección se agregarán los objectos
  $form['islandora_disco_add_to_collection'] = array(
    '#type' => 'textfield',
    '#title' => t("Collection to add objects by default."),
    '#description' => t("Import of disco objects (and child tracks) will be added to this colletion pid"),
    '#default_value' => variable_get('islandora_disco_add_to_collection', 'islandora:disco_collection'),
    '#size' => 20,
  );
  
  // Redo mp3 encoding when OBJ is MP3 mime type
  $form['islandora_disco_dont_mp3_redo'] = array(
    '#type' => 'checkbox',
    '#title' => t("Don't MP3 derivative redo."),
    '#description' => t("MP3 derivative will not be reencoded if original audio file is MP3 also."),
    '#default_value' => variable_get('islandora_disco_dont_mp3_redo', FALSE),
  );

  // Create OGG derivative or not
  $form['islandora_disco_allow_ogg_derivative'] = array(
    '#type' => 'checkbox',
    '#title' => t("Create OGG derivative"),
    '#description' => t("Create OGG derivative from original audio file. Path to FFMpeg-Theora must be valid."),
    '#default_value' => variable_get('islandora_disco_allow_ogg_derivative', FALSE),
  );

  // lame encoder url.
  $form['islandora_lame_url'] = array(
      '#type' => 'textfield',
      '#title' => t("Path to LAME"),
      '#description' => t('!LAME is required to create derivatives files.<br/>!msg',
                      array(
                        '!LAME' => l(t('LAME'), 'http://lame.sourceforge.net/'),
                        '!msg' => islandora_executable_available_message($lame),
                      )),
      '#default_value' => $lame,
      '#size' => 20,
      '#prefix' => '<div id="lame-wrapper">',
      '#suffix' => '</div>',
      '#ajax' => array(
        'callback' => 'islandora_disco_admin_form_lame_ajax_callback',
        'wrapper' => 'lame-wrapper',
        'effect' => 'fade',
        'event' => 'change',
      ),
    );
  // ffmpeg2theora encoder path.
  $form['islandora_theora_url'] = array(
      '#type' => 'textfield',
      '#title' => t("Path to Theora executable"),
      '#description' => t('!ffmpeg2theora is required to create derivatives files.<br/>!msg',
                      array(
                        '!THEORA' => l(t('THEORA'), 'http://v2v.cc/~j/ffmpeg2theora/'),
                        '!msg' => islandora_executable_available_message($theora),
                      )),
      '#default_value' => $theora,
      '#size' => 20,
      '#prefix' => '<div id="theora-wrapper">',
      '#suffix' => '</div>',
      '#ajax' => array(
        'callback' => 'islandora_disco_admin_form_theora_ajax_callback',
        'wrapper' => 'theora-wrapper',
        'effect' => 'fade',
        'event' => 'change',
      ),
    );

  //PDF
  // pdftotext text extractor path.
  $form['islandora_pdftotext_url'] = array(
      '#type' => 'textfield',
      '#title' => t("Path to PDFtoText executable"),
      '#description' => t('!PDFtoText is required to create derivatives files.<br/>!msg',
                      array(
                        '!PDFtoText' => l(t('PDFtoText'), 'http://www.foolabs.com/xpdf/'),
                        '!msg' => islandora_executable_available_message($pdftotext),
                      )),
      '#default_value' => $pdftotext,
      '#size' => 20,
      '#prefix' => '<div id="pdftotext-wrapper">',
      '#suffix' => '</div>',
      '#ajax' => array(
        'callback' => 'islandora_disco_admin_form_pdftotext_ajax_callback',
        'wrapper' => 'pdftotext-wrapper',
        'effect' => 'fade',
        'event' => 'change',
      ),
    );
  // Allow to upload text version of the pdf
  $form['islandora_disco_allow_text_upload'] = array(
    '#type' => 'checkbox',
    '#title' => t("Allow users to upload .txt files with PDFs"),
    '#description' => t("Uploaded text files are appended to PDFs as FULL_TEXT datastreams and are indexed into Solr."),
    '#default_value' => variable_get('islandora_disco_allow_text_upload', FALSE),
  );
  // Allow to create text version of the pdf using pdftotext
  $form['islandora_disco_create_fulltext'] = array(
    '#type' => 'checkbox',
    '#title' => t("Extract text streams from PDFs using pdftotext"),
    '#description' => t("Extracted text streams are appended to PDFs as FULL_TEXT datastreams and are indexed into Solr. Uploading a text file takes priority over text stream extraction.
                           </br><strong>Note:</strong> PDFs that contain visible text do not necessarily contain text streams (e.g. images scanned and saved as PDFs)."),
    '#default_value' => variable_get('islandora_disco_create_fulltext', FALSE),
  );

  $form['islandora_disco_img_fieldset'] = array(
    '#type' => 'fieldset',
    '#title' => t('CD Cover image'),
    '#description' => t('Settings for creating (IMG) CD cover image derivatives'),
  );

  $form['islandora_disco_img_fieldset']['islandora_disco_img_width'] = array(
    '#type' => 'textfield',
    '#title' => t('Max width'),
    '#description' => t('The maximum width of the img in pixels.'),
    '#element_validate' => array('element_validate_number'),
    '#default_value' => variable_get('islandora_disco_img_width', 500),
    '#size' => 5,
  );

  $form['islandora_disco_img_fieldset']['islandora_disco_img_height'] = array(
    '#type' => 'textfield',
    '#title' => t('Max height'),
    '#description' => t('The maximum height of the img in pixels.'),
    '#element_validate' => array('element_validate_number'),
    '#default_value' => variable_get('islandora_disco_img_height', 700),
    '#size' => 5,
  );

  $form['islandora_disco_img_fieldset']['islandora_disco_thumbnail_width'] = array(
    '#type' => 'textfield',
    '#title' => t('Width'),
    '#description' => t('The width of the thumbnail in pixels.'),
    '#element_validate' => array('element_validate_number'),
    '#default_value' => variable_get('islandora_disco_thumbnail_width', 200),
    '#size' => 5,
  );

  $form['islandora_disco_img_fieldset']['islandora_disco_thumbnail_height'] = array(
    '#type' => 'textfield',
    '#title' => t('Height'),
    '#description' => t('The height of the thumbnail in pixels.'),
    '#element_validate' => array('element_validate_number'),
    '#default_value' => variable_get('islandora_disco_thumbnail_height', 200),
    '#size' => 5,
  );

  module_load_include('inc', 'islandora', 'includes/solution_packs');
  $form += islandora_viewers_form('islandora_disco_viewers_audio', 'application/rss+xml');
  $form += islandora_viewers_form('islandora_disco_viewers_pdf', 'application/pdf');

  return system_settings_form($form);
}

/**
 * Ajax callback for the Lame textfield.
 *
 * @param array $form
 *   The Drupal form definition.
 * @param array $form_state
 *   The Drupal form state.
 *
 * @return array
 *   The element to render as part the ajax callback.
 */
function islandora_disco_admin_form_lame_ajax_callback(array $form, array $form_state) {
  return $form['islandora_lame_url'];
}

/**
 * Ajax callback for the FFMPEG-THEORA textfield.
 *
 * @param array $form
 *   The Drupal form definition.
 * @param array $form_state
 *   The Drupal form state.
 *
 * @return array
 *   The element to render as part the ajax callback.
 */
function islandora_disco_admin_form_theora_ajax_callback(array $form, array $form_state) {
  return $form['islandora_theora_url'];
}

/**
 * Ajax callback for the PDFtoText textfield.
 *
 * @param array $form
 *   The Drupal form definition.
 * @param array $form_state
 *   The Drupal form state.
 *
 * @return array
 *   The element to render as part the ajax callback.
 */
function islandora_disco_admin_form_pdftotext_ajax_callback(array $form, array $form_state) {
  return $form['islandora_pdftotext_url'];
}

/**
 * Admin form validate function.
 */
function islandora_disco_admin_validate($form, &$form_state) {
  if ($form_state['values']['islandora_disco_create_fulltext']) {
    $pdftotext = $form_state['values']['islandora_pdftotext_url'];
    exec($pdftotext, $output, $return_value);
    if ($return_value != 99) {
      form_set_error('', t('Cannot extract text from PDF without a valid path to pdftotext.'));
    }
  }
}