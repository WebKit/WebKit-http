#!/bin/sh

# Change these for your machine's setup

# Where the zip file will be output
zip_output_dir=/Data/develop/WebPositiveReleases
# This should be a directory with the proper layout for the zip.
# Any existing WebPositive binaries will be replaced.
release_zip_dir=/Data/develop/WebPositiveReleaseDir

# Should not need changing
app_name=WebPositive
build_dir=WebKitBuild
release_dir=$build_dir/Release
svn_revision_file=$release_dir/objects/WebKit/svn_revision
svn_revision=`cat $svn_revision_file`
date_string=`date +%Y-%m-%d`
zip_file_name=$app_name-gcc4-x86-r$svn_revision-$date_string.zip
full_zip_output=$zip_output_dir/$zip_file_name

webpositive_dir=$release_zip_dir/apps/$app_name
webpositive_lib_dir=$release_zip_dir/apps/$app_name/lib

if [ -e $full_zip_output ]; then
	printf "File already exists. Overwrite? (y or n) "
	read yes_or_no
	if [ $yes_or_no == 'y' ]; then
		rm $full_zip_output
	else
		exit
	fi
fi

echo Zip file name will be $zip_file_name
echo Will put WebPositive application in $webpositive_dir and associated libraries in $webpositive_lib_dir
echo The latest WebPositive code should have already been built before running this.
printf "Proceed? (y or n) "
read yes_or_no
if [ $yes_or_no == 'y' ]; then
	# Copy libs
	install -v $release_dir/JavaScriptCore/libjavascriptcore.so $webpositive_lib_dir
	install -v $release_dir/WebCore/libwebcore.so $webpositive_lib_dir
	install -v $release_dir/WebKit/libwebkit.so $webpositive_lib_dir
	# Copy application
	install -v $release_dir/WebKit/WebPositive $webpositive_dir

	# Make zip file
	pushd $release_zip_dir
	zip -r $full_zip_output .
	popd

	printf "Upload this file to haiku-files.org? (y or n) "
	read yes_or_no
	if [ $yes_or_no == 'y' ]; then
		scp $full_zip_output haiku_files@haiku-files.org:~/haiku-files.org/files/optional-packages
	fi
else
	echo Doing nothing.
fi
