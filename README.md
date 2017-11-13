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

The program can process one or more files or directories, mirroring them to a single URL.  For files, the actions are:

1. Upload (`PUT`) to the remote URL
2. Issue an HTTP `OPTIONS` request against the remote URL
3. Retrieve entity properties (`PROPFIND`)
4. Download (`GET`) from the remote URL (received data is discarded)
5. Download part of the file (`GET` with a `Range` header) from the remote URL
6. Remove (`DELETE`) the remote URL

Directories are scanned to produce an in-memory representation that is processed in a semi-random fashion (versus being processed in a fully depth- or breadth-first order).  Files encountered are processed (eventually) in the same sequence as above; for directories the (eventual) sequence is:

1. Create the directory (`MKCOL`) at the remote URL
2. Upload all child entities
3. Issue an HTTP `OPTIONS` request against the remote URL
4. Retrieve directory properties (`PROPFIND`)
5. Download all child entities
6. Download (`GET`) from the remote URL (received file listing is discarded)
7. Remove all child entities
8. Remove (`DELETE`) the remote URL

This procedure is performed once by default but can be repeated any number of times.  For example:

~~~~
$ ./urltest_webdav -u mud -p 'here is my passw0rd!' -U https://webdav.www.server.org/upload_dir -vnlg2 sample/

Mirroring content to 'https://webdav.www.server.org/upload_dir'
WARNING:  directory cycle would result for /home/user/sample/iso/me
Using modified base URL of https://webdav.www.server.org/upload_dir/sample/

Commencing 2 iterations...
201 📁 [0000 ↑] 272      /Users/frey/Desktop/sample
201 📁 [0000 ↑] 204      /Users/frey/Desktop/sample/uvc-util
201 📄 [0000 ↑] 5266     /Users/frey/Desktop/sample/uvc-util/README.md
201 📁 [0000 ↑] 102      /Users/frey/Desktop/sample/iso
201 📁 [0000 ↑] 476      /Users/frey/Desktop/sample/urltest_webdav
201 📁 [0000 ↑] 442      /Users/frey/Desktop/sample/lmdb
200 📄 [0000 …] 5266     /Users/frey/Desktop/sample/uvc-util/README.md
201 📁 [0000 ↑] 306      /Users/frey/Desktop/sample/uvc-util/src
201 📄 [0000 ↑] 1653     /Users/frey/Desktop/sample/urltest_webdav/http_ops.h
200 📁 [0000 …] 102      /Users/frey/Desktop/sample/iso
201 📄 [0000 ↑] 5083     /Users/frey/Desktop/sample/uvc-util/src/UVCValue.m
  :
200 📄 [0000 ↓] 462      /Users/frey/Desktop/sample/lmdb/lib/config.h
200 📄 [0000 ↓] 3161     /Users/frey/Desktop/sample/lmdb/lib/lmlog.c
204 📄 [0000 ✖︎] 3161     /Users/frey/Desktop/sample/lmdb/lib/lmlog.c
204 📄 [0000 ✖︎] 13162    /Users/frey/Desktop/sample/lmdb/lib/fscanln.c
204 📄 [0000 ✖︎] 7019     /Users/frey/Desktop/sample/lmdb/lib/lmfeature.h
204 📄 [0000 ✖︎] 45060    /Users/frey/Desktop/sample/lmdb/lib/lmdb.c
204 📄 [0000 ✖︎] 462      /Users/frey/Desktop/sample/lmdb/lib/config.h
200 📁 [0000 …] 544      /Users/frey/Desktop/sample/lmdb/lib
207 📁 [0000 ℹ] 544      /Users/frey/Desktop/sample/lmdb/lib
200 📁 [0000 ↓] 544      /Users/frey/Desktop/sample/lmdb/lib
204 📁 [0000 ✖︎] 544      /Users/frey/Desktop/sample/lmdb/lib
200 📁 [0000 …] 442      /Users/frey/Desktop/sample/lmdb
207 📁 [0000 ℹ] 442      /Users/frey/Desktop/sample/lmdb
200 📁 [0000 ↓] 442      /Users/frey/Desktop/sample/lmdb
204 📁 [0000 ✖︎] 442      /Users/frey/Desktop/sample/lmdb
200 📁 [0000 …] 272      /Users/frey/Desktop/sample
207 📁 [0000 ℹ] 272      /Users/frey/Desktop/sample
200 📁 [0000 ↓] 272      /Users/frey/Desktop/sample
204 📁 [0000 ✖︎] 272      /Users/frey/Desktop/sample
Generation 1 completed
201 📁 [0001 ↑] 272      /Users/frey/Desktop/sample
201 📁 [0001 ↑] 442      /Users/frey/Desktop/sample/lmdb
201 📁 [0001 ↑] 204      /Users/frey/Desktop/sample/lmdb/lmdb_ls
201 📁 [0001 ↑] 204      /Users/frey/Desktop/sample/uvc-util
201 📁 [0001 ↑] 170      /Users/frey/Desktop/sample/ud_slurm_addons
201 📁 [0001 ↑] 170      /Users/frey/Desktop/sample/ud_slurm_addons/job_submit
  :
207 📄 [0001 ℹ] 10351    /Users/frey/Desktop/sample/lmdb/lib/lmfeature.c
200 📄 [0001 ↓] 10351    /Users/frey/Desktop/sample/lmdb/lib/lmfeature.c
204 📄 [0001 ✖︎] 3161     /Users/frey/Desktop/sample/lmdb/lib/lmlog.c
204 📄 [0001 ✖︎] 4537     /Users/frey/Desktop/sample/lmdb/lib/util_fns.c
204 📄 [0001 ✖︎] 4744     /Users/frey/Desktop/sample/lmdb/lib/fscanln.h
204 📄 [0001 ✖︎] 10351    /Users/frey/Desktop/sample/lmdb/lib/lmfeature.c
200 📄 [0001 …] 142      /Users/frey/Desktop/sample/lmdb/lib/CMakeLists.txt
207 📄 [0001 ℹ] 142      /Users/frey/Desktop/sample/lmdb/lib/CMakeLists.txt
200 📄 [0001 ↓] 142      /Users/frey/Desktop/sample/lmdb/lib/CMakeLists.txt
204 📄 [0001 ✖︎] 142      /Users/frey/Desktop/sample/lmdb/lib/CMakeLists.txt
200 📁 [0001 …] 544      /Users/frey/Desktop/sample/lmdb/lib
207 📁 [0001 ℹ] 544      /Users/frey/Desktop/sample/lmdb/lib
200 📁 [0001 ↓] 544      /Users/frey/Desktop/sample/lmdb/lib
204 📁 [0001 ✖︎] 544      /Users/frey/Desktop/sample/lmdb/lib
200 📁 [0001 …] 442      /Users/frey/Desktop/sample/lmdb
207 📁 [0001 ℹ] 442      /Users/frey/Desktop/sample/lmdb
200 📁 [0001 ↓] 442      /Users/frey/Desktop/sample/lmdb
204 📁 [0001 ✖︎] 442      /Users/frey/Desktop/sample/lmdb
200 📁 [0001 …] 272      /Users/frey/Desktop/sample
207 📁 [0001 ℹ] 272      /Users/frey/Desktop/sample
200 📁 [0001 ↓] 272      /Users/frey/Desktop/sample
204 📁 [0001 ✖︎] 272      /Users/frey/Desktop/sample
Generation 2 completed
~~~~

Each time this command is repeated, the order of file/directory processing would look different.  The `--no-random-walk/-W` can be used to inhibit this function and reproduce the same order of operations on each invocation.

Other command line options are present to alter the verbosity of the program, provide static hostname-to-IP mappings, set HTTP basic authentication parameters, export HTTP timing statistics, and trigger dry-run testing (no actual HTTP transactions).

~~~~
$ ./urltest_webdav -h
version 1.0.0
built Nov 13 2017 14:04:43
usage:

  ./urltest_webdav {options} <entity> {<entity> ..}

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

  --base-url/-U <remote URL>   the base URL to which the content should be mirrored;
                               if this parameter is omitted then each <entity> must be
                               a pair of values:  the local file/directory and the base
                               URL to which to mirror it:

                                 <entity> = <file|directory> <remote URL>

                               if the --base-url/-U option is used, then <entity> is just
                               the <file|directory> portion
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
