# urltest_webdav

Want to stress-test your WebDAV server?  This project is a C program that uses libcurl to perform a sequence of random-order WebDAV-style uploads of a directory/file to a remote URL.  The fine-grain timing features present in libcurl are used to generate timing statistics for each directory/file present.  The data points embody the time measured by libcurl to:
- perform DNS lookup of the target host
- open a TCP connection to the target host
- establish optional TLS/SSL encryption on the connection
- send the HTTP request
- begin reading the HTTP response
as well as
- total time for HTTP request and response
- bytes transferred for the HTTP response
Statistics for the requests are aggregated by:
- all HTTP status codes
- 200-level HTTP status codes
- 300-level HTTP status codes
- 400-level HTTP status codes
- 500-level HTTP status codes

The program can process one or more files or directories, mirroring them to a single URL.  For files, the file is:

1. Uploaded (`PUT`) to the base URL
2. Checked for properties (`PROPFIND`)
3. Downloaded (`GET`) from the remove URL (all received data discarded)
4. Removed (`DELETE`) from the remote URL

Directories are scanned to produce an in-memory representation that is processed in a semi-random fashion (versus being processed in a fully depth- or breadth-first order).  Files encountered are processed (eventually) in the same sequence as above; for directories the (eventual) sequence is:

1. Create the directory (`MKCOL`) at the remote URL
2. Upload all child entities
3. Check for all properties (`PROPFIND`)
4. Download all child entities
5. Download (`GET`) from the remove URL (received file listing is discarded)
6. Remove all child entities
7. Remove (`DELETE`) at the remote URL

This procedure is performed once by default, but can be repeated any number of times.  For example:

~~~~
$ ./urltest_webdav -u mud -p 'here is my passw0rd!' -U https://webdav.www.server.org/upload_dir -vnlg2 Makefile

Mirroring content to 'https://webdav.www.server.org/upload_dir'

Commencing 1 iteration...
201 📄 [0000 ↑] 8138     Makefile
207 📄 [0000 ℹ] 8138     Makefile
200 📄 [0000 ↓] 8138     Makefile
204 📄 [0000 ✖︎] 8138     Makefile
Generation 1 completed
201 📄 [0001 ↑] 8138     Makefile
207 📄 [0001 ℹ] 8138     Makefile
200 📄 [0001 ↓] 8138     Makefile
204 📄 [0001 ✖︎] 8138     Makefile
Generation 2 completed
~~~~

Here is an example using this git repository directory itself (lines omitted and a colon shown for brevity):

~~~~
$ urltest_webdav -u mud -p 'here is my passw0rd!' -U https://webdav.www.server.org/upload_dir/urltest_webdav -vnl /tmp/urltest_webdav

Mirroring content to 'https://webdav.www.udel.edu/www1_farm_webdav/urltest_webdav'

Commencing 1 iteration...
201 📁 [0000 ↑] 510      /tmp/urltest_webdav
201 📄 [0000 ↑] 1363     /tmp/urltest_webdav/http_stats.h
201 📄 [0000 ↑] 17315    /tmp/urltest_webdav/fs_entity.c
201 📄 [0000 ↑] 2324     /tmp/urltest_webdav/util_fns.c
207 📄 [0000 ℹ] 2324     /tmp/urltest_webdav/util_fns.c
207 📄 [0000 ℹ] 1363     /tmp/urltest_webdav/http_stats.h
201 📄 [0000 ↑] 1266     /tmp/urltest_webdav/http_ops.h
207 📄 [0000 ℹ] 17315    /tmp/urltest_webdav/fs_entity.c
201 📄 [0000 ↑] 370      /tmp/urltest_webdav/util_fns.h
207 📄 [0000 ℹ] 370      /tmp/urltest_webdav/util_fns.h
207 📄 [0000 ℹ] 1266     /tmp/urltest_webdav/http_ops.h
201 📄 [0000 ↑] 2856     /tmp/urltest_webdav/fs_entity.h
200 📄 [0000 ↓] 1363     /tmp/urltest_webdav/http_stats.h
200 📄 [0000 ↓] 2324     /tmp/urltest_webdav/util_fns.c
201 📄 [0000 ↑] 3594     /tmp/urltest_webdav/README.md
204 📄 [0000 ✖︎] 1363     /tmp/urltest_webdav/http_stats.h
201 📄 [0000 ↑] 1734     /tmp/urltest_webdav/CMakeLists.txt
207 📄 [0000 ℹ] 3594     /tmp/urltest_webdav/README.md
204 📄 [0000 ✖︎] 2324     /tmp/urltest_webdav/util_fns.c
201 📄 [0000 ↑] 11352    /tmp/urltest_webdav/http_ops.c
200 📄 [0000 ↓] 370      /tmp/urltest_webdav/util_fns.h
200 📄 [0000 ↓] 3594     /tmp/urltest_webdav/README.md
207 📄 [0000 ℹ] 11352    /tmp/urltest_webdav/http_ops.c
207 📄 [0000 ℹ] 2856     /tmp/urltest_webdav/fs_entity.h
204 📄 [0000 ✖︎] 370      /tmp/urltest_webdav/util_fns.h
200 📄 [0000 ↓] 11352    /tmp/urltest_webdav/http_ops.c
207 📄 [0000 ℹ] 1734     /tmp/urltest_webdav/CMakeLists.txt
 :
204 📄 [0000 ✖︎] 424      /tmp/urltest_webdav/.git/hooks/pre-applypatch.sample
207 📄 [0000 ℹ] 478      /tmp/urltest_webdav/.git/hooks/applypatch-msg.sample
207 📄 [0000 ℹ] 189      /tmp/urltest_webdav/.git/hooks/post-update.sample
204 📁 [0000 ✖︎] 102      /tmp/urltest_webdav/.git/objects/cc
200 📄 [0000 ↓] 478      /tmp/urltest_webdav/.git/hooks/applypatch-msg.sample
207 📁 [0000 ℹ] 238      /tmp/urltest_webdav/.git/objects
200 📄 [0000 ↓] 189      /tmp/urltest_webdav/.git/hooks/post-update.sample
200 📄 [0000 ↓] 1348     /tmp/urltest_webdav/.git/hooks/pre-push.sample
200 📁 [0000 ↓] 238      /tmp/urltest_webdav/.git/objects
204 📁 [0000 ✖︎] 238      /tmp/urltest_webdav/.git/objects
204 📄 [0000 ✖︎] 1348     /tmp/urltest_webdav/.git/hooks/pre-push.sample
207 📄 [0000 ℹ] 544      /tmp/urltest_webdav/.git/hooks/pre-receive.sample
204 📄 [0000 ✖︎] 189      /tmp/urltest_webdav/.git/hooks/post-update.sample
200 📄 [0000 ↓] 544      /tmp/urltest_webdav/.git/hooks/pre-receive.sample
204 📄 [0000 ✖︎] 478      /tmp/urltest_webdav/.git/hooks/applypatch-msg.sample
204 📄 [0000 ✖︎] 544      /tmp/urltest_webdav/.git/hooks/pre-receive.sample
207 📁 [0000 ℹ] 408      /tmp/urltest_webdav/.git/hooks
200 📁 [0000 ↓] 408      /tmp/urltest_webdav/.git/hooks
204 📁 [0000 ✖︎] 408      /tmp/urltest_webdav/.git/hooks
207 📁 [0000 ℹ] 442      /tmp/urltest_webdav/.git
200 📁 [0000 ↓] 442      /tmp/urltest_webdav/.git
204 📁 [0000 ✖︎] 442      /tmp/urltest_webdav/.git
207 📁 [0000 ℹ] 510      /tmp/urltest_webdav
200 📁 [0000 ↓] 510      /tmp/urltest_webdav
204 📁 [0000 ✖︎] 510      /tmp/urltest_webdav
Generation 1 completed
~~~~

Were this same command to be repeated, the order of file/directory processing would look different.

Command line options are present to alter the verbosity of the program, provide static hostname-to-IP mappings, set HTTP basic authentication parameters, and trigger a dry-run testing (no actual HTTP transactions).

~~~~
$ ./urltest_webdav -h
version 1.0.0
built Nov 10 2017 16:48:10
usage:

  ./urltest_webdav {options} <directory> {<directory> ..}

 options:

  --help/-h                    show this information

  --long-listing/-l            list the discovered file hierarchy in an extended
                               format
  --short-listing/-s           list the discovered file hierarchy in a compact
                               format
  --no-listing/-n              do not list the discovered file hierarchy
  --ascii/-a                   restrict to ASCII characters

  --verbose/-v                 display additional information to stdout as the
                               program progresses
  --verbose-curl/-V            ask cURL to display verbose request progress to
                               stderr
  --dry-run/-d                 do not perform any HTTP requests, just show an
                               activity trace
  -t                           show HTTP timing statistics as a table to stdout
  --show-timings=<out>         show HTTP timing statistics at the end of the run

                                 <out> = <format>{:<path>}
                                 <format> = table | csv | tsv

  --generations/-g <#>         maximum number of generations to iterate

  --base-url/-U <URL>          the base URL to which the content should be mirrored
  --host-mapping/-m <hostmap>  provide a static DNS mapping for a hostname and TCP/IP
                               port

                                 <hostmap> = <hostname>:<port>:<ip address>

  --no-delete/-D               do not delete anything on the remote side
  --username/-u <string>       use HTTP basic authentication with the given string as
                               the username
  --password/-p <string>       use HTTP basic authentication with the given string as
                               the password
  --no-cert-verify/-k          do not require SSL certificate verfication for connections
                               to succeed
  --no-random-walk/-W          process the file list as a simple depth-first traversal
  --ranged-ops/-r              enable ranged GET operations
  --no-options/-O              disable OPTIONS operations

~~~~
