#!/usr/bin/env python
# Copyright (C) 2012 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
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

import hashlib
import json
import math
import re

from datetime import datetime
from datetime import timedelta
from google.appengine.ext import db
from google.appengine.api import memcache
from time import mktime


class NumericIdHolder(db.Model):
    owner = db.ReferenceProperty()
    # Dummy class whose sole purpose is to generate key().id()


def create_in_transaction_with_numeric_id_holder(callback):
    id_holder = NumericIdHolder()
    id_holder.put()
    id_holder = NumericIdHolder.get(id_holder.key())
    owner = None
    try:
        owner = db.run_in_transaction(callback, id_holder.key().id())
        if owner:
            id_holder.owner = owner
            id_holder.put()
    finally:
        if not owner:
            id_holder.delete()
    return owner


def delete_model_with_numeric_id_holder(model):
    id_holder = NumericIdHolder.get_by_id(model.id)
    model.delete()
    id_holder.delete()


def model_from_numeric_id(id, expected_kind):
    id_holder = NumericIdHolder.get_by_id(id)
    return id_holder.owner if id_holder and id_holder.owner and isinstance(id_holder.owner, expected_kind) else None


def _create_if_possible(model, key, name):

    def execute(id):
        if model.get_by_key_name(key):
            return None
        branch = model(id=id, name=name, key_name=key)
        branch.put()
        return branch

    return create_in_transaction_with_numeric_id_holder(execute)


class Branch(db.Model):
    id = db.IntegerProperty(required=True)
    name = db.StringProperty(required=True)

    @staticmethod
    def create_if_possible(key, name):
        return _create_if_possible(Branch, key, name)


class Platform(db.Model):
    id = db.IntegerProperty(required=True)
    name = db.StringProperty(required=True)

    @staticmethod
    def create_if_possible(key, name):
        return _create_if_possible(Platform, key, name)


class Builder(db.Model):
    name = db.StringProperty(required=True)
    password = db.StringProperty(required=True)

    @staticmethod
    def create(name, raw_password):
        return Builder(name=name, password=Builder._hashed_password(raw_password), key_name=name).put()

    def update_password(self, raw_password):
        self.password = Builder._hashed_password(raw_password)
        self.put()

    def authenticate(self, raw_password):
        return self.password == hashlib.sha256(raw_password).hexdigest()

    @staticmethod
    def _hashed_password(raw_password):
        return hashlib.sha256(raw_password).hexdigest()


class Build(db.Model):
    branch = db.ReferenceProperty(Branch, required=True, collection_name='build_branch')
    platform = db.ReferenceProperty(Platform, required=True, collection_name='build_platform')
    builder = db.ReferenceProperty(Builder, required=True, collection_name='builder_key')
    buildNumber = db.IntegerProperty(required=True)
    revision = db.IntegerProperty(required=True)
    chromiumRevision = db.IntegerProperty()
    timestamp = db.DateTimeProperty(required=True)

    @staticmethod
    def get_or_insert_from_log(log):
        builder = log.builder()
        key_name = builder.name + ':' + str(int(mktime(log.timestamp().timetuple())))

        return Build.get_or_insert(key_name, branch=log.branch(), platform=log.platform(), builder=builder,
            buildNumber=log.build_number(), timestamp=log.timestamp(),
            revision=log.webkit_revision(), chromiumRevision=log.chromium_revision())


# Used to generate TestMap in the manifest efficiently
class Test(db.Model):
    id = db.IntegerProperty(required=True)
    name = db.StringProperty(required=True)
    # FIXME: Storing branches and platforms separately is flawed since a test maybe available on
    # one platform but only on some branch and vice versa.
    branches = db.ListProperty(db.Key)
    platforms = db.ListProperty(db.Key)

    @staticmethod
    def cache_key(test_id, branch_id, platform_id):
        return 'runs:%d,%d,%d' % (test_id, branch_id, platform_id)

    @staticmethod
    def update_or_insert(test_name, branch, platform):
        existing_test = [None]

        def execute(id):
            test = Test.get_by_key_name(test_name)
            if test:
                if branch.key() not in test.branches:
                    test.branches.append(branch.key())
                if platform.key() not in test.platforms:
                    test.platforms.append(platform.key())
                test.put()
                existing_test[0] = test
                return None

            test = Test(id=id, name=test_name, key_name=test_name, branches=[branch.key()], platforms=[platform.key()])
            test.put()
            return test

        return create_in_transaction_with_numeric_id_holder(execute) or existing_test[0]

    def merge(self, other):
        assert self.key() != other.key()

        merged_results = TestResult.all()
        merged_results.filter('name =', other.name)

        # FIXME: We should be doing this check in a transaction but only ancestor queries are allowed
        for result in merged_results:
            if TestResult.get_by_key_name(TestResult.key_name(result.build, self.name)):
                return None

        branches_and_platforms_to_update = set()
        for result in merged_results:
            branches_and_platforms_to_update.add((result.build.branch.id, result.build.platform.id))
            result.replace_to_change_test_name(self.name)

        delete_model_with_numeric_id_holder(other)

        return branches_and_platforms_to_update


class TestResult(db.Model):
    name = db.StringProperty(required=True)
    build = db.ReferenceProperty(Build, required=True)
    value = db.FloatProperty(required=True)
    valueMedian = db.FloatProperty()
    valueStdev = db.FloatProperty()
    valueMin = db.FloatProperty()
    valueMax = db.FloatProperty()

    @staticmethod
    def key_name(build, test_name):
        return build.key().name() + ':' + test_name

    @classmethod
    def get_or_insert_from_parsed_json(cls, test_name, build, result):
        key_name = cls.key_name(build, test_name)

        def _float_or_none(dictionary, key):
            value = dictionary.get(key)
            if value:
                return float(value)
            return None

        if not isinstance(result, dict):
            return cls.get_or_insert(key_name, name=test_name, build=build, value=float(result))

        return cls.get_or_insert(key_name, name=test_name, build=build, value=float(result['avg']),
            valueMedian=_float_or_none(result, 'median'), valueStdev=_float_or_none(result, 'stdev'),
            valueMin=_float_or_none(result, 'min'), valueMax=_float_or_none(result, 'max'))

    def replace_to_change_test_name(self, new_name):
        clone = TestResult(key_name=TestResult.key_name(self.build, new_name), name=new_name, build=self.build,
            value=self.value, valueMedian=self.valueMedian, valueStdev=self.valueMin, valueMin=self.valueMin, valueMax=self.valueMax)
        clone.put()
        self.delete()
        return clone


class ReportLog(db.Model):
    timestamp = db.DateTimeProperty(required=True)
    headers = db.TextProperty()
    payload = db.TextProperty()
    commit = db.BooleanProperty()

    def _parsed_payload(self):
        if self.__dict__.get('_parsed') == None:
            try:
                self._parsed = json.loads(self.payload)
            except ValueError:
                self._parsed = False
        return self._parsed

    def get_value(self, keyName):
        if not self._parsed_payload():
            return None
        return self._parsed.get(keyName)

    def results(self):
        return self.get_value('results')

    def builder(self):
        return self._model_by_key_name_in_payload(Builder, 'builder-name')

    def branch(self):
        return self._model_by_key_name_in_payload(Branch, 'branch')

    def platform(self):
        return self._model_by_key_name_in_payload(Platform, 'platform')

    def build_number(self):
        return self._integer_in_payload('build-number')

    def webkit_revision(self):
        return self._integer_in_payload('webkit-revision')

    def chromium_revision(self):
        return self._integer_in_payload('chromium-revision')

    def _model_by_key_name_in_payload(self, model, keyName):
        key = self.get_value(keyName)
        if not key:
            return None
        return model.get_by_key_name(key)

    def _integer_in_payload(self, keyName):
        try:
            return int(self.get_value(keyName))
        except TypeError:
            return None
        except ValueError:
            return None

    # FIXME: We also have timestamp as a member variable.
    def timestamp(self):
        try:
            return datetime.fromtimestamp(self._integer_in_payload('timestamp'))
        except TypeError:
            return None
        except ValueError:
            return None


class PersistentCache(db.Model):
    value = db.TextProperty(required=True)

    @staticmethod
    def set_cache(name, value):
        memcache.set(name, value)

        def execute():
            cache = PersistentCache.get_by_key_name(name)
            if cache:
                cache.value = value
                cache.put()
            else:
                PersistentCache(key_name=name, value=value).put()

        db.run_in_transaction(execute)

    @staticmethod
    def get_cache(name):
        value = memcache.get(name)
        if value:
            return value
        cache = PersistentCache.get_by_key_name(name)
        if not cache:
            return None
        memcache.set(name, cache.value)
        return cache.value


class DashboardImage(db.Model):
    image = db.BlobProperty(required=True)
    createdAt = db.DateTimeProperty(required=True, auto_now=True)

    @classmethod
    def needs_update(cls, branch_id, platform_id, test_id, display_days, now=datetime.now()):
        if display_days < 10:
            return True
        image = DashboardImage.get_by_key_name(cls.key_name(branch_id, platform_id, test_id, display_days))
        duration = math.sqrt(display_days) / 10
        # e.g. 13 hours for 30 days, 23 hours for 90 days, and 46 hours for 365 days
        return not image or image.createdAt < now - timedelta(duration)

    @staticmethod
    def key_name(branch_id, platform_id, test_id, display_days):
        return '%d:%d:%d:%d' % (branch_id, platform_id, test_id, display_days)
