<?php
/**
 * @file
 * endpoint file to handle drupal site of the web flow
 */

/**
 * page callback of redmine sso endpoint
 * use OAuth\Common\Storage\Session;
 * use OAuth\Common\Consumer\Credentials;
 * use OAuth\Common\Http\Uri\Uri;
 * use OAuth\ServiceFactory;
 */
function redmine_sso_endpoint($account = NULL) {
  global $base_url, $user;

  $redirect = (!empty($_GET['redirect']))?$_GET['redirect']:url('<front>');

  // block if user already logged in skip if user want to link his account
  if(user_is_logged_in() && $account === NULL) {
    drupal_set_message(t('You\'re already logged in.'), 'status', FALSE);
    drupal_goto($redirect);
  }

  if(!empty($account)) {
    // skip if user one
    if(1 === $user->uid) {
      drupal_set_message(t('You cannot link drupal user one with an redmine account.'), 'error', FALSE);
      drupal_goto($redirect);
    }
    // skip if user is not link user
    if($account->uid != $user->uid) {
      drupal_set_message(t('You can only link your user-account with an redmine account.'), 'error', FALSE);
      drupal_goto($redirect);
    }
  }

  // load oauth lib
  libraries_load(OAUTHLIB);
  // get path
  $path = drupal_get_path('module', 'redmine_sso');

  // get settings
  $settings = variable_get('redmine_sso_settings', array());
  // check if emtpy
  if(empty($settings)) {
    drupal_set_message(t('You need to configure Redmine SSO first.'), 'error', FALSE);
    drupal_goto('<front>');
  }

  // Session storage
  $storage = new \OAuth\Common\Storage\Session();
  $callback = $base_url;
  $callback .= ($account === NULL)?'/user/redmine-sso-endpoint':'/user/' . $account->uid . '/redmine-sso/link';
  $callback .= '?redirect=' . $redirect;
  // Setup the credentials for the requests
  $credentials = new \OAuth\Common\Consumer\Credentials(
    $settings['client_id'],
    $settings['client_secret'],
    $callback
  );

  // Service Factory
  $serviceFactory = new \OAuth\ServiceFactory();

  // Instantiate the Redmine service using the credentials, http client and storage mechanism for the token
  $redmineService = $serviceFactory->createService('Redmine', $credentials, $storage, array(), new \OAuth\Common\Http\Uri\Uri($settings['protocol'] . $settings['url'] . '/oauth/'));

  if (!empty($_GET['oauth_token'])) {
    if(NULL === $account) {
      redmine_sso_endpoint_login_user($storage, $redmineService, $redirect);
    } else {
      redmine_sso_endpoint_link_user($storage, $redmineService, $redirect, $account);
    }
  } else {
    // extra request needed for oauth1 to request a request token :-)
    $token = $redmineService->requestRequestToken();

    $url = $redmineService->getAuthorizationUri(array('oauth_token' => $token->getRequestToken()));
    header('Location: ' . $url);
  }
}

/**
 * login user via redmine
 */
function redmine_sso_endpoint_login_user($storage, $redmineService, $redirect) {
  $token = $storage->retrieveAccessToken('Redmine');

  // This was a callback request from Redmine, get the token
  $accessToken = $redmineService->requestAccessToken(
      $_GET['oauth_token'],
      $_GET['oauth_verifier'],
      $token->getRequestTokenSecret()
  );

  // Send a request now that we have access token
  $result = json_decode($redmineService->request('user_info.json'));
  if(redmine_sso_check_redmine_user($result->user)) {
    if($user = redmine_sso_login_user($result->user, $token)) {
      drupal_set_message(t('You successfully logged in with your redmine account.'), 'status', FALSE);
    } else {
      drupal_set_message(t('Redmine sso cannot login user. Contact your admin.'), 'error', FALSE);
    }
  } else {
    drupal_set_message(t('Your redmine account is not allowed in this system. Contact your admin.'), 'error', FALSE);
  }
  drupal_goto($redirect);
}

/**
 * if user only want to link his account
 */
function redmine_sso_endpoint_link_user($storage, $redmineService, $redirect, $user) {
  $token = $storage->retrieveAccessToken('Redmine');

  // This was a callback request from Redmine, get the token
  $accessToken = $redmineService->requestAccessToken(
      $_GET['oauth_token'],
      $_GET['oauth_verifier'],
      $token->getRequestTokenSecret()
  );

  // Send a request now that we have access token
  $result = json_decode($redmineService->request('user_info.json'));
  if(redmine_sso_check_redmine_user($result->user)) {
    redmine_sso_write_identity($result->user, $user);
    redmine_sso_write_session($user, $token);
    drupal_set_message(t('Your account is successfully linked.'), 'status', FALSE);
  } else {
    drupal_set_message(t('Your redmine account is not allowed in this system. Contact your admin.'), 'error', FALSE);
  }
  drupal_goto($redirect);
}

function redmine_sso_login_user($redmine_user, $token) {
  $user = user_load_by_mail($redmine_user->mail);
  if(empty($user)) {
    // if user is empty create user
    $user = redmine_sso_create_user($redmine_user);
  }
  // if user exists login
  if($user) {
    // write current session into db
    redmine_sso_write_session($user, $token);

    $form = array();
    $form_state = array('uid' => $user->uid);
    user_login_submit($form, $form_state);
    return $user;
  }
  return FALSE;
}