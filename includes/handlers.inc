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
  // TODO: Backup existing query if required and restore after the loop.
  $_GET = $_POST['get_params'];

  $commands = array();
  foreach ($_POST['panes'] as $pane_data) {
    // Check for a context and decode it.
    $passed_context = array();
    if (!empty($pane_data['context'])) {
      $passed_context = unserialize(base64_decode($pane_data['context']));
    }
    $task_id = $passed_context['task'];
    $subtask_id = isset($passed_context['subtask']) ? $passed_context['subtask'] : '';

    // TODO: Decide whether we should throw a 404 given that we have not enough
    // context.
    $pid = !empty($passed_context['pid']) ? $passed_context['pid'] : 0;
    $did = !empty($passed_context['did']) ? $passed_context['did'] : 0;

    // Initialize emtpy array if not set.
    if (!isset($passed_context['args'])) {
      $passed_context['args'] = array();
    }

    // Load the page and the task object.
    $task = page_manager_get_task($task_id);
    $subtask = page_manager_get_task_subtask($task, $subtask_id);

    $contexts = ctools_context_handler_get_task_contexts($task, $subtask ? $subtask : array(), $passed_context['args']);

    list($display, $handler) = _responsive_panels_get_display_and_handler($pid, $task, $subtask_id, $contexts, $passed_context['args'], $did);

    // If the pane uses panelizer then we need to load the context of the entity/node being viewed.
    if (!empty($passed_context['panelizer'])) {
      $display->context['panelizer'] = new ctools_context;
      // Load a node for context. This is needed for entity field rendering on a panelizer page.
      if (
        !empty($passed_context['panelizer_argument'])
        && is_numeric($passed_context['panelizer_argument'])
        && !empty($passed_context['panelizer_keyword'])
      ) {
        if (isset($display->args) && is_array($display->args)) {
          $display->args[] = $passed_context['panelizer_argument'];
        }
        else {
          $display->args = array($passed_context['panelizer_argument']);
        }

        if ($passed_context['panelizer_keyword'] == 'node') {
          $display->context['panelizer']->data = node_load($passed_context['panelizer_argument']);
        }
      }
    }

    // Find the pane.
    $pane = $display->content[$pid];

    // Set flag so that we don't rewrite content.
    $pane->ajax_rendering = TRUE;

    if (!empty($passed_context['args'])) {
      $display->context = ctools_context_handler_get_handler_contexts($contexts, $handler);
      $display->args = $passed_context['args'];
    }

    // Find the renderer being used for this pane.
    $renderer = panels_get_renderer_handler(!empty($passed_context['renderer']) ? $passed_context['renderer'] : $display->renderer, $display);
    // Render the new content.
    $content = $renderer->render_pane($pane);

    $commands[] = ajax_command_replace('#' . $pane_data['id'], $content);
  }

  $page = array('#type' => 'ajax', '#commands' => $commands);
  return $page;
}
