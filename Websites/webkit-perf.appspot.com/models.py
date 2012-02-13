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
import re

from datetime import datetime
from google.appengine.ext import db


class NumericIdHolder(db.Model):
    owner = db.ReferenceProperty()
    # Dummy class whose sole purpose is to generate key().id()


def create_in_transaction_with_numeric_id_holder(callback):
    id_holder = NumericIdHolder()
    id_holder.put()
    id_holder = NumericIdHolder.get(id_holder.key())
    owner = db.run_in_transaction(callback, id_holder.key().id())
    if owner:
        id_holder.owner = owner
        id_holder.put()
    else:
        id_holder.delete()
    return owner


def delete_model_with_numeric_id_holder(model):
    id_holder = NumericIdHolder.get_by_id(model.id)
    model.delete()
    id_holder.delete()


def modelFromNumericId(id, expected_kind):
    id_holder = NumericIdHolder.get_by_id(id)
    return id_holder.owner if id_holder and id_holder.owner and isinstance(id_holder.owner, expected_kind) else None


class Branch(db.Model):
    id = db.IntegerProperty(required=True)
    name = db.StringProperty(required=True)


class Platform(db.Model):
    id = db.IntegerProperty(required=True)
    name = db.StringProperty(required=True)


class Builder(db.Model):
    name = db.StringProperty(required=True)
    password = db.StringProperty(required=True)

    def authenticate(self, raw_password):
        return self.password == hashlib.sha256(raw_password).hexdigest()

    @staticmethod
    def hashed_password(raw_password):
        return hashlib.sha256(raw_password).hexdigest()


class Build(db.Model):
    branch = db.ReferenceProperty(Branch, required=True, collection_name='build_branch')
    platform = db.ReferenceProperty(Platform, required=True, collection_name='build_platform')
    builder = db.ReferenceProperty(Builder, required=True, collection_name='builder_key')
    buildNumber = db.IntegerProperty(required=True)
    revision = db.IntegerProperty(required=True)
    chromiumRevision = db.IntegerProperty()
    timestamp = db.DateTimeProperty(required=True)


# Used to generate TestMap in the manifest efficiently
class Test(db.Model):
    id = db.IntegerProperty(required=True)
    name = db.StringProperty(required=True)
    branches = db.ListProperty(db.Key)
    platforms = db.ListProperty(db.Key)

    @staticmethod
    def cache_key(test_id, branch_id, platform_id):
        return 'runs:%d,%d,%d' % (test_id, branch_id, platform_id)


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


# Temporarily store log reports sent by bots
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
        return self._parsed.get(keyName, '')

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
        except ValueError:
            return None

    def timestamp(self):
        try:
            return datetime.fromtimestamp(self._integer_in_payload('timestamp'))
        except TypeError:
            return None
        except ValueError:
            return None


# Used when memcache entry is evicted
class PersistentCache(db.Model):
    value = db.TextProperty(required=True)
