#!/bin/bash
apps=`cat apps.txt`
#apps="blur"

halide_dir=${HOME}/ravi

start_time=`date "+%Y-%m-%d.%H%M%S"`
rundirname="${HOSTNAME}-schmoo-${start_time}"
mkdir -p $rundirname
rundir=`realpath $rundirname`
cd $halide_dir
dirty=$((`git status --porcelain | grep ' M ' | wc -l` > 1))
commit=`git rev-parse HEAD`
cd -
echo "hostname: ${HOSTNAME}" > $rundir/config.txt
echo "commit: ${commit}" >> $rundir/config.txt
echo "dirty: ${dirty}" >> $rundir/config.txt
echo "start_time: ${start_time}" >> $rundir/config.txt

errlog="${rundir}/err.log"

echo "Testing $apps"
echo "   in $rundir"

MEMSIZE=${MEMSIZE:-131072 524288 2097152 8388608 33554432}
BALANCE=${BALANCE:-1 2 3 5 7 10 14 19 28}
VEC=${VEC:-4 8}
PAR=${PAR:-1 2 4 8}
THREADS=${THREADS:-1 2 4 8}

#echo "This will remove {gen,run}.*.txt - OK?"; read
for app in $apps; do
    
    echo "============================================================" >> $errlog
    date "+$app (%Y-%m-%d %H:%M:%S)" >> $errlog
    echo ""
    echo "============================================================"
    pushd $app
    echo "$app"
    apprundir="${rundir}/${app}"
    mkdir -p "$apprundir"
    
    make -s clean
    #rm -f "gen.*.*.*.txt" "run.*.*.*.txt"

    for vec in $VEC
    do
        echo "  vec: $vec"
        for par in $PAR
        do
            echo "    par: $par"
            # 16kB  = 131072
            # 32kB  = 262144
            # 64kB  = 524288
            # 128kB = 1048576
            # 256kB = 2097152
            # 512kB = 4194304
            # 1MB   = 8388608
            # 2MB   = 16777216
            # 4MB   = 33554432
            # 8MB   = 67108864
            for memsize in $MEMSIZE
            do
                echo "      memsize: $memsize"
                for balance in $BALANCE
                do
                    echo "        balance: $balance"
                    genfile="$apprundir/gen.$par.$vec.$balance.$memsize.txt"
                    export HL_AUTO_PARALLELISM=$par
                    export HL_AUTO_VEC_LEN=$vec
                    export HL_AUTO_BALANCE=$balance
                    export HL_AUTO_FAST_MEM_SIZE=$memsize
                    # if [[ -f ./regen.sh ]]; then
                    #     bash ./regen.sh 2>> $errlog > $genfile
                    # else
                        # rm -f "*.o"
                        # make -s cleangen 2>> $errlog
                        # make -s gen > $genfile 2>> $errlog
                        # echo "$WARN<--!$ must cull out naive gen log data!!" >> $genfile # Gen data gets shoved in twice - need to remove duplicate keys
                    # fi
                    make -s clean_auto 2>> $errlog
                    make -s auto > $genfile 2>> $errlog
                    for runthreads in $THREADS
                    do
                        echo "          numthreads: $runthreads"
                        runfile="$apprundir/run.$par.$vec.$balance.$memsize.$runthreads.txt"
                        echo "[$app.$par.$vec.$balance.$memsize.$runthreads]" > $runfile
                        cat $genfile >> $runfile
                        ./test.sh auto $runthreads >> \
                            $runfile \
                            2> $errlog
                    done

                done
            done
        done
    done
    popd
done
