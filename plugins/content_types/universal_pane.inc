<?php
/**
 * @file
 * Plugin definition.
 */

/**
 * Implements hook_ctools_content_types().
 */
function ctools_pane_wizard_universal_pane_ctools_content_types() {
  return array(
    'title' => t('Universal pane'),
    'single' => TRUE,
    'category' => t('Page elements'),
    'description' => t('Custom content pane with configurable structure.'),
    'content_types' => array('universal_pane'),
    'render callback' => 'universal_pane_render',
    'edit form' => 'universal_pane_settings_form',
    'admin title' => 'universal_pane_admin_title',
    'admin info' => 'universal_pane_admin_info',
    'defaults' => array(
      'admin_title' => '',
      'delta' => '',
      'fields' => array(),
      'field_values' => array(),
      'template' => '',
      'use_template_file' => FALSE,
    ),
  );
}

/**
 * Administrative title callback.
 *
 * @param string $subtype
 *   Pane subtype.
 * @param array $conf
 *   Pane config.
 *
 * @return string
 *   Title value.
 */
function universal_pane_admin_title($subtype, $conf) {
  return empty($conf['admin_title']) ? t('Universal pane') : $conf['admin_title'];
}

/**
 * Administrative info callback, renders preview.
 *
 * @param string $subtype
 *   Pane subtype.
 * @param array $conf
 *   Pane config.
 *
 * @return object
 *   Pane admin info.
 */
function universal_pane_admin_info($subtype, $conf) {
  $output = new stdClass();
  $output->title = t('Preview content');

  $output->content = theme('universal_pane_inline_template_preview', array(
    'field_settings' => $conf['fields'],
    'field_values' => $conf['field_values'],
    'template' => $conf['template'],
  ));

  return $output;
}

/**
 * Pane render callback.
 */
function universal_pane_render($subtype, $conf, $panel_args, $contexts) {
  $pane = new stdClass();
  $pane->module = 'ctools_pane_wizard';
  $pane->delta = empty($conf['delta']) ? 'universal_pane' : $conf['delta'];

  if ($conf['use_template_file']) {
    $fields = array();

    foreach ($conf['field_values'] as $key => $value) {
      $classes = array(
        'universal-pane-field',
        'field-' . str_replace('_', '-', $key),
      );
      $render_callback = 'universal_pane_render_' . $conf['fields'][$key]['type'];
      $fields[$key] = new stdClass();
      $fields[$key]->content = $render_callback($value, $conf['fields'][$key]);
      $fields[$key]->wrapper_prefix = '<div class="' . implode(' ', $classes) . '">';
      $fields[$key]->wrapper_suffix = '</div>';
    }

    $pane->content = array(
      '#theme' => 'universal_pane',
      '#fields' => $fields,
    );
  }
  else {
    $pane->content = array(
      '#theme' => 'universal_pane_inline_template',
      '#field_settings' => $conf['fields'],
      '#field_values' => $conf['field_values'],
      '#template' => $conf['template'],
    );
  }

  return $pane;
}

/**
 * Pane settings form callback.
 *
 * Navigates through steps of config.
 */
function universal_pane_settings_form($form, &$form_state) {
  // This line removes errors upon file upload.
  // @see http://drupal.stackexchange.com/questions/59467/ctools-custom-content-type-plugin-with-image-field
  // @see http://drupal.stackexchange.com/questions/39818/ajax-calls-in-ctools-content-type-plugin/126994#126994
  ctools_form_include($form_state, 'universal_pane', 'ctools_pane_wizard', 'plugins/content_types');

  $form['#attached']['css'][] = drupal_get_path('module', 'ctools_pane_wizard') . '/css/universal_pane.admin.css';

  // Hide redundant elements like 'override title'.
  foreach (element_children($form) as $key) {
    $form[$key]['#access'] = $key == 'buttons';
  }

  // Initialize temporary config from original one.
  if (empty($form_state['temp_conf'])) {
    $form_state['temp_conf'] = $form_state['conf'];
  }
  $temp_conf = $form_state['temp_conf'];

  // Validate and submit callbacks for 'Save' button.
  $form['buttons']['return']['#validate'][] = 'universal_pane_settings_validate';
  $form['buttons']['return']['#submit'][] = 'universal_pane_settings_submit';
  $form['buttons']['#weight'] = 100;

  // Put buttons permission into variables to avoid redundant calls.
  $access_fields = user_access('ctools_pane_wizard configure pane fields');
  $access_template = user_access('ctools_pane_wizard edit pane template');

  // Populate step buttons.
  $form['steps'] = array(
    '#type' => 'container',
    '#access' => $access_fields || $access_template,
  );
  $form['steps']['description'] = array(
    '#type' => 'item',
    '#prefix' => '<em>',
    '#markup' => t('* Changes are committed on step change.'),
    '#suffix' => '</em>',
  );
  $form['steps']['step_fields'] = array(
    '#type' => 'submit',
    '#value' => t('Fields'),
    '#submit' => array('universal_pane_values_commit', 'universal_pane_step_switch'),
    '#access' => $access_fields,
    '#weight' => -15,
  );
  $form['steps']['step_template'] = array(
    '#type' => 'submit',
    '#value' => t('Template'),
    '#submit' => array('universal_pane_values_commit', 'universal_pane_step_switch'),
    '#access' => $access_template,
    '#weight' => -10,
  );
  $form['steps']['step_values'] = array(
    '#type' => 'submit',
    '#value' => t('Values'),
    '#submit' => array('universal_pane_values_commit', 'universal_pane_step_switch'),
    '#access' => $access_fields || $access_template,
    '#weight' => -5,
  );
  $form['steps']['#weight'] = -20;

  // Determine step.
  $step = 'step_values';
  if (!empty($form_state['pane_inner_step'])) {
    $step = $form_state['pane_inner_step'];
  }
  else {
    if (empty($temp_conf['fields'])) {
      $step = 'step_fields';
    }
  }
  $form['steps'][$step]['#attributes']['class'][] = 'active';
  $function = 'universal_pane_settings_' . $step;

  // Update pane title.
  $step_titles = array(
    'step_fields' => t('fields'),
    'step_values' => t('values'),
    'step_template' => t('template'),
  );
  $admin_title = empty($temp_conf['admin_title']) ? $form_state['plugin']['title'] : $temp_conf['admin_title'];
  $form_state['title'] = t('Configure %title !step.', array(
    '%title' => $admin_title,
    '!step' => $step_titles[$step],
  ));

  return $function($form, $form_state);
}

/**
 * Commit possible changes to config.
 *
 * Handles all possible steps.
 */
function universal_pane_values_commit($form, &$form_state) {
  $values = $form_state['values'];
  $temp_conf = &$form_state['temp_conf'];

  // Fields step.
  if (isset($values['fields'])) {
    $fields = $values['fields'];
    unset($fields['universal_pane_new_field']);
    foreach ($fields as $key => $field) {
      $temp_conf['fields'][$key]['label'] = $field['label'];
      $temp_conf['fields'][$key]['format'] = $field['format'];
      $temp_conf['fields'][$key]['required'] = $field['required'];
    }
  }
  if (isset($values['admin_title'])) {
    $temp_conf['admin_title'] = $values['admin_title'];
  }
  if (isset($values['delta'])) {
    $temp_conf['delta'] = $values['delta'];
  }

  // Template step.
  if (isset($values['template'])) {
    $temp_conf['template'] = $values['template'];
  }

  if (isset($values['use_template_file'])) {
    $temp_conf['use_template_file'] = $values['use_template_file'];
  }

  // Values step.
  if (isset($values['field_values'])) {
    foreach ($values['field_values'] as $key => $value) {
      // Separate save callback for each value type
      // since result structure may differ.
      $function = 'universal_pane_field_save_' . $temp_conf['fields'][$key]['type'];
      $temp_conf['field_values'][$key] = $function($value);
    }
  }
}

/**
 * Pane settings form step switcher.
 */
function universal_pane_step_switch($form, &$form_state) {
  $form_state['rebuild'] = TRUE;
  $form_state['pane_inner_step'] = end($form_state['clicked_button']['#parents']);
}

/**
 * Pane field values config.
 */
function universal_pane_settings_step_values($form, &$form_state) {
  $temp_conf = $form_state['temp_conf'];

  if (empty($temp_conf['fields'])) {
    drupal_set_message(t('No fields added yet. Proceed to %fields step.', array('%fields' => t('Fields'))), 'error');
  }
  else {
    $form['field_values'] = array(
      '#type' => 'container',
      '#tree' => TRUE,
    );

    foreach ($temp_conf['fields'] as $machine_name => $info) {
      $callback = 'universal_pane_field_widget_' . $info['type'];
      $field = $callback($info);

      // Allow not to fill required fields during pane development.
      $field['#required'] = $info['required'] && !user_access('ctools_pane_wizard ignore required fields');
      $field['#title'] = $info['label'];
      $field['#default_value'] = !empty($temp_conf['field_values'][$machine_name]) ? $temp_conf['field_values'][$machine_name] : NULL;

      $form['field_values'][$machine_name] = $field;
    }
  }

  return $form;
}

/**
 * Generate renderable array for text field widget.
 *
 * @param array $info
 *   Field config.
 *
 * @return array
 *   Basic setup of field widget.
 */
function universal_pane_field_widget_textfield($info) {
  $field = array(
    '#type' => 'textfield',
  );

  return $field;
}

/**
 * Process textfield value before save.
 *
 * @param string $value
 *   Value from form_state.
 *
 * @return string
 *   Processed value.
 */
function universal_pane_field_save_textfield($value) {
  return $value;
}

/**
 * Generate renderable array for textarea field widget.
 *
 * @param array $info
 *   Field config.
 *
 * @return array
 *   Basic setup of field widget.
 */
function universal_pane_field_widget_textarea($info) {
  $field = array(
    '#type' => 'text_format',
    '#base_type' => 'textarea',
    '#format' => $info['format'],
    '#after_build' => array('universal_pane_field_widget_textarea_after_build'),
  );

  return $field;
}

/**
 * Process textarea value before save.
 *
 * @param array $value
 *   Value from form_state.
 *
 * @return string
 *   Processed value.
 */
function universal_pane_field_save_textarea($value) {
  return $value['value'];
}

/**
 * Generate renderable array for image field widget.
 *
 * @param array $info
 *   Field config.
 *
 * @return array
 *   Basic setup of field widget.
 */
function universal_pane_field_widget_image($info) {
  $field = array(
    '#type' => 'managed_file',
    '#upload_location' => file_default_scheme() . '://',
    '#upload_validators' => array(
      'file_validate_size' => array(parse_size(file_upload_max_size())),
      'file_validate_extensions' => array('png gif jpg jpeg'),
    ),
  );

  return $field;
}

/**
 * Process image value before save.
 *
 * @param int $value
 *   Value from form_state.
 *
 * @return string
 *   Processed value.
 */
function universal_pane_field_save_image($value) {
  return $value;
}

/**
 * After build callback for textarea, hide format selection.
 */
function universal_pane_field_widget_textarea_after_build($element, $form_state) {
  // Check whether format element group is already added.
  if (!empty($element['format'])) {
    $element['format']['format']['#title_display'] = 'invisible';
    $element['format']['format']['#attributes']['style'][] = 'display: none;';

    // Remove all formats except selected one.
    $element['format']['format']['#options'] = array_intersect_key($element['format']['format']['#options'], array($element['#format'] => ''));
  }

  return $element;
}

/**
 * Pane field config.
 */
function universal_pane_settings_step_fields($form, &$form_state) {
  $temp_conf = $form_state['temp_conf'];

  // Administrative components.
  $form['admin']['admin_title'] = array(
    '#type' => 'textfield',
    '#title' => t('Administrative title'),
    '#default_value' => $temp_conf['admin_title'],
    '#description' => t('Use this field to modify title displayed on panel overview.<br/> Machine name will be used as pane delta, so it is possible to alter specific pane.'),
  );
  $form['admin']['delta'] = array(
    '#type' => 'machine_name',
    '#title' => t('Machine name'),
    '#title_display' => 'invisible',
    '#machine_name' => array(
      'exists' => 'universal_pane_admin_name_check',
      'source' => array('admin', 'admin_title'),
    ),
    '#default_value' => $temp_conf['delta'],
    '#required' => FALSE,
  );

  $acceptable_fields = universal_pane_field_list();
  $types = array();
  $formats_all = array();
  foreach ($acceptable_fields as $type => $info) {
    $types[$type] = $info['title'];
    $formats_all[$type] = $info['formats'];
  }

  $form['fields_description'] = array(
    '#type' => 'item',
    '#prefix' => '<label>',
    '#markup' => t('Fields'),
    '#suffix' => '</label>',
  );

  $form['fields'] = array(
    '#type' => 'container',
    '#tree' => TRUE,
    '#title' => t('Fields'),
    '#theme' => 'universal_pane_fields_table',
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#header' => array(
      'label' => t('Label'),
      'type' => t('Type'),
      'format' => t('Format'),
      'required' => t('Required'),
    ),
  );

  // Settings for existing fields.
  if (!empty($temp_conf['fields'])) {
    foreach ($temp_conf['fields'] as $machine_name => $info) {
      $form['fields'][$machine_name]['label'] = array(
        '#type' => 'textfield',
        '#default_value' => $info['label'],
        '#required' => TRUE,
      );
      $form['fields'][$machine_name]['type'] = array(
        '#type' => 'markup',
        '#markup' => $info['type'],
      );
      $form['fields'][$machine_name]['format'] = array(
        '#type' => 'select',
        '#options' => $acceptable_fields[$info['type']]['formats'],
        '#default_value' => $info['format'],
        '#required' => TRUE,
      );
      $form['fields'][$machine_name]['required'] = array(
        '#type' => 'checkbox',
        '#default_value' => $info['required'],
      );
    }
  }

  // New field settings.
  $form['fields']['universal_pane_new_field'] = array();
  $form['fields']['universal_pane_new_field']['label'] = array(
    '#type' => 'textfield',
    '#title' => t('New field label'),
    '#title_display' => 'invisible',
  );
  $form['fields']['universal_pane_new_field']['machine_name'] = array(
    '#type' => 'machine_name',
    '#title' => t('New field name'),
    '#title_display' => 'invisible',
    '#machine_name' => array(
      'exists' => 'universal_pane_machine_name_check',
      'source' => array('fields', 'universal_pane_new_field', 'label'),
    ),
    '#required' => FALSE,
  );
  $form['fields']['universal_pane_new_field']['type'] = array(
    '#type' => 'select',
    '#title' => t('Type of new field'),
    '#title_display' => 'invisible',
    '#empty_option' => t('- Select a field type -'),
    '#options' => $types,
    '#attributes' => array('class' => array('field-type-select')),
  );
  $form['fields']['universal_pane_new_field']['format'] = array(
    '#type' => 'select',
    '#title' => t('Format for new field'),
    '#title_display' => 'invisible',
    '#empty_option' => t('- Select a format -'),
    '#options' => $formats_all,
    '#attributes' => array('class' => array('widget-type-select')),
    '#attached' => array(
      'js' => array(
        drupal_get_path('module', 'field_ui') . '/field_ui.js',
        array(
          'data' => array('fieldWidgetTypes' => $formats_all),
          'type' => 'setting',
        ),
      ),
    ),
  );
  $form['fields']['universal_pane_new_field']['required'] = array(
    '#type' => 'checkbox',
  );

  $form['add_field'] = array(
    '#type' => 'submit',
    '#value' => t('Add field'),
    '#validate' => array('universal_pane_settings_fields_add_field_validate'),
    '#submit' => array('universal_pane_settings_fields_add_field_submit'),
  );

  return $form;
}

/**
 * Generate array of available fields.
 *
 * @return array
 *   key => setting array of available field types.
 */
function universal_pane_field_list() {
  $fields = array();

  // Single-line text field.
  $fields['textfield'] = array(
    'title' => t('Text'),
    'formats' => array(
      'text' => t('Text field'),
    ),
  );

  // Text area with formatting.
  $text_formats = array();
  foreach (filter_formats() as $machine_name => $info) {
    $text_formats[$machine_name] = $info->name;
  }
  $fields['textarea'] = array(
    'title' => t('Long text'),
    'formats' => $text_formats,
  );

  // Image with imagestyle.
  $image_formats = array(
    '_none' => t('Original image'),
  );
  foreach (image_styles() as $machine_name => $settings) {
    $image_formats[$machine_name] = $settings['label'];
  }
  $fields['image'] = array(
    'title' => t('Image'),
    'formats' => $image_formats,
  );

  return $fields;
}

/**
 * Check if field's machine-name is unique.
 */
function universal_pane_machine_name_check($name, $element, $form_state) {
  return array_key_exists($name, $form_state['temp_conf']['fields']);
}

/**
 * Dummy function for 'exists' callback of pane machine name field.
 */
function universal_pane_admin_name_check($name, $element, $form_state) {
  return FALSE;
}

/**
 * Validate new field config.
 */
function universal_pane_settings_fields_add_field_validate($form, &$form_state) {
  $new_field = $form_state['values']['fields']['universal_pane_new_field'];

  // Validate field properties.
  $properties = array('label', 'type', 'format');
  foreach ($properties as $key) {
    $parents = array('fields', 'universal_pane_new_field', $key);
    if (empty($new_field[$key])) {
      $form_element = drupal_array_get_nested_value($form, $parents);
      form_set_error(implode('][', $parents), t('<strong>%field</strong> required.', array('%field' => $form_element['#title'])));
    }
  }

  // Handle machine name separately
  // since error should be shown when label is filled.
  if (!empty($new_field['label']) && empty($new_field['machine_name'])) {
    $parents = array('fields', 'universal_pane_new_field', 'machine_name');
    $form_element = drupal_array_get_nested_value($form, $parents);
    form_set_error(implode('][', $parents), t('<strong>%field</strong> required.', array('%field' => $form_element['#title'])));
  }
}

/**
 * Commit new field to pane config.
 */
function universal_pane_settings_fields_add_field_submit($form, &$form_state) {
  $temp_conf = &$form_state['temp_conf'];

  $new_field = $form_state['values']['fields']['universal_pane_new_field'];
  $machine_name = $new_field['machine_name'];

  $temp_conf['fields'][$machine_name] = array(
    'label' => $new_field['label'],
    'type' => $new_field['type'],
    'format' => $new_field['format'],
    'required' => $new_field['required'],
  );

  // Clean up input array from processed values
  // to show 'New field' elements empty on refreshed form.
  unset($form_state['input']['fields']['universal_pane_new_field']);

  $form_state['rebuild'] = TRUE;
  $form_state['pane_inner_step'] = 'step_fields';
}

/**
 * Pane template config.
 */
function universal_pane_settings_step_template($form, &$form_state) {
  $temp_conf = $form_state['temp_conf'];

  $tokens = array(
    'title' => '',
    'type' => 'ul',
    'items' => array(),
  );

  // Fill tokens based on previously defined fields.
  foreach ($temp_conf['fields'] as $machine_name => $info) {
    $tokens['items'][] = t('!!machine_name - field named "%label"', array(
      '!machine_name' => $machine_name,
      '%label' => $info['label'],
    ));
  }

  $form['use_template_file'] = array(
    '#type' => 'checkbox',
    '#title' => t('Use template file'),
    '#description' => 'Check to use file instead of inline template.',
    '#default_value' => $temp_conf['use_template_file'],
  );

  $delta_suggestion = '';
  if (!empty($temp_conf['delta'])) {
    $delta_suggestion = '--' . str_replace('_', '-', $temp_conf['delta']);
  }

  $form['template_name'] = array(
    '#type' => 'item',
    '#title' => t('Template name:'),
    '#markup' => 'universal-pane' . $delta_suggestion . '.tpl.php',
    '#prefix' => '<em>',
    '#suffix' => '</em>',
    '#states' => array(
      'invisible' => array(
        ':input[name="use_template_file"]' => array('checked' => FALSE),
      ),
    ),
  );

  $form['template_default'] = array(
    '#type' => 'item',
    '#title' => t('This is the default theme template:'),
    '#markup' => '<pre>' . check_plain(file_get_contents(drupal_get_path('module', 'ctools_pane_wizard') . '/universal-pane.tpl.php')) . '</pre>',
    '#states' => array(
      'invisible' => array(
        ':input[name="use_template_file"]' => array('checked' => FALSE),
      ),
    ),
  );

  $form['template'] = array(
    '#type' => 'textarea',
    '#title' => t('Template markup'),
    '#description' => t("You may use values of defined fields by following tokens: \n !token_list", array(
      '!token_list' => theme('item_list', $tokens),
    )),
    '#default_value' => $temp_conf['template'],
    '#states' => array(
      'invisible' => array(
        ':input[name="use_template_file"]' => array('checked' => TRUE),
      ),
    ),
  );

  return $form;
}

/**
 * Validate callback for all pane settings.
 */
function universal_pane_settings_validate(&$form, &$form_state) {
  universal_pane_values_commit($form, $form_state);

  $temp_conf = $form_state['temp_conf'];

  if (empty($temp_conf['template'])) {
    form_set_error('buttons][step_template', t('Template can not be empty.'));
    $form['steps']['step_template']['#attributes']['class'][] = 'error';
  }

  if (empty($temp_conf['fields'])) {
    form_set_error('buttons][step_fields', t('At least one field should be added.'));
    $form['steps']['step_fields']['#attributes']['class'][] = 'error';
  }
}

/**
 * Pane config submit callback.
 */
function universal_pane_settings_submit($form, &$form_state) {
  global $user;
  $form_state['conf'] = $form_state['temp_conf'];

  foreach ($form_state['conf']['fields'] as $key => $info) {
    if ($info['type'] == 'image' && !empty($form_state['conf']['field_values'][$key])) {
      // Register file usage to make it permanent.
      $file = file_load($form_state['conf']['field_values'][$key]);
      file_usage_add($file, 'ctools_pane_wizard', 'user', $user->uid);
    }
  }
}
