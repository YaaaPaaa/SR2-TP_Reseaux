#!/bin/bash
# Licence d'Informatique - UE SR2 - E. Lavinal
# Université Toulouse III - Paul Sabatier / FSI / DDI
# v1.0

# --------------------------------------------------
# TEST SCRIPT for reliable data transfer protocols
# --------------------------------------------------

BLUE='\033[0;34m'
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

LOG=./log
TEST=./tests
TIMEOUT=20
CONFIG=config.txt

function restore_default_config {

    cp $TEST/c01-txt-no-error.txt ./config.txt
}

function kill_running_processes {

    #for p in `pgrep emetteur`; do kill $p; done
    #for p in `pgrep recepteur`; do kill $p; done
    pkill emetteur
    pkill recepteur
}

# $1 sender's PID file; $2 receiver's PID file
function wait_timeout {

    sleep $TIMEOUT;
    if [[ -e $1 ]]; then
        echo -e "*** ${RED}ERROR - TIMEOUT SENDER !${NC} ***"
        echo -e "*** ${RED}CHECK LOG...${NC} ***"
        pkill emetteur
    fi
    if [[ -e $2 ]]; then
        echo -e "*** ${RED}ERROR - TIMEOUT RECEIVER !${NC} ***"
        echo -e "*** ${RED}CHECK LOG...${NC} ***"
        pkill recepteur
    fi
}

# $1 test number
function run_processes {

    pidFileSender=".pidsender"
    pidFileReceiver=".pidreceiver"
    rm $pidFileReceiver $pidFileSender 2> /dev/null
    echo "--> Running receiver..."
    (./bin/recepteur > $LOG/$1_rec_log.txt 2>&1 ; rm $pidFileReceiver ;) &
    pidR=$!
    echo $pidR > $pidFileReceiver
    sleep 1
    echo "--> Running sender..."
    (./bin/emetteur > $LOG/$1_sen_log.txt 2>&1 ; rm $pidFileSender ;) &
    pidE=$!
    echo $pidE > $pidFileSender
    wait_timeout $pidFileSender $pidFileReceiver &
    timeoutPid=$!
    wait $pidE $pidR 2> /dev/null
    sleep 1
    # kill wait_timewait if sender and receiver terminated properly
    kill $timeoutPid 2> /dev/null
    wait $timeoutPid 2> /dev/null
    # The kill utility exits 0 on success, and >0 if an error occurs
    # Error means we could not kill wait_timeout because TIMEOUT has expired
    # There must be an infinite loop in sender or receiver... 
    # return $?
}

# $1 test number; $2 test name
function run_test {

    printf "\n*** Running test $1 ($2) ***\n"
    
    # run sender and receiver
    run_processes $1

    # check files
    F1=`cat $CONFIG | grep FICHIER_IN | cut -d ' ' -f 2`
    F2=`cat $CONFIG | grep FICHIER_OUT | cut -d ' ' -f 2`

    diff $F1 $F2 > /dev/null
    if [[ $? -eq 0 ]]; then
        printf "%s %s [${GREEN}OK${NC}]\n" $1 $2
        rm $F2 # clean output file
        return 1
    else
        printf "%s %s [${RED}Different files${NC}]\n" $1 $2
        rm $F2 # clean output file
        return 0
    fi
}

# =============================================================================

function run_tests_1 {

    echo ""
    echo "************************************"
    echo -e "**** ${BLUE}Tests sans erreur ni perte${NC} ****"
    echo "************************************"
    # TXT sans erreur/perte
    cp $TEST/c01-txt-no-error.txt ./config.txt
    run_test 1.1 TXT-NO-ERROR-NO-LOSS    
    # JPG sans erreur/perte
    cp $TEST/c02-jpg-no-error.txt ./config.txt
    run_test 1.2 JPG-NO-ERROR-NO-LOSS
}

function run_tests_2 {

    echo ""
    echo "**************************************************"
    echo -e "**** ${BLUE}Tests erreurs sur les paquets de données${NC} ****"
    echo "**************************************************"
    # TXT erreurs sur les données
    cp $TEST/c03-txt-error-data.txt ./config.txt
    run_test 2.1 TXT-ERROR-DATA
}

function run_tests_3 {

    echo ""
    echo "************************************************************"
    echo -e "**** ${BLUE}Tests erreurs et pertes sur les paquets de données${NC} ****"
    echo "************************************************************"
    # TXT erreurs et pertes sur données
    cp $TEST/c04-txt-error-loss-data.txt ./config.txt
    run_test 3.1 TXT-ERROR-LOSS-DATA    
    # JPG erreurs et pertes sur données
    cp $TEST/c05-jpg-error-loss-data.txt ./config.txt
    run_test 3.2 JPG-ERROR-LOSS-DATA
}

function run_tests_4 {

    echo ""
    echo "*****************************************************************"
    echo -e "**** ${BLUE}Tests erreurs et pertes sur les paquets d'acquittements${NC} ****"
    echo "*****************************************************************"
    # TXT erreurs et pertes sur ack
    cp $TEST/c06-txt-error-loss-ack.txt ./config.txt
    run_test 4.1 TXT-ERROR-LOSS-ACK    
    # JPG erreurs et pertes sur ack
    cp $TEST/c07-jpg-error-loss-ack.txt ./config.txt
    run_test 4.2 JPG-ERROR-LOSS-ACK
    # TXT loss last ack
    cp $TEST/c08-txt-loss-last-ack.txt ./config.txt
    run_test 4.3 TXT-LOSS-LAST-ACK    
}

function run_tests_5 {

    echo ""
    echo "******************************************************"
    echo -e "**** ${BLUE}Tests erreurs et pertes sur tous les paquets${NC} ****"
    echo "******************************************************"
    # JPG erreurs et pertes sur tous les paquets
    cp $TEST/c09-jpg-error-loss-all.txt ./config.txt
    run_test 5.1 JPG-ERROR-LOSS-ALL
}

# =============================================================================
#                 Script de tests
# =============================================================================

if [[ ! -d "$LOG" ]]; then
    mkdir $LOG;
fi

if [[ ! -f "bin/recepteur" || ! -f "bin/emetteur" ]]; then
    echo "Fichiers executables introuvables !"
    echo "Vous devez compiler vos programmes avant d'utiliser le script de tests."
    exit 1
fi

kill_running_processes

no_error_no_loss="Sans erreur ni perte"
error_data="Erreurs sur les paquets de données"
error_loss_data="Erreurs et Pertes sur les paquets de données"
error_loss_ack="Erreurs et Pertes sur les paquets d'acquittements"
error_loss_all="Erreurs et Pertes sur tous les paquets"

options=("$no_error_no_loss" "$error_data" "$error_loss_data" "$error_loss_ack" "$error_loss_all" "CHAOS! Run all!")

echo ">>> TESTS TP RESEAUX SR2 <<<"
echo "Choisir les tests que vous voulez exécuter :"

select opt in "${options[@]}"
do
    # $REPLY is the selected number
    case $REPLY in
    1) run_tests_1 ; break ;;
    2) run_tests_2 ; break ;;
    3) run_tests_3 ; break ;;
    4) run_tests_4 ; break ;;
    5) run_tests_5 ; break ;;
    6) run_tests_1; run_tests_2; run_tests_3; run_tests_4; run_tests_5 ; break ;;
    *) echo "Option invalide" ;;
    esac
done

# Clean...
kill_running_processes
restore_default_config

exit 0
