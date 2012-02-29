#! /bin/sh

for i in count stats histo dump merge query qhisto qdump qmerge cite; do
    echo "\\subsection{$i}"
    jellyfish $i --help | ruby option_to_tex /dev/fd/0
    echo
done
