#!/bin/sh

. mqtt_params.sh
. utils.sh

echodate 'Publishing to MQTT topic "'"$TOPIC"'" on host "'"$HOST"'"'

for message in '10500|' '20500|' '30500|' '40500|' '00900|' '00000|'; do
    echodate 'Sending: "'"$message"'"'
    mosquitto_pub -h $HOST -t "$TOPIC" -m "$message"
    sleep 3
done

echodate "Done."

