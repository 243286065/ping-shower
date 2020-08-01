#!/usr/bin/env python

"""
    A pure python ping implementation using raw socket.
    Note that ICMP messages can only be sent from processes running as root.
    Derived from ping.c distributed in Linux's netkit. That code is
    copyright (c) 1989 by The Regents of the University of California.
    That code is in turn derived from code written by Mike Muuss of the
    US Army Ballistic Research Laboratory in December, 1983 and
    placed in the public domain. They have my thanks.
    Bugs are naturally mine. I'd be glad to hear about them. There are
    certainly word - size dependenceies here.
    Copyright (c) Matthew Dixon Cowles, <http://www.visi.com/~mdc/>.
    Distributable under the terms of the GNU General Public License
    version 2. Provided with no warranties of any sort.
    Original Version from Matthew Dixon Cowles:
      -> ftp://ftp.visi.com/users/mdc/ping.py
    Rewrite by Jens Diemer:
      -> http://www.python-forum.de/post-69122.html#69122
    Revision history
    ~~~~~~~~~~~~~~~~
    March 11, 2010
    changes by Samuel Stauffer:
    - replaced time.clock with default_timer which is set to
      time.clock on windows and time.time on other systems.
    May 30, 2007
    little rewrite by Jens Diemer:
     -  change socket asterisk import to a normal import
     -  replace time.time() with time.clock()
     -  delete "return None" (or change to "return" only)
     -  in checksum() rename "str" to "source_string"
    November 22, 1997
    Initial hack. Doesn't do much, but rather than try to guess
    what features I (or others) will want in the future, I've only
    put in what I need now.
    December 16, 1997
    For some reason, the checksum bytes are in the wrong order when
    this is run under Solaris 2.X for SPARC but it works right under
    Linux x86. Since I don't know just what's wrong, I'll swap the
    bytes always and then do an htons().
    December 4, 2000
    Changed the struct.pack() calls to pack the checksum and ID as
    unsigned. My thanks to Jerome Poincheval for the fix.
    Januari 27, 2015
    Changed receive response to not accept ICMP request messages.
    It was possible to receive the very request that was sent.
    Last commit info:
    ~~~~~~~~~~~~~~~~~
    $LastChangedDate: $
    $Rev: $
    $Author: $
"""


import os, sys, socket, struct, select, time
from sys import version_info

if sys.platform == "win32":
    # On Windows, the best timer is time.clock()
    default_timer = time.clock
else:
    # On most other platforms the best timer is time.time()
    default_timer = time.time

# From /usr/include/linux/icmp.h; your milage may vary.
ICMP_ECHO_REQUEST = 8 # Seems to be the same on Solaris.

seq_num = 0
target_addr = ""


def checksum(source_bytes):
    """
    I'm not too confident that this is right but testing seems
    to suggest that it gives the same answers as in_cksum in ping.c
    """
    sum = 0
    countTo = (len(source_bytes)/2)*2
    count = 0
    while count<countTo:
        if version_info.major == 2:
            thisVal = ord(source_bytes[count + 1])*256 + ord(source_bytes[count])
        else:
            thisVal = source_bytes[count + 1]*256 + source_bytes[count]
        sum = sum + thisVal
        sum = sum & 0xffffffff # Necessary?
        count = count + 2

    if countTo<len(source_bytes):
        if version_info.major == 2:
            sum = sum + ord(source_bytes[len(source_bytes) - 1])
        else:
            sum = sum + source_bytes[len(source_bytes) - 1]
        sum = sum & 0xffffffff # Necessary?

    sum = (sum >> 16)  +  (sum & 0xffff)
    sum = sum + (sum >> 16)
    answer = ~sum
    answer = answer & 0xffff

    # Swap bytes. Bugger me if I know why.
    answer = answer >> 8 | (answer << 8 & 0xff00)

    return answer


def receive_one_ping(my_socket, ID, timeout):
    """
    receive the ping from the socket.
    """
    timeLeft = timeout
    global seq_num
    while True:
        startedSelect = default_timer()
        whatReady = select.select([my_socket], [], [], timeLeft)
        howLongInSelect = (default_timer() - startedSelect)
        if whatReady[0] == []: # Timeout
            return False,timeout*1000,seq_num

        timeReceived = default_timer()
        recPacket, addr = my_socket.recvfrom(1024)
        icmpHeader = recPacket[20:32]

        type, code, checksum, packetID, sequence = struct.unpack(
            "bbHHI", icmpHeader
        )

        # Filters out the echo request itself. 
        # This can be tested by pinging 127.0.0.1 
        # You'll see your own request
        if type != 8 and packetID == ID and sequence == seq_num :
            bytesInDouble = struct.calcsize("d")
            timeSent = struct.unpack("d", recPacket[32:32 + bytesInDouble])[0]
            return True, timeReceived - timeSent, sequence

        timeLeft = timeLeft - howLongInSelect
        if timeLeft <= 0:
            return False,timeout*1000,seq_num


def send_one_ping(my_socket, dest_addr, ID):
    """
    Send one ping to the given >dest_addr<.
    """
    global target_addr
    global seq_num
    if target_addr != dest_addr:
        target_addr = dest_addr
        seq_num = 0

    dest_addr  =  socket.gethostbyname(dest_addr)

    # Header is type (8), code (8), checksum (16), id (16), sequence (32)
    my_checksum = 0

    seq_num = seq_num + 1
    # Make a dummy heder with a 0 checksum.
    header = struct.pack("bbHHI", ICMP_ECHO_REQUEST, 0, my_checksum, ID, seq_num)
    bytesInDouble = struct.calcsize("d")
    payload = (192 - bytesInDouble) * "Q"
    if version_info.major == 2:
        data = struct.pack("d", default_timer()) + payload
    else:
        data = struct.pack("d", default_timer()) + payload.encode()

    # Calculate the checksum on the data and the dummy header.
    my_checksum = checksum(header + data)

    # Now that we have the right checksum, we put that in. It's just easier
    # to make up a new header than to stuff it into the dummy.
    header = struct.pack(
        "bbHHI", ICMP_ECHO_REQUEST, 0, socket.htons(my_checksum), ID, seq_num
    )
    packet = header + data
    try:
        my_socket.sendto(packet, (dest_addr, 1)) # Don't know about the 1
    except:
        return False,dest_addr

    return True,dest_addr


def do_one(dest_addr, timeout):
    """
    Returns either the delay (in seconds) or none on timeout.
    """
    icmp = socket.getprotobyname("icmp")
    try:
        my_socket = socket.socket(socket.AF_INET, socket.SOCK_RAW, icmp)
    except socket.error as err:
        if err.errno == 1:
            # Operation not permitted
            msg = msg + (
                " - Note that ICMP messages can only be sent from processes"
                " running as root."
            )
            raise socket.error(msg)
        raise # raise the original error
    my_ID = os.getpid() & 0xFFFF

    valid, ip = send_one_ping(my_socket, dest_addr, my_ID)
    if not valid:
        global seq_num
        return valid, ip, timeout*1000, seq_num

    valid, delay, sequence = receive_one_ping(my_socket, my_ID, timeout)

    my_socket.close()
    return valid, ip, delay, sequence


def verbose_ping(dest_addr, timeout = 2, count = 1):
    """
    Send >count< ping to >dest_addr< with the given >timeout< and display
    the result.
    """
    for i in range(count):
        # print "ping %s..." % dest_addr,
        try:
            valid, ip, delay, sequence  =  do_one(dest_addr, timeout)
        except socket.error as e:
            print("failed. (socket error: '%s')" % e)
            raise
        except Exception as e:
            print("failed. (%s)" % (e) ) 
            raise
        if not valid or delay  ==  None:
            print("failed. (timeout within %ssec.)" % timeout)
            return valid, sequence, delay
        else:
            delay  =  delay * 1000
            print("Reply from %s(%s) icmp_seq=%d time=%0.2f ms" % (dest_addr, ip, sequence, delay))
            return valid, sequence, delay


if __name__ == '__main__':
    verbose_ping("192.168.1.1")