#include <fstream>
#include <iomanip>
#include <iostream>
#include "DeviceInterface.h"
#include <unistd.h>

using namespace std;

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

void ReadbackAndLog(SSPDAQ::DeviceInterface& dev, string outfn)
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
  }
  outf.close();
  cout << "Monitor values logged." << endl;
}

int main(int argc, char** argv)
{
  
  //Open SSP device
  unsigned long board_id=inet_network("192.168.1.108");
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
  /* read values back from the registers and log the value to file */
  /* read values every x seconds. */
  while(1)
  {
    ReadbackAndLog(dev, outfn);
    usleep(30e6); /// in microseconds
  }
  
}
