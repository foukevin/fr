Fr - A command line font rasterizing tool
=========================================

Copyright (c) 2015 Kévin Delbrayelle

About Fr
-----------------------------------------------------------------------

Fr is a command line tool for rasterizing True Type Fonts (or any
other format supported by the [FreeType project](http://www.freetype.org)
library) into png files. It also exports various glyph metrics into
an helper file such as size and texture coordinates for use in other
programs.

Basic usage
-----------------------------------------------------------------------

	$ fr -o dejavu.png DejaVuSans.ttf

This command will use the DejaVuSans.ttf font to export the glyphs for
the ascii set into the file 'dejavu.png'.

You can specify the unicode point (rune) you want to export:

	$ fr -o dejavu.png DejaVuSans.ttf --rune 65

Several runes can be specified at once using a comma separator:

	$ fr -o dejavu.png DejaVuSans.ttf --rune 65,66,80

For convenience, it is also possible to specify rune ranges:

	$ fr -o dejavu.png DejaVuSans.ttf --rune 65:80

Ranges are inclusives. You can also specify extension range:

	$ fr -o dejavu.png DejaVuSans.ttf --rune 65+26

Runes and rune ranges can be mixed together using a separating comma:

	$ fr -o dejavu.png DejaVuSans.ttf --rune 65+26,45,46:67
