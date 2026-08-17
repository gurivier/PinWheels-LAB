#!/bin/sh

. mqtt_params.sh
. utils.sh

echodate '>>> Listening to MQTT topic "'"$TOPIC"'" on host "'"$HOST"'"'

mosquitto_sub -F '[@Y-@m-@d @H:@M:@S] Message: "%p"' -h $HOST -t "$TOPIC"

