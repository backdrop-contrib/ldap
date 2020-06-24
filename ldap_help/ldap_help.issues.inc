<?php

/**
 * @file
 * The ldap_help issues provides a filtered watchdog view for ldap issues.
 */

/**
 *
 */
function ldap_help_issues() {

  $text = '<h3>' . t('LDAP Help Module') . '</h3><p>' .
  t('How to report bugs in LDAP Project:') . '</p>';
  ldap_servers_module_load_include('inc', 'ldap_help', 'ldap_help.status');
  $ldap_config = ldap_help_status('html');

  $path = backdrop_get_path("module", "ldap_servers");

  $text .= <<<EOT

  <ol>
  <li>Search for existing issues. Include all statuses and versions
      in you search; a closed issue may be relevant:
<a href="https://github.com/backdrop-contrib/ldap/issues?q=">All issues</a>
</li>
<li>Create a new issue:
<a href="https://github.com/backdrop-contrib/ldap/issues">Issue queue</a>
</li>
</ol>

<div style="border: 1px solid #000000; padding: 10px; margin: 10px; text-align: center;">
  <p>Cut and past the html below into a file called <code>ldap_config.html</code> and attach it when reporting errors.
  <br/>The table below is the same. Check that no sensitive data is in it.</p>
  <form>
  <textarea rows=10 cols=120 border=1 style="border: 1px solid #000000">$ldap_config</textarea>
  </form>
</div>
$ldap_config
EOT;

  return $text;
}
