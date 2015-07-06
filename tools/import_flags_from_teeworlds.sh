#! /bin/sh

if [ "x${1}" = 'x-h' -o "x${1}" = '--help' -o "x${1}" = 'x' -o "x${2}" = 'x' ]
then
	echo "Usage: ${0} [XQF's source root directory] [Teeworlds's source root directory"
	exit
fi

xqf_repo="${1}"
teeworlds_repo="${2}"

xqf_flags_path="${xqf_repo}/pixmaps/flags/country"
teeworlds_flags_path="${teeworlds_repo}/datasrc/countryflags"

(cd "${teeworlds_flags_path}"; ls ??.png ) | while read upName
do
	lowName="$(echo "${upName}" | tr '[:upper:]' '[:lower:]')"
	echo convert "${teeworlds_flags_path}/${upName}" to "${xqf_flags_path}/${lowName}"
	convert "${teeworlds_flags_path}/${upName}" -resize "14x14^" -gravity center -crop 22x14+0+0 +repage "${xqf_flags_path}/${lowName}"
done

#EOF
