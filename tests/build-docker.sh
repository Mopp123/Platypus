#!/bin/bash

echo "Building docker image..."
docker build . -t platypus-web-test

echo "creating container..."
docker compose -f docker-compose.yml create
