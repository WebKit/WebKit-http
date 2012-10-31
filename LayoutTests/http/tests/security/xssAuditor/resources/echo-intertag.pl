#!/usr/bin/perl -wT
use strict;
use CGI;

my $cgi = new CGI;

# Passing semicolons through the url to this script is problematic. The raw
# form truncates the input and the %-encoded form isn't being decoded. Hence
# this set of hard-coded headers. 
if ($cgi->param('disable-protection')) {
    print "X-XSS-Protection: 0\n";
}
if ($cgi->param('enable-full-block')) {
    print "X-XSS-Protection: 1; mode=block\n";
}
if ($cgi->param('valid-header') == 1) {
    print "X-XSS-Protection:   1  ;MoDe =  bLocK   \n";
}
if ($cgi->param('valid-header') == 2) {
    print "X-XSS-Protection: 1; \n";
}
if ($cgi->param('malformed-header') == 1) {
    print "X-XSS-Protection: 12345678901234567\n";
}
if ($cgi->param('malformed-header') == 2) {
    print "X-XSS-Protection: red\n";
}
if ($cgi->param('malformed-header') == 3) {
    print "X-XSS-Protection: 1; mode=purple\n";
}
if ($cgi->param('malformed-header') == 4) {
    print "X-XSS-Protection: 1; mode=block-a-block-block\n";
}

print "Content-Type: text/html; charset=";
print $cgi->param('charset') ? $cgi->param('charset') : "UTF-8";
print "\n\n";

print "<!DOCTYPE html>\n";
print "<html>\n";
if ($cgi->param('relay-target-ids-for-event')) {
    print "<script>\n";
    print "document.addEventListener('" . $cgi->param('relay-target-ids-for-event') . "', function(event) {\n";
    print "    window.parent.postMessage(event.target.id, '*');\n";
    print "}, true);\n";
    print "</script>\n";
}
print "<body>\n";
print $cgi->param('q');
if ($cgi->param('clutter')) {
    print $cgi->param('clutter');
}
if ($cgi->param('q2')) {
    print $cgi->param('q2');
}
if ($cgi->param('showAction')) {
    print "<script>\n";
    print "if (window.testRunner)\n";
    print "    alert('Form action set to ' + document.forms[0].action);\n";
    print "</script>\n";
}
if ($cgi->param('notifyDone')) {
    print "<script>\n";
    print "if (window.testRunner)\n";
    print "    testRunner.notifyDone();\n";
    print "</script>\n";
}
if ($cgi->param('enable-full-block')) {
    print "<p>If you see this message then the test FAILED.</p>\n";
}
if ($cgi->param('alert-cookie')) {
    print "<script>if (/xssAuditorTestCookie/.test(document.cookie)) { alert('FAIL: ' + document.cookie); document.cookie = 'xssAuditorTestCookie=remove; max-age=-1'; } else alert('PASS');</script>\n";
}
print "</body>\n";
print "</html>\n";
