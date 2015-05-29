<?php
/**
 * @file
 * Default template for universal pane.
 *
 * Variables available:
 *  - $fields: Array of fields added from admin interface. Each one is an object, that contains:
 *    - $field->content: The output of the field.
 *    - $field->wrapper_prefix: Opening wrapper tag.
 *    - $field->wrapper_suffix: Closing wrapper tag.
 */
?>

<div class="<?php print $classes; ?>">
  <?php foreach ($fields as $id => $field): ?>
    <?php print $field->wrapper_prefix; ?>
    <?php print $field->content; ?>
    <?php print $field->wrapper_suffix; ?>
  <?php endforeach; ?>
</div>
