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
    'description' => t('Separate subtitle for use anywhere on page.'),
    'content_types' => array('subtitle_pane'),
    'render callback' => 'universal_pane_render',
    'edit form' => 'universal_pane_settings_form',
  );
}

/**
 * Pane render callback.
 */
function universal_pane_render($subtype, $conf, $panel_args, $contexts) {
  $pane = new stdClass();
  $pane->module = 'ctools_pane_wizard';
  $pane->delta = empty($conf['delta']) ? 'universal_pane' : $conf['delta'];

  $pane->content = array(
    '#theme' => 'universal_pane_render_template',
    '#field_settings' => $conf['fields'],
    '#template' => $conf['template'],
    '#field_values' => $conf['field_values'],
  );

  return $pane;
}

/**
 * Pane settings form callback.
 */
function universal_pane_settings_form($form, &$form_state) {
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
  $form['buttons']['step_fields'] = array(
    '#type' => 'submit',
    '#value' => t('Fields'),
    '#submit' => array('universal_pane_values_commit', 'universal_pane_step_switch'),
    '#access' => user_access('ctools_pane_wizard configure pane fields'),
    '#weight' => -15,
  );
  $form['buttons']['step_template'] = array(
    '#type' => 'submit',
    '#value' => t('Template'),
    '#submit' => array('universal_pane_values_commit', 'universal_pane_step_switch'),
    '#access' => user_access('ctools_pane_wizard edit pane template'),
    '#weight' => -10,
  );
  $form['buttons']['step_values'] = array(
    '#type' => 'submit',
    '#value' => t('Values'),
    '#submit' => array('universal_pane_values_commit', 'universal_pane_step_switch'),
    '#weight' => -5,
  );
  $form['buttons']['#weight'] = -20;

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
  $form['buttons'][$step]['#attributes']['class'][] = 'active';
  $function = 'universal_pane_settings_' . $step;

  return $function($form, $form_state);
}

/**
 * Commit possible changes to config.
 */
function universal_pane_values_commit($form, &$form_state) {
  $values = $form_state['values'];
  $temp_conf = &$form_state['temp_conf'];
  if (isset($values['fields'])) {
    $fields = $values['fields'];
    unset($fields['universal_pane_new_field']);
    foreach ($fields as $key => $field) {
      $temp_conf['fields'][$key]['label'] = $field['label'];
      $temp_conf['fields'][$key]['format'] = $field['format'];
      $temp_conf['fields'][$key]['required'] = $field['required'];
    }
  }
  if (isset($values['template'])) {
    $temp_conf['template'] = $values['template'];
  }
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
  $form['field_values'] = array(
    '#type' => 'container',
    '#tree' => TRUE,
  );
  foreach ($temp_conf['fields'] as $machine_name => $info) {
    $callback = 'universal_pane_field_widget_' . $info['type'];
    $field = $callback($info);

    $field['#required'] = $info['required'] && !user_access('');
    $field['#title'] = $info['label'];
    $field['#default_value'] = !empty($temp_conf['field_values'][$machine_name]) ? $temp_conf['field_values'][$machine_name] : NULL;

    $form['field_values'][$machine_name] = $field;

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
    '#upload_location' => 'public://',
    '#upload_validators' => array(
      'file_validate_size' => array(1048576),
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
  // @todo: don't forget to make file permanent on submit.
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
    // Remove all formats but selected one.
    $element['format']['format']['#options'] = array_intersect_key($element['format']['format']['#options'], array($element['#format'] => ''));
  }
  return $element;
}

/**
 * Pane field config.
 */
function universal_pane_settings_step_fields($form, &$form_state) {
  $temp_conf = $form_state['temp_conf'];

  $acceptable_fields = universal_pane_field_list();
  $types = array();
  $formats_all = array();
  foreach ($acceptable_fields as $type => $info) {
    $types[$type] = $info['title'];
    $formats_all[$type] = $info['formats'];
  }

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
  $fields['textfield'] = array(
    'title' => t('Text'),
    'formats' => array(
      'text' => t('Text field'),
    ),
  );
  $text_formats = array();
  foreach (filter_formats() as $machine_name => $info) {
    $text_formats[$machine_name] = $info->name;
  }
  $fields['textarea'] = array(
    'title' => t('Long text'),
    'formats' => $text_formats,
  );

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
  return FALSE;
}

/**
 * Validate new field config.
 */
function universal_pane_settings_fields_add_field_validate($form, &$form_state) {
  $new_field = $form_state['values']['fields']['universal_pane_new_field'];
  $properties = array('label', 'type', 'format');
  foreach ($properties as $key) {
    $parents = array('fields', 'universal_pane_new_field', $key);
    if (empty($new_field[$key])) {
      $form_element = drupal_array_get_nested_value($form, $parents);
      form_set_error(implode('][', $parents), t('<strong>%field</strong> required.', array('%field' => $form_element['#title'])));
    }
  }
  // Handle separately since error should be shown when label is filled.
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
  $new_field = $form_state['values']['fields']['universal_pane_new_field'];
  $temp_conf = &$form_state['temp_conf'];

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
  foreach ($temp_conf['fields'] as $machine_name => $info) {
    $tokens['items'][] = t('!!machine_name - field named "%label"', array(
      '!machine_name' => $machine_name,
      '%label' => $info['label'],
    ));
  }

  $form['template'] = array(
    '#type' => 'textarea',
    '#title' => t('Template markup'),
    '#description' => t("You may use values of defined fields by following tokens: \n !token_list", array(
      '!token_list' => theme('item_list', $tokens),
    )),
    '#default_value' => empty($temp_conf['template']) ? '' : $temp_conf['template'],
  );

  return $form;
}

/**
 * Validate callback for all pane settings.
 */
function universal_pane_settings_validate($form, &$form_state) {
  universal_pane_values_commit($form, $form_state);
  $temp_conf = $form_state['temp_conf'];
  if (empty($temp_conf['template'])) {
    form_set_error('buttons][step_template', t('Template can not be empty.'));
  }
  if (empty($temp_conf['fields'])) {
    form_set_error('buttons][step_fields', t('At least one field should be given.'));
  }
}

/**
 * Pane config submit callback.
 */
function universal_pane_settings_submit($form, &$form_state) {
  $form_state['conf'] = $form_state['temp_conf'];
}
