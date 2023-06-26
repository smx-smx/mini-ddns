#!/bin/sh
IPADDR="$1"
. ./conf.sh
while ! yaft -L "${USER}:${PASS}" -d "http://update.dyndns.it/?hostname=${DOMAIN}&myip=${IPADDR}"; do
	>&2 echo "Failed, retrying..."
	sleep 1
done
