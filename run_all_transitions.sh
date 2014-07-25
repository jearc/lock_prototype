for files in libslock/core_transitions*
do
    echo $files
    ./$files -a 0 -p 0 -l 1 -n 4
done
