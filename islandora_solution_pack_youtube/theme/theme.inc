<?php

/**
 * @file
 * This file contains all theme and preprocess functions
 */

/**
 * Prepares variables for islandora-youtube templates.
 *
 * Default template: islandora-youtube.tpl.php.
 *
 * @param array $variables
 *   An associative array containing:
 *   - islandora_object: An AbstractObject from Islandora.
 */
function islandora_youtube_preprocess_islandora_youtube(&$variables) {

  drupal_add_js('misc/form.js');
  drupal_add_js('misc/collapse.js');
  $islandora_object = $variables['islandora_object'];
  $repository = $islandora_object->repository;
  module_load_include('inc', 'islandora', 'includes/datastream');
  module_load_include('inc', 'islandora', 'includes/utilities');
  module_load_include('inc', 'islandora', 'includes/metadata');

  // We should eventually remove the DC object and dc_array code as it only
  // exists to not break legacy implementations.
  try {
    if (isset($islandora_object['DC']) && islandora_datastream_access(ISLANDORA_VIEW_OBJECTS, $islandora_object['DC'])) {
      $dc = $islandora_object['DC']->content;
      $dc_object = DublinCore::importFromXMLString($dc);
    }
  }
  catch (Exception $e) {
    drupal_set_message(t('Error retrieving object %s %t', array('%s' => $islandora_object->id, '%t' => $e->getMessage())), 'error', FALSE);
  }
  $variables['islandora_dublin_core'] = isset($dc_object) ? $dc_object : NULL;
  $variables['dc_array'] = isset($dc_object) ? $dc_object->asArray() : array();
  $variables['islandora_object_label'] = $islandora_object->label;
  $variables['theme_hook_suggestions'][] = 'islandora_youtube__' . str_replace(':', '_', $islandora_object->id);
  $variables['parent_collections'] = islandora_get_parents_from_rels_ext($islandora_object);
  $variables['metadata'] = islandora_retrieve_metadata_markup($islandora_object);
  $variables['description'] = islandora_retrieve_description_markup($islandora_object);

  global $base_url;

  module_load_include('inc', 'islandora', 'includes/solution_packs');
  $viewer = islandora_get_viewer(array(), 'islandora_youtube_viewers', $islandora_object);

  if ($viewer) {
    $variables['islandora_content'] = $viewer;
  }
  else {
    // Thumbnail.
    if (isset($islandora_object['TN'])) {
      $thumbnail_size_url = url("islandora/object/{$islandora_object->id}/datastream/TN/view");
      $params = array(
        'title' => $islandora_object->label,
        'path' => $thumbnail_size_url,
      );
      $variables['islandora_thumbnail_img'] = theme('image', $params);
    }
    // CDcover image + link.
    if (isset($islandora_object['IMG'])) {
      $img_url = url("islandora/object/{$islandora_object->id}/datastream/IMG/view");
      $params = array(
        'title' => $islandora_object->label,
        'path' => $img_url,
      );
      $variables['islandora_img_img'] = theme('image', $params);
      $variables['islandora_content'] = l($variables['islandora_img_img'], $variables['islandora_full_url'], array('html' => TRUE));
      // Sanitize this object name a bit.
      $sanitized_label = preg_replace('/[^A-Za-z0-9_\-]|\.pdf$/', '_', $islandora_object->label);
    }

  }
}
