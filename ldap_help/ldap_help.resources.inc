<?php

/**
 * @file
 * The ldap_help resources are just links.
 */

/**
 *
 */
function ldap_help_main() {

  $text = <<<EOT

  <h3>LDAP Module Resources</h3>
  <ul>
    <li>The <a href="https://backdropcms.org/project/ldap">Backdropcms.org Documentation</a> covers basics of the module.</li>
    <li>Search the <a href="https://github.com/backdrop-contrib/ldap/issues">issue queue</a>;  For best results,
    select version and category before searching.</li>
    <li><a href="https://github.com/backdrop-contrib/ldap/issues?q=">View all issues</a></li>
    <li><a href="https://docs.moodle.org/20/en/LDAP_authentication">Moodle LDAP module documentation</a> is
    well done and provides insight into LDAP in a PHP environment.</li>
  </ul>

  <h3>Your local LDAP Documentation and Administrators</h3>
  <p>You would be surprised how much is documented about your local LDAP. Find your organization's LDAP documentation and
     support staff before you struggle blindly. Know whether LDAP or LDAPS is used. Know if an odd port is used.
     A sample email to an LDAP Administrator is available at the page of <a href="https://backdropcms.org/project/ldap#getting_the_relevant_information">Getting the relevant information</a>.
     Know how service accounts are expected to be used.
     Below are examples of ldap implementation documentation:</p>
  <ul>
  <li><a href="https://www.it.northwestern.edu/bin/docs/CentralAuthenticationServicesThroughLDAP.pdf">Northwestern University</a></li>
  </ul>

  <h3>General Backdrop CMS Support Help</h3>
  <ul>
  <li><a href="https://backdropcms.org/support">Get help with Backdrop CMS</a></li>
  <li><a href="https://backdropcms.org/resources/backdrop-cms-github">Issue queues, repositories, and more</a></li>
  <li><a href="https://backdropcms.org/support/resources">Backdrop CMS communication</a></li>
  </ul>

  <h3>General LDAP Resources</h3>
  <ul>
  <li><a href="https://us.php.net/manual/en/book.ldap.php">PHP LDAP</a></li>
  <li><a href="https://directory.apache.org/studio/">Apache Directory Studio</a> LDAP Browser and Directory Client.</li>
  <li><a href="https://www.netiq.com/documentation/edirectory-92/">Micro Focus eDirectory</a></li>
  <li><a href="https://images.apple.com/server/docs/Open_Directory_TB_v10.4.pdf">Apple Open Directory</a></li>
  <li><a href="https://msdn.microsoft.com/en-us/library/aa705886(VS.85).aspx">Microsoft Active Directory LDAP</a></li>
  </ul>

EOT;

  return $text;
}
