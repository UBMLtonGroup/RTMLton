# CDsml
https://www.cs.purdue.edu/sss/projects/cdx/

SML version of the CDx benchmarks. Implements only the Collision detector not ATS. CD reads from a text file retrieved from the binary dump of frames available. Runs periodically processing one one frame per period. 

To Generate the frames text file (in Simulator folder): 

javac ReadFrames.java && java ReadFrames col.bin

To run the benchmark: 


mlton runCD.mlb && ./runCD
