README.md
=========

Redmine SSO module for drupal 7.
Allow users to login in drupal with redmine user account
by using Redmine OAuth-Provider-Plugin.



FEATURES
--------

* Create or link existing drupal users.
* Disable drupal login.
* Allow only redmine users from specific projects or groups.
* Automatic role assigning for new users.



INSTALLATION
------------

* Install OAuth-Provider-Plugin in redmine and create a client.
* Install module and configure admin/config/people/redmine-sso



USAGE
-----

* For new users just hit the Redmine SSO link on login page or in the user login block.
* For existing users go to your profile and click the redmine sso status tab.



TODO
----

* Redmine OAuth Plugin
  * client crud should be api accessible
  * Permission for client crud
  * create an oauth user
  * Invalidate Tokens / Clean tokens
* Refactor redmine_sso
* D8 Compatibility



PROBLEMS
--------

Just contact me over https://github.com/westberliner.


RESOURCES
---------

* REST API:   http://www.redmine.org/projects/redmine/wiki/Rest_api
* PROVIDER:   https://github.com/suer/redmine_oauth_provider
* OAUTHLIB:   https://github.com/Lusitanian/PHPoAuthLib
* HYBRIDAUTH: https://www.drupal.org/project/hybridauth