jQuery(document).ready(function() {
  jwplayer("mediaplayer").setup({
    playlist: Drupal.settings.islandora_jwplaylist.file,
    //file: Drupal.settings.islandora_jwplaylist.file,
    //image: Drupal.settings.islandora_jwplaylist.thumbnail,
    width: Drupal.settings.islandora_jwplaylist.width,
    height: Drupal.settings.islandora_jwplaylist.height,
	listbar: {
		position: Drupal.settings.islandora_jwplaylist.listbarposition,
		size: Drupal.settings.islandora_jwplaylist.listbarsize
	}
  });
});