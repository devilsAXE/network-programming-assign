<h2> Part 1 </h1> 
<p align="justify" > You are required to build a bash-like shell for the following requirements. Your program should not use temporary files, popen(), system() library calls. It should only use system-call wrappers from the library. <i> It should not use sh or bash shells to execute a command. </i></p>
<p>
<ul>
<li>Shell should wait for the user to enter a command. User can enter a command with multiple
arguments. Program should parse these arguments and pass them to execv() call. For every
command, shell should search for the file in PATH and print any error. Shell should also print the
pid, status of the process before asking for another command.</li>
<li>shell should create a new process group for every command. When a command is run with & at
end, it is counted as background process group. Otherwise it should be run as fore-ground process
group (look at tcsetpgrp()). That means any signal generated in the terminal should go only to the
command running, not to the shell process. <b>fg</b> command should bring the background job to fore
ground. <b>bg </b> command starts the stopped job in the background.</li>
<li>shell should support any number of commands in the pipeline. e.g. 
<b>ls|wc|wc|wc </b>. 
Print details
such as pipe fds, process pids and the steps. Redirection operators can be used in combination with
pipes. </li>
<li>Shell should support <b> # </b> operator. The meaning of this: it carries same semantics as pipe but use
message queue instead of pipe. The operator <b>##</b> works in this way: <b>ls ## wc , sort</b>. output of
ls should be replicated to both <b>wc</b> and <b>sort</b> using message queues
</li>
<li>Shell should support <b>S</b> operator. The meaning of this: it carries same semantics as pipe but use
shared memory instead of pipe. The operator <b>SS</b> works in this way: Using example, <b>ls SS wc,
sort</b>. Output of <b>ls</b> should be replicated to both <b>wc</b> and <b>sort </b> using shared memory. 
</li>
<li>Shell should support a command <b>daemonize</b> which takes the form <b>daemonize <program></b> and
converts the program into a daemon process.</li>
<li>shell should support <b><, >, and >></b> redirection operators. Print details such as fd of the file,
remapped fd.</li>
</ul>
</p>

<h2>Part 2</h2>
<p>Cluster Shell. In this problem you are required to extend the shell features to a cluster of machines,
each identified by a name. The name to ip mapping is available in a config file, whose path is specified at
the start of the shell. Assume that N nodes in the cluster are named as <i>n1, n2 ..... nN</i>.</p>
<p>
<ul><li>
Cluster shell is run on any one of the nodes. When a command is run e.g. ls it executed on the
local system. When n2.ls is run in n1, it is executed on node n2 and the output is listed on n1.
When <b> n*.ls </b> is run on n1, <b>ls</b> is run on all nodes and output is displayed on n1. This applies to
other commands as well. By default, all commands on a remote node are executed in the home
directory of the user logged in n1. That is, it is necessary to have the same user on all systems.
When <b>n2.cd <path></b> or <b>n*.cd <path></b> is executed, directory is changed.</li>
<li>
When the command <b>n1.cat file|n2.sort|n3.uniq </b> is executed on n5, the commands are
executed on different nodes taking input from the previous command but the last output is
displayed on the node n5 it is executed on.</li>
<li>
The command <b>nodes</b> displays the list of nodes (name, ip) currently active in the cluster.
</li>

</p>

