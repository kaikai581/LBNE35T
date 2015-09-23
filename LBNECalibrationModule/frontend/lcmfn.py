from Tkinter import *
from shutil import copyfile
import os
import ttk
import tkMessageBox

def isfloat(value):
  try:
    float(value)
    return True
  except ValueError:
    return False

def hex2dec(hcurr, hmin, hmax, dmin, dmax):
  return dmin + float((dmax-dmin))/(int(hmax,16)-int(hmin,16))*(int(hcurr,16)-int(hmin,16))

def dec2hex(dcurr, dmin, dmax, hmin, hmax):
  return int(hmin,16) + float((dcurr-dmin))/(dmax-dmin)*(int(hmax,16)-int(hmin,16))

class App:

  def __init__(self, master):

    # load configuration from existing config file
    projectdir = os.getenv('PROJECT_ROOT', '..')
    self.config_fname = projectdir + '/lcm.conf'
    self.GetConfigParams(self.config_fname)
    #~ for x in self.config_params:
      #~ print x + ' ' + self.config_params[x]

    frame = Frame(master)

    ################################################################################
    # bias control widgets
    Label(master, text='bias control:').grid(row=0, sticky=E)
    self.dft_bs_ctl = StringVar(master)
    self.dft_bs_ctl.set(self.config_params['bias_control'])
    self.opt_bias_control = OptionMenu(master, self.dft_bs_ctl, "0", "1")
    self.opt_bias_control.grid(row=0, column=1, columnspan=2, sticky = W)

    ################################################################################
    # ip widgets
    Label(master, text='ip:').grid(row=1, sticky=E)
    self.entry_ip = Entry(master)
    self.entry_ip.grid(row=1, column=1, columnspan=2, sticky=W)
    self.entry_ip.insert(0, self.config_params['ip'][1:-1])

    ################################################################################
    # pulse sets widgets
    Label(master, text='number of pulse sets (0 to 131071):').grid(row=2, sticky=E)
    # self.combo_npulse = ttk.Combobox(master)
    # self.combo_npulse.grid(row=2, column=1, columnspan=2, sticky=W)
    # combo_npulse_opts = list()
    # for i in range(0, 131072):
    #   combo_npulse_opts.append(str(i))
    # self.combo_npulse['value'] = combo_npulse_opts
    self.entry_npulse = Entry(master)
    self.entry_npulse.grid(row=2, column=1, columnspan=2, sticky=W)
    number_shown = self.config_params['pulse_sets']
    if self.config_params['pulse_sets'].startswith('0x'):
      number_shown = str(int(self.config_params['pulse_sets'],16))
    self.entry_npulse.insert(0, number_shown)

    ################################################################################
    # pulse height widgets
    Label(master, text='pulse height (0 to 30 V):').grid(row=3, sticky=E)
    self.entry_pulse_height = Entry(master)
    self.entry_pulse_height.grid(row=3, column=1, sticky=W)
    number_shown = self.config_params['pulse_height']
    if self.config_params['pulse_height'].startswith('0x'):
      number_shown = str(hex2dec(self.config_params['pulse_height'], "0x00040000", "0x00040FFF", 0, 30))
    self.entry_pulse_height.insert(0, number_shown)
    Label(master, text='V').grid(row=3, column=2, sticky=W)

    ################################################################################
    # register field labels
    Label(master, text='NOvA enable:').grid(row=5, sticky=E)
    Label(master, text='trigger source:').grid(row=6, sticky=E)
    Label(master, text='pulse delay (0 to 13650 ns):').grid(row=7, sticky=E)
    Label(master, text='second pulse width (0 to 850 ns):').grid(row=8, sticky=E)
    Label(master, text='first pulse width (0 to 850 ns):').grid(row=9, sticky=E)
    Label(master, text='IU').grid(row=4, column=1, columnspan=2)
    Label(master, text='TPC').grid(row=4, column=3, columnspan=2)
    Label(master, text='PD').grid(row=4, column=5, columnspan=2)

    ################################################################################
    # entry for each register fields, total 3x5=15
    self.default_iu_nova_enable = StringVar(master)
    self.default_iu_nova_enable.set(self.config_params['iu_nova_enable'])
    self.opt_iu_nova_enable = OptionMenu(master, self.default_iu_nova_enable, "0", "1")
    self.opt_iu_nova_enable.grid(row=5, column=1, columnspan=2)
    
    self.default_tpc_nova_enable = StringVar(master)
    self.default_tpc_nova_enable.set(self.config_params['tpc_nova_enable'])
    self.opt_tpc_nova_enable = OptionMenu(master, self.default_tpc_nova_enable, "0", "1")
    self.opt_tpc_nova_enable.grid(row=5, column=3, columnspan=2)

    self.default_pd_nova_enable = StringVar(master)
    self.default_pd_nova_enable.set(self.config_params['pd_nova_enable'])
    self.opt_pd_nova_enable = OptionMenu(master, self.default_pd_nova_enable, "0", "1")
    self.opt_pd_nova_enable.grid(row=5, column=5, columnspan=2)

    self.default_iu_trigger_source = StringVar(master)
    self.default_iu_trigger_source.set(self.config_params['iu_trigger_source'])
    self.opt_iu_trigger_source = OptionMenu(master, self.default_iu_trigger_source, "0", "1", "2", "3", "4", "5", "6", "7")
    self.opt_iu_trigger_source.grid(row=6, column=1, columnspan=2)

    self.default_tpc_trigger_source = StringVar(master)
    self.default_tpc_trigger_source.set(self.config_params['tpc_trigger_source'])
    self.opt_tpc_trigger_source = OptionMenu(master, self.default_tpc_trigger_source, "0", "1", "2", "3", "4", "5", "6", "7")
    self.opt_tpc_trigger_source.grid(row=6, column=3, columnspan=2)

    self.default_pd_trigger_source = StringVar(master)
    self.default_pd_trigger_source.set(self.config_params['pd_trigger_source'])
    self.opt_pd_trigger_source = OptionMenu(master, self.default_pd_trigger_source, "0", "1", "2", "3", "4", "5", "6", "7")
    self.opt_pd_trigger_source.grid(row=6, column=5, columnspan=2)

    self.entry_iu_pulse_delay = Entry(master)
    self.entry_iu_pulse_delay.grid(row=7, column=1)
    number_shown = self.config_params['iu_pulse_delay']
    if self.config_params['iu_pulse_delay'].startswith('0x'):
      number_shown = str(hex2dec(self.config_params['iu_pulse_delay'], '0x0', '0xFFF', 0, 13650))
    self.entry_iu_pulse_delay.insert(0, number_shown)
    Label(master, text='ns').grid(row=7, column=2, sticky=W)

    self.entry_tpc_pulse_delay = Entry(master)
    self.entry_tpc_pulse_delay.grid(row=7, column=3)
    self.entry_tpc_pulse_delay.insert(0, str(hex2dec(self.config_params['tpc_pulse_delay'], '0x0', '0xFFF', 0, 13650)))
    Label(master, text='ns').grid(row=7, column=4, sticky=W)

    self.entry_pd_pulse_delay = Entry(master)
    self.entry_pd_pulse_delay.grid(row=7, column=5)
    self.entry_pd_pulse_delay.insert(0, str(hex2dec(self.config_params['pd_pulse_delay'], '0x0', '0xFFF', 0, 13650)))
    Label(master, text='ns').grid(row=7, column=6, sticky=W)

    self.entry_iu_pulse_width_2 = Entry(master)
    self.entry_iu_pulse_width_2.grid(row=8, column=1)
    self.entry_iu_pulse_width_2.insert(0, str(hex2dec(self.config_params['iu_pulse_width_2'], '0x0', '0xFF', 0, 850)))
    Label(master, text='ns').grid(row=8, column=2, sticky=W)

    self.entry_tpc_pulse_width_2 = Entry(master)
    self.entry_tpc_pulse_width_2.grid(row=8, column=3)
    self.entry_tpc_pulse_width_2.insert(0, str(hex2dec(self.config_params['tpc_pulse_width_2'], '0x0', '0xFF', 0, 850)))
    Label(master, text='ns').grid(row=8, column=4, sticky=W)

    self.entry_pd_pulse_width_2 = Entry(master)
    self.entry_pd_pulse_width_2.grid(row=8, column=5)
    self.entry_pd_pulse_width_2.insert(0, str(hex2dec(self.config_params['pd_pulse_width_2'], '0x0', '0xFF', 0, 850)))
    Label(master, text='ns').grid(row=8, column=6, sticky=W)

    self.entry_iu_pulse_width_1 = Entry(master)
    self.entry_iu_pulse_width_1.grid(row=9, column=1)
    self.entry_iu_pulse_width_1.insert(0, str(hex2dec(self.config_params['iu_pulse_width_1'], '0x0', '0xFF', 0, 850)))
    Label(master, text='ns').grid(row=9, column=2, sticky=W)

    self.entry_tpc_pulse_width_1 = Entry(master)
    self.entry_tpc_pulse_width_1.grid(row=9, column=3)
    self.entry_tpc_pulse_width_1.insert(0, str(hex2dec(self.config_params['tpc_pulse_width_1'], '0x0', '0xFF', 0, 850)))
    Label(master, text='ns').grid(row=9, column=4, sticky=W)

    self.entry_pd_pulse_width_1 = Entry(master)
    self.entry_pd_pulse_width_1.grid(row=9, column=5)
    self.entry_pd_pulse_width_1.insert(0, str(hex2dec(self.config_params['pd_pulse_width_1'], '0x0', '0xFF', 0, 850)))
    Label(master, text='ns').grid(row=9, column=6, sticky=W)

    self.button_run = Button(master, text='Run', command=self.WriteConfig)
    self.button_run.grid(row=10, column=5, columnspan=2)


  def say_hi(self):
    print "hi there, everyone!"

  def GetConfigParams(self, fname):
    self.config_params = dict()
    with open(fname) as old_file:
      for line in old_file:
        if (not (line.startswith('#') or line.startswith('//'))) and ('=' in line):
          keyvalue = line.split('=')
          key = keyvalue[0].strip()
          value = keyvalue[1].split(';')[0].strip()
          self.config_params[key] = value

  def WriteConfig(self):
    # check the value of each entry
    try:
      entry_value = int(self.entry_npulse.get())
    except ValueError:
      tkMessageBox.showwarning('not a number', 'input number of pulses is not an integer')
      return
    if(not 0 <= entry_value <= 131071):
      tkMessageBox.showwarning('invalid number of pulses', 'number of pulses not within [0, 131071]')
      return

    try:
      entry_value = float(self.entry_pulse_height.get())
    except ValueError:
      tkMessageBox.showwarning('not a number', 'input pulse height is not a number')
      return
    if(not 0. <= entry_value <= 30.):
      tkMessageBox.showwarning('out of range', 'pulse height not within [0, 30] V')
      return

    try:
      entry_value = float(self.entry_iu_pulse_delay.get())
    except ValueError:
      tkMessageBox.showwarning('not a number', 'input IU pulse delay is not a number')
      return
    if(not 0. <= entry_value <= 13650.):
      tkMessageBox.showwarning('out of range', 'IU pulse delay not within [0, 13650] ns')
      return

    try:
      entry_value = float(self.entry_tpc_pulse_delay.get())
    except ValueError:
      tkMessageBox.showwarning('not a number', 'input TPC pulse delay is not a number')
      return
    if(not 0. <= entry_value <= 13650.):
      tkMessageBox.showwarning('out of range', 'TPC pulse delay not within [0, 13650] ns')
      return

    try:
      entry_value = float(self.entry_pd_pulse_delay.get())
    except ValueError:
      tkMessageBox.showwarning('not a number', 'input PD pulse delay is not a number')
      return
    if(not 0. <= entry_value <= 13650.):
      tkMessageBox.showwarning('out of range', 'PD pulse delay not within [0, 13650] ns')
      return

    try:
      entry_value = float(self.entry_iu_pulse_width_2.get())
    except ValueError:
      tkMessageBox.showwarning('not a number', 'input IU second pulse width is not a number')
      return
    if(not 0. <= entry_value <= 850.):
      tkMessageBox.showwarning('out of range', 'IU second pulse width not within [0, 850] ns')
      return

    try:
      entry_value = float(self.entry_tpc_pulse_width_2.get())
    except ValueError:
      tkMessageBox.showwarning('not a number', 'input TPC second pulse width is not a number')
      return
    if(not 0. <= entry_value <= 850.):
      tkMessageBox.showwarning('out of range', 'TPC second pulse width not within [0, 850] ns')
      return

    try:
      entry_value = float(self.entry_pd_pulse_width_2.get())
    except ValueError:
      tkMessageBox.showwarning('not a number', 'input PD second pulse width is not a number')
      return
    if(not 0. <= entry_value <= 850.):
      tkMessageBox.showwarning('out of range', 'PD second pulse width not within [0, 850] ns')
      return

    try:
      entry_value = float(self.entry_iu_pulse_width_1.get())
    except ValueError:
      tkMessageBox.showwarning('not a number', 'input IU first pulse width is not a number')
      return
    if(not 0. <= entry_value <= 850.):
      tkMessageBox.showwarning('out of range', 'IU first pulse width not within [0, 850] ns')
      return

    try:
      entry_value = float(self.entry_tpc_pulse_width_1.get())
    except ValueError:
      tkMessageBox.showwarning('not a number', 'input TPC first pulse width is not a number')
      return
    if(not 0. <= entry_value <= 850.):
      tkMessageBox.showwarning('out of range', 'TPC first pulse width not within [0, 850] ns')
      return

    try:
      entry_value = float(self.entry_pd_pulse_width_1.get())
    except ValueError:
      tkMessageBox.showwarning('not a number', 'input PD first pulse width is not a number')
      return
    if(not 0. <= entry_value <= 850.):
      tkMessageBox.showwarning('out of range', 'PD first pulse width not within [0, 850] ns')
      return

    # represent each number in hexadecimal value and write to the value dictionary
    self.config_params['bias_control'] = self.dft_bs_ctl.get()
    self.config_params['ip'] = '\"' + self.entry_ip.get() + '\"'
    self.config_params['pulse_sets'] = str(hex(int(self.entry_npulse.get())))
    self.config_params['pulse_height'] = str(hex(int(round(dec2hex(float(self.entry_pulse_height.get()), 0., 30., '0x40000', '0x40FFF')))))
    
    self.config_params['iu_nova_enable'] = self.default_iu_nova_enable.get()
    self.config_params['iu_trigger_source'] = self.default_iu_trigger_source.get()
    self.config_params['iu_pulse_delay'] = str(hex(int(round(dec2hex(float(self.entry_iu_pulse_delay.get()), 0., 13650., '0x0', '0xFFF')))))
    self.config_params['iu_pulse_width_2'] = str(hex(int(round(dec2hex(float(self.entry_iu_pulse_width_2.get()), 0., 850., '0x0', '0xFF')))))
    self.config_params['iu_pulse_width_1'] = str(hex(int(round(dec2hex(float(self.entry_iu_pulse_width_1.get()), 0., 850., '0x0', '0xFF')))))
    
    self.config_params['tpc_nova_enable'] = self.default_tpc_nova_enable.get()
    self.config_params['tpc_trigger_source'] = self.default_tpc_trigger_source.get()
    self.config_params['tpc_pulse_delay'] = str(hex(int(round(dec2hex(float(self.entry_tpc_pulse_delay.get()), 0., 13650., '0x0', '0xFFF')))))
    self.config_params['tpc_pulse_width_2'] = str(hex(int(round(dec2hex(float(self.entry_tpc_pulse_width_2.get()), 0., 850., '0x0', '0xFF')))))
    self.config_params['tpc_pulse_width_1'] = str(hex(int(round(dec2hex(float(self.entry_tpc_pulse_width_1.get()), 0., 850., '0x0', '0xFF')))))
    
    self.config_params['pd_nova_enable'] = self.default_pd_nova_enable.get()
    self.config_params['pd_trigger_source'] = self.default_pd_trigger_source.get()
    self.config_params['pd_pulse_delay'] = str(hex(int(round(dec2hex(float(self.entry_pd_pulse_delay.get()), 0., 13650., '0x0', '0xFFF')))))
    self.config_params['pd_pulse_width_2'] = str(hex(int(round(dec2hex(float(self.entry_pd_pulse_width_2.get()), 0., 850., '0x0', '0xFF')))))
    self.config_params['pd_pulse_width_1'] = str(hex(int(round(dec2hex(float(self.entry_pd_pulse_width_1.get()), 0., 850., '0x0', '0xFF')))))
    #tkMessageBox.showinfo('value', self.config_params['pd_pulse_width_1'])
    
    # write values from the dictionary back to the configuration file
    new_file_lines = list()
    with open(self.config_fname) as source_file:
      for line in source_file:
        # if this line is not a comment and contains a '=', overwrite with the new value
        if (not (line.startswith('#') or line.startswith('//'))) and ('=' in line):
          keyvalue = line.split('=')
          key = keyvalue[0].strip()
          new_line = key + ' = ' + self.config_params[key] + ';\n'
          new_file_lines.append(new_line)
        else:
          new_file_lines.append(line)
    copyfile(self.config_fname, self.config_fname+'.bak')
    with open(self.config_fname, 'w') as new_file:
      for line in new_file_lines:
        new_file.write(line)
        

if __name__ == '__main__':
  root = Tk()

  app = App(root)

  root.wm_title("ANL Calibration Module Front End")
  root.attributes("-topmost", True)
  root.resizable(0,0)
  root.mainloop()
  # root.destroy() # optional; see description below
