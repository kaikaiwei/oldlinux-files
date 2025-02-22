#!/bin/bash
# Exercising the 'date' command

echo "The number of days since the year's beginning is `date +%j`."
# Needs a leading '+' to invoke formatting.
# %j gives day of year.

echo "The number of seconds elapsed since 01/01/1970 is `date +%s`."
#  %s yields number of seconds since "UNIX epoch" began,
#+ but how is this useful?

prefix=temp
suffix=`eval date +%s`  # The "+%s" option to 'date' is GNU-specific.
filename=$prefix.$suffix
echo $filename
#  It's great for creating "unique" temp filenames,
#+ even better than using $$.

# Read the 'date' man page for more formatting options.

exit 0
