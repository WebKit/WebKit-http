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

import webapp2
from google.appengine.ext import db

import json

from controller import schedule_dashboard_update
from models import Builder
from models import Branch
from models import NumericIdHolder
from models import Platform
from models import create_in_transaction_with_numeric_id_holder


class CreateHandler(webapp2.RequestHandler):
    def post(self, model):
        self.response.headers['Content-Type'] = 'text/plain; charset=utf-8'

        try:
            payload = json.loads(self.request.body)
            key = payload.get('key', '')
            name = payload.get('name', '')
            password = payload.get('password', '')
        except:
            self.response.out.write("Failed to parse the payload: %s" % self.request.body)
            return

        if model == 'builder':
            error = self._create_builder(name, password)
        elif model == 'branch':
            error = self._create_branch(key, name)
        elif model == 'platform':
            error = self._create_platform(key, name)
        else:
            error = "Unknown model type: %s\n" % model

        # No need to clear manifest or runs since they only contain ones with test results
        schedule_dashboard_update()
        self.response.out.write(error if error else 'OK')

    def _create_builder(self, name, password):
        if not name or not password:
            return 'Invalid name or password'

        def execute():
            message = None
            bot = Builder.get_by_key_name(name)
            if bot:
                message = 'Updating the password since bot "%s" already exists' % name
                bot.update_password(password)
            else:
                Builder.create(name, password)
            return message

        return db.run_in_transaction(execute)

    def _create_branch(self, key, name):
        if not key or not name:
            return 'Invalid key or name'
        return None if Branch.create_if_possible(key, name) else 'Branch "%s" already exists' % key

    def _create_platform(self, key, name):
        if not key or not name:
            return 'Invalid key name'
        return None if Platform.create_if_possible(key, name) else 'Platform "%s" already exists' % key
