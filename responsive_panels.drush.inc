<?php

/**
 * Implements hook_drush_command().
 */
function responsive_panels_drush_command() {
 $items = array();

  $items['rpp-migrate-bpp-settings'] = array(
   'description' => "Migrate settings from Breakpoint Panels module",
   'options' => array(
      'delete' => 'Delete settings of breakpoint panels module. Breakpoint panles module does not have uninstall hook to clean up its settings.',
    ),
  );

 return $items;
}

/**
 * Migrate settings from BPP to RPP.
 */
function _responsive_panels_migrate_settings(&$configuration, $delete) {
  $changed = FALSE;
  if (isset($configuration['breakpoints'])) {
    $changed = TRUE;
    $configuration['rpp_breakpoints'] = $configuration['breakpoints'];
    if ($delete) {
      unset($configuration['breakpoints']);
    }
    if (isset($configuration['breakpoints_group'])) {
      $configuration['rpp_breakpoints_group'] = $configuration['breakpoints_group'];
      if ($delete) {
        unset($configuration['breakpoints_group']);
      }
    }
  }
  if ($delete && isset($configuration['breakpoint_pane_render_always'])) {
    $changed = TRUE;
    unset($configuration['breakpoint_pane_render_always']);
  }
  return $changed;
}

function drush_responsive_panels_rpp_migrate_bpp_settings() {
  $delete  = drush_get_option('delete', FALSE);

  //Migrate configuration settings.
  $load_hidden_admins = variable_get('breakpoint_panels_load_hidden_admins', 'yes');
  if (isset($load_hidden_admins)) {
    variable_set('responsive_panels_load_hidden_admins', $load_hidden_admins);
  }
  $load_hidden = variable_get('breakpoint_panels_load_hidden', 'no');
  if (isset($load_hidden)) {
    variable_set('responsive_panels_load_hidden', $load_hidden);
  }
  $reload = variable_get('breakpoint_panels_breakpoint_reload', 'no');
  if (isset($reload)) {
    variable_set('responsive_panels_reload', $reload);
  }
  $show_types = variable_get('breakpoint_panels_show_types', 'all');
  if (isset($show_types)) {
    variable_set('responsive_panels_show_types', $show_types);
  }

  if ($delete) {
    variable_del('breakpoint_panels_load_hidden_admins');
    variable_del('breakpoint_panels_load_hidden');
    variable_del('breakpoint_panels_breakpoint_reload');
    variable_del('breakpoint_panels_show_types');
  }

  ctools_include('plugins', 'panels');

  $result = db_select('panels_display', 'd')
    ->fields('d', array('did'))
    ->execute();
  while($row = $result->fetch()) {
    $display = panels_load_display($row->did);
    print dt('Processing display with did "@did", storage type "@storage_type" and storage_id "@storage_id" ...', array('@storage_type' => $display->storage_type, '@did' => $display->did, '@storage_id' => $display->storage_id)) . PHP_EOL;
    panels_clear_cached_content($display);
    $changed = FALSE;
    foreach ($display->content as &$pane) {
      print dt('  Processing pane of type "@type" and subtype "@subtype" in panel "@panel" with pid "@pid" ...', array('@type' => $pane->type, '@subtype' => $pane->subtype, '@panel' => $pane->panel, '@pid' => $pane->pid)) . PHP_EOL;
      $changed |= _responsive_panels_migrate_settings($pane->configuration, $delete);
      // Process mlpanels settings for languages if exists.
      if (isset($pane->configuration['mlpanels'])) {
        foreach ($pane->configuration['mlpanels'] as &$configuration) {
          $changed |= _responsive_panels_migrate_settings($configuration, $delete);
        }
      }
    }
    if ($changed) {
      panels_save_display($display);
      print dt('  Display configuration changes has been saved.') . PHP_EOL;
    }
    else {
      print dt('  No changes required.') . PHP_EOL;
    }
    print PHP_EOL;
  }
}
