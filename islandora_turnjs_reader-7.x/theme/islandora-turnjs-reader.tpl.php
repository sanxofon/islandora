<?php

/**
 * @file
 *
 * Theme template file for the islandora turn.js Reader.
 */

?>
<div id="islandora-turnjs-reader">
  <iframe class="pdf" webkitallowfullscreen mozallowfullscreen allowfullscreen frameborder="no" width="99%" height="800px" src="<?php print $viewer_url; ?>"><?php print $file_url; ?></iframe>
</div>
