#!/bin/bash

config_name=${1}
echo "Config for tank: $config_name"

docker run \
    -v $(pwd):/var/loadtest \
    --net host \
    -it yandex/yandex-tank -c $config_name ammo.txt

#    -v $SSH_AUTH_SOCK:/ssh-agent -e SSH_AUTH_SOCK=/ssh-agent \
