#!/bin/bash

# Define variables
SRC_DIR="$(pwd)"  # Current directory as source
DEST_USER="edgar"
DEST_IP="192.168.0.154"
DEST_PATH="/cygdrive/c/Users/edgar/OneDrive/Desktop/bot_files/gfx"

# Rsync command
rsync -avz --exclude=".git" --exclude="build" "$SRC_DIR/" "$DEST_USER@$DEST_IP:$DEST_PATH"
