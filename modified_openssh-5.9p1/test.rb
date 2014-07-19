#!/usr/bin/env ruby

	start_sshd_mod = "ssh -i /home/xcai/.ssh/grasshopper_rsa root@130.245.150.135 \"/home/xcai/project/fingerprinting/openssh/modified_openssh-5.9p1/sshd -f /home/xcai/project/fingerprinting/openssh/modified_openssh-5.9p1/sshd_config 2>/dev/null\""

	#start plain ssh server
	start_sshd_plain = "ssh -i /home/xcai/.ssh/grasshopper_rsa root@130.245.150.135 \"/usr/sbin/sshd -p 30005 2>/dev/null\""

	sshdpid_mod = fork do
		exec "#{start_sshd_mod}"
	end
	Process.detach(sshdpid_mod)
	
	sleep(5)

	sshdpid_plain = fork do
		exec "#{start_sshd_plain}"
	end
	Process.detach(sshdpid_plain)

	sleep(5)
	

	#start modified ssh client
	start_ssh_mod = "./ssh -i /home/xcai/.ssh/grasshopper_rsa -ND 30000 root@130.245.150.135 -p 30000 2>/dev/null"
	#start plain ssh client
	start_ssh_plain = "ssh -i /home/xcai/.ssh/grasshopper_rsa -ND 30005 root@130.245.150.135 -p 30005 2>/dev/null"

	sshpid_mod = fork do
		exec "#{start_ssh_mod}"
	end
	Process.detach(sshpid_mod)

	sleep(5)

	sshpid_plain = fork do
		exec "#{start_ssh_plain}"
	end
	Process.detach(sshpid_plain)

