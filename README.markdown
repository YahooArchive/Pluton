# The Pluton Framework

## Introduction

Pluton is a simple framework which makes network applications
lighter-weight, easier to debug, lower latency *and* potentially less
resource intensive. The core features of the framework include:

* an interface which encourages and simplifies parallel requests to reduce latency (without the complexity of threads!)

* a convenient, fast and simple API available in C, C++, php, perl
and Java on FreeBSD and RHEL.
* encapsulation of services into self-contained, independent
executables
* a serialization-agnostic interface between clients and services -
you get to choose fast or pretty

Pluton was designed and developed by Mark Delany at Yahoo! and is used
across many parts of the Yahoo network to substantially reduce
web-serving latency and increase resiliency.

## HTML Documentation link

Documentation corresponding to the latest version in the tree is
[here](http://markdelany.github.com/Pluton/1.0/index.html>here).

## 3rd Party Software Notice

Except as specifically stated below, the 3rd party software packages are not distributed as part of
this project, but instead are separately downloaded from the respective provider and built on the
developer's machine as a pre-build step. 


* State Threads Library for Internet Applications
(A library that offers a threading API)
[link](http://state-threads.sourceforge.net/)

## Copyright Notice

Copyright @copy; 2010, Yahoo! Inc. All rights reserved.

Redistribution and use of this software in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this list of conditions 
and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions 
and the following disclaimer in the documentation and/or other materials provided with the distribution.
* Neither the name of Yahoo! Inc. nor the names of its contributors may be used to endorse or promote 
products derived from this software without specific prior written permission of Yahoo! Inc.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR 
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND 
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER 
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT 
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
