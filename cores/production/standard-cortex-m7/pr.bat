echo off
iccarm -I src --silent --preprocess=n temp.i --preinclude %1 %2
type temp.i
del temp.i