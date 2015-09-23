#!/usr/bin/python
# usage:
# $ python trigger_rate.py [filename]

import sys
import csv
import os.path
import math

# open input file #
infilepath = '../../sample_data/'
infilename = 'light_leak_background_3-0_2015-09-22_10-27-10.csv'
infilepathname = infilepath + infilename
if len(sys.argv) == 2:
  infilepathname = infilepath + str(sys.argv[1])
if not os.path.isfile(infilepathname):
  print 'input file does not exist'
  sys.exit()

triggertime = dict()
timestamp_all = list()
with open(infilepathname, 'rb') as csvfile:
  spamreader = csv.reader(csvfile)
  spamreader.next() # remove the first line
  for row in spamreader:
    if len(row) == 37:
      # bit string
      # details are in the email titled "Internal Timestamp from LBNEWare"
      timestamp_all.append(int(row[27])*int(math.pow(2,32))+int(row[26])*int(math.pow(2,16))+int(row[25]))
      # row 12 is the channel id

print float(len(timestamp_all))/(timestamp_all[-1]-timestamp_all[0])*150e6
