#!/bin/sh

cd $1

echo "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">
<html>
	<head>
		<title>$(basename $1) index</title>
	</head>
	<body>" > index.html

for FILE in *
do
	echo "		<a href=\"$2/$FILE\">$FILE</a><br>" >> index.html
done

echo "	</body>
</html>" >> index.html

