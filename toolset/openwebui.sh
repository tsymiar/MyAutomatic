#!/bin/bash
# Docker: ensure open-webui container is running
ID=$(docker ps --filter name=open-webui -q 2>/dev/null)
if [ -n "$ID" ]; then
    docker restart "$ID"
else
    docker run -d \
        --sysctl net.ipv6.conf.all.disable_ipv6=1 \
        -e WEBUI_NAME="JetsonLLM" \
        -p 8083:8080 \
        --add-host=host.docker.internal:host-gateway \
        -v ~/shenyrion/software/openwebui/backend:/app/backend \
        --name open-webui \
        --restart always \
        ghcr.io/open-webui/open-webui:main
fi
