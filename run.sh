#!/bin/bash


# compile the server and client
gcc -o client client.c
gcc -o server server.c

# create the output file
touch output.txt
cat /dev/null > output.txt
losses=('0.1%' '0.5%' '1%')
delays=('10ms' '50ms' '100ms')
tcps=('reno' 'cubic')

sudo tc qdisc add dev lo root netem loss 0.1%
sudo tc qdisc add dev lo root netem delay 10ms
for loss in "${losses[@]}"; do
	for delay in "${delays[@]}"; do
		for tcp in "${tcps[@]}"; do
			# setting the loss and delay
			sudo tc qdisc change dev lo root netem loss $loss
			sudo tc qdisc change dev lo root netem delay $delay

			# TCP cubic
			echo "Loss = $loss, delay = $delay, tcp $tcp
Throughputs for 20 runs:
" | cat >> output.txt

			# run the program 20 times
			sum="0"
			sum_square="0"
			for i in {0..19}
			do
				./server cubic
				num=$(./client $tcp | tail -n 1)
				echo "$num" | cat >> output.txt
				sum=$(awk '{print $1 + $2}' <<<"${sum} ${num}")
				sum_square=$(awk '{print $1 + $2^2}' <<<"${sum_square} ${num}")
			done

			avg=$(awk '{print $1/20}' <<<"${sum}")
			std=$(awk '{print sqrt($1/20)}' <<<"${sum_square}")

			echo "
Mean: $avg" | cat >> output.txt
			echo "Standard Deviation: $std

--------------------------------------------------------------------
" | cat >> output.txt
		done
	done
done





