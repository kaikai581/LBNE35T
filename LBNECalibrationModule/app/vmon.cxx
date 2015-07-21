#include <fstream>
#include <iomanip>
#include <iostream>
#include "DeviceInterface.h"
#include <unistd.h>
#include "json/json.h"
#include "zmq.hpp"

using namespace std;

void Configure(SSPDAQ::DeviceInterface& dev){

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

  dev.SetRegisterArrayByName("bias_config",           0x00040FFF); //# 30V
   //        #ALL_bias_config:           0x00040E21 # 26.5V - bit 0x40000 enables bias, bits 0xFFF set value
   //     #                                      # in range 0-30V

   }

string GetCurrentTimeStamp()
{
  time_t rawtime;
  struct tm * timeinfo;
  char buffer [80];
  time (&rawtime);
  timeinfo = localtime (&rawtime);
  strftime (buffer,80,"%m/%d/%Y %T",timeinfo);
  return string(buffer);
}

string GetOutputFileName()
{
  /* FORMAT: SSP-MM_DD_YYYY-HH_MM_SS-1.csv */
  string resstr = "SSP-";
  time_t rawtime;
  struct tm * timeinfo;
  char buffer [80];

  time (&rawtime);
  timeinfo = localtime (&rawtime);
  
  strftime (buffer,80,"%m_%d_%Y-%H_%M_%S-1.csv",timeinfo);
  resstr += buffer;
  
  return resstr;
}

int SignedDecimal16Bit(unsigned int val)
{
  return (0x8000&val ? (int)(0x7FFF&val)-0x8000 : val);
}

double Register2Voltage(double regVal, double biasRb, double AGND, double AREF,
                        double& monVolt, double& monAmp)
{
  double vol = 0;
  double K1 = 4096./(AREF-AGND);
  double K2 = -K1*AGND;
  double R1 = 10000, R2 = 10000000, R3 = 1000000, R4 = 4990;
  double NV = 11*(K1*regVal+K2)/1000;
  biasRb = biasRb/((double)0x00000FFF)*30;
  double current = (biasRb-NV)/R1-NV/(R2+R3);
  vol = NV-current*2*R4;
  
  /// output results
  monVolt = vol;
  monAmp = current;
  //cout << hex << (int)regVal << " " << dec << vol << " " << NV << " " << biasRb << " " << current << endl;
  return vol;
}

void ReadbackAndLog(SSPDAQ::DeviceInterface& dev, string outfn, zmq::socket_t& socket)
{
  vector<unsigned int> monBiasVals, monValueVals, biasReadback;
  vector<double> monVolts, monAmps;
  dev.ReadRegisterArrayByName("mon_bias", monBiasVals);
  dev.ReadRegisterArrayByName("mon_value", monValueVals);
  dev.ReadRegisterArrayByName("bias_readback", biasReadback);
  /// The formula from register readings to voltage is provided in the SSP
  /// manual
  unsigned int uAGND = (monValueVals[1] & 0x0000FFFF);
  unsigned int uAREF = (monValueVals[2] & 0x0000FFFF);
  int AGND = SignedDecimal16Bit(uAGND);
  int AREF = SignedDecimal16Bit(uAREF);
  //cout << "AGND " << hex << AGND << " " << dec << AGND << endl;
  //cout << "AREF " << hex << AREF << endl;
  
  for(unsigned int i = 0; i < monBiasVals.size(); i++)
  {
    monBiasVals[i] &= 0x0000FFFF;
    biasReadback[i] &= 0x00000FFF;
    double monVolt, monAmp;
    Register2Voltage(monBiasVals[i], biasReadback[i], AGND, AREF, monVolt, monAmp);
    monVolts.push_back(monVolt);
    monAmps.push_back(monAmp);
  }
  
  /* get current timestamp and use it for every readings */
  string ts = GetCurrentTimeStamp();
  /* log values to be monitored */
  ofstream outf(outfn.c_str(), ofstream::out | ofstream::app);
  for(unsigned int i = 0; i < monVolts.size(); i++)
  {
    stringstream Vmon, Imon, Vset;
    Vmon << "35T.SSP_Vmon_ch" << i << ".F_CV";
    Imon << "35T.SSP_Imon_ch" << i << ".F_CV";
    Vset << "35T.SSP_Vset_ch" << i << ".F_CV";
    outf << Vmon.str() << "," << ts << "," << monVolts[i] << endl;
    outf << Imon.str() << "," << ts << "," << monAmps[i] << endl;
    outf << Vset.str() << "," << ts << "," << biasReadback[i]/((double)0x00000FFF)*30 << endl;
    
    /* set up JSON message with jsoncpp */
    Json::Value jsonV, jsonI;
    Json::FastWriter fastWriterV, fastWriterI;
    stringstream varnameV, varnameI;
    varnameV << "SSP08_Vmon_ch" << i;
    varnameI << "SSP08_Imon_ch" << i;
    /// standard fields
    jsonV["type"] = "moni";
    jsonV["service"] = "test_service";
    jsonV["varname"] = varnameV.str().c_str();
    jsonV["value"] = monVolts[i];
    jsonI["type"] = "moni";
    jsonI["service"] = "test_service";
    jsonI["varname"] = varnameI.str().c_str();
    jsonI["value"] = monAmps[i];
    std::string jsonMessageV = fastWriterV.write(jsonV);
    std::string jsonMessageI = fastWriterI.write(jsonI);
    zmq::message_t messageV(jsonMessageV.length());
    zmq::message_t messageI(jsonMessageI.length());
    memcpy(messageV.data(), jsonMessageV.c_str(), jsonMessageV.length());
    memcpy(messageI.data(), jsonMessageI.c_str(), jsonMessageI.length());
    cout << "voltage sent? " << socket.send(messageV) << endl;
    cout << "current sent? " << socket.send(messageI) << endl;
  }
  outf.close();
  cout << "Monitor values logged." << endl;
}

int main(int argc, char** argv){
  
  //Open SSP device
  unsigned long board_id=inet_network("192.168.1.107");
  SSPDAQ::DeviceInterface dev(SSPDAQ::kEthernet,board_id);
  //dev.Initialize();
  dev.OpenSlowControl();
  //Configure(dev);
  
  /* set output filename with current time */
  string outfn = GetOutputFileName();
  ofstream outf(outfn.c_str());
  outf << "[Data]" << endl;
  outf << "Tagname,TimeStamp,Value" << endl;
  outf.close();
  /* setup the server connection with ZeroMQ */
  zmq::context_t zmq_context(1);
  zmq::socket_t zmq_socket(zmq_context, ZMQ_PUSH);
  //zmq_socket.connect("tcp://lbne-moni:5000"); // this is a test machine at UMD
  zmq_socket.connect("tcp://lbnedaq1:5000");
  /* read values back from the registers and log the value to file */
  /* read values every x seconds. */
  while(1)
  {
    ReadbackAndLog(dev, outfn, zmq_socket);
    usleep(30e6); /// in microseconds
  }
  zmq_socket.close();
}
