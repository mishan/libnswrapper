libnswrapper
============

Name Services Wrapper LD_PRELOAD Hack. Useful for quickly overriding host lookups without changing /etc/hosts.

Usage:

`make`

```NSWRAPPER_HOST=hosttooverride.com:replacementhost.com,tooverride2.com:replacement2.com,[,...] LD_PRELOAD=./libnswrapper.so $BROWSER```
Or use something other than browser.

You can also install this onto your system -- currently it will copy stuff to /usr/local/lib


KNOWN ISSUES:
* setuid programs like ping do not honor LD_PRELOAD -- might consider using dynamic HOSTALIASES file
