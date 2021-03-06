#!/bin/bash

datpath="summary"

rm ${datpath}/*.dat
logs=`find result -name '*.log' | sort`

if [ ! -d "${datpath}" ];
then 
mkdir ${datpath}
fi

for log in $logs
do
echo $log

scale=$(sed -n 's/SCALE: //p' < $log)
mpi_proc=$(sed -n 's/num_mpi_processes: //p' < $log)
gpus=$(sed -n 's/gpus_per_process: //p' < $log)
mean_teps=$(sed -n 's/harmonic_mean_TEPS: //p' < $log)
mean_time=$(sed -n 's/mean_time: //p' < $log)
mean_exp=$(sed -n 's/mean_local_bfs_time: //p' < $log)
mean_queue=$(sed -n 's/mean_local_queue_time: //p' < $log)
mean_rest=$(sed -n 's/mean_rest_time: //p' < $log)
mean_validation=$(sed -n 's/mean_validation_time: //p' <$log)
mean_row_com=$(sed -n 's/mean_row_com_time: //p' <$log)
mean_col_com=$(sed -n 's/mean_column_com_time: //p' <$log)
mean_predlistred=$(sed -n 's/mean_predecessor_list_reduction_time: //p' <$log)
construction=$(sed -n 's/construction_time: //p' <$log)
graph_generation=$(sed -n 's/graph_generation: //p' <$log)

#echo $log $scale $mpi_proc

if [ -n "$scale" ];
then
	#Output files
	if [ -z "${gpus}" ];
	then
		mproc_out="${datpath}/mprocesses_`printf "%04d" ${mpi_proc}`.dat"
	else	
		mproc_out="${datpath}/mprocesses_`printf "%04d" ${mpi_proc}`_gpus${gpus}.dat"
	fi
	scale_out="${datpath}/scale_`printf "%02d" ${scale}`.dat"

	if [ -z "$gpus" ];
	then 
		gpus="-"
	fi 
	if [ -z "$mean_teps" ];
	then 
		mean_teps="-"
	fi
	if [ -z "$mean_time" ];
	then 
		mean_time="-"
	fi
	if [ -z "$mean_exp" ];
	then 
		mean_exp="-"
	fi 
	if [ -z "$mean_queue" ];
	then 
		mean_queue="-"
	fi 
	if [ -z "$mean_rest" ];
	then 
		mean_rest="-"
	fi 
	if [ -z "$mean_validation" ];
	then 
		mean_validation="-"
	fi 
	if [ -z "$mean_row_com" ];
	then 
		mean_row_com="-"
	fi 
	if [ -z "$mean_col_com" ];
	then 
		mean_col_com="-"
	fi 
	if [ -z "$mean_predlistred" ];
	then 
		mean_predlistred="-"
	fi 
	if [ -z "$construction" ];
	then 
		construction="-"
	fi 
	if [ -z "$mean_predlistred" ];
	then 
		mean_predlistred="-"
	fi 


	if [ ! -f "$mproc_out" ];
	then
		echo \# G500 runs with ${mpi_proc} mpi processes with ${gpus} gpu\(s\) each `date` > "$mproc_out"
		echo \"scale\" \"TEPS\" \"time\" \"expansion\" \"queue handling\" \"rest\" \"validation\" \"row com.\"  \"column com.\" \"pred. list red.\" \"construction\" \"graph_generation\" >> "$mproc_out"
	fi

	if [ ! -f "$scale_out" ];
	then
		echo \# G500 runs with scalefactor ${scale} `date` > "$scale_out"
		echo \"mpi processes\" \"gpus\" \"TEPS\" \"time\" \"expansion\" \"queue handling\" \"rest\" \"validation\" \"row com.\"  \"column com.\" \"pred. list red.\" \"construction\" \"graph_generation\" >> "$scale_out"
	fi

	echo ${scale} ${mean_teps} ${mean_time} ${mean_exp} ${mean_queue} ${mean_rest} ${mean_validation} ${mean_row_com} ${mean_col_com} ${mean_predlistred} ${construction} ${graph_generation} >> "$mproc_out"
	echo ${mpi_proc} ${gpus} ${mean_teps} ${mean_time} ${mean_exp} ${mean_queue} ${mean_rest} ${mean_validation} ${mean_row_com} ${mean_col_com} ${mean_predlistred} ${construction} ${graph_generation} >> "$scale_out"

else
	echo No valid entrie in file $log.
fi

done
