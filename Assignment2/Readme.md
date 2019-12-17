<h2>Part 1 : Bigfs (Custom Distributed File System) </h2>
<p align="justify"> You are required to build a custom-made file system “bigfs” as described below. The purpose of this
file system is to store large files on multiple systems.</p>
<p align="justify">
<ul>
  <li>Any file larger than 1 MB is split into blocks of 1MB. Each block is stored on a separate physical
machine using local file system available on that m/c.</li>
  <li>A client provides commands to the user to create, copy, or move a file just like a local file system
provides. Commands such as <b>cat, cp, mv, ls, rm</b> should be provided in client interface for the
user.</li>
  <li>A client should also be able to copy from local file system to bigfs and vice-versa.</li>
  <li>A server named “FileNameServer” running on one of the systems is responsible for maintaining the
file system hierarchy. A server named “FileDataServer” running on all m/cs is responsible for
reading/writing the data blocks stored on that machine.</li>
  <li>A client contacts FileNameServer first for executing all commands. Commands such as ls, mv will
execute completely on FileNameServer without a need to contact FileDataServers. Commands
such as <b> cat, cp, rm </b>require the client to contact both FileNameServer and FileDataServers.</li>
  <li>When a command such as <b>cat or cp</b> is issued, client should read a file in parallel from multiple
FileDataServers. Similarly, when writing a file using <b>cp </b>, file should be written to multiple
FileDataServers in parallel.</li>
  <li>All servers should be TCP based and implemented using event-driven architecture (I/O
Multiplexing with non-blocking I/O) and threads or processes can be used to delegate “block
reads/writes”.</li>
  <li>Bigfs should be run in at least 3 physical machines or VMs. It will be tested by copying a directory
(containing large files) in local file system to bigfs and using other commands.</li>
</ul>
</p>
<h2>Part 2 : Custom Message Queue (Publisher-Subscriber) </h2>
<p align="justify">In this problem let us extend Message Queues network wide for the following characteristics.</p>
<p align="justify">
<ul>
  <li>One who writes a message is called a publisher and one who reads is called as subscriber. A
publisher tags a message with a topic. Anyone who subscribed to that topic can read that
message. There can be many subscribers and publishers for a topic but there can only be one
publisher for a given message.</li>
  <li>Publisher program should provide an interface for the user to (i) create a topic. Publisher also
provides commands for (ii) sending a message, (iii) taking a file and send it as a series of
messages. When sending a message, topic must be specified. Each message can be up to 512
bytes.  </li>
  <li>Publisher program takes address of a Broker server as CLA. There can be several broker servers on
separate machines or on a single machine. The role of a broker server is to receive messages from
a publisher and store them on disk and send messages to a subscriber when requested,</li>
  <li>Publishers and subscribers may be connected to different brokers. The messages should reach the
right subscriber.</li>
  <li>Subscriber program takes the address of a broker server as CLA at the startup. It allows a user to
(i) subscribe to a topic (ii) retrieve next message (iii) retrieve continuously all messages. Subscriber
should print the message id, and the message.</li>
  <li>All brokers are connected in a circular topology. For message routing, the broker connected to a
subscriber, queries its neighbor brokers and they query further and so on. Each query retrieves a
bulk of messages limited by BULK_LIMIT (default=10).</li>
  <li>Brokers store messages for a period of MESSAGE_TIME_LIMIT (default=1minute)</li>
  <li>This system doesn’t guarantee FIFO order of messages. Think and propose any mechanism that
can guarantee FIFO order.</li>
</ul>
</p>
