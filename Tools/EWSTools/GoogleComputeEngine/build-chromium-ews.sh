#!/bin/sh
# Copyright (c) 2012 Google Inc. All rights reserved.
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

if [[ $# -ne 1 ]];then
echo "Usage: build-cr-linux-ews.sh BOT_NUMBER"
exit 1
fi

QUEUE_TYPE=chromium-ews
BOT_ID=gce-cr-linux-$1
BUGZILLA_USERNAME=webkit.review.bot@gmail.com
read -s -p "Bugzilla Password: " BUGZILLA_PASSWORD && echo

# FIXME: We should use gcutil to find a zone that's actually up.
ZONE=us-east-b
IMAGE=projects/google/images/ubuntu-10-04-v20120621

gcutil addinstance $BOT_ID --machine_type=standard-4-cpu-ephemeral-disk --image=$IMAGE --zone=$ZONE --wait_until_running

echo "Sleeping for 30s to let the server spin up ssh..."
sleep 30

gcutil ssh $BOT_ID "sudo apt-get install subversion -y && svn checkout http://svn.webkit.org/repository/webkit/trunk/Tools/EWSTools tools && cd tools && bash cold-boot.sh $QUEUE_TYPE $BOT_ID $BUGZILLA_USERNAME $BUGZILLA_PASSWORD"
