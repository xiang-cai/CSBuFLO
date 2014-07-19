#!/usr/bin/env ruby

logfolder = ARGV[0] #"./log_turtle/"
client_ips = ARGV[1] #"./clientips.txt"
server_ips = ARGV[2] #"./serverips.txt"
proxy_port_min = ARGV[3]
proxy_port_max = ARGV[4]
capfolder = ARGV[5] #'./caps/'

web_start = ARGV[6].to_i
web_end = ARGV[7].to_i
trial_start = ARGV[8].to_i
trial_end = ARGV[9].to_i

web_start.upto(web_end) do |i|
	trial_start.upto(trial_end) do |j|
		`./capfilter #{logfolder} #{client_ips} #{server_ips} #{proxy_port_min} #{proxy_port_max} 1 #{capfolder}#{i}_#{j}.cap`
	end
end
