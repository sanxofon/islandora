<?php
function inah_d3_connectToFedora(){
 	global $user;
 	$connection = islandora_get_tuque_connection($user);
 	$connection->reuseConnection = TRUE;
 	$repository = $connection->repository;
 	return $repository;
}
function inah_d3_init() {
	$root_pid = variable_get('islandora_repository_pid', 'islandora:root');
	$query = '
	SELECT ?object ?title
	FROM <#ri>
	WHERE {
	?object <fedora-rels-ext:isMemberOfCollection> <info:fedora/'.$root_pid.'> ;
	    <fedora-model:label> ?title .
	}
	';
	$repository = inah_d3_connectToFedora();
	$result = $repository->ri->sparqlQuery($query,'unlimited');
	$data = array(
		'name' => 'Inicio',
		'children' => inah_d3_getChildren($result,$repository),
	);

	header('Content-Type: application/json');
	echo jsonpp(json_encode($data));
	exit;
}
function inah_d3_getChildren($result,$repository) {
	$nodo = array();
	foreach ($result as $r) {
		$query = '
			SELECT ?object ?title
			FROM <#ri>
			WHERE {
			?object <fedora-rels-ext:isMemberOfCollection> <info:fedora/'.$r['object']['value'].'> ;
			    <fedora-model:label> ?title .
			}
		';
		$result2 = $repository->ri->sparqlQuery($query,'unlimited');
		$children = inah_d3_getChildren($result2,$repository);
		if (!empty($children)) {
			$nodo[] = array(
				'name' => $r['title']['value'],
				'children' => inah_d3_getChildren($result2,$repository),
			);
		} else {
			$nodo[] = array(
				'name' => $r['title']['value'],
			);
		}
	}
	return $nodo;
}
/**
* jsonpp - Pretty print JSON data
*
* In versions of PHP < 5.4.x, the json_encode() function does not yet provide a
* pretty-print option. In lieu of forgoing the feature, an additional call can
* be made to this function, passing in JSON text, and (optionally) a string to
* be used for indentation.
*
* @param string $json The JSON data, pre-encoded
* @param string $istr The indentation string
*
* @link https://github.com/ryanuber/projects/blob/master/PHP/JSON/jsonpp.php
*
* @return string
*/
function jsonpp($json, $istr=' ')
{
$result = '';
for($p=$q=$i=0; isset($json[$p]); $p++)
{
$json[$p] == '"' && ($p>0?$json[$p-1]:'') != '\\' && $q=!$q;
if(!$q && strchr(" \t\n\r", $json[$p])){continue;}
if(strchr('}]', $json[$p]) && !$q && $i--)
{
strchr('{[', $json[$p-1]) || $result .= "\n".str_repeat($istr, $i);
}
$result .= $json[$p];
if(strchr(',{[', $json[$p]) && !$q)
{
$i += strchr('{[', $json[$p])===FALSE?0:1;
strchr('}]', $json[$p+1]) || $result .= "\n".str_repeat($istr, $i);
}
}
return $result;
}