#!/bin/sh

#LOCKS="USE_RW_SCALABLE_FAIR_LOCKS"
# LOCKS="USE_HCLH_LOCKS USE_SPINLOCK_LOCKS USE_TTAS_LOCKS USE_MCS_LOCKS USE_CLH_LOCKS USE_ARRAY_LOCKS USE_RW_LOCKS USE_TICKET_LOCKS USE_MUTEX_LOCKS USE_HTICKET_LOCKS USE_TTAS_SIMP_LOCKS"
# LOCKS="USE_SPINLOCK_LOCKS USE_TTAS_LOCKS USE_MCS_LOCKS USE_CLH_LOCKS USE_ARRAY_LOCKS USE_TICKET_LOCKS USE_TTAS_SIMP_LOCKS USE_RW_LOCKS USE_RW_FAIR_LOCKS USE_RW_SCALABLE_FAIR_LOCKS"
LOCKS="USE_SPINLOCK_LOCKS USE_CLH_LOCKS USE_RW_LOCKS USE_RW_FAIR_LOCKS USE_RW_SCALABLE_FAIR_LOCKS USE_RW_BOUNDED_LOCKS"

MAKE="";
UNAME=`uname`;
PF='PLATFORM=-DXEON -DHASWELL -DCORE_NUM=4 -DUSE_CDELAY -DNO_RW_BACKOFF -DNO_TICKET_BACKOFF'
if [ $UNAME = "Linux" ];
then
    MAKE=make;
    # jda() { cd $(pwd | sed "s/\(\/$@\/\).*/\1/g"); }
    # jda primitives
else
    MAKE=gmake;
fi;



usage()
{
    echo "$0 [-v] [-s suffix]";
    echo "    -v             verbose";
    echo "    -s suffix      suffix the executable with suffix";
}


USUFFIX="";
VERBOSE=0;
 while getopts "hs:v" OPTION
 do
      case $OPTION in
          h)
	      usage;
              exit 1
              ;;
          s)
              USUFFIX="_$OPTARG"
	      echo "Using suffix: $USUFFIX"
              ;;
          v)
              VERBOSE=1
              ;;
          ?)
	      usage;
              exit;
              ;;
      esac
 done

for lock in $LOCKS
do
    echo "Building: $lock";
    touch Makefile;
    if [ $VERBOSE -eq 1 ]; then
	$MAKE "$PF" all LOCK_VERSION=-D$lock
    else
	$MAKE "$PF" all LOCK_VERSION=-D$lock > /dev/null;
    fi
    suffix=`echo $lock | sed -e "s/USE_//g" -e "s/_LOCK\?//g" | tr "[:upper:]" "[:lower:]"`;
    mv bank bank_$suffix$USUFFIX;
    mv bank_one bank_one_$suffix$USUFFIX;
    mv bank_simple bank_simple_$suffix$USUFFIX;
    mv stress_test stress_test_$suffix$USUFFIX;
    mv stress_one stress_one_$suffix$USUFFIX;
    mv core_transitions core_transitions_$suffix$USUFFIX;
    #mv stress_latency stress_latency_$suffix$USUFFIX;
    mv uncontended uncontended_$suffix$USUFFIX;
    mv test_correctness test_correctness_$suffix$USUFFIX;
    mv test_correctness_rw test_correctness_rw_$suffix$USUFFIX;
done;
