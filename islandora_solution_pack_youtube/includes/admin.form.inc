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
function islandora_youtube_admin(array $form, array &$form_state) {

  //AUDIO
  module_load_include('inc', 'islandora', 'includes/utilities');
  $get_default_value = function($name, $default) use(&$form_state) {
    return isset($form_state['values'][$name]) ? $form_state['values'][$name] : variable_get($name, $default);
  };

  $form = array();

  //AUDIO
  
  // A que collección se agregarán los objectos
  $form['islandora_youtube_add_to_collection'] = array(
    '#type' => 'textfield',
    '#title' => t("Collection to add objects by default."),
    '#description' => t("Import of youtube objects will be added to this colletion pid"),
    '#default_value' => variable_get('islandora_youtube_add_to_collection', 'islandora:youtube_collection'),
    '#size' => 20,
  );

  $form['islandora_youtube_img_fieldset'] = array(
    '#type' => 'fieldset',
    '#title' => t('Youtube image'),
    '#description' => t('Settings for creating IMG derivatives'),
  );

  $form['islandora_youtube_img_fieldset']['islandora_youtube_img_width'] = array(
    '#type' => 'textfield',
    '#title' => t('Max width'),
    '#description' => t('The maximum width of the img in pixels.'),
    '#element_validate' => array('element_validate_number'),
    '#default_value' => variable_get('islandora_youtube_img_width', 500),
    '#size' => 5,
  );

  $form['islandora_youtube_img_fieldset']['islandora_youtube_img_height'] = array(
    '#type' => 'textfield',
    '#title' => t('Max height'),
    '#description' => t('The maximum height of the img in pixels.'),
    '#element_validate' => array('element_validate_number'),
    '#default_value' => variable_get('islandora_youtube_img_height', 700),
    '#size' => 5,
  );

  $form['islandora_youtube_img_fieldset']['islandora_youtube_thumbnail_width'] = array(
    '#type' => 'textfield',
    '#title' => t('Width'),
    '#description' => t('The width of the thumbnail in pixels.'),
    '#element_validate' => array('element_validate_number'),
    '#default_value' => variable_get('islandora_youtube_thumbnail_width', 200),
    '#size' => 5,
  );

  $form['islandora_youtube_img_fieldset']['islandora_youtube_thumbnail_height'] = array(
    '#type' => 'textfield',
    '#title' => t('Height'),
    '#description' => t('The height of the thumbnail in pixels.'),
    '#element_validate' => array('element_validate_number'),
    '#default_value' => variable_get('islandora_youtube_thumbnail_height', 200),
    '#size' => 5,
  );

  module_load_include('inc', 'islandora', 'includes/solution_packs');
  $form += islandora_viewers_form('islandora_youtube_viewers', 'application/rss+xml');

  return system_settings_form($form);
}

/**
 * Admin form validate function.
 */
function islandora_youtube_admin_validate($form, &$form_state) {
  
}