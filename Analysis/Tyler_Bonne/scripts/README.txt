There are a few things that must be done to your LArsoft directory in order to run this analysis.

First, go to srcs/lbnecode/lbne and do 'mkdir HitDumper' 
Copy everything from /lbne/app/users/tboone/newdevdir/srcs/HitDumper to your own Hitdumper folder
Now go to your srcs/lbnecode/lbne and edit your CMakeLists.txt to include the new HitDumper directory

You must also edit your srcs/lbnecode/lbne/AnaTree/AnaTree_module.cc
Comment out these lines:
Line 573::   art::FindMany<recob::Track>       fmtk (hitListHandle  , evt, fTrackModuleLabel);
Line 587::   if (fmtk.at (i).size()!=0){
Line 588::       hit_trkid[i] = fmtk.at (i)[0]->ID();
Line 589::   }
Save the file and then recompile LArsoft by going to your build directory and typing 'make install'