#!/usr/bin/env bash

install() {
    if ! [[ -f "$1/hl_linux" ]]; then
        echo "The installer was unable to locate the Half-Life launcher (hl_linux) at $1."
        exit 1
    fi

    if ! [[ -f "HLFixes.so" ]]; then
        echo "The installer was unable to locate HLFixes.so."
        exit 1
    fi

    echo Copying HLFixes...
    cp HLFixes.so "$1/hl.fx"

    # only make a backup if the current launcher is seemingly okay
    if ! [[ -f "$1/hl_linux.bak" ]] && grep -Fxq 'hw.so' "$path/hl_linux"; then
        echo Making a backup of the launcher...
        cp "$1/hl_linux" "$1/hl_linux.bak"
    fi

    echo Patching the launcher...
    sed -i 's/hw.so/hl.fx/g' "$1/hl_linux"

    echo Done.
}

uninstall() {
    if ! [[ -f "$1/hl_linux" ]]; then
        echo "The installer was unable to locate the Half-Life launcher (hl_linux) at $1."
        exit 1
    fi

    if ! [[ -f "$1/hl_linux.bak" ]]; then
        echo "The installer was unable to locate the backup of the Half-Life launcher (hl_linux.bak) at $1. Please verify integrity in Steam to uninstall."
        exit 1
    fi

    echo Restoring launcher backup...
    cp "$1/hl_linux.bak" "$1/hl_linux"

    echo Cleaning up...
    rm "$1/hl_linux.bak"
    rm "$1/hl.fx"

    echo Done.
}

if [[ $1 == "--install" ]]; then
    path=$2
    if [[ -z $2 ]]; then
        # path wasn't specified, try the default
        path=~/.steam/steam/steamapps/common/Half-Life
    fi

    install "$path"
elif [[ $1 == "--uninstall" ]]; then
    path=$2
    if [[ -z $2 ]]; then
        # path wasn't specified, try the default
        path=~/.steam/steam/steamapps/common/Half-Life
    fi

    uninstall "$path"
else
    # interactive mode

    path=$1
    if [[ -z $1 ]]; then
        # path wasn't specified, try the default
        path=~/.steam/steam/steamapps/common/Half-Life
    fi

    while ! [[ -f "$path/hl_linux" ]]; do
        echo "The installer was unable to locate the Half-Life launcher (hl_linux) at $path."
        read -r -p "Enter the absolute path to Half-Life: " path
        echo
    done

    while :; do
        PS3='Choose an option: '
        options=()

        if grep -Fxq 'hl.fx' "$path/hl_linux"; then
            options+=("Reinstall HLFixes")
        else
            options+=("Install HLFixes")
        fi

        if [[ -f "$path/hl_linux.bak" ]]; then
            options+=("Uninstall HLFixes")
        fi

        options+=("Quit")

        select opt in "${options[@]}"; do
            echo
            case $opt in
            "Reinstall HLFixes")
                install "$path"
                break
                ;;
            "Install HLFixes")
                install "$path"
                break
                ;;
            "Uninstall HLFixes")
                uninstall "$path"
                break
                ;;
            "Quit")
                exit 0
                ;;
            *) ;;
            esac
        done

        echo
    done
fi
