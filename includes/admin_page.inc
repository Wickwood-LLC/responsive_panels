<?php
/**
 * @file
 * Admin configuration form and related code.
 */

 /**
  * Menu callback: Displays the module's configuration settings form.
  */
 function responsive_panels_settings($form, $form_state) {
   $form['responsive_panels_load_hidden_admins'] = array(
     '#title' => t('Load Hidden for Admins (recommended)'),
     '#type' => 'radios',
     '#options' => array(
       'no' => t('Do Not Load'),
       'yes' => t('Load Hidden'),
     ),
     '#default_value' => variable_get('responsive_panels_load_hidden_admins', 'yes'),
     '#description' => t('If selected, content will be loaded, but hidden via css for layout purposes.'),
   );
   $form['responsive_panels_load_hidden'] = array(
     '#title' => t('Load Hidden'),
     '#type' => 'radios',
     '#options' => array(
       'no' => t('Do Not Load'),
       'yes' => t('Load Hidden'),
     ),
     '#default_value' => variable_get('responsive_panels_load_hidden', 'no'),
     '#description' => t('If selected, content will be loaded, but hidden via css for development purposes.'),
   );
   $form['responsive_panels_reload'] = array(
     '#title' => t('Auto Reload'),
     '#type' => 'radios',
     '#options' => array(
       'no' => t('Do Not Load'),
       'yes' => t('Auto Reload'),
     ),
     '#default_value' => variable_get('responsive_panels_reload', 'no'),
     '#description' => t('If selected, content will be refreshed on resize if media queries change.  This will annoy users but may be handy for development.'),
   );
   $form['responsive_panels_show_types'] = array(
     '#title' => t('Show Types'),
     '#type' => 'select',
     '#options' => array(
       'all' => t('All Breakpoints'),
       'active' => t('Active Breakpoints'),
       'custom' => t('Custom Breakpoints'),
       'module' => t('Module Defined Breakpoints'),
       'theme' => t('Theme Defined Breakpoints'),
     ),
     '#default_value' => variable_get('responsive_panels_show_types', 'all'),
     '#description' => t('When assigning breakpoints for a pane to be displayed, show the following types of breakpoints. You may want to switch it to custom if you have modules installed that created breakpoints that you do not want listed.'),
   );
   return system_settings_form($form);
 }
