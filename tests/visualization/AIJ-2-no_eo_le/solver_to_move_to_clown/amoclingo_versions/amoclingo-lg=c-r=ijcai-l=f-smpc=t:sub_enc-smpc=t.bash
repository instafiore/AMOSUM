# s: amoclingo-lg=c-r=ijcai-l=f-smpc=t:sub_enc-smpc=t.bash
#!/bin/bash

trap ":" 24 15
DIRNAME=`dirname $0`
FILENAME=$1
INSTANCE_FOLD=`dirname $1`


e="encoding-amosum-amo"
ENCODING=$INSTANCE_FOLD/$e.asp

if [[ $FILENAME == *encoding* || $FILENAME == *checker* ]]; then
        echo "Ignoring $FILENAME"
        exit 0
fi

lazy=false
reason=ijcai
lang=cpp
static_mpc=true

instance_basename=$(basename "$FILENAME")

cmn="amoclingo -e $ENCODING -i $FILENAME -l $lazy -r $reason -lg $lang -smpc $static_mpc"

echo "executing $cmn"

eval "$cmn"
exicode=$?

exit $exicode
