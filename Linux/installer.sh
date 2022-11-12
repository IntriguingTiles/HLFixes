#!/usr/bin/env bash

if [[ $1 == "install" ]]; then
    path=$2
    if [[ -z $2 ]]; then
        # path wasn't specified, try the default
        path=~/.steam/steam/steamapps/common/Half-Life
    fi

    if ! [[ -f "$path/hl_linux" ]]; then
        echo "The installer was unable to locate the Half-Life launcher (hl_linux) at $path."
        exit 1
    fi

    if ! [[ -f "HLFixes.so" ]]; then
        echo "The installer was unable to locate HLFixes.so."
        exit 1
    fi

    echo Copying HLFixes...
    cp HLFixes.so "$path/hl.fx"

    if ! [[ -f "$path/hl_linux.bak" ]]; then
        echo Making a backup of the launcher...
        cp "$path/hl_linux" "$path/hl_linux.bak"
    fi

    echo Patching the launcher...
    sed -i 's/hw.so/hl.fx/g' "$path/hl_linux"

    echo Done.
elif [[ $1 == "uninstall" ]]; then
    path=$2
    if [[ -z $2 ]]; then
        # path wasn't specified, try the default
        path=~/.steam/steam/steamapps/common/Half-Life
    fi

    if ! [[ -f "$path/hl_linux" ]]; then
        echo "The installer was unable to locate the backup of the Half-Life launcher (hl_linux.bak) at $path. Please verify integrity in Steam to uninstall."
        exit 1
    fi

    echo Restoring launcher backup...
    cp "$path/hl_linux.bak" "$path/hl_linux"

    echo Cleaning up...
    rm "$path/hl_linux.bak"
    rm "$path/hl.fx"

    echo Done.
else
    echo "Usage: $0 <install/uninstall> [/path/to/half-life]"
fi
