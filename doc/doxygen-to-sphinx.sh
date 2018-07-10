#!/bin/bash -e
#
# Helper script to convert doxygenisms to sphinx-isms

outdir=$1
fname=$2

! test -z "$outdir" || exit 1
! test -z "$fname" || exit 1

file="$fname"
outfile="$(basename --suffix='.dox' $fname).rst"

# awk commands:
# indent anything between the verbatim tags
# indent anything between the code tags
# add a empty line before the first list item (line starting with -)
cat "$file" | \
awk \
    '$0 ~ /.*@verbatim$/ { inside=1; print $0 "\n"; next; }
     $0 ~ /@endverbatim/ { inside=0; }
     inside == 1 { print "     " $0 }
     inside == 0 { print }' | \
awk \
    '$0 ~ /@code$/ { inside=1; print $0 "\n"; next; }
     $0 ~ /@endcode/ { inside=0; }
     inside == 1 { print "     " $0 }
     inside == 0 { print }' | \
awk \
    '$0 ~ /@dot$/ { inside=1; print $0 "\n"; next; }
     $0 ~ /@enddot/ { inside=0; }
     inside == 1 { print "    " $0 }
     inside == 0 { print }' | \
awk \
    '$0 ~ /<dd>/ { inside=1; print $0 "\n"; next; }
     $0 ~ /<\/dd>/ { inside=0; }
     inside == 1 { print "    " $0 }
     inside == 0 { print }' | \
awk \
    '/^-/{
	 if (!in_list && a != "") print ""; in_list=1
	 }
     /^$/ {in_list=0}
     {a=$0; print}' | \
sed \
  -e 's|@page \([^ ]*\) \(.*\)|.. _\1:\n\n==============================================================================\n\2\n==============================================================================|' \
  -e 's|@section \([^ ]*\) \(.*\)|.. _\1:\n\n------------------------------------------------------------------------------\n\2\n------------------------------------------------------------------------------|' \
  -e 's|@subsection \([^ ]*\) \(.*\)|.. _\1:\n\n..............................................................................\n\2\n..............................................................................|' \
  -e ':a;/@[[:alpha:]]\+$/{N;s/\(@[[:alpha:]]\+\)\n/\n\1 /;ba}' \
  -e 's|@see \(LIBINPUT_[_[:alpha:]]\+\)|**\1**|' \
  -e 's|@ref \(LIBINPUT_[_[:alpha:]]\+\)|**\1**|' \
  -e 's|@ref \(libinput_[_[:alpha:]]\+\)|**\1**|' \
  -e 's|\(libinput_[_[:alpha:]]\+()\)|**\1**|' \
  -e 's|[ ]*<dt>||' \
  -e 's|</dt>||' \
  -e 's|[ ]*<dd>|    |' \
  -e 's|</dd>||' \
  -e '/<\/\?dl>/d' \
  -e 's|<b>|**|' \
  -e 's|</b>|**|' \
  -e 's|\*40|\\*40|' \
  -e 's|\*50|\\*50|' \
  -e 's|@note|.. note::|' \
  -e 's|@warning|.. warning::|' \
  -e 's|@dotfile \(.*\)|.. graphviz:: \1|' \
  -e 's|@dot[ ]*|.. graphviz::\n\n|' \
  -e 's|@enddot||' \
  -e 's|@code[ ]*|::\n\n|' \
  -e 's|`[^`]\+|`\0|g' \
  -e 's|[^`]\+`$|\0`|g' \
  -e 's|@ref \([-[:alnum:]_]*\) "\(.*\)"|:ref:`\2 <\1>`|' \
  -e 's|@ref \([-[:alnum:]_]*\)|:ref:`\1`|' \
  -e 's|@endcode||' \
  -e 's|@tableofcontents[ ]*|.. contents::\n    :local:\n    :backlinks: entry\n|' \
  -e 's|@verbatim[ ]*|::\n|' \
  -e 's|@endverbatim[ ]*||' \
  -e 's|@image html \([^ ]*\) "\?\(.*\)"\?|.. figure:: \1\n    :align: center\n\n    \2|' \
  -e 's|<a href="\(.*\)">\(.*\)</a>|`\2 <\1>`_|' \
  -e 's|\[\(.*\)\](\(.*\))|`\1 <\2>`_|' \
  -e 's|^ \+$||' \
  -e '/^\*\//d' \
  -e '/^\/\*\*$/d' \
  > "$outdir/$outfile"
