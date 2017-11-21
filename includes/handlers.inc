<?php

/**
 * Special wrapper delivery callback for ajax_deliver() to handle site offline
 * situation.
 * @param array $page_callback_result
 */
function responsive_panels_ajax_deliver($page_callback_result) {
  if (is_int($page_callback_result)) {
    switch ($page_callback_result) {
      case MENU_SITE_OFFLINE:
        $page_callback_result = array();
    }
  }
  ajax_deliver($page_callback_result);
}

/**
 * Menu handler to serve panel-panes via AJAX.
 *
 * If the pane uses context like with panelizer it can be passed as a base64 encoded serialized
 * array.
 */
function responsive_panels_get_panes() {
  if (empty($_POST['panes'])) {
    // TODO: Proper response.
    return array();
  }

  // Make sure required ctools functions/methods will be available.
  ctools_include('plugins', 'panels');
  ctools_include('content');
  ctools_include('context');
  ctools_include('context-task-handler');
  // Backup existing query.
  $old_q = $_GET['q'];

  $commands = array();
  foreach ($_POST['panes'] as $pane_data) {
    $_GET['q'] = html_entity_decode($pane_data['query']);

    $encoded_context = $pane_data['context'];
    $element_id = $pane_data['id'];

    // TODO: may be able to better accomodate a more universal context support.
    // Check for a context and decode it.
    $context = array();
    if (!empty($encoded_context)) {
      $context = unserialize(base64_decode($encoded_context));
    }
    $task_id = $context['task'];
    $subtask_id = isset($context['subtask']) ? $context['subtask'] : '';

    // TODO: Decide whether we should throw a 404 given that we have not enough
    // context.
    $pid = !empty($context['pid']) ? $context['pid'] : 0;
    $did = !empty($context['did']) ? $context['did'] : 0;

    // Initialize emtpy array if not set.
    if (!isset($context['args'])) {
      $context['args'] = array();
    }

    // Load the page and the task object.
    $task = page_manager_get_task($task_id);
    $subtask = page_manager_get_task_subtask($task, $subtask_id);

    $contexts = ctools_context_handler_get_task_contexts($task, $subtask ? $subtask : array(), $context['args']);

    list($display, $handler) = _responsive_panels_get_display_and_handler($pid, $task, $subtask_id, $contexts, $context['args'], $did);

    // If the pane uses panelizer then we need to load the context of the entity/node being viewed.
    if (!empty($context['panelizer'])) {
      $display->context['panelizer'] = new ctools_context;
      // Load a node for context. This is needed for entity field rendering on a panelizer page.
      if (
        !empty($context['panelizer_argument'])
        && is_numeric($context['panelizer_argument'])
        && !empty($context['panelizer_keyword'])
      ) {
        if (isset($display->args) && is_array($display->args)) {
          $display->args[] = $context['panelizer_argument'];
        }
        else {
          $display->args = array($context['panelizer_argument']);
        }

        if ($context['panelizer_keyword'] == 'node') {
          $display->context['panelizer']->data = node_load($context['panelizer_argument']);
        }
      }
    }

    // Find the pane.
    $pane = $display->content[$pid];

    // Set flag so that we don't rewrite content.
    $pane->ajax_rendering = TRUE;

    if (!empty($context['args'])) {
      $display->context = ctools_context_handler_get_handler_contexts($contexts, $handler);
      $display->args = $context['args'];
    }

    // Find the renderer being used for this pane.
    $renderer = panels_get_renderer_handler($display->renderer, $display);

    // Render the new content.
    $content = $renderer->render_pane_content($pane);

    // Setup the variables array and theme it with the panes_pane template.
    $variables = array(
      'content' => $content,
      'pane' => $pane,
      'display' => $display,
    );

    $replace = theme('panels_pane', $variables);
    $commands[] = ajax_command_replace('#' . $element_id, $replace);
  }
  // Restore original query.
  $_GET['q'] = $old_q;

  $page = array('#type' => 'ajax', '#commands' => $commands);
  return $page;
}
