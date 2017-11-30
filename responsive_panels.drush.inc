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
  if (isset($configuration['breakpoints'])) {
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
  if ($delete) {
    unset($configuration['breakpoint_pane_render_always']);
  }
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
    panels_clear_cached_content($display);
    foreach ($display->content as &$pane) {
      print dt('Migrating configuration for pane @pid...', array('@pid' => $pane->pid)) . PHP_EOL;
      _responsive_panels_migrate_settings($pane->configuration, $delete);
      // Process mlpanels settings for languages if exists.
      if (isset($pane->configuration['mlpanels'])) {
        foreach ($pane->configuration['mlpanels'] as &$configuration) {
          _responsive_panels_migrate_settings($configuration, $delete);
        }
      }
    }
    panels_save_display($display);
  }
}
