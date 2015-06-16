#!/bin/sh

#
# This script automatically updates translation template from
# the source code of XQF
#
POT=po/xqf.pot
VERSION=$(git describe --abbrev=0 --tags)

find ./ -name '*.c' -o -name '*.ui' -o -name 'xqf.desktop.in' | sort | xgettext \
\--from-code=UTF-8 --package-name="XQF" --copyright-holder="XQF Team" --msgid-bugs-address="https://github.com/xqf/xqf/issues" --no-location --package-version="${VERSION}" -o ${POT} -k_ -k__ -kCG_TranslateString -ktrap_TranslateString -kTRANSLATE -f -
