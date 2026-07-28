#!/bin/bash

version=$1

if [ -z "$version" ]; then
    echo "Usage: $0 <version>"
    echo "Example: $0 v0.98"
    exit 1
fi

# Detect OS and architecture
OS=$(uname -s)

# Set the appropriate file and platform directory based on OS
if [ "$OS" = "Darwin" ]; then
    file="ftx-macos"
    platform="mac"
elif [ "$OS" = "Linux" ]; then
    file="ftx-linux"
    platform="lin"
else
    echo "Unsupported operating system: $OS"
    exit 1
fi

toolDir=./tools/bin/$platform/ftx
url="https://github.com/willll/ftx/releases/download/${version}/$file"
target="$toolDir/$file"

if [ ! -d "$toolDir" ]; then
  mkdir -p $toolDir
else
  if [ "$(ls -A $toolDir)" ]; then
    echo "ftx directory is not empty! Proceeding will clear all of its contents."
    read -r -p "Are you sure? [y/N] " response

    case "$response" in
    [yY][eE][sS]|[yY]) 
        rm -rf $toolDir/*
        ;;
    *)
        exit
        ;;
    esac
  fi
fi

# Ensure parent directories exist
mkdir -p $(dirname $toolDir)
cd $toolDir
wget $url # -q --show-progress

if [ ! -f $file ]; then
  echo "Installation failed!";
  exit
fi

# Rename the binary to just 'ftx'
mv $file ftx

printf "\nSetting permissions\n";
chmod -R +x .
cd ../../..
