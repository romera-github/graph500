#!/bin/bash

#
# This script will search for the file slurm-<JOBID>.out and will generate R-Code for with the extracted data.
#

function create_r_file {
    local r_file="file$1.r"
    rm -f $r_file
    touch $r_file
    res=$?
    if [ $res -ne 0 ]; then
        echo "Error creating $r_file"
        exit 1
    fi
    eval "$2=$r_file"
}

function write_r {
    echo_null=`echo "$1" | tee -a "$2"`
    res=$?
    echo_null=`echo "" | tee -a "$2"`

    if [ $res -ne 0 ]; then
        echo "Write error."
        exit 1
    fi
}

function iterate_files {
    if [ ! -f "slurm-$1.out" ]; then
        echo "Error. file slurm-$1.out must exist."
        exit 1
    else
        file="slurm-$1.out"
    fi
    full_vars="`tail -121 $file`"
    validation_line=`echo "$full_vars" | head -1`
    variables=`echo "$full_vars" | tail -120`
    scale_factor=`echo "$variables" | grep "SCALE"`
    scale_factor=${scale_factor##*:\ }
    gpus=`echo "$variables" | grep "gpus_per_process"`
    gpus=${gpus##*:\ }
    total_tasks=`echo "$variables" | grep "num_mpi_processes"`
    total_tasks=${total_tasks##*:\ }
    is_valid=`echo $validation_line | grep "Validation: passed"` 

    if [ "x$is_valid" != "x" ]; then
        echo "-> File $file. Validation passed (Tasks: $total_tasks, GPUS/Task: $gpus, Scale Factor: $scale_factor). "
    else
        echo "Error. execution ended with error. File $file"
        exit 1
    fi
    eval "$2=$scale_factor"
    eval "$3=$gpus"
    eval "$4=$total_tasks"
}

function generate_r_variables {
    result_code=""
    transformed_line=
    transformed_block=
    local id=$2
    for block in "$1"; do
	transformed_block=`echo "$block" | sed 's/ //'`
    done 
    for line in $transformed_block; do 
        transformed_line=`echo "$line" | sed 's/:/=/' | sed 's/^\(.*\)/id__'$id'__\1/'`
        result_code="$result_code"`echo ""; echo "$transformed_line"`
    done

    write_r "# Variable definition for file slurm-$2.out" $3
    write_r "$result_code" $3

}

function generate_r_plotcode {
# mean_time
    id_list="$2"
    tsk_list="$4"
    num_tasks=$3
    values_total_time="'mean_time'"
    values="'mean_local_bfs_time' 'mean_row_com_time' 'mean_column_com_time' 'mean_predecessor_list_reduction_time'"
    labels_x_text="'Expansion','Row com' ,'Col com','Pred list red' ,'Total time'"
    num_labels_x=`echo "$values" | wc -w`

    local val_list=`echo "$values" | sed 's/ /,/g'`
    local tasks=`echo "$tsk_list" | sed 's/^ //;s/ /,/g'`
    local ids=`echo "$id_list" | sed 's/ /,/g'`

  function_ggcols="ccols<-function(n){
      res<-c('red', 'green', 'darkcyan','purple', 'blue')
      return(res)
  }"


  labels="labs<-c(\"4P2N\", \"4P3N\", \"4P4N\", \"9P8N\", \"16P8N\")"
  labels="labs<-c(\"4P2N Row Opti\",\"4P2N Col OptiM\",\"4P2N Non Optim\")"

  plot="par(xpd = TRUE, mar = c(5.1, 4.1, 4.1, 7.1))
          barplot(matriz, axes = FALSE, axisnames = FALSE,
            main = '', border = NA,
            col = ccols($num_labels_x))
        axis(1, at = barplot(matriz, plot = FALSE), labels = labs)
        axis(2, at = seq(0, 1.75, 0.05), labels = seq(0, 1.75, 0.05))
        legend('topright', inset = c(-0.25, 0), fill = ccols(length(rownames(matriz))),
        legend = c($labels_x_text))"

  totals_points="points(x=bp, y=totals, col=\"blue\")
                 lines(x=bp, y=totals, col=\"blue\")"

    write_r "# Main Plot Code" $1
    write_r "require(grDevices)" $1

    write_r "$labels" $1
    write_r "$function_ggcols" $1

    write_r "ids <- c($ids)" $1
    write_r "tasks_list <- c($tasks)" $1
    labels_x=""
    for i in `seq $num_tasks`; do
        labels_x_paste="paste(tasks_list[$i], ' Task(s) ',$gpus,' GPUs/Task',sep='')"
        if [ "x$labels_x" = "x" ]; then
            labels_x="$labels_x_paste"
        else
            labels_x="$labels_x, $labels_x_paste"
        fi
    done
    labels_x="c($labels_x)"

    num_labes_y=0
    labels_y=""
    for i in $values; do
        for j in $id_list; do
            labels_y_paste="get(paste('id__',$j,'__',$i,sep=''))"
            if [ "x$labels_y" = "x" ]; then
                labels_y="$labels_y_paste"
            else
                labels_y="$labels_y, $labels_y_paste"
            fi
        done
    done
    labels_y="c($labels_y)"
    num_labels_y=`echo "$values" | wc -w`

    labels_total_time=""
    for i in $values_total_time; do
        for j in $id_list; do
            labels_total_time_paste="get(paste('id__',$j,'__',$i,sep=''))"
            if [ "x$labels_total_time" = "x" ]; then
                labels_total_time="$labels_total_time_paste"
            else
                labels_total_time="$labels_total_time, $labels_total_time_paste"
            fi
        done
    done
    labels_total_time="c($labels_total_time)"


    write_r "matriz <- matrix($labels_y, nrow = $num_labels_, ncol = $num_tasks, byrow = TRUE, \
        dimnames = list(c($val_list), \
        $labels_x))" $1
    write_r "totals<-$labels_total_time" $1  

    # write_r "barplot(matriz, legend.text = c($labels_x_text),
    #                args.legend = list(x = 'topleft', bty ='n'), beside = FALSE, col = c('red', 'green', 'darkcyan',
    #            'purple', 'blue'))" $1

    write_r "$plot" $1
    write_r "$totals_points" $1
   	

    write_r "title(main = 'Execution on Fermi. Scale Factor $scale_factor', font.main = 4)" $1
}

function build {
    echo ""
    ids="$1"
    id="-`echo "$ids" | sed 's/ /-/g'`" 
    common_sf=""
    common_gpus=""
    tasks_list=""
    result_code=""

    create_r_file "$id" result_file

    for i in $ids; do

        iterate_files $i scale_factor gpus tasks

        if [ "x$common_sf" = "x" ]; then
            common_sf=$scale_factor
        else
            if [ "$common_sf" != "$scale_factor" ]; then
                echo "Error. Scale Factor should be equal in all files."
                exit 1
            fi
        fi
        if [ "x$common_gpus" = "x" ]; then
            common_gpus=$gpus
        else
            if [ "$common_gpus" != "$gpus" ]; then
                echo "Error. GPU number per node should be equal in all files."
                exit 1
            fi
        fi
        id="$id-$i"
        tasks_list="$tasks_list $tasks"
        variables=`echo "$variables" | sed 's/SCALE:/\nSCALE:/'`

        generate_r_variables "$variables" $i "$result_file"

    done

    generate_r_plotcode "$result_file" "$ids" $2 "$tasks_list"

    echo "-> Created file \"$result_file\". "
    echo "-> R-Code successfully generated.";
    echo ""
}





if [ $# -lt 1 ] || [ "x$1" = "x-h" ] || [ "x$1" = "x--help" ]; then
    echo "This script will search for the file slurm-<JOBID>.out and will generate R-Code for with the extracted data."
    echo "Usage: $0 <JOBID> [JOBID] [JOBID] [JOBID] ..."
    exit 1
fi

build "$*" $#

