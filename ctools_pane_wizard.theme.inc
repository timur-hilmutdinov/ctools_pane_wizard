<?php
/**
 * @file
 * Theme callbacks.
 */

/**
 * Theme callback for pane content.
 */
function theme_universal_pane_render_template($variables) {
  return '';
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
    'attributes' => array('id' => 'field-overview'))) . drupal_render_children($container);
}
