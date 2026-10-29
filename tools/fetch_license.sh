#!/bin/sh
# Downloads the official, unmodified AGPL-3.0 text into ./LICENSE
set -e
cd "$(dirname "$0")/.."
curl -fsSL https://www.gnu.org/licenses/agpl-3.0.txt -o LICENSE
echo "LICENSE written ($(wc -l < LICENSE) lines)."
