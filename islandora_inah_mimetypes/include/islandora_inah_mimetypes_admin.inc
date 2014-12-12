<?php

function islandora_inah_mimetypes_set_mimes() {
	$mimex = explode(",",variable_get('block_mimetipo_string', t('text/plain')));

	//CONFIGURACIÓN DE MIMES
	//Los types a buscar en OBJ y los DS que pueden estar presentes
	$mims = array(
		array(
			'id' => 'image',
			'orden' => 0,
			'icon' => '12.png',
			'title' => 'Imágenes',
			'types' => array(
				'image/jpeg',
				'image/jpg',
				/*'image/jp2',
				'image/tiff',
				'image/png',*/
			),
			'ds' => array(
				/*'PREVIEW',
				'JPG',
				'IMG',
				'JPG',
				'JP2',*/
			),
		),
		array(
			'id' => 'audio',
			'orden' => 2,
			'icon' => '10.png',
			'title' => 'Audios',
			'types' => array(
				'audio/mpeg',
				'audio/vnd.wave',
				'audio/ogg',
			),
			'ds' => array(
				'MP3',
				'PROXY_MP3',
				'PROXY_OGG',
				'PLAYLIST',
			),
		),
		array(
			'id' => 'video',
			'orden' => 4,
			'icon' => '13.png',
			'title' => 'Videos',
			'types' => array(
				'video/mp4',
				'video/x-matroska',
				'video/ogg',
				'video/quicktime',
				'video/m4v',
			),
			'ds' => array(
				'MP4',
				'OGG',
				'MKV',
				'URL', //Para youtube
			),
		),
		array(
			'id' => 'text',
			'orden' => 6,
			'icon' => '09.png',
			'title' => 'Textos',
			'types' => array(
				'application/pdf',
			),
			'ds' => array(
				'PDF',
				'FULL_TEXT',
				'OCR',
				'HOCR',
			),
		),
		array(
			'id' => 'multi',
			'orden' => 8,
			'icon' => '21.png',
			'title' => 'Multimedia',
			'types' => array(
				'text/plain',
				'application/x-shockwave-flash',
			),
			'cm' => array( //Content model
				'islandora:sp-webpageCModel',
			),
			'ds' => array(
				//'URL',
			),
		),
		array(
			'id' => 'threed',
			'orden' => 10,
			'icon' => '01.png',
			'title' => '3D',
			'types' => array(
				'application/json',
			),
			'ds' => array(
				//'3D',
			),
		),
				//'text/plain',
				//'text/html',
				//'text/xml',
				//'application/rdf+xml',
				//'application/octet-stream',
	);
	$mimes = array();
	$mim_exists = array();
	foreach ($mimex as $mim) {
		foreach ($mims as $v) {
			if (in_array('id',array_keys($v)) && in_array($mim,$v['types']) && !in_array($v['id'], $mim_exists)) {
				$mim_exists[] = $v['id'];
				$mimes[] = $v; 
			}
		}
	}
	usort($mimes, 'islandora_inah_mimetypes_ordenar_mimes');
	//drupal_set_message(str_replace('  ', '&nbsp; ', nl2br(print_r($mimes,true))));
	return $mimes;
}

function islandora_inah_mimetypes_ordenar_mimes($a,$b) {
	if ($a['orden'] == $b['orden']) {
        return 0;
    }
    return ($a['orden'] < $b['orden']) ? -1 : 1;
}

function islandora_inah_mimetypes_get_mimes($repository, $ds='') {
	if (empty($ds)) {
		$dis = '?dis';
	} else {
		$dis = '<info:fedora/*/'.$ds.'>';
	}
	$query = 'SELECT DISTINCT ?mim
	FROM <#ri>
	WHERE {
		?pid <info:fedora/fedora-system:def/view#disseminates> ?dss .
		?dss <fedora-view:disseminationType> '.$dis.' .
		?dss <fedora-view:mimeType> ?mim
	}';
	$ret = array();
	$mimes = $repository->ri->sparqlQuery($query,'unlimited');
	foreach ($mimes as $mim) {
		$ret[] = $mim['mim']['value'];
	}
	return $ret;
}


function islandora_inah_mimetypes_admin($mime) {
	$repo = islandora_inah_mimetypes_connectToFedora();
	$mimes = islandora_inah_mimetypes_set_mimes();
	$objs_ids = islandora_inah_mimetypes_get_objects($mimes,$mime,$repo);

	$mimes_id = array();
	foreach ($mimes as $v) {
		$mimes_id[] = $v['id'];
	}
	//drupal_set_message(str_replace('  ', '&nbsp; ', nl2br(print_r($mimes_id,true))));
	$i=array_search($mime,$mimes_id);
	if (is_int($i)) {
		$output = islandora_inah_mimetypes_print_header($mimes,$mime,$mimes[$i]['icon'],$mimes[$i]['title']);
		$output .= '<div class="clearfix"></div>';
		
		if ($objs_ids && count($objs_ids)>0) {

			$output .= islandora_inah_mimetypes_print_objects($objs_ids);

		} else {


			$output .= '
			<div>No se encontaron resultados</div>
			';
		}
		return $output;
	} else {
		return "No se encontró el tipo mime solicitado: <strong>".$mime."</strong>";
	}
}

function islandora_inah_mimetypes_print_header($mimes,$mime,$icono,$title) {

		/*$output = '
					<h1 id="page-title" class="title" style="float:left;">
						<img align="top" height="32" src="'.$GLOBALS['base_url'].'/sites/all/themes/inah_bartik/images/iconitos/'.$icono.'">
						'.$title.'
					</h1>
		';*/

		/*$output = islandora_inah_mimetypes_mime_block_print($mimes,0);

		return $output;*/

		return '';
}

/**
 * Imprime los objectos recibidos desde el query
 * @param  array $objs_ids Array of islandora object id's
 * @return string $output Output html formatted string
 */
function islandora_inah_mimetypes_print_objects($objs_ids) {

	module_load_include('inc', 'islandora', 'includes/utilities');

	$per_page = 16;
	$current_page = pager_default_initialize(count($objs_ids), $per_page);
	$chunks = array_chunk($objs_ids,$per_page, TRUE);

	$output = '<br><div class="clearfix"></div><br><div align="center">'.theme('pager', array('quantity',count($objs_ids))).'</div>';

	$output .= '
	<div class="clearfix"></div>
	<div>
		<div id="block-system-main" class="block block-system">
			<div class="content">
				<div class="islandora-basic-collection-wrapper">
					<div class="islandora-basic-collection clearfix">

						<div class="islandora islandora-basic-collection">
							<div class="islandora-basic-collection-grid clearfix">
    ';

	foreach ($chunks[$current_page] as $oid) {
		//$obj = islandora_object_load($oid);

		$object_url = url("islandora/object/".$oid[0]);
		$obj_nsid = str_replace(":", "-", $oid[0]);
		$output .= '
        <dl class="islandora-basic-collection-object '.$obj_nsid.'">
          <dt class="islandora-basic-collection-thumb">
            <a href="'.$object_url.'" title="'.$oid[1].'">
              <img typeof="foaf:Image" src="'.$object_url.'/datastream/TN/view" alt="">
            </a>
          </dt>
          <dd class="islandora-basic-collection-caption">
            <a href="'.$object_url.'" title="'.$oid[1].'">
              '.$oid[1].'
            </a>
          </dd>
        </dl>
		';
	}
	
	$output .= '
							</div>
						</div>

					</div>
				</div>
			</div>
		</div>
	</div>
	<div class="clearfix"></div>
    ';

	$output .= '<div align="center">'.theme('pager', array('quantity',count($objs_ids))).'</div>';

	$output .= '<div class="clearfix"></div>';

	return $output;
}

function islandora_inah_mimetypes_connectToFedora(){
 	global $user;
 	$connection = islandora_get_tuque_connection($user);
 	$connection->reuseConnection = TRUE;
 	$repository = $connection->repository;
 	return $repository;
}

function islandora_inah_mimetypes_get_objects($mimes,$mime,$repository) {
	$mimes_id = array();
	foreach ($mimes as $v) {
		$mimes_id[] = $v['id'];
	}
	$i=array_search($mime,$mimes_id);
	if (is_int($i)) {
		$datstrm = @$mimes[$i]['ds']; //datstream id's other than OBJ
		$cmodels = @$mimes[$i]['cm']; //CModels to apply to
		$types = @$mimes[$i]['types']; //Mime types associated to $mime
	} else {
		return false;
	}
	//drupal_set_message(str_replace('  ', '&nbsp; ', nl2br(print_r($types,true))));


	//Datastreams
	$query1 = array();
	for ($i=0; $i < count($datstrm) ; $i++) { 
		$query1[$i] = '$ds <fedora-view:disseminationType> <info:fedora/*/'.$datstrm[$i].'>';
	}
	$query1 = implode(' or ',$query1);

	//MimeTypes
	$query2 = array();
	for ($i=0; $i < count($types) ; $i++) { 
		$query2[$i] = '$ds <fedora-view:mimeType> \''.$types[$i].'\'';
	}
	$query2 = implode(' or ',$query2);

	//Content Models
	$query3 = array();
	for ($i=0; $i < count($cmodels) ; $i++) { 
		$query3[$i] = '$object <fedora-model:hasModel> <info:fedora/'.$cmodels[$i].'>';
	}
	$query3 = implode(' or ',$query3);

	$query = '
       $ds <fedora-view:disseminationType> <info:fedora/*/OBJ>
       ';

    if ($query2!='') {
    	$query .= '
	       and (
	       	'.$query2.'
	       )
		';
	}

	if ($query3!='') {
		$query .= '
			and (
	        	'.$query3.'
	   		)
		';
	}
	if (count($datstrm)>0) { //Si hay datastreams a incluir
		$query = '
	        	and (
		           	'.$query1.'
		           	or (
		            	'.$query.'
	           		)
		        )
			';
	} else { //Si no hay datastreams fuera del OBJ
		$query = '
	        	and 
	        	'.$query;
	}

	$query = '
		select $object $created $model $label from <#ri> where 
		($object <fedora-model:label> $label
		and $object <fedora-model:createdDate> $created
		and $object <fedora-model:hasModel> $model
		and $object <fedora-model:state> <info:fedora/fedora-system:def/model#Active>
		and $object <fedora-view:disseminates> $ds
		'.$query.')
		minus
		$object <fedora-model:hasModel> <info:fedora/islandora:collectionCModel>
		minus
		$object <fedora-model:hasModel> <info:fedora/islandora:transformCModel>
		order by $label asc';
		/*'minus
		$object <fedora-model:hasModel> <info:fedora/islandora:pageCModel>
		minus
		$object <fedora-model:hasModel> <info:fedora/islandora:compoundCModel>
		order by $label asc';*/

	//drupal_set_message(htmlentities($query));

	$ret = array();
	$objects = $repository->ri->itqlQuery($query,'100');
	if (count($objects)<1) return false;
	$ya_existe = array();
	foreach ($objects as $object) {
		if (!in_array($object['object']['value'],$ya_existe)) {
			$ya_existe[]=$object['object']['value'];
			$ret[] = array($object['object']['value'],$object['label']['value'],$object['model']['value'],$object['created']['value']);
		}
	}
	return $ret;
 
}

function islandora_inah_mimetypes_mime_block() {
	$repo = islandora_inah_mimetypes_connectToFedora();
	$mimes = islandora_inah_mimetypes_set_mimes();
	$output = islandora_inah_mimetypes_mime_block_print($mimes,3);
	return $output;
}

function islandora_inah_mimetypes_mime_block_print($mimes,$dividir=0) {
	$tabla = '<div class="mime-block">';//'<table style="width:240px;"><tbody><tr>';
	$i=-1;
	foreach ($mimes as $v) {
		$i++;
		if ($dividir>0 && $i>0 && $dividir<count($mimes) && $i/$dividir == intval($i/$dividir)) {
			$tabla .= '';
		}
		$tabla .= '<div class="colecs_mime"><a class="'.$v['id'].'-icon" href="'.$GLOBALS['base_url'].'/inah/mimetype/arguments/'.$v['id'].'" title="'.$v['title'].'">'.$v['title'].'</a></div>';
	}
	$tabla .= '</div>';
	return $tabla;
}

function islandora_inah_mimetypes_random_block() {
	$repo = islandora_inah_mimetypes_connectToFedora();
	$objects = islandora_inah_mimetypes_random_objects($repo);
	$output = islandora_inah_mimetypes_random_block_print($objects);
	return $output;
}

function islandora_inah_mimetypes_random_block_print($objects) {
	$lis = array();
	foreach ($objects as $oid) {

		//Collection TN
		module_load_include('inc', 'islandora_inah_tabs', 'include/utilities');
		$root_pid = variable_get('islandora_repository_pid', 'islandora:root');
    	$collection_pid = array(false,$oid[0]);
    	while (!$collection_pid[0]) {
    		$collection_pid = islandora_inah_tabs_get_top_collection($collection_pid[1],$root_pid);
    		if (empty($collection_pid)) break;
    	}
    	if (!empty($collection_pid)) {
	    	$collection_url = url("islandora/object/".$collection_pid[1], array('absolute' => TRUE));
			$collection_img = "<img class='islandora-object-collection-img' src='".$collection_url."/datastream/TN/view'>";
			$coleccionObj = islandora_object_load($collection_pid[1]);
			$collectionTitle = $coleccionObj->label;
		} else {
			$collection_img = "<img style='height:20px;margin-left:4px;margin-right:4px;' src='".$GLOBALS['base_url']."/".drupal_get_path('module', 'islandora_inah_tabs')."/images/x.png' align='left'>";
			$collectionTitle = 'Sin colecci&oacute;n';
		}
		$oid[1] = str_replace('"','',$oid[1]);
		$oid[1] = str_replace("'","",$oid[1]);

		$object_url = url("islandora/object/".$oid[0]);
		$obj_nsid = str_replace(":", "-", $oid[0]);

		$thumbnail_url	= $object_url."/datastream/TN/view";
		$object_label	= $oid[1];

		$object_format	= $oid[2];
		$object_type		= $oid[3];
		$object_identifier	= @$oid[4];
		
		$li = '<li><div class="related-object-link">        
					<div class="portfolio-item media">
						<figure>
							<div class="mediaholder">
								<a href="'.$object_url.'">
									<span class="thumbnail" style="background-image:url('.$thumbnail_url.')"></span>
								</a>
			    			</div>
							<figcaption class="item-description meta-data">
			    			<div class="islandora-object-collection-div">'.$collection_img.'</div>
			    			<div class="islandora-object-info">
			    				<div><span>'.$collectionTitle.'</span></div>
								<h5>'.$object_label.'</h5>';
			/*					
		if (!empty($object_type))
			$li .= '<div><span class="label" >'.t('Type').'</span><span>'.$object_type.'</span></div>';
		if (!empty($object_format))
			$li .= '<div><span class="label" >'.t('Format').'</span><span>'.$object_format.'</span></div>';
		if (!empty($object_identifier))
			$li .= '<div><span class="label" >'.t('Identifier').'</span><span>'.$object_identifier.'</span></div>';
		*/	
		$li .= '				</div>
							</figcaption>
						</figure>
					</div>
				</div></li>';
		$lis[] = $li;
	}
	$ul="<ul>".implode("\n",$lis)."</ul>";
	
	$block_view['showbiz'] = array(
		'#type' => 'container',
		'#attributes' => array(
			'class' => array('block_carrusel_inah','islandora-object'),
		),
	);
	$block_view['showbiz']['showbiz-navigation'] = array(
		'#type' => 'markup',
		'#markup' => '<div class="showbiz-navigation">
						<div id="showbiz_left_random_obj" class="sb-navigation-left"><i class="icon-angle-left"></i></div>
						<div id="showbiz_right_random_obj" class="sb-navigation-right"><i class="icon-angle-right"></i></div>
					</div><div class="clearfix"></div>' ,
	);

	$block_view['showbiz']['wrapper'] = array(
		'#type' => 'container',
		'#attributes' => array(
			'class' => array('showbiz'),
			'data-left'		=> '#showbiz_left_random_obj',
			'data-right'	=> '#showbiz_right_random_obj',
		),
		
	);
	
	$block_view['showbiz']['wrapper']['object_list'] = array(
        '#type' => 'markup',
		'#markup' => $ul,
		'#prefix' => '<div class="overflowholder">',
		'#suffix' => '</div><div class="clearfix"></div>'
    );

	
	$module_base = drupal_get_path('module','inah_islandora_search_custom_block');
	
	$url_css = "$module_base/css/default-directory-related-object.css";
	$url_js = "$module_base/js/utils.js";
	$url_js2 = "$module_base/js/utils2.js";
	
	drupal_add_css($url_css,'file');

	if (drupal_is_front_page()) {
		drupal_add_js($url_js2,'file');
	} else {
		drupal_add_js($url_js,'file');
	}

	return 	$block_view;
}

/*function islandora_inah_mimetypes_random_block_print_old($objects) {
	$data = array();
	foreach ($objects as $oid) {
		$object_url = url("islandora/object/".$oid[0]);
		$obj_nsid = str_replace(":", "-", $oid[0]);

		//Collection TN
		module_load_include('inc', 'islandora_inah_tabs', 'include/utilities');
		$root_pid = variable_get('islandora_repository_pid', 'islandora:root');
    	$collection_pid = array(false,$oid[0]);
    	while (!$collection_pid[0]) {
    		$collection_pid = islandora_inah_tabs_get_top_collection($collection_pid[1],$root_pid);
    		if (empty($collection_pid)) break;
    	}
    	if (!empty($collection_pid)) {
	    	$collection_url = url("islandora/object/".$collection_pid[1], array('absolute' => TRUE));
			$collection_img = "<img style='height:20px;margin-left:4px;margin-right:4px;' src='".$collection_url."/datastream/TN/view' align='left'>";
		} else {
			$collection_img = "<img style='height:20px;margin-left:4px;margin-right:4px;' src='".$GLOBALS['base_url']."/".drupal_get_path('module', 'islandora_inah_tabs')."/images/x.png' align='left'>";
		}
		$oid[1] = str_replace('"','',$oid[1]);
		$oid[1] = str_replace("'","",$oid[1]);
		$data[] = array(
			'content' => "<div class='slide_inner'><a title='".$oid[1]."' class='photo_link' href='".$object_url."' style='background-image:url(".$object_url."/datastream/TN/view);'><!--img class='photo' src='".$object_url."/datastream/TN/view' alt='".$oid[1]."'--></a><a class='caption' href='".$object_url."'>".$collection_img.truncate_utf8($oid[1], 18, FALSE, TRUE)."</a></div>",
			'content_button' => "<div class='thumb'><img src='".$object_url."/datastream/TN/view' alt='".$oid[1]."'></div><p>Agile Carousel Place Holder</p>",
		);
	}

	$data = json_encode($data);
	//$agile_path = $GLOBALS['base_url'].'/'.drupal_get_path('module', 'islandora_inah_mimetypes');
	$agilepath = libraries_get_path("agile");
	drupal_add_css($agilepath . "/agile_carousel/agile_carousel.css", array(
      'preprocess' => FALSE,
    ));
    drupal_add_js($agilepath . "/agile_carousel/agile_carousel.min.js", array(
      'preprocess' => FALSE,
    ));
	$output = '
	<div class="slideshow" id="multiple_slides_visible"></div>
    <script type="text/javascript">

        jQuery(document).ready(function () {
            jQuery("#multiple_slides_visible").agile_carousel({
                carousel_data: jQuery.parseJSON( "'.str_replace('"',"\\\"",$data).'"),
                carousel_outer_height: 230,
                carousel_height: 200,
                slide_height: 200,
                carousel_outer_width: 480,
                slide_width: 160,
                number_slides_visible: 3,
                transition_time: 330,
                control_set_1: "previous_button,next_button",
                control_set_2: "group_numbered_buttons",
                persistent_content: "<p class=\'persistent_content\'>Mediateca INAH - Objetos al azar</p>"
            });
        });

    </script>
    ';
	return $output;
}*/

/**
 * Gets a set of random objects
 * @param  object $repository tuque conection to repository
 * @return array $ret an array containing PID [0], Label [1], CModel [2] adn Created [3] of each object found
 */
function islandora_inah_mimetypes_random_objects($repository) {
	$rand = rand(0,9);
	$rand2 = rand(0,45);
	$res_limit = variable_get('block_mimetipo_random', 9);
	$query = '
	SELECT DISTINCT ?pid ?created ?label ?model
	WHERE {
		?pid <fedora-model:label> ?label ;
		<fedora-model:createdDate> ?created ;
		<fedora-model:hasModel> ?model ;
		FILTER (
			regex(str(?pid), \''.$rand.'$\' )
			&&
			?model != <info:fedora/islandora:collectionCModel> 
			&&
			?model != <info:fedora/islandora:compoundCModel> 
			&&
			?model != <info:fedora/islandora:transformCModel>  
			&&
			?model != <info:fedora/islandora:pageCModel>
			&&
			?model != <info:fedora/fedora-system:FedoraObject-3.0>
		) .
	}
	ORDER BY DESC (?created) OFFSET '.$rand2.' LIMIT '.$res_limit.'
	';
	/*
	The line: ?model != <info:fedora/fedora-system:FedoraObject-3.0>
	removes results that does not have a CModel asociated to it
	Maybe removes too much?
	 */
	$objects = $repository->ri->sparqlQuery($query);//,$res_limit);
	$ret = array();
	foreach ($objects as $object) {
		$obj = islandora_object_load($object['pid']['value']);
		if (!isset($obj['TN'])) continue;
		$dc = $obj['DC']->content;
		$dc_object = DublinCore::importFromXMLString($dc);
		$variables = isset($dc_object) ? $dc_object->asArray() : array();
		$formato = '';
		if (!empty($variables['dc:format']['value'])) {
			$formato = $variables['dc:format']['value'];
		}
		$tipo = '';
		if (!empty($variables['dc:type']['value'])) {
			$tipo = $variables['dc:type']['value'];
		}
		$indenti = '';
		if (!empty($variables['dc:identifier']['value'])) {
			$indenti = $variables['dc:identifier']['value'];
		}
		$ret[] = array($object['pid']['value'],$object['label']['value'],$formato,$tipo,$indenti);
	}
	return $ret;
}