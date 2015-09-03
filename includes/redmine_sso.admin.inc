<?php
/**
 * @file
 * admin page inc file used for settings
 */
/**
 * admin form callback
 */
function redmine_sso_admin_settings($form, $form_state) {
  $form = array();
  $settings = variable_get('redmine_sso_settings',redmine_sso_get_default_settings());

  redmine_sso_check_library_dependency();

  // Server Settings
  $form['redmine_sso_server'] = array(
    '#type' => 'fieldset',
    '#title' => t('Redmine Server Settings'),
    '#collapsible' => true,
  );
  // baseurl of redmine
  $form['redmine_sso_server']['url'] = array(
    '#type' => 'textfield',
    '#title' => t('URL'),
    '#default_value' => $settings['url']
  );
  // baseurl of redmine
  $form['redmine_sso_server']['protocol'] = array(
    '#type' => 'select',
    '#title' => t('Protocol'),
    '#description' => t('Be sure before disable ssl protocol. There are no other security mechanism.'),
    '#options' => array(
      'https://' => 'https',
      'http://' => 'http'
    ),
    '#default_value' => $settings['protocol']
  );
  // redmine api key
  $form['redmine_sso_server']['api_key'] = array(
    '#type' => 'textfield',
    '#title' => t('API Key'),
    '#default_value' => $settings['api_key']
  );
  // clientID
  $form['redmine_sso_server']['client_id'] = array(
    '#type' => 'textfield',
    '#title' => t('ClientID'),
    '#default_value' => $settings['client_id']
  );
  // ClientSecret
  $form['redmine_sso_server']['client_secret'] = array(
    '#type' => 'textfield',
    '#title' => t('Client Secret'),
    '#default_value' => $settings['client_secret']
  );

  // SSO Service Behavior
  $form['redmine_sso_behavior'] = array(
    '#type' => 'fieldset',
    '#title' => t('Service Behavior'),
    '#collapsible' => true,
  );
  // User Filter Mode
  $form['redmine_sso_behavior']['user_filter_mode'] = array(
    '#type' => 'radios',
    '#title' => t('User Filter Mode'),
    '#description' => t('Set which users can access this drupal site from redmine.'),
    '#options' => array(
      '' => t('No Filter'),
      'project_filter' => t('Filter by Project'),
      'group_filter' => t('Filter by Group'),
    ),
    '#default_value' => $settings['user_filter_mode']
  );
  // projectID
  $form['redmine_sso_behavior']['project_id'] = array(
    '#type' => 'textfield',
    '#title' => t('Project ID'),
    '#description' => t('Set the project id for allow only these projectmembers.'),
    '#states' => array(
      'visible' => array(
        ':input[name="user_filter_mode"]' => array('value' => 'project_filter'),
      ),
    ),
    '#default_value' => $settings['project_id']
  );
  // GroupID
  $form['redmine_sso_behavior']['group_id'] = array(
    '#type' => 'textfield',
    '#title' => t('Group ID'),
    '#description' => t('Set the project id for allow only these groupmembers.'),
    '#states' => array(
      'visible' => array(
        ':input[name="user_filter_mode"]' => array('value' => 'group_filter'),
      ),
    ),
    '#default_value' => $settings['group_id']
  );
  // Register Behavior
  $form['redmine_sso_behavior']['new_user_access'] = array(
    '#type' => 'checkboxes',
    '#title' => t('New User Access'),
    '#description' => t('Check if new user can login without admin approval.'),
    '#options' => array(
      'unblocked' => t('Unblocked'),
    ),
    '#default_value' => $settings['new_user_access']
  );

  // Asigning Roles
  $form['redmine_sso_behavior']['role_assigning'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Role Assigning'),
    '#description' => t('Check the roles which should be assigned to new redmine sso users.'),
    '#options' => user_roles(true),
    '#default_value' => $settings['role_assigning']
  );
  $form['redmine_sso_behavior']['extras'] = array(
    '#type' => 'fieldset',
    '#title' => t('Service Behavior Extras'),
    '#collapsible' => true,
    '#collapsed' => true,
  );
  // Daily Checkup
  $form['redmine_sso_behavior']['extras']['daily_checkup'] = array(
    '#type' => 'checkbox',
    '#title' => t('Check Daily'),
    '#description' => t('Kills user session after 24h to enforce an active redmine account. Cron must be active.'),
    '#default_value' => $settings['daily_checkup']
  );

  // Disable Login
  $form['redmine_sso_behavior']['extras']['disable_login'] = array(
    '#type' => 'checkbox',
    '#title' => t('Disable Default Login'),
    '#description' => t('Disable default login to allow only redmine sso login.'),
    '#default_value' => $settings['disable_login']
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save configuration')
  );

  return $form;
}

/**
 * validate redmine sso options
 */
function redmine_sso_admin_settings_validate($form, $form_state) {
  /**
   * TODO
   * check url
   * check if group exsists
   * check if project exsists
   */
}
/**
 * save redmine options
 */
function redmine_sso_admin_settings_submit($form, $form_state) {
  $list = redmine_sso_get_setting_keys();
  $settings = array();
  foreach ($list as $key_setting) {
    $settings[$key_setting] = $form_state['values'][$key_setting];
  }
  variable_set('redmine_sso_settings',$settings);

  drupal_set_message(t('Redmine sso configuration saved.'), 'status', FALSE);
}

/**
 * list of setting key
 */
function redmine_sso_get_setting_keys() {
  return array(
    'url',
    'protocol',
    'api_key',
    'client_id',
    'client_secret',
    'user_filter_mode',
    'project_id',
    'group_id',
    'new_user_access',
    'role_assigning',
    'daily_checkup',
    'disable_login'
  );
}