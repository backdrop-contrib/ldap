<?php

/**
 * @file
 * Test script for testing ldap functionality outside of Backdrop.
 */

// Remove this line after moving out the scripts outside of Backdrop root.
die('Move this script outside of Backdrop and uncomment the die statement in config.inc to run.');

/**
 *
 */
function ldap_help_config() {

  // These variables are just for convenience, they can be hard coded directly into the $config array.
  $provision_root = 'dc=local';
  $provision_root_group = 'ou=groups,' . $provision_root;
  $provision_root_user = 'ou=people,' . $provision_root;
  $query_root = 'ou=groups,dc=local';
  $user_root = 'ou=people,dc=local';
  $bind_user = 'cn=admin,dc=local';
  $bind_pwd = 'admin';
  $server_address = 'simpleldap.local';

  $config = [
    'servers' => [
      'default test' => [
        'server_address' => $server_address,
        'server_port'  => 389,
        'server_tls'  => FALSE,
        // LDAP_SERVERS_BIND_METHOD_SERVICE_ACCT OR LDAP_SERVERS_BIND_METHOD_ANON.
        'server_bind_method'  => LDAP_SERVERS_BIND_METHOD_SERVICE_ACCT,
        'server_base_dn' => $user_root,
        'server_bind_dn' => $bind_user,
        'server_bind_pw' => $bind_pwd,
        'test_queries' => [
          'user' => [
            'filter'  => 'cn=ldapuser',
            'show_attr' => ['dn', 'cn', 'sn', 'mail'],
          ],
        ],
        'test_provisions' => [],
      ],
      'query test' => [
        'server_address' => $server_address,
        'server_port'  => 389,
        'server_tls'  => FALSE,
        // LDAP_SERVERS_BIND_METHOD_SERVICE_ACCT OR LDAP_SERVERS_BIND_METHOD_ANON.
        'server_bind_method'  => LDAP_SERVERS_BIND_METHOD_SERVICE_ACCT,
        'server_base_dn' => $query_root,
        'server_bind_dn' => $bind_user,
        'server_bind_pw' => $bind_pwd,
        'test_queries' => [
          'group' => [
            'filter'  => 'cn=ldapgroup',
            'show_attr' => ['dn', 'cn', 'member'],
          ],
        ],
        'test_provisions' => [],
      ],
      'provision test' => [
        'server_address' => $server_address,
        'server_port'  => 389,
        'server_tls'  => FALSE,
         // LDAP_SERVERS_BIND_METHOD_SERVICE_ACCT OR LDAP_SERVERS_BIND_METHOD_ANON.
        'server_bind_method'  => LDAP_SERVERS_BIND_METHOD_SERVICE_ACCT,
        'server_base_dn' => $provision_root,
        'server_bind_dn' => $bind_user,
        'server_bind_pw' => $bind_pwd,
        'test_queries' => [],
        'test_provisions' => [
          'test_user' => [
            'delete_if_exists' => TRUE,
            'find_filter' => "cn=test_user,$provision_root_user",
            'dn' => "cn=test_user,$provision_root_user",
            'attr' => [
              "sn" => "BackdropCMS",
              "cn" => 'test_user',
              'mail' => 'test_user@blah.com',
              "objectClass" => [
                "inetOrgPerson",
              ],
            ],
          ],
          'test_group' => [
            'delete_if_exists' => TRUE,
            'find_filter' => "cn=test_group,$provision_root_group",
            'dn' => "cn=test_group,$provision_root_group",
            'attr' => [
              "cn" => 'test_group',
              "objectClass" => [
                "groupOfNames",
              ],
              "member" => [
                "cn=test-user,$provision_root_user",
                "cn=ldapuser,$provision_root_user",
              ],
            ],
          ],
        ],
      ],
    ],
  ];

  return $config;

}
