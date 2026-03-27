16-Dec-23
This directory uses my own way of storing HDF5 files for my projects
and converts them into CERN root files. 

In my files I have the follwing information:
-H5_VersionInformation containing a version number in case I chagnge format
- H5_Logger_Header a string containing the filename, time of creation and a name for the dataset. 
- A string describing the layout of the variables in the form var1:var2:...

I can use the last one to create the variable name header for root
The dataset name can be used in making the TFile

