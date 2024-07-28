#!/bin/bash

# Set configurations for each test
declare -A test_configs=(
  ["prueba_plani_memoria"]="config/memoria_plani.config"
  ["prueba_plani_cpu"]="config/cpu_plani.config"
  ["prueba_plani_kernel"]="config/kernel_plani.config"
  ["prueba_plani_entradasalida"]="config/SLP1.config"

  ["prueba_deadlock_memoria"]="config/memoria_deadlock.config"
  ["prueba_deadlock_cpu"]="config/cpu_deadlock.config"
  ["prueba_deadlock_kernel"]="config/kernel_deadlock.config"
  ["prueba_deadlock_entradasalida"]="config/ESPERA.config"

  ["prueba_memoria_tlb_memoria"]="config/memoria_mem_tlb.config"
  ["prueba_memoria_tlb_cpu"]="config/cpu_memoria_tlb.config"
  ["prueba_memoria_tlb_kernel"]="config/kernel_memoria_tlb.config"
  ["prueba_memoria_tlb_entradasalida"]="config/IO_GEN_SLEEP.config"

  ["prueba_io_memoria"]="config/memoria_io.config"
  ["prueba_io_cpu"]="config/cpu_io.config"
  ["prueba_io_kernel"]="config/kernel_io.config"
  ["prueba_io_entradasalida"]="config/GENERICA.config config/TECLADO.config config/MONITOR.config"

  ["prueba_fs_memoria"]="config/memoria_fs.config"
  ["prueba_fs_cpu"]="config/cpu_fs.config"
  ["prueba_fs_kernel"]="config/kernel_fs.config"
  ["prueba_fs_entradasalida"]="config/FS.config config/TECLADO.config config/MONITOR.config"

  ["prueba_salvations_edge_memoria"]="config/memoria_salvations_edge.config"
  ["prueba_salvations_edge_cpu"]="config/cpu_salvations_edge.config"
  ["prueba_salvations_edge_kernel"]="config/kernel_salvations_edge.config"
  ["prueba_salvations_edge_entradasalida"]="config/GENERICA.config config/TECLADO.config config/MONITOR.config config/ESPERA.config config/SLP1.config"
)

# Function to compile and run a module in a new terminal
run_module() {
  local module=$1
  local config=$2
  local position=$3
  local size=$4

  echo "Running module: $module with config: $config"
  gnome-terminal -- bash -c "cd $module && make clean && make all && ./bin/$module $config; exec bash"
  sleep 1
  window_id=$(xdotool search --sync --onlyvisible --class gnome-terminal | tail -1)
  xdotool windowmove $window_id $position
  xdotool windowsize $window_id $size
}

# Function to run specific commands for the tests in a new terminal
run_test() {
  local test_name=$1
  local no_memoria=$2
  local no_cpu=$3
  local no_kernel=$4
  local no_entradasalida=$5

  echo "Executing test: $test_name"
  if [ "$no_memoria" != "true" ]; then
    run_module "memoria" "${test_configs[${test_name}_memoria]}" "0 0" "940 460"
  fi
  if [ "$no_kernel" != "true" ]; then
    run_module "kernel" "${test_configs[${test_name}_kernel]}" "0 500" "940 500"
  fi
  if [ "$no_cpu" != "true" ]; then
    run_module "cpu" "${test_configs[${test_name}_cpu]}" "940 0" "940 460"
  fi
  if [ "$no_entradasalida" != "true" ]; then
    # Split entradasalida configs and run each in a new terminal
    IFS=' ' read -r -a entradasalida_configs <<< "${test_configs[${test_name}_entradasalida]}"
    local positions=("940 500" "940 680" "940 860" "940 1020" "940 1200")
    local sizes=("950 140" "950 140" "950 140" "950 140" "950 140")
    local i=0
    for config in "${entradasalida_configs[@]}"; do
      run_module "entradasalida" "$config" "${positions[$i]}" "${sizes[$i]}"
      i=$((i+1))
    done
  fi
}

# Parse arguments
no_memoria="false"
no_cpu="false"
no_kernel="false"
no_entradasalida="false"

while [[ $# -gt 0 ]]; do
  key="$1"
  case $key in
    -no_memoria)
      no_memoria="true"
      shift
      ;;
    -no_cpu)
      no_cpu="true"
      shift
      ;;
    -no_kernel)
      no_kernel="true"
      shift
      ;;
    -no_entradasalida)
      no_entradasalida="true"
      shift
      ;;
    *)
      test_name="$1"
      shift
      ;;
  esac
done

if [ -z "$test_name" ]; then
  echo "Usage: $0 <test_name> [-no_memoria] [-no_cpu] [-no_kernel] [-no_entradasalida]"
  exit 1
fi

run_test "$test_name" "$no_memoria" "$no_cpu" "$no_kernel" "$no_entradasalida"
