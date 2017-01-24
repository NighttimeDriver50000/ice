#!/bin/bash
if [ "$1" ]; then
    proj="$1"
else
    proj=.
fi

echo "$(dirname "$0")/ice" -Tj "$proj" "@$proj/.iced-profile" refresh -asd
"$(dirname "$0")/ice" -Tj "$proj" "@$proj/.iced-profile" refresh -asd
