# Copyright (C) 2009 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#    * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#    * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import datetime
import os
import StringIO
import threading

from webkitpy.common.config.committers import CommitterList, Reviewer
from webkitpy.common.checkout.commitinfo import CommitInfo
from webkitpy.common.checkout.scm import CommitMessage
from webkitpy.common.net.bugzilla import Bug, Attachment
from webkitpy.common.system.deprecated_logging import log
from webkitpy.common.system.executive import ScriptError
from webkitpy.common.system.filesystem_mock import MockFileSystem


def _id_to_object_dictionary(*objects):
    dictionary = {}
    for thing in objects:
        dictionary[thing["id"]] = thing
    return dictionary

# Testing


_patch1 = {
    "id": 10000,
    "bug_id": 50000,
    "url": "http://example.com/10000",
    "name": "Patch1",
    "is_obsolete": False,
    "is_patch": True,
    "review": "+",
    "reviewer_email": "foo@bar.com",
    "commit-queue": "+",
    "committer_email": "foo@bar.com",
    "attacher_email": "Contributer1",
}


_patch2 = {
    "id": 10001,
    "bug_id": 50000,
    "url": "http://example.com/10001",
    "name": "Patch2",
    "is_obsolete": False,
    "is_patch": True,
    "review": "+",
    "reviewer_email": "reviewer2@webkit.org",
    "commit-queue": "+",
    "committer_email": "non-committer@example.com",
    "attacher_email": "eric@webkit.org",
}


_patch3 = {
    "id": 10002,
    "bug_id": 50001,
    "url": "http://example.com/10002",
    "name": "Patch3",
    "is_obsolete": False,
    "is_patch": True,
    "review": "?",
    "attacher_email": "eric@webkit.org",
}


_patch4 = {
    "id": 10003,
    "bug_id": 50003,
    "url": "http://example.com/10002",
    "name": "Patch3",
    "is_obsolete": False,
    "is_patch": True,
    "review": "+",
    "commit-queue": "?",
    "reviewer_email": "foo@bar.com",
    "attacher_email": "Contributer2",
}


_patch5 = {
    "id": 10004,
    "bug_id": 50003,
    "url": "http://example.com/10002",
    "name": "Patch5",
    "is_obsolete": False,
    "is_patch": True,
    "review": "+",
    "reviewer_email": "foo@bar.com",
    "attacher_email": "eric@webkit.org",
}


_patch6 = { # Valid committer, but no reviewer.
    "id": 10005,
    "bug_id": 50003,
    "url": "http://example.com/10002",
    "name": "ROLLOUT of r3489",
    "is_obsolete": False,
    "is_patch": True,
    "commit-queue": "+",
    "committer_email": "foo@bar.com",
    "attacher_email": "eric@webkit.org",
}


_patch7 = { # Valid review, patch is marked obsolete.
    "id": 10006,
    "bug_id": 50002,
    "url": "http://example.com/10002",
    "name": "Patch7",
    "is_obsolete": True,
    "is_patch": True,
    "review": "+",
    "reviewer_email": "foo@bar.com",
    "attacher_email": "eric@webkit.org",
}


# This matches one of Bug.unassigned_emails
_unassigned_email = "webkit-unassigned@lists.webkit.org"
# This is needed for the FlakyTestReporter to believe the bug
# was filed by one of the webkitpy bots.
_commit_queue_email = "commit-queue@webkit.org"


_bug1 = {
    "id": 50000,
    "title": "Bug with two r+'d and cq+'d patches, one of which has an "
             "invalid commit-queue setter.",
    "reporter_email": "foo@foo.com",
    "assigned_to_email": _unassigned_email,
    "cc_emails": [],
    "attachments": [_patch1, _patch2],
    "bug_status": "UNCONFIRMED",
    "comments": [],
}


_bug2 = {
    "id": 50001,
    "title": "Bug with a patch needing review.",
    "reporter_email": "foo@foo.com",
    "assigned_to_email": "foo@foo.com",
    "cc_emails": ["abarth@webkit.org", ],
    "attachments": [_patch3],
    "bug_status": "ASSIGNED",
    "comments": [{"comment_date":  datetime.datetime(2011, 6, 11, 9, 4, 3),
                  "comment_email": "bar@foo.com",
                  "text": "Message1.",
        },
    ],
}


_bug3 = {
    "id": 50002,
    "title": "The third bug",
    "reporter_email": "foo@foo.com",
    "assigned_to_email": _unassigned_email,
    "cc_emails": [],
    "attachments": [_patch7],
    "bug_status": "NEW",
    "comments": [],
}


_bug4 = {
    "id": 50003,
    "title": "The fourth bug",
    "reporter_email": "foo@foo.com",
    "assigned_to_email": "foo@foo.com",
    "cc_emails": [],
    "attachments": [_patch4, _patch5, _patch6],
    "bug_status": "REOPENED",
    "comments": [],
}


_bug5 = {
    "id": 50004,
    "title": "The fifth bug",
    "reporter_email": _commit_queue_email,
    "assigned_to_email": "foo@foo.com",
    "cc_emails": [],
    "attachments": [],
    "bug_status": "RESOLVED",
    "dup_id": 50002,
    "comments": [],
}


class MockBugzillaQueries(object):

    def __init__(self, bugzilla):
        self._bugzilla = bugzilla

    def _all_bugs(self):
        return map(lambda bug_dictionary: Bug(bug_dictionary, self._bugzilla),
                   self._bugzilla.bug_cache.values())

    def fetch_bug_ids_from_commit_queue(self):
        bugs_with_commit_queued_patches = filter(
                lambda bug: bug.commit_queued_patches(),
                self._all_bugs())
        return map(lambda bug: bug.id(), bugs_with_commit_queued_patches)

    def fetch_attachment_ids_from_review_queue(self):
        unreviewed_patches = sum([bug.unreviewed_patches()
                                  for bug in self._all_bugs()], [])
        return map(lambda patch: patch.id(), unreviewed_patches)

    def fetch_patches_from_commit_queue(self):
        return sum([bug.commit_queued_patches()
                    for bug in self._all_bugs()], [])

    def fetch_bug_ids_from_pending_commit_list(self):
        bugs_with_reviewed_patches = filter(lambda bug: bug.reviewed_patches(),
                                            self._all_bugs())
        bug_ids = map(lambda bug: bug.id(), bugs_with_reviewed_patches)
        # NOTE: This manual hack here is to allow testing logging in
        # test_assign_to_committer the real pending-commit query on bugzilla
        # will return bugs with patches which have r+, but are also obsolete.
        return bug_ids + [50002]

    def fetch_patches_from_pending_commit_list(self):
        return sum([bug.reviewed_patches() for bug in self._all_bugs()], [])

    def fetch_bugs_matching_search(self, search_string, author_email=None):
        return [self._bugzilla.fetch_bug(50004), self._bugzilla.fetch_bug(50003)]


_mock_reviewers = [Reviewer("Foo Bar", "foo@bar.com"),
                   Reviewer("Reviewer2", "reviewer2@webkit.org")]


# FIXME: Bugzilla is the wrong Mock-point.  Once we have a BugzillaNetwork
#        class we should mock that instead.
# Most of this class is just copy/paste from Bugzilla.
class MockBugzilla(object):

    bug_server_url = "http://example.com"

    bug_cache = _id_to_object_dictionary(_bug1, _bug2, _bug3, _bug4, _bug5)

    attachment_cache = _id_to_object_dictionary(_patch1,
                                                _patch2,
                                                _patch3,
                                                _patch4,
                                                _patch5,
                                                _patch6,
                                                _patch7)

    def __init__(self):
        self.queries = MockBugzillaQueries(self)
        self.committers = CommitterList(reviewers=_mock_reviewers)
        self._override_patch = None

    def create_bug(self,
                   bug_title,
                   bug_description,
                   component=None,
                   diff=None,
                   patch_description=None,
                   cc=None,
                   blocked=None,
                   mark_for_review=False,
                   mark_for_commit_queue=False):
        log("MOCK create_bug")
        log("bug_title: %s" % bug_title)
        log("bug_description: %s" % bug_description)
        if component:
            log("component: %s" % component)
        if cc:
            log("cc: %s" % cc)
        if blocked:
            log("blocked: %s" % blocked)
        return 50004

    def quips(self):
        return ["Good artists copy. Great artists steal. - Pablo Picasso"]

    def fetch_bug(self, bug_id):
        return Bug(self.bug_cache.get(int(bug_id)), self)

    def set_override_patch(self, patch):
        self._override_patch = patch

    def fetch_attachment(self, attachment_id):
        if self._override_patch:
            return self._override_patch

        attachment_dictionary = self.attachment_cache.get(attachment_id)
        if not attachment_dictionary:
            print "MOCK: fetch_attachment: %s is not a known attachment id" % attachment_id
            return None
        bug = self.fetch_bug(attachment_dictionary["bug_id"])
        for attachment in bug.attachments(include_obsolete=True):
            if attachment.id() == int(attachment_id):
                return attachment

    def bug_url_for_bug_id(self, bug_id):
        return "%s/%s" % (self.bug_server_url, bug_id)

    def fetch_bug_dictionary(self, bug_id):
        return self.bug_cache.get(bug_id)

    def attachment_url_for_id(self, attachment_id, action="view"):
        action_param = ""
        if action and action != "view":
            action_param = "&action=%s" % action
        return "%s/%s%s" % (self.bug_server_url, attachment_id, action_param)

    def reassign_bug(self, bug_id, assignee=None, comment_text=None):
        log("MOCK reassign_bug: bug_id=%s, assignee=%s" % (bug_id, assignee))
        if comment_text:
            log("-- Begin comment --")
            log(comment_text)
            log("-- End comment --")

    def set_flag_on_attachment(self,
                               attachment_id,
                               flag_name,
                               flag_value,
                               comment_text=None,
                               additional_comment_text=None):
        log("MOCK setting flag '%s' to '%s' on attachment '%s' with comment '%s' and additional comment '%s'" % (
            flag_name, flag_value, attachment_id, comment_text, additional_comment_text))

    def post_comment_to_bug(self, bug_id, comment_text, cc=None):
        log("MOCK bug comment: bug_id=%s, cc=%s\n--- Begin comment ---\n%s\n--- End comment ---\n" % (
            bug_id, cc, comment_text))

    def add_attachment_to_bug(self,
                              bug_id,
                              file_or_string,
                              description,
                              filename=None,
                              comment_text=None):
        log("MOCK add_attachment_to_bug: bug_id=%s, description=%s filename=%s" % (bug_id, description, filename))
        if comment_text:
            log("-- Begin comment --")
            log(comment_text)
            log("-- End comment --")

    def add_patch_to_bug(self,
                         bug_id,
                         diff,
                         description,
                         comment_text=None,
                         mark_for_review=False,
                         mark_for_commit_queue=False,
                         mark_for_landing=False):
        log("MOCK add_patch_to_bug: bug_id=%s, description=%s, mark_for_review=%s, mark_for_commit_queue=%s, mark_for_landing=%s" %
            (bug_id, description, mark_for_review, mark_for_commit_queue, mark_for_landing))
        if comment_text:
            log("-- Begin comment --")
            log(comment_text)
            log("-- End comment --")

    def add_cc_to_bug(self, bug_id, ccs):
        pass

    def obsolete_attachment(self, attachment_id, message=None):
        pass

    def reopen_bug(self, bug_id, message):
        pass

    def close_bug_as_fixed(self, bug_id, message):
        pass

    def clear_attachment_flags(self, attachment_id, message):
        pass


class MockBuilder(object):
    def __init__(self, name):
        self._name = name

    def name(self):
        return self._name

    def results_url(self):
        return "http://example.com/builders/%s/results" % self.name()

    def accumulated_results_url(self):
        return "http://example.com/f/builders/%s/results/layout-test-results" % self.name()

    def force_build(self, username, comments):
        log("MOCK: force_build: name=%s, username=%s, comments=%s" % (
            self._name, username, comments))


class MockFailureMap(object):
    def __init__(self, buildbot):
        self._buildbot = buildbot

    def is_empty(self):
        return False

    def filter_out_old_failures(self, is_old_revision):
        pass

    def failing_revisions(self):
        return [29837]

    def builders_failing_for(self, revision):
        return [self._buildbot.builder_with_name("Builder1")]

    def tests_failing_for(self, revision):
        return ["mock-test-1"]

    def failing_tests(self):
        return set(["mock-test-1"])


class MockBuildBot(object):
    def __init__(self):
        self._mock_builder1_status = {
            "name": "Builder1",
            "is_green": True,
            "activity": "building",
        }
        self._mock_builder2_status = {
            "name": "Builder2",
            "is_green": True,
            "activity": "idle",
        }

    def builder_with_name(self, name):
        return MockBuilder(name)

    def builder_statuses(self):
        return [
            self._mock_builder1_status,
            self._mock_builder2_status,
        ]

    def red_core_builders_names(self):
        if not self._mock_builder2_status["is_green"]:
            return [self._mock_builder2_status["name"]]
        return []

    def red_core_builders(self):
        if not self._mock_builder2_status["is_green"]:
            return [self._mock_builder2_status]
        return []

    def idle_red_core_builders(self):
        if not self._mock_builder2_status["is_green"]:
            return [self._mock_builder2_status]
        return []

    def last_green_revision(self):
        return 9479

    def light_tree_on_fire(self):
        self._mock_builder2_status["is_green"] = False

    def failure_map(self):
        return MockFailureMap(self)


class MockSCM(object):
    def __init__(self, filesystem=None, executive=None):
        self.checkout_root = "/mock-checkout"
        self.added_paths = set()
        self._filesystem = filesystem or MockFileSystem()
        self._executive = executive or MockExecutive()

    def add(self, destination_path, return_exit_code=False):
        self.added_paths.add(destination_path)
        if return_exit_code:
            return 0

    def ensure_clean_working_directory(self, force_clean):
        pass

    def supports_local_commits(self):
        return True

    def ensure_no_local_commits(self, force_clean):
        pass

    def exists(self, path):
        # TestRealMain.test_real_main (and several other rebaseline tests) are sensitive to this return value.
        # We should make those tests more robust, but for now we just return True always (since no test needs otherwise).
        return True

    def absolute_path(self, *comps):
        return self._filesystem.join(self.checkout_root, *comps)

    def changed_files(self, git_commit=None):
        return ["MockFile1"]

    def changed_files_for_revision(self, revision):
        return ["MockFile1"]

    def head_svn_revision(self):
        return 1234

    def create_patch(self, git_commit, changed_files=None):
        return "Patch1"

    def commit_ids_from_commitish_arguments(self, args):
        return ["Commitish1", "Commitish2"]

    def committer_email_for_revision(self, revision):
        return "mock@webkit.org"

    def commit_locally_with_message(self, message):
        pass

    def commit_with_message(self, message, username=None, password=None, git_commit=None, force_squash=False, changed_files=None):
        pass

    def merge_base(self, git_commit):
        return None

    def commit_message_for_local_commit(self, commit_id):
        if commit_id == "Commitish1":
            return CommitMessage("CommitMessage1\n" \
                "https://bugs.example.org/show_bug.cgi?id=50000\n")
        if commit_id == "Commitish2":
            return CommitMessage("CommitMessage2\n" \
                "https://bugs.example.org/show_bug.cgi?id=50001\n")
        raise Exception("Bogus commit_id in commit_message_for_local_commit.")

    def diff_for_file(self, path, log=None):
        return path + '-diff'

    def diff_for_revision(self, revision):
        return "DiffForRevision%s\nhttp://bugs.webkit.org/show_bug.cgi?id=12345" % revision

    def show_head(self, path):
        return path

    def svn_revision_from_commit_text(self, commit_text):
        return "49824"

    def delete(self, path):
        if not self._filesystem:
            return
        if self._filesystem.exists(path):
            self._filesystem.remove(path)


class MockDEPS(object):
    def read_variable(self, name):
        return 6564

    def write_variable(self, name, value):
        log("MOCK: MockDEPS.write_variable(%s, %s)" % (name, value))


class MockCommitMessage(object):
    def message(self):
        return "This is a fake commit message that is at least 50 characters."

class MockCheckout(object):

    _committer_list = CommitterList()

    def commit_info_for_revision(self, svn_revision):
        # The real Checkout would probably throw an exception, but this is the only way tests have to get None back at the moment.
        if not svn_revision:
            return None
        return CommitInfo(svn_revision, "eric@webkit.org", {
            "bug_id": 50000,
            "author_name": "Adam Barth",
            "author_email": "abarth@webkit.org",
            "author": self._committer_list.contributor_by_email("abarth@webkit.org"),
            "reviewer_text": "Darin Adler",
            "reviewer": self._committer_list.committer_by_name("Darin Adler"),
            "changed_files": [
                "path/to/file",
                "another/file",
            ],
        })

    def is_path_to_changelog(self, path):
        return os.path.basename(path) == "ChangeLog"

    def bug_id_for_revision(self, svn_revision):
        return 12345

    def recent_commit_infos_for_files(self, paths):
        return [self.commit_info_for_revision(32)]

    def modified_changelogs(self, git_commit, changed_files=None):
        # Ideally we'd return something more interesting here.  The problem is
        # that LandDiff will try to actually read the patch from disk!
        return []

    def commit_message_for_this_commit(self, git_commit, changed_files=None):
        return MockCommitMessage()

    def chromium_deps(self):
        return MockDEPS()

    def apply_patch(self, patch, force=False):
        pass

    def apply_reverse_diffs(self, revision):
        pass

    def suggested_reviewers(self, git_commit, changed_files=None):
        return [_mock_reviewers[0]]


class MockUser(object):

    @classmethod
    def prompt(cls, message, repeat=1, raw_input=raw_input):
        return "Mock user response"

    @classmethod
    def prompt_with_list(cls, list_title, list_items, can_choose_multiple=False, raw_input=raw_input):
        pass

    def __init__(self):
        self.opened_urls = []

    def edit(self, files):
        pass

    def edit_changelog(self, files):
        pass

    def page(self, message):
        pass

    def confirm(self, message=None, default='y'):
        log(message)
        return default == 'y'

    def can_open_url(self):
        return True

    def open_url(self, url):
        self.opened_urls.append(url)
        if url.startswith("file://"):
            log("MOCK: user.open_url: file://...")
            return
        log("MOCK: user.open_url: %s" % url)


class MockIRC(object):

    def post(self, message):
        log("MOCK: irc.post: %s" % message)

    def disconnect(self):
        log("MOCK: irc.disconnect")


class MockStatusServer(object):

    def __init__(self, bot_id=None, work_items=None):
        self.host = "example.com"
        self.bot_id = bot_id
        self._work_items = work_items or []

    def patch_status(self, queue_name, patch_id):
        return None

    def svn_revision(self, svn_revision):
        return None

    def next_work_item(self, queue_name):
        if not self._work_items:
            return None
        return self._work_items.pop(0)

    def release_work_item(self, queue_name, patch):
        log("MOCK: release_work_item: %s %s" % (queue_name, patch.id()))

    def update_work_items(self, queue_name, work_items):
        self._work_items = work_items
        log("MOCK: update_work_items: %s %s" % (queue_name, work_items))

    def submit_to_ews(self, patch_id):
        log("MOCK: submit_to_ews: %s" % (patch_id))

    def update_status(self, queue_name, status, patch=None, results_file=None):
        log("MOCK: update_status: %s %s" % (queue_name, status))
        return 187

    def update_svn_revision(self, svn_revision, broken_bot):
        return 191

    def results_url_for_status(self, status_id):
        return "http://dummy_url"


# FIXME: Unify with common.system.executive_mock.MockExecutive.
class MockExecutive(object):
    def __init__(self, should_log=False, should_throw=False):
        self._should_log = should_log
        self._should_throw = should_throw
        # FIXME: Once executive wraps os.getpid() we can just use a static pid for "this" process.
        self._running_pids = [os.getpid()]

    def check_running_pid(self, pid):
        return pid in self._running_pids

    def run_and_throw_if_fail(self, args, quiet=False, cwd=None):
        if self._should_log:
            log("MOCK run_and_throw_if_fail: %s, cwd=%s" % (args, cwd))
        return "MOCK output of child process"

    def run_command(self,
                    args,
                    cwd=None,
                    input=None,
                    error_handler=None,
                    return_exit_code=False,
                    return_stderr=True,
                    decode_output=False):
        if self._should_log:
            log("MOCK run_command: %s, cwd=%s" % (args, cwd))
        if self._should_throw:
            raise ScriptError("MOCK ScriptError")
        return "MOCK output of child process"


class MockOptions(object):
    """Mock implementation of optparse.Values."""

    def __init__(self, **kwargs):
        # The caller can set option values using keyword arguments. We don't
        # set any values by default because we don't know how this
        # object will be used. Generally speaking unit tests should
        # subclass this or provider wrapper functions that set a common
        # set of options.
        for key, value in kwargs.items():
            self.__dict__[key] = value


class MockPort(object):
    def name(self):
        return "MockPort"

    def layout_tests_results_path(self):
        return "/mock-results/results.html"

    def check_webkit_style_command(self):
        return ["mock-check-webkit-style"]

    def update_webkit_command(self):
        return ["mock-update-webkit"]

    def build_webkit_command(self, build_style=None):
        return ["mock-build-webkit"]

    def run_bindings_tests_command(self):
        return ["mock-run-bindings-tests"]

    def prepare_changelog_command(self):
        return ['mock-prepare-ChangeLog']

    def run_python_unittests_command(self):
        return ['mock-test-webkitpy']

    def run_perl_unittests_command(self):
        return ['mock-test-webkitperl']

    def run_javascriptcore_tests_command(self):
        return ['mock-run-javacriptcore-tests']

    def run_webkit_tests_command(self):
        return ['mock-run-webkit-tests']


class MockTestPort1(object):
    def skips_layout_test(self, test_name):
        return test_name in ["media/foo/bar.html", "foo"]


class MockTestPort2(object):
    def skips_layout_test(self, test_name):
        return test_name == "media/foo/bar.html"


class MockPortFactory(object):
    def get_all(self, options=None):
        return {"test_port1": MockTestPort1(), "test_port2": MockTestPort2()}


class MockPlatformInfo(object):
    def display_name(self):
        return "MockPlatform 1.0"


class MockWatchList(object):
    def determine_cc_and_messages(self, diff):
        log("MockWatchList: determine_cc_and_messages")
        return {'cc_list': ['abarth@webkit.org', 'levin@chromium.org'], 'messages': ['Message1.', 'Message2.'], }


class MockWorkspace(object):
    def find_unused_filename(self, directory, name, extension, search_limit=10):
        return "%s/%s.%s" % (directory, name, extension)

    def create_zip(self, zip_path, source_path):
        return object()  # Something that is not None


class MockWeb(object):
    def get_binary(self, url, convert_404_to_None=False):
        return "MOCK Web result, convert 404 to None=%s" % convert_404_to_None


class MockTool(object):
    def __init__(self, log_executive=False):
        self.wakeup_event = threading.Event()
        self.bugs = MockBugzilla()
        self.buildbot = MockBuildBot()
        self.executive = MockExecutive(should_log=log_executive)
        self.web = MockWeb()
        self.workspace = MockWorkspace()
        self._irc = None
        self.user = MockUser()
        self._scm = MockSCM()
        self._chromium_buildbot = MockBuildBot()
        # Various pieces of code (wrongly) call filesystem.chdir(checkout_root).
        # Making the checkout_root exist in the mock filesystem makes that chdir not raise.
        self.filesystem = MockFileSystem(dirs=set([self._scm.checkout_root]))
        self._port = MockPort()
        self._checkout = MockCheckout()
        self.status_server = MockStatusServer()
        self.irc_password = "MOCK irc password"
        self.port_factory = MockPortFactory()
        self.platform = MockPlatformInfo()
        self._watch_list = MockWatchList()

    def scm(self):
        return self._scm

    def checkout(self):
        return self._checkout

    def chromium_buildbot(self):
        return self._chromium_buildbot

    def watch_list(self):
        return self._watch_list

    def ensure_irc_connected(self, delegate):
        if not self._irc:
            self._irc = MockIRC()

    def irc(self):
        return self._irc

    def path(self):
        return "echo"

    def port(self):
        return self._port


class MockBrowser(object):
    params = {}

    def open(self, url):
        pass

    def select_form(self, name):
        pass

    def __setitem__(self, key, value):
        self.params[key] = value

    def submit(self):
        return StringIO.StringIO()
