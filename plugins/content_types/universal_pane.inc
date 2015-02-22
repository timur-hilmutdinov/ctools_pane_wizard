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
    '#values' => $conf,
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

  // Validate and submit callbacks for 'Save' button.
  $form['buttons']['finish']['#validate'][] = 'universal_pane_settings_validate';
  $form['buttons']['finish']['#submit'][] = 'universal_pane_settings_submit';
  $form['buttons']['fields'] = array(
    '#type' => 'submit',
    '#value' => t('Fields'),
    '#submit' => array('universal_pane_step_switch'),
    '#weight' => -15,
  );
  $form['buttons']['template'] = array(
    '#type' => 'submit',
    '#value' => t('Template'),
    '#submit' => array('universal_pane_step_switch'),
    '#weight' => -10,
  );
  $form['buttons']['values'] = array(
    '#type' => 'submit',
    '#value' => t('Values'),
    '#submit' => array('universal_pane_step_switch'),
    '#weight' => -5,
  );
  $form['buttons']['#weight'] = -20;

  // Determine step.
  $step = 'values';
  if (!empty($form_state['pane_inner_step'])) {
    $step = $form_state['pane_inner_step'];
  }
  else {
    if (empty($conf['fields'])) {
      $step = 'fields';
    }
  }
  $form['buttons'][$step]['#attributes']['class'][] = 'active';
  $function = 'universal_pane_settings_' . $step;

  return $function($form, $form_state);
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
function universal_pane_settings_values($form, &$form_state) {
  return $form;
}

/**
 * Pane field config.
 */
function universal_pane_settings_fields($form, &$form_state) {
  $conf = $form_state['temp_conf'];

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

  if (!empty($conf['fields'])) {
    foreach ($conf['fields'] as $machine_name => $info) {
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
    'title' => t('Textfield'),
    'formats' => array(
      'text' => t('Text field'),
    ),
  );
  $text_formats = array();
  foreach (filter_formats() as $machine_name => $info) {
    $text_formats[$machine_name] = $info->name;
  }
  $fields['textarea'] = array(
    'title' => t('Textarea'),
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
  $form_state['pane_inner_step'] = 'fields';
}

/**
 * Pane template config.
 */
function universal_pane_settings_template($form, &$form_state) {
  $conf = $form_state['temp_conf'];

  $form['template'] = array(
    '#type' => 'textarea',
    '#title' => t('Template markup'),
    '#description' => t('You may use values of defined fields with tokens.'),
    '#default_value' => empty($conf['template']) ? '' : $conf['template'],
  );

  return $form;
}

/**
 * Validate callback for all pane settings.
 */
function universal_pane_settings_validate($form, &$form_state) {

}

/**
 * Pane config submit callback.
 */
function universal_pane_settings_submit($form, &$form_state) {
  $form_state['conf'] = $form_state['temp_conf'];
}
