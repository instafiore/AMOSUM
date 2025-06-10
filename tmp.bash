while read LINE; do
    f=$(basename $LINE);
    f=${f%.out};
    f=${f%.asp};
    echo $f;
    python prop_clingo/run.py -problem kn -enc encoding-amosum-amo -i $f -lang c
    
done < /Users/instafiore/Desktop/wrong_instances