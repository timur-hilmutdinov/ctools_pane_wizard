<?php
/**
 * @file
 * Theme callbacks.
 */

/**
 * Theme callback for pane content.
 */
function theme_universal_pane_render_template($variables) {
  $tokens = array();
  foreach ($variables['field_values'] as $key => $value) {
    $render_callback = 'universal_pane_render_' . $variables['field_settings'][$key]['type'];
    $tokens["!$key"] = $render_callback($value, $variables['field_settings'][$key]);
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
    'attributes' => array('id' => 'field-overview'))) . drupal_render_children($container);
}

/**
 * Render textfield.
 *
 * @param string $value
 *   Actual value.
 * @param array $config
 *   Field config
 *
 * @return string
 *   Rendered value.
 */
function universal_pane_render_textfield($value, $config) {
  return check_plain($value);
}

/**
 * Render textarea.
 *
 * @param string $value
 *   Actual value.
 * @param array $config
 *   Field config
 *
 * @return string
 *   Rendered value.
 */
function universal_pane_render_textarea($value, $config) {
  return check_markup($value, $config['format']);
}

/**
 * Render image.
 *
 * @param int $fid
 *   Image fid.
 * @param array $config
 *   Field config
 *
 * @return string
 *   Rendered value.
 */
function universal_pane_render_image($fid, $config) {
  if (!$image = file_load($fid)) {
    return '';
  }

  $variables = array(
    'path' => $image->uri,
  );
  switch ($config['format']) {
    case '_none':
      $hook = 'image';
      break;

    default:
      $hook = 'image_style';
      $variables['style_name'] = $config['format'];
      break;
  }
  return theme($hook, $variables);
}
