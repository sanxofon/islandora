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
function islandora_youtube_videos_upload_form(array $form, array &$form_state) {
  $upload_size = min((int) ini_get('post_max_size'), (int) ini_get('upload_max_filesize'));
  $videos_extensions = array('txt');

  return array(
    'videos_txt' => array(
      '#title' => t('Youtube video urls file.'),
      '#type' => 'managed_file',
      '#required' => TRUE,
      '#description' => t('Select a file to upload.<br/>Must be a text file with one youtube video url per line.<br/>Files must be less than <b>@size MB.</b><br/>Allowed file types: <b>@ext.</b>', array('@size' => $upload_size, '@ext' => $videos_extensions[0])),
      '#default_value' => isset($form_state['values']['videos_txt']) ? $form_state['values']['videos_txt'] : NULL,
      '#upload_location' => 'temporary://',
      '#upload_validators' => array(
        'file_validate_extensions' => $videos_extensions,
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
function islandora_youtube_videos_upload_form_submit(array $form, array &$form_state) {
  //DEBUG
  file_put_contents('/home/santiago/114_RADIOINAH/testing.log', print_r(array('islandora_youtube_videos_upload_form_submit'),true));

  global $user;
  //Load needed modules
  module_load_include('inc', 'islandora', 'includes/utilities');
  module_load_include('inc', 'islandora_youtube', 'includes/derivatives');
  //module_load_include('inc', 'islandora_youtube', 'includes/create_tracks');

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
  $videos_txt = file_load($form_state['values']['videos_txt']);

  //Get TN from parent disco object for child track objects
  $width = variable_get('islandora_youtube_thumbnail_width', 200);
  $height = variable_get('islandora_youtube_thumbnail_height', 200);
  
  //Adds OBJ datastream
  $obj->setContentFromFile(drupal_realpath($videos_txt->uri), FALSE);
  if ($obj->label != $videos_txt->filename) {
    $obj->label = $videos_txt->filename;
  }
  if ($obj->mimetype != $videos_txt->filemime) {
    $obj->mimetype = $videos_txt->filemime;
  }

  //Read video urls
  $videos_uri = drupal_realpath($videos_txt->uri);
  $videos_txt = explode("\n", trim(str_replace("\r","",file_get_contents($videos_uri))));
  $files_to_add = array();
  for ($i=0; $i < count($videos_txt) ; $i++) { 
    $files_to_add[$i] = islandora_youtube_getYoutubeData(islandora_youtube_getYoutubeIdFromUrl(trim($videos_txt[$i])));
  }

  $playlist_files = array();
  $object_url = url("islandora/object/{$object->id}", array('absolute' => TRUE));
  $insertion_point = 0;

  //Adds default IMG and TN if they dont exist
  if (!isset($object['IMG'])) {
    islandora_youtube_add_datastream($object, 'IMG', $files_to_add[0]['img'], 'Medium size image');
    //islandora_youtube_add_datastream($object, 'TN', $files_to_add[0]['tn'], 'Thumbnail size image');
  }
  //Default URL ds
  islandora_youtube_add_datastream($object, 'URL', $files_to_add[0]['url'], 'URL of the video in youtube');

  foreach ($files_to_add as $video) {
    $video['num'] = ++$insertion_point;
    $video['str'] = "VIDEO_".$video['num'];
    $playlist_files[$insertion_point]=$video;
    $playlist_files[$insertion_point]['tn']=$object_url.'/datastream/TN_'.$video['num'].'/view';
    
    //Adds VIDEO datastreams (IMG, TN, PLAYLIST)
    islandora_youtube_add_datastream($object, 'IMG_'.$video['num'], $video['img'], 'Medium size image');
    islandora_youtube_add_datastream($object, 'TN_'.$video['num'], $video['tn'], 'Thumbnail size image');
    islandora_youtube_add_datastream($object, 'URL_'.$video['num'], $video['url'], 'URL of the video in youtube', TRUE);
  }

  //Crea e ingesta el playlist DS, luego bora el archivo temporal
  $playlist = islandora_youtube_playlist_create_rss($playlist_files, $object->id);
  islandora_youtube_add_datastream($object, 'PLAYLIST', $playlist);

  //DEBUG
  //file_put_contents('/home/santiago/114_RADIOINAH/testing.log', print_r(array('playlist_files',$playlist_files,'playlist',$playlist),true),FILE_APPEND);

  //Ingest tracks
  /*foreach ($tracks as $track) {
    $repository->ingestObject($track);
  }
  */
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
function islandora_youtube_playlist_create_rss(array $playlist_files, $objectid) {
  $ofile = drupal_get_path('module', 'islandora_youtube') . '/includes/playlist.rss';

  $xml = new SimpleXMLElement($ofile, NULL, TRUE);
  foreach ($playlist_files as $track) {
    $item = $xml->channel->addChild('item');
    $item->addChild('title', (string)$track['title']);
    $item->addChild('description', (string)$track['description']);

    $item->addChild('image', $track['tn'],'http://rss.jwpcdn.com/');

    //DEBUG
    file_put_contents('/home/santiago/114_RADIOINAH/testing.log', print_r(array('track[tn]',$track['tn']),true),FILE_APPEND);

    $source = $item->addChild('source', FALSE,'http://rss.jwpcdn.com/');
    $source->addAttribute('file', $track['url']);
  }

  //Creates temporary rss file
  $file_name = str_replace(":", "-", $objectid) . ".rss";
  $file_uri = file_create_filename($file_name, 'temporary://');
  $file_out = drupal_realpath($file_uri);
  file_put_contents($file_out,$xml->asXML());
  return $file_out;
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
/*function islandora_youtube_img_upload_form(array $form, array &$form_state) {
  $upload_size = min((int) ini_get('post_max_size'), (int) ini_get('upload_max_filesize'));
  $extensions = array('jpg jpeg png');
  return array(
    'img_file' => array(
      '#title' => t('Youtube video or series image'),
      '#type' => 'managed_file',
      '#required' => FALSE,
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
}*/

/**
 * Submit handler, adds uploaded image to ingest object IMG.
 *
 * @param array $form
 *   The Drupal form.
 * @param array $form_state
 *   The Drupal form state.
 */
/*function islandora_youtube_img_upload_form_submit(array $form, array &$form_state) {
  if (!empty($form_state['values']['img_file'])) {
    $object = $form_state['islandora']['objects'][0];
    if (empty($object['IMG'])) {
      $ds = $object->constructDatastream('IMG', 'M');
      $object->ingestDatastream($ds);
    } else {
      $ds = $object['IMG'];
    }
    $img_file = file_load($form_state['values']['img_file']);
    if (is_object($img_file)) {
      $path = drupal_realpath($img_file->uri);
      $ds->setContentFromFile($path, FALSE);
      $ds->label = $img_file->filename;
      $ds->mimetype = $img_file->filemime;
    }
  }
}*/

/**
 * Get Youtube video ID from URL
 *
 * @param string $url
 * @return mixed Youtube video ID or FALSE if not found
 */
function islandora_youtube_getYoutubeIdFromUrl($url) {
    $parts = parse_url($url);
    if(isset($parts['query'])){
        parse_str($parts['query'], $qs);
        if(isset($qs['v'])){
            return $qs['v'];
        }else if(isset($qs['vi'])){
            return $qs['vi'];
        }
    }
    if(isset($parts['path'])){
        $path = explode('/', trim($parts['path'], '/'));
        return $path[count($path)-1];
    }
    return false;
}

function islandora_youtube_getYoutubeData($id){
  $video_feed = file_get_contents("http://gdata.youtube.com/feeds/api/videos/$id");
  $sxml = new SimpleXmlElement($video_feed);

  //set up nodes
  $namespaces = $sxml->getNameSpaces(true);
  $media = $sxml->children($namespaces['media']);
  $yt = $media->children($namespaces['yt']);
  $yt_attrs = $yt->duration->attributes();

  //vars
  $vars = array(
    (array) $sxml->title,
    (array) $sxml->content,
    (array) $media->group->keywords,
    (array) $yt_attrs['seconds'],
  );
  //DEBUG
  //file_put_contents('/home/santiago/114_RADIOINAH/testing.log', print_r(array('vars: '.$id,$vars),true),FILE_APPEND);
  return array(
      'title' => @$vars[0][0],
      'description' => @$vars[1][0],
      'keywords' => @$vars[2][0],
      'length' => @$vars[3][0],
      'url' => 'http://www.youtube.com/watch?v='.$id,
      'img' => 'http://i.ytimg.com/vi/'.$id.'/0.jpg',//<-Big
      'tn' => 'http://i.ytimg.com/vi/'.$id.'/mqdefault.jpg',//mqdefault.jpg<-Medium OR //default.jpg <-Small
      //'img' => base64_encode(file_get_contents('http://i.ytimg.com/vi/'.$id.'/0.jpg')),//<-Big
      //'tn' => base64_encode(file_get_contents('http://i.ytimg.com/vi/'.$id.'/mqdefault.jpg')),//mqdefault.jpg<-Medium OR //default.jpg <-Small
    );
}