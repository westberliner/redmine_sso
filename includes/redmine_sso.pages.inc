<?php
/**
 * @file
 * pages file for additional page callbacks
 */

/**
 * status callback
 */
function redmine_sso_user($account) {
  global $user;

  $out = array();
  $options = array('query' => array('redirect' => $_GET['q']));

  if($redmine_account = redmine_sso_load_indentity($account->uid)) {
    $redmine_account = json_decode($redmine_account['data']);

    $out[] = array(
      '#markup' => t('Linked Account'),
      '#prefix' => '<h2>',
      '#suffix' => '</h2>',
    );
    // data
    $infofields = array('id', 'login', 'firstname', 'lastname', 'mail');
    $header = array();
    $rows = array();
    foreach ($infofields as $field) {
      $header[] = ucfirst($field);
      $rows[0][] = $redmine_account->$field;
    }
    if($user->uid === $account->uid) {
      $header[] = 'Actions';
      $rows[0][] = l(t('Unlink Account'), 'user/' . $account->uid . '/redmine-sso/unlink', $options);
    }
    $out[] = array(
      '#theme' => 'table',
      '#header' => $header,
      '#rows' => $rows
    );
  } else {
    $out[] = array(
      '#type' => 'link',
      '#href' => 'user/' . $account->uid . '/redmine-sso/link',
      '#title' => t('Link Redmine Account'),
      '#options' => $options
    );
  };

  return $out;
}
/**
 * unlink callback
 */
function redmine_sso_unlink($account) {
  global $user;

  $redirect = (!empty($_GET['redirect']))?$_GET['redirect']:'<front>';
  if($account->uid != $user->uid) {
    drupal_set_message(t('You can only unlink your own account.'), 'status', FALSE);
  }
  if($identity = redmine_sso_load_indentity($account->uid)) {
    redmine_sso_delete_identity($account->uid);
    redmine_sso_delete_session($account->uid);
    drupal_set_message(t('You successfully unlinked your account.'), 'status', FALSE);
  } else {
    drupal_set_message(t('Your account is not linked with any redmine account.'), 'error', FALSE);
  }
  drupal_goto($redirect);
}

/**
 * logout callback
 * workaround to display message
 */
function redmine_sso_logout() {
  // you must be logged in first :)
  if(!user_is_logged_in()) {
    drupal_goto();
  }

  watchdog('user', 'Session closed for %name.', array('%name' => $user->name));

  module_invoke_all('user_logout', $user);

  session_destroy();
  $url = url('<front>');
  drupal_add_http_header('refresh', "5;url=$url");
  drupal_set_message(t('Your account is blocked by redmine sso. Please contact your admin.'), 'error', FALSE);

  return '';
}