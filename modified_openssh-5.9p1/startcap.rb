#!/usr/bin/env ruby

server_ip = ARGV[0]
sshfolder = ARGV[1]
trial_start = ARGV[2].to_i
trial_end = ARGV[3].to_i
web_start = ARGV[4].to_i
web_end = ARGV[5].to_i
id = ARGV[6].to_i



client_tau = 4096
server_tau = 32768

while client_tau <= 4096 do

	#start modified ssh server
	start_sshd_mod = "ssh -i /home/xcai/.ssh/local_rsa root@#{server_ip} \"#{sshfolder}modified_openssh-5.9p1/sshd -f #{sshfolder}modified_openssh-5.9p1/sshd_config_#{id} 2>/dev/null\""

	#start plain ssh server
#	start_sshd_plain = "ssh -i /home/xcai/.ssh/local_rsa root@#{server_ip} \"/usr/sbin/sshd -p 30001 2>/dev/null\""

	sshdpid_mod = fork do
		exec "#{start_sshd_mod}"
	end
	Process.detach(sshdpid_mod)
	
	sleep(15)

#	sshdpid_plain = fork do
#		exec "#{start_sshd_plain}"
#	end
#	Process.detach(sshdpid_plain)
#	sleep(15)
	

	#start modified ssh client
	start_ssh_mod = "./ssh -i /home/xcai/.ssh/local_rsa -ND #{30000+id} root@#{server_ip} -p #{30000+id} 2>/dev/null"
	#start plain ssh client
#	start_ssh_plain = "ssh -i /home/xcai/.ssh/local_rsa -ND 30001 root@#{server_ip} -p 30001 2>/dev/null"

	sshpid_mod = fork do
		exec "#{start_ssh_mod}"
	end
	Process.detach(sshpid_mod)

	sleep(15)

#	sshpid_plain = fork do
#		exec "#{start_ssh_plain}"
#	end
#	Process.detach(sshpid_plain)
#	sleep(15)

	`ruby modsshcapture.rb /var/tmp/localstcap/ modssh_#{id} 130.245.150.51 #{trial_start} #{trial_end} #{web_start} #{web_end} #{client_tau} #{server_tau} #{id+30000}`
#	`ruby modsshcapture.rb /var/tmp/para_cluster/ modssh_#{id} 130.245.150.51 #{trial_start} #{trial_end} #{web_start} #{web_end} #{client_tau} #{server_tau} #{id+30000}`
	
	sleep(5)

	#kill server
	
	client_tau *= 2

	Process.kill(9,sshpid_mod)
#	Process.kill(9,sshpid_plain)
	
end
