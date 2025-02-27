#include "easylogging++.h"
#include "macro.hpp"
#include "paddr/paddr_interface.hpp"
#include "testbench/axi.hpp"
#include "nemu/isa.hpp"
#include "Vmycpu_top.h"
#include "soc.hpp"
#include "generated/autoconf.h"
#include "testbench/inst_timer.hpp"
#include "testbench/sim_state.hpp"
#include "testbench/dpic.hpp"
#include "testbench/cp0_checker.hpp"

#define wave_file_t MUXDEF(CONFIG_EXT_FST,VerilatedFstC,VerilatedVcdC)
#define __WAVE_INC__ MUXDEF(CONFIG_EXT_FST,"verilated_fst_c.h","verilated_vcd_c.h")
#ifdef CONFIG_WAVE_ON
#include __WAVE_INC__
#endif

extern uint64_t ticks;
extern uint64_t total_times;
extern el::Logger* mycpu_log;
extern FILE* golden_trace;
#define RST_TIME 128

inline static void sim_ending(int nemu_end_state){/*{{{*/
    switch (nemu_end_state) {
        case NEMU_END: 
            sim_status = SIM_END; 
            break;
        case NEMU_ABORT: 
            sim_status = SIM_NEMU_QUIT; 
            break;
        default:
            __ASSERT_SIM__(0, "unexpected nemu quit state {}", nemu_end_state);
            break;
    }
}/*}}}*/

static bool sim_end_statistics(){/*{{{*/
    bool res = false;
    switch (sim_status) {
        case SIM_ABORT:
            mycpu_log->info("mycpu has error and quit");
            break;
        case SIM_END:
            mycpu_log->info("mycpu pass test");
            res = true;
            break;
        case SIM_INT:
            mycpu_log->info("mycpu stop test for key board interrupt");
            break;
        case SIM_NEMU_QUIT:
            mycpu_log->info("mycpu stop test for nemu abnormal exit");
            break;
        default:
            mycpu_log->info("mycpu quit with not defined state %v", sim_status);
            break;
    }
    return res;
}/*}}}*/

static void check_cpu_state(diff_state* mycpu){/*{{{*/
    bool res = nemu->ref_checkregs(mycpu);
    if (!res){
        __ASSERT_SIM__(0, "MyCPU execution\t{} error !!!",
                nemu->isa_disasm_inst());
        nemu->ref_log_error(mycpu);
    }
}/*}}}*/

bool mainloop(
        Vmycpu_top* top,
        axi_paddr* axi,
        std::string wave_name,
        dual_soc& soc
        ){/*{{{*/

    diff_state mycpu;
    sim_status = SIM_RUN;

    IFDEF(CONFIG_WAVE_ON,Verilated::traceEverOn(true));
    IFDEF(CONFIG_WAVE_ON,wave_file_t tfp);
    IFDEF(CONFIG_WAVE_ON,top->trace(&tfp,0));
    IFDEF(CONFIG_WAVE_ON,tfp.open((CONFIG_WAVE_DIR"/"+wave_name + "." + CONFIG_WAVE_EXT).c_str()));
    IFDEF(CONFIG_CP0_DIFF, cp0_checker mycpu_cp0_checker);

    ticks = 0;
    top->aclk = 0;
    top->aresetn = 0;
    IFDEF(CONFIG_COMMIT_WAIT, uint64_t last_commit = ticks);
    IFDEF(CONFIG_PERF_ANALYSES, perf_timer(AddrIntv(0x1fc00000,bit_mask(22))));

    while (ticks < (RST_TIME & ~0x1)) {
        ++ticks;
        axi->reset();
        nemu->reset();
        top->aclk = !top->aclk;
        top->eval();
        IFDEF(CONFIG_WAVE_ON,tfp.dump(ticks));
    }

    top->aresetn = 1;

    while (!Verilated::gotFinish()) {
        /* if need count perf_timer TIMED_SCOPE(one_clk,"one_clk"); */
        /* posedge edge comming {{{*/
        ++ticks;
        top->aclk = !top->aclk;

        /* update SoC and nemu clock */
        soc.tick();
        nemu->ref_tick_and_int(0);

        /* update mycpu */
        axi->calculate_output();
        top->eval();
        axi->update_output();

        /* record waveform */
        IFDEF(CONFIG_WAVE_ON,tfp.dump(ticks));

        /* check mainloop condition */
        if (sim_status!=SIM_RUN) break;

        /* record coprocessor 0 change for later difftest */
        IFDEF(CONFIG_CP0_DIFF, mycpu_cp0_checker.check_change());

        /* get mycpu instructions commit status */
        uint8_t commit_num = dpi_retire();

        /* run nemu and check difference {{{*/
        if (commit_num > 0) {
            uint8_t mycpu_int = dpi_interrupt_seq();
            for (size_t i = 0; i < commit_num; i++) {
                // TIMED_SCOPE(nemu_once, "nemu_once");
                if (!nemu->ref_exec_once(i+1 == mycpu_int)) {
                    sim_ending(nemu_state.state);
                    goto negtive_edge;
                }
                Decode& inst = nemu->inst_state;
                if (inst.skip) nemu->arch_state.gpr[inst.wnum] = dpi_regfile(inst.wnum);
                IFDEF(CONFIG_CP0_DIFF, mycpu_cp0_checker.check_value(inst.pc, nemu->cp0));
                IFDEF(CONFIG_PERF_ANALYSES, if (nemu->analysis) \
                    perf_timer.add_inst(nemu->inst_state, ((consume_t)(ticks-last_commit))/commit_num, ticks));
            }
            dpi_api_get_state(&mycpu);
            check_cpu_state(&mycpu);
            IFDEF(CONFIG_COMMIT_WAIT, last_commit = ticks);
        }/*}}}*/

        /*}}}*/
        /* negtive edge comming {{{*/
negtive_edge: 
        ++ticks;
        top->aclk = !top->aclk;
        top->eval();
        IFDEF(CONFIG_WAVE_ON,tfp.dump(ticks));
        IFDEF(CONFIG_COMMIT_WAIT, __ASSERT_SIM__(ticks-last_commit<CONFIG_COMMIT_TIME_LIMIT, \
                    "{} ticks not commit inst", \
                    CONFIG_COMMIT_TIME_LIMIT));/*}}}*/
    }

    IFDEF(CONFIG_WAVE_ON,tfp.close());
    IFDEF(CONFIG_PERF_ANALYSES, perf_timer.save_date(wave_name+".bin"));
    return sim_end_statistics();
}/*}}}*/
