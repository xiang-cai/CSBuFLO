#!/bin/sh

server_ip=$1
sshfolder=$2
profile=$3
web_start=$4
web_end=$5

rm -rf /tmp/*

#cmd="Xvfb :100 -fp /usr/share/fonts/X11/misc/ -noreset &"
#eval $cmd


cmd="ruby startcap.rb "$server_ip" "$sshfolder" 1 2 "$web_start" "$web_end" "$profile" &"
eval $cmd

#profile=$((1+$profile))
#cmd="ruby startcap.rb "$server_ip" "$sshfolder" 28 36 "$web_start" "$web_end" "$profile" &"
#eval $cmd

#profile=$((1+$profile))
#cmd="ruby startcap.rb "$server_ip" "$sshfolder" 1 20 281 281 "$profile" &"
#eval $cmd

