#!/bin/sh

function read_revision()
{
	lastRevision=$(cat mergelog.log | grep "\-\-\- Merging" | tail -n 1 | cut -d " " -f 8)
	if [ "$lastRevision" == "'local':" ]
	then
		lastRevision=$(cat mergelog.log | grep "\-\-\- Merging" | tail -n 1 | cut -d " " -f 6)
	fi
}

read_revision
echo Merging from $lastRevision to HEAD

svn merge -$lastRevision:HEAD http://svn.webkit.org/repository/webkit/trunk local >> mergelog.log

oldRevision=$lastRevision
read_revision

echo Merged from $oldRevision to $lastRevision
echo $lastRevision > local/webkit_revision

