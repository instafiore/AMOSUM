#!/bin/bash

trap ":" 24 15
DIRNAME=`dirname $0`
FILENAME=$1
INSTANCE_FOLD=`dirname $1`


ENCODING=$INSTANCE_FOLD/encoding-plain-amo.asp
if [[ $FILENAME == *encoding* ]];then
        echo Ignoring $FILENAME;
        exit 0
fi

instance_basename=$(basename $FILENAME)
        
cmn="clingo $ENCODING $FILENAME --stats --outf=1"
echo "executing $cmn"
output=$($cmn)
exicode=$?
echo "$output"
exit $exicode