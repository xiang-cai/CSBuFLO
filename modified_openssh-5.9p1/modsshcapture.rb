#!/usr/bin/env ruby

# this script reads local ip,  all websites' urls from db.
# for each website, it starts ff, goes to the website, and start capturing packets
# it stops after interval seconds, dumps the logs to db

require 'mysql2'
require 'rubygems'
require 'timeout'
require 'socket'
require 'watir-webdriver'

def poke(ip, port, seconds=1)
	Timeout::timeout(seconds) do
		begin
			TCPSocket.new(ip, port).close
		rescue Errno::ECONNREFUSED, Errno::EHOSTUNREACH
			puts "refuse or unreachable"
		end
	end
rescue Timeout::Error
	puts "timeout"	
end


def capture(v_ip, proname, foldername, dbh, id, address, lt, trial, client_tau, server_tau, port)

  begin
	#capture packets from address, local_ip is stored in local_ip

	loadtime = 30		# time to load a website
	
	fname = "#{foldername}#{id}_#{trial}.cap"
	puts "#{fname}"
	lockname = "/tmp/st#{port}.lock"

	if (File.exists?("#{lockname}"))
		`rm -f #{lockname}`
	end

	while File.exists?("#{lockname}") do
		sleep(5)
	end


#	`sudo tshark -i eth0 -f "(dst net 130.245.150.132 and src net 199.48.147.35) || (src net 130.245.150.132 and dst net 199.48.147.35)" -T fields -e ip.src -e ip.dst -e ip.proto -e frame.len -e tcp.len -a duration:#{captime} >> #{fname} &`

#	`sudo tshark -i eth0 -f "(dst net 130.245.150.132 and src net #{guardip}) || (src net 130.245.150.132 and dst net #{guardip})" -a duration:#{captime} -w -  >> #{fname} &`

#	`sudo tshark -i eth0  -f "(dst net 130.245.150.132 and src net #{guardip}) || (src net 130.245.150.132 and dst net #{guardip})" -w -  > #{fname} &`
	
#	ENV['DISPLAY']=":100"
#	tspid = fork do
#		exec("tshark", "-i", "any", "-f", "tcp && port #{port}", "-w", "#{fname}")
#	end
	
#	sleep(2)

#	cmd = "ssh -i /home/localxcai/.ssh/local_rsa -t -t root@192.168.0.1  \"tshark -i any -f \'length > 0 and port #{port} and (src net 192.168.0.2 || dst net 130.245.150.18)\' -w  #{fname} 2>/dev/null\""
	tspid = fork do
#		exec("#{cmd}")
		exec("tshark", "-i", "any", "-f", "tcp && port #{port}", "-w", "#{fname}")
	end

	Process.detach(tspid)

#	pipe = IO.popen("ps o pid= --ppid #{tspid}")
#	child_pids = pipe.readlines.map! {|line| line.strip()}
	cnt = 0

	client = Selenium::WebDriver::Remote::Http::Default.new
	client.timeout = 180 # seconds - default is 60 
	driver = Selenium::WebDriver.for(:firefox, :http_client => client, :profile => "#{proname}")

	ff = Watir::Browser.new(driver)

#	ff = Watir::Browser.new :firefox, :profile => "#{proname}"
	ff.goto("#{address}")

#	ffpid = fork do
#		exec("firefox", "-no-remote", "-P",  "#{proname}", "#{address}")
	#	exec("xvfb-run", "-n", "#{port-30000+99}", "firefox", "-no-remote", "-P",  "#{proname}", "#{address}")
#	end
	
#	Process.detach(ffpid)

#	sleep(loadtime)
	while File.exists?("#{lockname}") do
		sleep(5)
	end


	ff.close
#	puts "ff killed"
	cnt += 1
	sleep(15)	# let ssh control packets finish
#	puts "to call poke"
#	poke('1.2.3.4', port)

	Process.kill(15,tspid)
#	puts "tspid #{tspid} killed"

#	child_pids.each do |p|
#		pid = p.to_i
#		Process.kill(15, pid)
#	end

  rescue Exception => e
#	`echo "exception catched: #{$!}" >> /var/tmp/output`
  ensure
	if cnt == 0
#		puts "ff killed in ensure"
#		sleep(15)
		begin	
			if (File.exists?("#{lockname}"))
				`rm -f #{lockname}`
			end

			Process.kill(15,tspid)
#			`echo "tspid #{tspid} killed in ensure" >> /var/tmp/log`

#			child_pids.each do |p|
#				pid = p.to_i
#				`echo "child #{pid} in ensure:" >> /var/tmp/log`
#				Process.kill(15, pid)
#				`echo "child #{pid} killed in ensure" >> /var/tmp/log`
#			end
		
#			`echo "bef ff close in ensure" >> /var/tmp/log`
			ff.close if ff
			
#			`echo "ff closed in ensure" >> /var/tmp/log`
		rescue Exception => ex
			#`echo "ex catched: #{$!} in ensure" >> /var/tmp/log`
#			`echo "ex catched in ensure" >> /var/tmp/log`
		end
		sleep(15)
	end	
	sleep(5)
  end
end


begin	
# setting up db handler
dbh = Mysql2::Client.new(:host => "130.245.150.132", :username => "root", :password => "splat", :database => "TCPlog")
	
#read website urls from database
#res = dbh.query("select id, address from modsshtest where id <= 200 order by id")
res = dbh.query("select id, address, loadtime from websites_top_sequence where id <= 1500 order by id")


Element = Struct.new(:id, :address, :loadtime)
websites = Array.new

id = 1
res.each do |row|
#	if(row["address"][0..4] != "https")
		tmp = Element.new(row["id"].to_i, row["address"], row["loadtime"])
		websites.push(tmp)
		id += 1
#	end
end

foldername = ARGV[0]
proname = ARGV[1]
v_ip = ARGV[2]

i = ARGV[3].to_i
while i <= ARGV[4].to_i do
	j = ARGV[5].to_i - 1
	while j < ARGV[6].to_i do
		capture(v_ip, proname, foldername, dbh, websites[j].id, websites[j].address, websites[j].loadtime, i, ARGV[7].to_i, ARGV[8].to_i, ARGV[9].to_i)
		puts "website #{websites[j].id}, trial #{i} finished"
#		sshfoldername = "#{foldername}plainssh/"
#		capture(v_ip, "ssh", sshfoldername, dbh, websites[j].id, websites[j].address, i, ARGV[7].to_i, ARGV[8].to_i)
#		puts "website #{websites[j].id}, trial #{i} finished"

		j += 1
	end
	i += 1
end


#puts "Number of website urls returned: #{res.num_rows}"
#system("sendmail xcai.stonybrook@gmail.com < body.txt")

rescue Mysql2::Error => e
puts "Error code: #{e.errno}"
puts "Error message: #{e.error}"
puts "Error SQLSTATE: #{e.sqlstate}" if e.respond_to?("sqlstate")

ensure
# disconnect from server
dbh.close if dbh
end
