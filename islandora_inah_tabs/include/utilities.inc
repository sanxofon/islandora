<?php

/**
 * Calcula la colección más alta a la que pertenece un objeto.
 * Recursively calculates the highest parent collection of an object, just below root colecction
 * @param  string $object_pid Islandora PID of the object to analize
 * @param  string $root_pid   Top collection (root) to etablish height
 * @return array              Tuple of: TRUE/FALSE "I found root" and the highest collection PID found.
 */
function islandora_inah_tabs_get_top_collection($object_pid,$root_pid) {
  	module_load_include('inc', 'islandora', 'includes/utilities');
  	$object = islandora_object_load($object_pid);
  	if (!$object) return 0;
	$parent_collections = islandora_get_parents_from_rels_ext($object);
  	$collection_pid = 0;
	//Get last collection of object
	$termine = false;
	foreach ($parent_collections as $collection) {
		$collection_pid = $collection->id;
		$parent_object = islandora_object_load($collection_pid);
		$parent_cols = islandora_get_parents_from_rels_ext($parent_object);
		foreach ($parent_cols as $par_col) {
			if ($par_col->id==$root_pid) {
				$termine = true;
				break;
			}
		}
		if ($termine) {
			break;
		}
	}
	return array($termine,$collection_pid);
}

/**
 * Pinta los tabs abajo de cada objeto de manera bastante cruda
 * Adds jquery tabbed metadata to each object -> must be called from islandora.module (LINE 1032 aprox)
 * 
 *   if (!isset($object['COLLECTION_POLICY']) && variable_get('islandora_inah_tabs_show', FALSE)) {
 *     module_load_include('inc', 'islandora_inah_tabs', 'include/utilities');
 *     $output[] = pintarTabs($object, $page_number, $page_size);
 *   }
 *   
 * @param  object $object      Objeto de Islandora actual
 * @param  int    $page_number Optional pager parameters.
 * @param  int    $page_size   Optional pager parameters.
 * @return string              Modified output to print -> self slyled
 */
function pintarTabs($object, $page_number, $page_size) {

  	module_load_include('inc', 'islandora', 'includes/utilities');
  	module_load_include('inc', 'islandora', 'includes/datastream');
  	module_load_include('inc', 'islandora', 'includes/metadata');

    $module_path = url(drupal_get_path('module', 'islandora_inah_tabs'));

	//Collections
	$parent_collections = islandora_get_parents_from_rels_ext($object);
	$collections = '';
	if($parent_collections) {
		$collections .= '<ul>';
    	foreach ($parent_collections as $collection) {
            $pid = $collection->id;
            $img = url('islandora/object/'.$pid.'/datastream/TN/view');
            $href = "islandora/object/$pid";
            $collections .=
				'<li>'
					.l("<img src='$img'>",$href , array('html' => true,'attributes'=> array('class' => 'collection-image')))
            		.l($collection->label, $href)
            	.'</li>';
        }
    	$collections .= '</ul>';

		$root_pid = variable_get('islandora_repository_pid', 'islandora:root');
    	$collection_pid = array(false,$object->id);
    	while (!$collection_pid[0]) {
    		$collection_pid = islandora_inah_tabs_get_top_collection($collection_pid[1],$root_pid);
    		if (empty($collection_pid)) break;
    	}
    	if (empty($collection_pid)) {
			$parent_collections = islandora_get_parents_from_rels_ext(islandora_object_load($object->id));
			$colecciones = array();
			foreach ($parent_collections as $collection) {
				$colecciones[] = '<strong>'.$collection->label.'</strong> ('.$collection->id.')';
			}
    		drupal_set_message('No se encontró el objeto o colección padre: <div>'.implode("<br>",$colecciones).'</div>','warning');
    		//return '';
    	}
    	$collection_pid = $collection_pid[1];

		$object_url = url("islandora/object/{$collection_pid}", array('absolute' => TRUE));
		drupal_set_title('<img src="'.$object_url.'/datastream/TN/view" style="height:32px;float:left;background-color:#cc0000;border-radius:50%;border:3px solid #cc0000;" height="32" align="top">&nbsp; '.$object->label,PASS_THROUGH);
    } else {
		drupal_set_message('No se encontró el objeto o colección padre.','warning');
    }

    //Description
    $description = trim(preg_replace('/<h2>[^>]+<\/h2>/i','',islandora_retrieve_description_markup($object)));

    //Metadata
    $metadata = preg_replace('/<fieldset.*<dl /i','<dl \1',str_replace("\n","",str_replace("\r","",islandora_retrieve_metadata_markup($object))));
    $metadata = preg_replace('/<\/dl>.*/i','</dl><br style="clear:all;" clear="all"/>',$metadata);

    //Datastreams
    $no_show_ds = array(
		'COLLECTION_POLICY',
		'DC',
		'DS-COMPOSITE-MODEL',
		'DS-COMPOSITE-MODEL-SCHEMA',
		'DSINPUTSPEC-1.1-SCHEMA',
		'FULL_TEXT',
		'HOCR',
		'IMG_1',
		'ISLANDORACM',
		'JP2',
		'JPG1',
		'JPG2',
		'METHODMAP-1.1-SCHEMA',
		'METHODMAP-SCHEMA',
		'MODS',
		'OAI_DC-SCHEMA',
		'OCR',
		'ONTOLOGY',
		'OWL-SCHEMA',
		'PDF2',
		'PDF3',
		'PDF4',
		'POLICY-SCHEMA',
		'PREVIEW',
		'RDF-SCHEMA',
		'RDFS-SCHEMA',
		'RELS-EXT',
		'RELS-EXT-SCHEMA',
		'RELS-INT',
		'RELS-INT-SCHEMA',
		'SIMPLEDC-SCHEMA',
		'TN',
		'TN_1',
		'URL',
		'URL_1',
		'WSDL-SCHEMA',
		'XML-SCHEMA',
    );
    $datastreams = array();
    foreach ($object as $ds) {
    	try {
    		$pid = $object->id;
    		$id = $ds->id;
    		if (in_array($id, $no_show_ds)) continue;
    		$label = $ds->label;
    		$download_path = islandora_datastream_get_url($ds, 'download');
    		$view_path = islandora_datastream_get_url($ds, 'view');
    		$datastreams[$id]['id'] = $id;
    		$datastreams[$id]['label'] = $label;
    		//$datastreams[$id]['label_link'] = islandora_datastream_access(ISLANDORA_VIEW_OBJECTS, $ds) ? l($label, $download_path) : $label;
    		$datastreams[$id]['download_url'] = $download_path;
    		$datastreams[$id]['view_url'] = $view_path;
    		$datastreams[$id]['mimetype'] = $ds->mimetype;
    		$datastreams[$id]['size'] = islandora_datastream_get_human_readable_size($ds);
    		$datastreams[$id]['created_date'] = $ds->createdDate->format("Y-m-d");
    		$datastreams[$id]['class'] = drupal_strtolower(preg_replace('/[^A-Za-z0-9]/', '-', $id));
    		//$datastreams[$id]['label_link'] = str_replace('>',' class="inah-tabs">',$datastreams[$id]['label_link']);
    	}
    	catch (RepositoryException $e) {

    	}
    }

    //Dublin Core
    //$dc = utf8_decode($object['DC']->content);
    $dc = $object['DC'];

    //Share
    $pageu = "http://" . $_SERVER["SERVER_NAME"] . $_SERVER["REQUEST_URI"];
    $pageurl = urlencode($pageu);
    $compartir = '
<table border="0" width="100%">
	<tr>
		<td width="30%" style="padding:12px;background:#ffffff;">
			<div style="border-right:1px dotted #404040 !important;">
			<a class="inah-tabs" href="https://www.facebook.com/sharer/sharer.php?u='.$pageurl.'" target="_blank"><img class="inah-tabs" src="'.$module_path.'/images/facebook.png" height="20" align="top"> Facebook</a>
			<br><br>
			<a class="inah-tabs" href="https://twitter.com/home?status='.$pageurl.'" target="_blank"><img class="inah-tabs" src="'.$module_path.'/images/twitter.png" height="20" align="top"> Twitter</a>
			<br><br>
			<a class="inah-tabs" href="https://plus.google.com/share?url'.$pageurl.'" target="_blank"><img class="inah-tabs" src="'.$module_path.'/images/gplus.png" height="20" align="top"> Google+</a>
			</div>
		</td>
		<td width="70%" style="padding:12px;background:#ffffff;">
			
			<a class="inah-tabs" href=
"mailto:example@mail.com?body='.$pageurl.'" target="_blank"><img class="inah-tabs" src="'.$module_path.'/images/mail.png" height="20" align="top"> Enviar link por mail</a>
			<br><br>
			<a class="inah-tabs" href="'.$pageu.'" target="_blank"><img class="inah-tabs" src="'.$module_path.'/images/link.png" height="20" align="top"> Link permanente</a>
			<br>
			<input onclick="this.select();" type="text" size="50" value="'.$pageu.'">
		</td>
	</tr>
</table>';


	drupal_add_library('system', 'ui.tabs');

	drupal_add_js (
		'jQuery(document).ready(function(){
			jQuery("#inah-tabs").tabs();
		});
	' , 'inline' );

	$output = '
	<style>
		div[class$="-metadata"]{display:none;}
		dd[class^="dc-"]{padding-left:36px;}
	</style>
	';

	$output .= '
	<div id="inah-tabs" class="islandora-inline-metadata islandora-metadata-fields islandora-object-fields">
		<ul>';
		$chk_desc = trim(strip_tags($description));
	if (!empty($chk_desc)) 
	$output .= '		<li><a href="#inah-tabs-description">Información</a></li>';
	$output .= '		<li><a href="#inah-tabs-metadata">Datos</a></li>';
	$output .= '		<li><a href="#inah-tabs-files">Archivos</a></li>';
	$output .= '		<li><a href="#inah-tabs-collections">Relaciones</a></li>';
	//$output .= '		<li><a href="#inah-tabs-dc">Dublin Core</a></li>';
	$output .= '		<li><a href="#inah-tabs-share">Compartir</a></li>';
	$output .= '		<li style="float:right;"><a href="#inah-tabs-mimediateca">+ Agregar a Mediateca</a></li>';
	if (isset($object['PDF'])) {
		$pdf_view = theme('islandora_pdf', array('islandora_object' => $object));
		$output .= '	<li><a href="#inah-tabs-pdf">PDF</a></li>';
	}

	$issueCModel = "islandora:journalIssueCModel";
	$discoCModel = "islandora:sp-discoCModel";
	$audioCModel = "islandora:sp-audioCModel";
	$modelos = $object->models;
	$is_issueCModel = false;
	$is_discoCModel = false;
	$is_audioCModel = false;
	$disco_view = '';
	if (in_array($issueCModel, $modelos)) {
		$is_issueCModel = true;
		$isue_view = islandora_inah_tabs_journal_print(islandora_inah_tabs_journal_get_articles($object));
		$output .= '	<li><a href="#inah-tabs-issue">Índice</a></li>';
	} elseif (in_array($discoCModel, $modelos)) {
		$is_discoCModel = true;
		$disco_view = islandora_inah_tabs_disco_print($object); //Disco
	} elseif (in_array($audioCModel, $modelos)) {
		$is_audioCModel = true;
		$audio_view = islandora_inah_tabs_disco_print($object,true); //Audio
	}

	$output .= '	</ul>';
	if (!empty($chk_desc)) {
		$output .= '	<div id="inah-tabs-description">
				<p>';
		if (!empty($disco_view)) {
			$output .= $disco_view;
		} elseif (!empty($audio_view)) {
			$output .= $description;
			$output .= $audio_view;
		} else {
			$output .= $description;
		}

		/*if (isset($object['DC'])) {
			$dc = $object['DC']->content;
			$dc_object = DublinCore::importFromXMLString($dc);
			$dc_array = $dc_object->asArray();
			$output .= $dc_array['dc:description']['value'];
		}*/

		$output .= '
				</p>
			</div>';
	}
	$output .= '<div id="inah-tabs-metadata">';
	$download_path = islandora_datastream_get_url($dc, 'download');
	$output .= '<div align="right" class="download_dublincore" ><a href="'.$GLOBALS['base_url'].'/'.$download_path.'">'.t('XML Dublin Core').'</a></div>';

	$metadata = str_replace("Title","Título",$metadata);
	$metadata = str_replace("Creator","Creador",$metadata);
	$metadata = str_replace("Subject","Tema",$metadata);
	$metadata = str_replace("Description","Descripción",$metadata);
	$metadata = str_replace("Publisher","Editor",$metadata);
	$metadata = str_replace("Contributor","Colaborador",$metadata);
	$metadata = str_replace("Date","Fecha",$metadata);
	$metadata = str_replace("Type","Tipo",$metadata);
	$metadata = str_replace("Format","Formato",$metadata);
	$metadata = str_replace("Identifier","Identificador",$metadata);
	$metadata = str_replace("Source","Fuente",$metadata);
	$metadata = str_replace("Language","Lenguaje",$metadata);
	$metadata = str_replace("Relation","Relación",$metadata);
	$metadata = str_replace("Coverage","Cobertura",$metadata);
	$metadata = str_replace("Rights","Derechos",$metadata);

	$output .= $metadata;
	/*if (isset($object['DC'])) {
		$dc = $object['DC']->content;
		$dc_object = DublinCore::importFromXMLString($dc);
		$dc_array = $dc_object->asArray();
		foreach ($dc_array as $k=>$v) {
		  if (!empty($v['value']) && $k!='dc:description') {
		    $output .= '<li><strong>'.t($v['label']).'</strong>: <p style="padding-left:20px;">'.$v['value'].'</p></li>';
		  }
		}
	}*/

	$output .= '
		</div>
		<div id="inah-tabs-files">';
		$output .= '<table style="width:80%;">';
	/*$output .= '
	  <tr>
	    <!--th style="text-align:left;font-weight:bold;border-bottom:1px solid #dddddd;border-right:1px solid #dddddd;">'.t('ID').'</th-->
	    <th style="text-align:left;font-weight:bold;padding:10px;border-bottom:1px solid #dddddd;border-right:1px solid #dddddd;">'.t('Label').'</th>
	    <th style="text-align:left;font-weight:bold;padding:10px;border-bottom:1px solid #dddddd;border-right:1px solid #dddddd;">'.t('Size').'</th>
	    <!--th style="text-align:left;font-weight:bold;padding:10px;border-bottom:1px solid #dddddd;">'.t('Mimetype').'</th-->
	    <!--th style="text-align:left;font-weight:bold;padding:10px;">'.t('Created').'</th-->
	  </tr>';*/
	foreach($datastreams as $key => $value) {
		$output .= '<tr>
	      <!--td style="border-bottom:1px solid #dddddd;" class="inah-tabs-text">'.$value['id'].'</td-->
	      <td style="border-bottom:1px solid #dddddd;" class="inah-tabs-text"><span class="mime-icon-'.str_replace("/","-",$value['mimetype']).'"></span>'.$value['label'].'</td>
	      <td style="border-bottom:1px solid #dddddd;text-align:right;" class="inah-tabs-text">'.$value['size'].'</td>
	      <td style="border-bottom:1px solid #dddddd;text-align:right;">
	      	<div style="display:inline-block;">';
	    if ($key!='OBJ') {
			$output .= '<div class="inah-tabs-ds">
			      		<a class="view-icon inah-tabs-text" title="ver" target="_blank" href="'.$GLOBALS['base_url'].'/'.$value['view_url'].'">
			      			<span style="margin-left:-3px;">ver</span>
			      		</a>
			      	</div>';
		}
		$output .= '<div class="inah-tabs-ds">
		      		<a class="download-icon inah-tabs-text" title="descargar" href="'.$GLOBALS['base_url'].'/'.$value['download_url'].'">
		      			<span style="margin-left:-17px;">descargar</span>
		      		</a>
		      	</div>
	      	</div>
	      </td>
	      <!--td style="border-bottom:1px solid #dddddd;">'.$value['mimetype'].'</td-->
	      <!--td>'.$value['created_date'].'</td-->
	  </tr>';
	}
	$output .= '</table>';

	$object_url = url("islandora/object/{$object->id}", array('absolute' => TRUE));
	/*foreach ($object as $ds) {
		$output .= '<li><strong><a href="'.$object_url.'/datastream/'.$ds->id.'" target="_blank">'.strtoupper($ds->id).'</a></strong><br>('.$ds->mimetype.')<br></li>';
	}*/

	$output .= '</div>';

	$output .= '<div id="inah-tabs-collections" style="padding:40px;">';

	$output .= $collections;

	$output .= '</div>';
	//$output .= '<div id="inah-tabs-dc">';
	//$output .= '<p style="background: none repeat scroll 0 0 rgba(0, 0, 0, 0.063);padding:15px;padding-top:0px;">'.str_replace("  ","&nbsp; ",nl2br(htmlentities($dc))).'</p>';

	//$output .= '</div>';
	$output .= '<div id="inah-tabs-share">';
	$output .= $compartir;
	$output .= '</div>';

	if (isset($object['PDF'])) {
		$output .= '<div id="inah-tabs-pdf">';
		$output .= $pdf_view;
		$output .= '</div>';
	}
	if ($is_issueCModel) {
		$output .= '<div id="inah-tabs-issue">';
		$output .= $isue_view;
		$output .= '</div>';
	}

	//Mi mediateca
	$output .= '<div id="inah-tabs-mimediateca">';
	$output .= 'Mi mediateca';
	$output .= '</div>';

	$output .= '</div>';

	return $output;
}


function islandora_inah_tabs_disco_print($object,$is_audio=false) {
	if ($is_audio) {
		//get parent compound
		$relationships = $object->relationships;
		$cols = $relationships->get('isMemberOfCollection');
		$col = '';
		foreach ($cols as $c) {
			if ($c['predicate']['value']=='isMemberOfCollection' && $c['object']['value']!='islandora:audio_collection') {
				$col=$c['object']['value'];
			}
		}
		if (!empty($col)) {
			$col_url = url("islandora/object/".$col);
			$object  = islandora_object_load($col);
			$ret = '<hr>
				<a href="'.$col_url.'"><strong>'.$object->label.'</strong></a>
			';
			return $ret;
		}
	}
	if (isset($object['PLAYLIST'])) {
		//get the playlist directly
		$pl_xml = simplexml_load_string($object['PLAYLIST']->content);

		$namespaces = $pl_xml->getNamespaces(true);
		$pl_array = array();
		$i=-1;
		foreach ($pl_xml->{'channel'}->item as $item) {

			$i++;
			foreach ($item as $k => $v) {
				$va = (array) $v;
				if (count($va)>0)
					$pl_array[$i][$k] = trim($va[0],' ;,.\t\r\n');
				else
					$pl_array[$i][$k] = '';
			}
			$ns_rss = $item->children('http://search.yahoo.com/mrss/');
			$ns_rssg = $ns_rss->group->children('http://search.yahoo.com/mrss/');
			$attr = (array) $ns_rssg->content->attributes()->{'url'};
			$pl_array[$i]['url'] = preg_replace('/\/datastream\/.*$/i','',$attr[0]);
		}
		$ret = '<h3>Pistas en este disco:</h3><ol style="margin:24px;">';
		foreach ($pl_array as $p) {
			$ret .= '
				<li>
					<a href="'.$p['url'].'"><strong>'.preg_replace('/^[0-9\- \.]+/i','',$p['title']).'</strong></a> <p><em>'.$p['description'].'</em></p>
				</li>
			';
		}
		$ret .= '</ol>';
		return $ret;
	} else return false;
}

function islandora_inah_tabs_journal_print($article_index) {
	//<h2>'.t('Content Table').'</h2>
   
	$ret = '
    <ul class="islandora-journal-article-index">
    ';
    foreach ($article_index as $article_item) {
    	$page = explode('-', $article_item['pages']);
			$ret .= '
        <li class="page-select" page="'.$page[0].'">'.$article_item['label'] .' (pp '. $article_item['pages'] .')</li>
        ';
    }
    $ret .= '
    </ul>
    ';
    return $ret;
}

function islandora_inah_tabs_journal_get_articles(AbstractObject $object) {
  $query = <<<EOQ
PREFIX islandora-rels-ext: <http://islandora.ca/ontology/relsext#>
PREFIX fedora-rels-ext: <info:fedora/fedora-system:def/relations-external#>
SELECT ?object ?label ?pages
FROM <#ri>
WHERE {
  ?object fedora-rels-ext:isMemberOf <info:fedora/{$object->id}> ;
       <fedora-model:hasModel> <info:fedora/islandora:journalArticleCModel> ;
       <fedora-model:label> ?label .
  ?object islandora-rels-ext:isPageOf ?pages
  !optionals
  !filters
}
EOQ;
//drupal_set_message( $query );
  // XXX: Can't really use the fully featured query_filter and query_statements
  // hook as it will return some things we don't want. Perhaps change the return
  // structure in the future to specify which module they are coming from? For
  // now we will just get XACML's directly.
  $query_optionals = (array) module_invoke('islandora_xacml_api', 'islandora_basic_collection_get_query_optionals', 'view');
  $query_optionals[] = '?object islandora-rels-ext:dateIssued ?issued';
  $query_filters = (array) module_invoke('islandora_xacml_api', 'islandora_basic_collection_get_query_filters');

  $filter_map = function ($filter) {
    return "FILTER($filter)";
  };

  $query = format_string($query, array(
    '!optionals' => !empty($query_optionals) ? ('OPTIONAL {{' . implode('} UNION {', $query_optionals) . '}}') : '',
    '!filters' => implode(' ', array_map($filter_map, $query_filters)),
  ));

  $results = $object->repository->ri->sparqlQuery($query);
  // Map the results using a default Datetime for missing issued dates.
  $map_results = function($o) {
    try {
      @$issued = new DateTime($o['issued']['value']);
    }
    catch (Exception $e) {
      // Use the current time as a place holder.
      $issued = new DateTime();
      $msg  = 'Failed to get issued date from SPARQL query for @pid';
      $vars = array('@pid' => $o['object']['value']);
      watchdog_exception('islandora_journal', $e, $msg, $vars, WATCHDOG_ERROR);
    }
    return array(
      'obj' => $o,
      'pid' => $o['object']['value'],
      'label' => $o['label']['value'],
      'pages' => $o['pages']['value'],
      //'sequence' => $o['sequence']['value'],
      //'issued' => $issued,
    );
  };
  $issues = array_map($map_results, $results);
  // Grab the PIDs...
  $get_pid = function($o) {
    return $o['pid'];
  };
  $pids = array_map($get_pid, $issues);
  // Make the PIDs the keys.
  return count($pids) ? array_combine($pids, $issues) : array();
}