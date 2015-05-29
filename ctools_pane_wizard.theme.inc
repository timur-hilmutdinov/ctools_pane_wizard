<?php
/**
 * @file
 * Theme callbacks.
 */

/**
 * Theme callback for pane content.
 */
function theme_universal_pane_inline_template($variables) {
  $tokens = array();

  foreach ($variables['field_values'] as $key => $value) {
    $render_callback = 'universal_pane_render_' . $variables['field_settings'][$key]['type'];
    $tokens["!$key"] = $render_callback($value, $variables['field_settings'][$key]);
  }

  return format_string($variables['template'], $tokens);
}

/**
 * Theme callback for pane content.
 */
function theme_universal_pane_inline_template_preview($variables) {
  $tokens = array();

  foreach ($variables['field_values'] as $key => $value) {
    if ($variables['field_settings'][$key]['type'] != 'image') {
      $render_callback = 'universal_pane_render_' . $variables['field_settings'][$key]['type'];
      $tokens["!$key"] = $render_callback($value, $variables['field_settings'][$key]);
    }
    else {
      $tokens["!$key"] = t('Image placeholder');
    }
  }

  return format_string($variables['template'], $tokens);
}

/**
 * Theme callback for field config table.
 */
function theme_universal_pane_fields_table(&$variables) {
  $container = &$variables['container'];

  $rows = array();

  foreach (element_children($container) as $key) {
    $field = &$container[$key];

    $row = array();
    $row['label'] = drupal_render($field['label']);
    if (!empty($field['machine_name'])) {
      $row['label'] .= drupal_render($field['machine_name']);
    }
    $row['type'] = drupal_render($field['type']);
    $row['format'] = drupal_render($field['format']);
    $row['required'] = drupal_render($field['required']);
    $rows[$key] = array('data' => $row, 'class' => array('draggable'));
  }

  return theme('table', array(
    'header' => $container['#header'],
    'rows' => $rows,
    'empty' => t('No fields currently exist.'),
    'attributes' => array('id' => 'field-overview'),
  )) . drupal_render_children($container);
}
