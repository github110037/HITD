#include "common.hpp"
#include "testbench/sim_state.hpp"
#include <bits/getopt_core.h>
#include <cstdint>
#include <cstdlib>
#include <getopt.h>
#include <map>
#include <string>

static std::map<std::string, int> img_str_to_code{
    std::make_pair("func", TEST_NAME_FUNC),
    std::make_pair("perf", TEST_NAME_PERF),
    std::make_pair("sys",  TEST_NAME_SYST),
    std::make_pair("uboot",TEST_NAME_UBOOT),
    std::make_pair("ucore",TEST_NAME_UCORE),
    std::make_pair("linux",TEST_NAME_LINUX),
};
static const struct option table[] = {
    {"batch", no_argument, NULL, 'b'},
    {"log", required_argument, NULL, 'l'},
    {"wave", required_argument, NULL, 'w'},
    {"help", no_argument, NULL, 'h'},
};
const char* arg_log_file = "trace.log";
bool arg_batch_mode = false;
uint64_t arg_wave_on_tick = CONFIG_WAVE_START_FROM;
void parse_args(int argc, char *argv[]) {
  int o;
  while ((o = getopt_long(argc, argv, "bl:w:", table, NULL)) != -1) {
    switch (o) {
    case 'l':
      arg_log_file = optarg;
      break;
    case 'b':
      arg_batch_mode = true;
      break;
    case 'w':
      arg_wave_on_tick = atol(optarg);
      break;
    default:
      printf("Usage: %s [OPTION...] [args]\n\n", argv[0]);
      printf("\t-b,--batch              run with batch mode\n");
      printf("\t-l,--log=FILE           output log to FILE\n");
      printf("\n");
      exit(0);
    }
  }
}
