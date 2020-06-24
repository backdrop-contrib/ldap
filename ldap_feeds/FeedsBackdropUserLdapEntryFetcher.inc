<?php

/**
 * @file
 * FeedsBackdropUserLdapEntryFetcher.
 */

define('LDAP_FEEDS_BACKDROP_USER_FETCHER_FILTER_AUTHENTICATED', FALSE);
define('LDAP_FEEDS_BACKDROP_USER_FETCHER_FILTER_ROLES', FALSE);
/**
 *
 */
class FeedsBackdropUserLdapEntryFetcherResult extends FeedsFetcherResult {

  public $filterLdapAuthenticated;
  public $availableBackdropUserAttributes;
  public $filterRoles;

  /**
   * Constructor.
   */
  public function __construct($source_config) {
    $this->availableBackdropUserAttributes = ldap_feeds_backdrop_user_attributes();
    $this->filterLdapAuthenticated = isset($source_config['filterLdapAuthenticated']) ? $source_config['filterLdapAuthenticated'] : LDAP_FEEDS_BACKDROP_USER_FETCHER_FILTER_AUTHENTICATED;
    $this->filterRoles = isset($source_config['filterRoles']) ? $source_config['filterRoles'] : LDAP_FEEDS_BACKDROP_USER_FETCHER_FILTER_ROLES;

    parent::__construct('');
    $this->ldap_result = $this->getRaw();
    return $this->ldap_result;
  }

  /**
   * Overrides parent::getRaw();.
   */
  public function getRaw() {

    $results = [];
    // Needs to loop through all users, and query ldap for each, one at a time.
    $query = new EntityFieldQuery();
    $entities = $query
      ->entityCondition('entity_type', 'user')
      ->execute();
    $users = entity_load('user', array_keys($entities['user']));
    if ($this->filterRoles) {
      $selectedRoles = array_filter($this->filterRoles);
      $filterOnRoles = (boolean) (count($selectedRoles));
    }
    else {
      $filterOnRoles = FALSE;
    }

    foreach ($users as $uid => $user) {
      if (
        $uid == 0 ||
        $uid == 1 ||
        ($this->filterLdapAuthenticated && !isset($user->data['ldap_user'])) ||
        ($filterOnRoles && !array_intersect(array_values($selectedRoles), array_keys($user->roles)))
        ) {
        continue;
      }

      if ($ldap_user = ldap_servers_get_user_ldap_data($user)) {
        unset($ldap_user['mail']);
        $ldap_user['attr']['count'] = $ldap_user['attr']['count'] + count($this->availableBackdropUserAttributes);
        foreach ($this->availableBackdropUserAttributes as $attr_name => $attr_conf) {
          $ldap_user['attr'][] = $attr_conf['token'];
          $ldap_user['attr'][$attr_conf['token']]['count'] = 1;
          $ldap_user['attr'][$attr_conf['token']][0] = (string) $user->{$attr_name};
        }

        $results[] = $ldap_user;
      }
    }
    $results['count'] = count($results);
    return $results;
  }

}

/**
 * Fetches data via LDAP Query.
 */
class FeedsBackdropUserLdapEntryFetcher extends FeedsFetcher {

  /**
   * Implements FeedsFetcher::fetch().
   */
  public function fetch(FeedsSource $source) {
    $source_config = $source->getConfigFor($this);
    $result = new FeedsBackdropUserLdapEntryFetcherResult($source_config);
    return $result;
  }

  /**
   * Override parent::configDefaults().
   */
  public function configDefaults() {
    return [
      'filterLdapAuthenticated' => 0,
      'availableBackdropUserAttributes' => ldap_feeds_backdrop_user_attributes(),
      'filterRoles' => [],
    ];
  }

  /**
   * Override parent::configForm().
   */
  public function configForm(&$form_state) {

    $form = [];
    $form['filterLdapAuthenticated'] = [
      '#type' => 'checkbox',
      '#title' => t('Only return ldap authenticated users.'),
      '#default_value' => $this->config['filterLdapAuthenticated'],
      '#description' => t('If checked, only users who are associated with ldap accounts will be returned.'),
    ];

    return $form;
  }

  /**
   * Override parent::sourceFormValidate().
   */
  public function sourceFormValidate(&$values) {

  }

  /**
   * Override parent::sourceForm().
   */
  public function sourceForm($source_config) {
    $tokens = [
      '!edit_link' => l(t('Edit Feed'), 'admin/structure/feeds/' . $this->id),
    ];

    $form_state = [];
    $form = $this->configForm($form_state);
    $form['addendum'] = [
      '#type' => 'markup',
      '#markup' => t('This import is configured at !edit_link.', $tokens),
    ];

    return $form;
  }

}
