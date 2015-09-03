<?php
/**
 * @file
 * collection of helper files
 */

/**
 * check if library is installed
 */
function redmine_sso_check_library_dependency() {
  $library_info = libraries_detect(OAUTHLIB);
  if(!$library_info['installed']) {
    drupal_set_message(
      t('You need to install %name from !url.',
        array(
          '%name' => $library_info['name'],
          '!url' => l('Github', $library_info['download url'])
        )
      ), 'error', FALSE);
  }
}

/**
 * save new user after first login
 */
function redmine_sso_create_user($user) {
  $msg = 'Your redmine user has no valid mail address.';
  if(valid_email_address($user->mail)) {
    $msg = 'Loginname or mail address are already in use.';
    if(redmine_sso_user_is_unique($user)) {
      $settings = variable_get('redmine_sso_settings', redmine_sso_get_default_settings());

      $password = user_password(12);

      //set up the user fields
      $fields = array(
        'name' => $user->login,
        'mail' => $user->mail,
        'pass' => $password,
        'status' => (!empty($settings['new_user_access']['unblocked']))?1:0,
        'init' => 'email address',
        'roles' => $settings['role_assigning']
      );

      //the first parameter is left blank so a new user is created
      $account = user_save('', $fields);
      // link redmine user with drupal user
      redmine_sso_write_identity($user, $account);
      // send message to admin if user is blocked
      if(empty($settings['new_user_access']['unblocked'])) {
        $params['account'] = $account;
        drupal_mail('user', 'register_pending_approval_admin', variable_get('site_mail', ini_get('sendmail_from')), language_default(), $params);
        drupal_set_message(t('Your account is currently blocked. You will be notified by mail after the admin approved your account.'), 'status', FALSE);
      } else {
        // set message with password
      drupal_set_message(t('A new password has been set for your new Drupal account (!password). This password works only within this drupal system and won\'t change your redmine password.', array('!password' => $password)), 'status', FALSE);
      }
      return $account;
    }
  }
  drupal_set_message(t($msg), 'error', FALSE);
  return FALSE;
}

/**
 * check if username and mail are unique
 */
function redmine_sso_user_is_unique($user) {
  $account_user_by_mail = user_load_by_mail($user->mail);
  $account_user_by_name = user_load_by_name($user->login);
  if(!empty($account_user_by_mail) || !empty($account_user_by_name)) {
    return FALSE;
  }
  return TRUE;
}

/**
 * check redmine user based on redmine sso settings
 */
function redmine_sso_check_redmine_user($user) {
  if($user->status != 1) {
    return FALSE;
  }
  $settings = variable_get('redmine_sso_settings', redmine_sso_get_default_settings());
  if(!empty($settings['user_filter_mode'])) {
    $bool = FALSE;
    if('project_filter' == $settings['user_filter_mode']) {
      $memberships = redmine_sso_get_project_memberships();
      foreach ($memberships->memberships as $membership) {
        if(isset($membership->user)) {
          if($user->id == $membership->user->id) {
            $bool = TRUE;
          }
        }
      }
    }
    if('group_filter' == $settings['user_filter_mode']) {
      $memberships = redmine_sso_get_group_memberships();
      foreach ($memberships->users as $group_user) {
        if($user->id == $group_user->id) {
          $bool = TRUE;
        }
      }
    }
    return $bool;
  }
  return TRUE;
}

/**
 *
 */

/**
 * default settings as fallback
 */
function redmine_sso_get_default_settings() {
  return array(
    'url' => 'localhost:3000',
    'protocol' => 'https://',
    'api_key' => '',
    'client_id' => '',
    'client_secret' => '',
    'user_filter_mode' => 0,
    'project_id' => '',
    'group_id' => '',
    'new_user_access' => array(),
    'role_assigning' => array(),
    'daily_checkup' => 1,
    'disable_login' => 0
  );
}

/**
 * link user
 */
function redmine_sso_write_identity($redmine_user, $user) {
  $db_name = 'redmine_sso_identity';
  $data = array(
    'id' => $redmine_user->id,
    'uid' => $user->uid,
    'data' => json_encode($redmine_user),
  );
  db_merge($db_name)
    ->key(array('uid' => $user->uid))
    ->fields($data)
    ->execute();
}

/**
 * save redmine oauth session
 */
function redmine_sso_write_session($user, $token) {
  $db_name = 'redmine_sso_session';

  $data = array(
    'uid' => $user->uid,
    'data' => redmine_sso_serialize_auth_token($token),
    'updated' => REQUEST_TIME,
  );
  db_merge($db_name)
    ->key(array('uid' => $user->uid))
    ->fields($data)
    ->execute();
}

/**
 * delete linked user
 */
function redmine_sso_delete_identity($uid) {
  $db_name = 'redmine_sso_identity';
  db_delete($db_name)
    ->condition('uid', $uid)
    ->execute();
}

/**
 * delete redmine oauth session
 */
function redmine_sso_delete_session($uid) {
  $db_name = 'redmine_sso_session';
  db_delete($db_name)
    ->condition('uid', $uid)
    ->execute();
}

/**
 * load redmine link of user
 */
function redmine_sso_load_indentity($uid) {
  $result = db_select('redmine_sso_identity', 's')
    ->fields('s')
    ->condition('uid', $uid, '=')
    ->execute()
    ->fetchAssoc();

  return $result;
}

/**
 * load session of user
 */
function redmine_sso_load_session($uid) {
  $result = db_select('redmine_sso_session', 's')
    ->fields('s')
    ->condition('uid', $uid, '=')
    ->execute()
    ->fetchAssoc();

  return $result;
}

/**
 * create serialized oauth token
 */
function redmine_sso_serialize_auth_token($token) {
  $token_array = $_SESSION['lusitanian-oauth-token'];
  return json_encode($token_array);
}

/**
 * load redmine user from current user
 * work in progress
 */
function redmine_sso_load_redmine_user() {
  if(!empty($_SESSION['lusitanian-oauth-token'])) {
    global $base_url;
    libraries_load(OAUTHLIB);
    $settings = variable_get('redmine_sso_settings', array());

    // Session storage
    $storage = new \OAuth\Common\Storage\Session();
    // Setup the credentials for the requests
    $credentials = new \OAuth\Common\Consumer\Credentials(
      $settings['client_id'],
      $settings['client_secret'],
      $base_url . '/redmine-sso/endpoint'
    );

    // Service Factory
    $serviceFactory = new \OAuth\ServiceFactory();

    // Instantiate the Redmine service using the credentials, http client and storage mechanism for the token
    $redmineService = $serviceFactory->createService('Redmine', $credentials, $storage, array(), new \OAuth\Common\Http\Uri\Uri($settings['protocol'] . $settings['url'] . '/oauth/'));
    $result = json_decode($redmineService->request('user_info.json'));

    return $result->user;
  }
}

/**
 * api call get project membership
 */
function redmine_sso_get_project_memberships($project_id = NULL) {
  $settings = variable_get('redmine_sso_settings', redmine_sso_get_default_settings());
  if(empty($project_id)) {
    $project_id = $settings['project_id'];
  }
  $url = url($settings['protocol'] . $settings['url'] . '/projects/' . $project_id . '/memberships.json',
    array(
      'query' => array('key' => $settings['api_key']),
      'absolute' => TRUE
    )
  );
  $output = drupal_http_request($url);
  return json_decode($output->data);
}

/**
 * api call get group membership
 */
function redmine_sso_get_group_memberships($group_id = NULL) {
  $settings = variable_get('redmine_sso_settings', redmine_sso_get_default_settings());
  if(empty($project_id)) {
    $project_id = $settings['group_id'];
  }

  $url = url($settings['protocol'] . $settings['url'] . '/groups.json',
    array(
      'query' => array('key' => $settings['api_key']),
      'absolute' => TRUE
    )
  );
  $groups = drupal_http_request($url);
  $groups = (json_decode($groups->data));

  $id = 0;
  foreach ($groups->groups as $group) {
    if($settings['group_id'] == $group->name) {
      $id = $group->id;
    }
  }
  if($id) {
    $url = url($settings['protocol'] . $settings['url'] . '/groups/' . $id . '.json',
      array(
        'query' => array(
          'key' => $settings['api_key'],
          'include' => 'users'
        ),
        'absolute' => TRUE
      )
    );
    $output = drupal_http_request($url);
    return json_decode($output->data)->group;
  }
  return FALSE;
}

/**
 * get all uids with session after x hours
 */
function redmine_sso_get_linked_user_sessions($timestamp) {
  $query = db_select('sessions','s');
  $query->condition('s.timestamp',$timestamp,'<');
  $query->leftJoin('redmine_sso_identity','i', 's.uid = i.uid');
  $query->condition('i.uid', 'NULL', '!=');
  $query->fields('s',array('uid'));
  $results = $query->execute()->fetchAllAssoc('uid');
  return array_keys($results);
}

/**
 * delete user sessions
 */
function redmine_sso_delete_linked_user_sessions($users) {
  $query = db_delete('sessions');
  $query->condition('uid', $users, 'IN');
  $query->execute();
}