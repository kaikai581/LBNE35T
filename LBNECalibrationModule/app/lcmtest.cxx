#include <fstream>
#include <iomanip>
#include <iostream>
#include <libconfig.h++>
#include "DeviceInterface.h"
#include <unistd.h>
#include "json/json.h"
#include "zmq.hpp"

using namespace libconfig;
using namespace std;

void Configure(SSPDAQ::DeviceInterface& dev, Setting& cfgroot){

  dev.SetRegisterByName("eventDataInterfaceSelect", 0x00000001);
  dev.SetRegisterByName("c2c_control",             0x00000007);
  dev.SetRegisterByName("c2c_master_intr_control", 0x00000000);
  dev.SetRegisterByName("comm_clock_control",      0x00000001);
  dev.SetRegisterByName("comm_led_config",         0x00000000);
  dev.SetRegisterByName("comm_led_input",          0x00000000);
  dev.SetRegisterByName("qi_dac_config",           0x00000000);
  dev.SetRegisterByName("qi_dac_control",          0x00000001);
  dev.SetRegisterByName("mon_config",              0x0012F000);
  dev.SetRegisterByName("mon_select",              0x00FFFF00);
  dev.SetRegisterByName("mon_gpio",                0x00000000);
  dev.SetRegisterByName("mon_control",             0x00010001);
  dev.SetRegisterByName("module_id",               0x00000001);
  dev.SetRegisterByName("c2c_slave_intr_control",  0x00000000);
  //dev.SetRegisterArrayByName("ALL_channel_control",      0x80F00401); //rising edge
  //dev.SetRegisterByName("#ALL_channel_control",      0x00006001 #front panel
  //dev.SetRegisterByName("#ALL_channel_control",      0x00F0E081 #timestamp
  dev.SetRegisterArrayByName("led_threshold",         500);
  dev.SetRegisterArrayByName("cfd_parameters",        0x1800);
  dev.SetRegisterArrayByName("readout_pretrigger",    100);
  dev.SetRegisterArrayByName("readout_window",        500);
  dev.SetRegisterArrayByName("p_window",              0x20);
  dev.SetRegisterArrayByName("i2_window",             500);
  dev.SetRegisterArrayByName("m1_window",             10);
  dev.SetRegisterArrayByName("m2_window",             10);
  dev.SetRegisterArrayByName("d_window",              20);
  dev.SetRegisterArrayByName("i1_window",             500);
  dev.SetRegisterArrayByName("disc_width",            10);
  dev.SetRegisterArrayByName("baseline_start",        0x0000);
  
  dev.SetRegisterByName("trigger_input_delay",       0x00000020);
  dev.SetRegisterByName("gpio_output_width",         0x00001000);
  dev.SetRegisterByName("front_panel_config",        0x00001101);// # standard config?
  dev.SetRegisterByName("dsp_led_config",            0x00000000);
  dev.SetRegisterByName("dsp_led_input",             0x00000000);
  dev.SetRegisterByName("baseline_delay",            5);
  dev.SetRegisterByName("diag_channel_input",        0x00000000);
  dev.SetRegisterByName("qi_config",                 0x0FFF1F00);
  dev.SetRegisterByName("qi_delay",                  0x00000000);
  dev.SetRegisterByName("qi_pulse_width",            0x00000000);
  dev.SetRegisterByName("external_gate_width",       0x00008000);
  dev.SetRegisterByName("dsp_clock_control",         0x00000013);// # 0x1  - use ext clock to drive ADCs
                                                                 // # 0x2  - use NOvA clock (0 value uses front panel input)
                                                                 //# 0x10 - Enable clock jitter correction

   //        # dsp_clock_control:         0x00000000 # Use internal clock to drive ADCs, front panel
   //     #                                        # clock for sync

  dev.SetRegisterArrayByName("bias_config", (unsigned int)cfgroot["pulse_height"]);
   //        #ALL_bias_config:           0x00040E21 # 26.5V - bit 0x40000 enables bias, bits 0xFFF set value
   //     #                                      # in range 0-30V
  dev.SetRegisterByName("bias_control", (unsigned int)cfgroot["bias_control"]);
  
  unsigned int iu_cal_config = ((unsigned int)cfgroot["iu_nova_enable"]) << 31 |
                            ((unsigned int)cfgroot["iu_trigger_source"]) << 28 |
                            ((unsigned int)cfgroot["iu_pulse_delay"]) << 16 |
                            ((unsigned int)cfgroot["iu_pulse_width_2"]) << 8 |
                            ((unsigned int)cfgroot["iu_pulse_width_1"]);
  unsigned int tpc_cal_config = ((unsigned int)cfgroot["tpc_nova_enable"]) << 31 |
                            ((unsigned int)cfgroot["tpc_trigger_source"]) << 28 |
                            ((unsigned int)cfgroot["tpc_pulse_delay"]) << 16 |
                            ((unsigned int)cfgroot["tpc_pulse_width_2"]) << 8 |
                            ((unsigned int)cfgroot["tpc_pulse_width_1"]);
  unsigned int pd_cal_config = ((unsigned int)cfgroot["pd_nova_enable"]) << 31 |
                            ((unsigned int)cfgroot["pd_trigger_source"]) << 28 |
                            ((unsigned int)cfgroot["pd_pulse_delay"]) << 16 |
                            ((unsigned int)cfgroot["pd_pulse_width_2"]) << 8 |
                            ((unsigned int)cfgroot["pd_pulse_width_1"]);
  dev.SetRegisterArrayByName("iu_cal_config",            iu_cal_config);
  dev.SetRegisterArrayByName("tpc_cal_config",           tpc_cal_config);
  dev.SetRegisterArrayByName("pd_cal_config",            pd_cal_config);
  dev.SetRegisterByName("cal_count", (unsigned int)cfgroot["pulse_sets"]);
  dev.SetRegisterByName("cal_trigger",               0x00000001);
}

int main(int argc, char** argv){
  
  // LCM configuration
  Config cfg;
  char* proj_root = getenv("PROJECT_ROOT");
  // open configuration file
  try {
    cfg.readFile((string(proj_root)+"/lcm.conf").c_str());
  }
  catch(const FileIOException &fioex) {
    std::cerr << "error reading the confiuration file." << std::endl;
    return(EXIT_FAILURE);
  }
  catch(const ParseException &pex) {
    std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
    << " - " << pex.getError() << std::endl;
    return(EXIT_FAILURE);
  }
  // get configurations from file
  string ip = "192.168.1.107";
  Setting& cfgroot = cfg.getRoot();
  if(cfgroot.exists("ip")) ip = cfgroot["ip"].c_str();
  
  // Open SSP device
  unsigned long board_id = inet_network(ip.c_str());
  SSPDAQ::DeviceInterface dev(SSPDAQ::kEthernet,board_id);
  dev.Initialize();
  //dev.OpenSlowControl();
  Configure(dev, cfgroot);
}
