#include "DeessEngine.h"
#include <fstream>
#include <vector>
#include <iostream>
#include <cstdlib>
int main(int argc,char** argv){
 if(argc<10){std::cerr<<"input.f32 output.f32 metrics.csv sampleRate channels threshold ratio low gain\n";return 1;}
 std::ifstream f(argv[1],std::ios::binary|std::ios::ate); if(!f)return 2;
 auto bytes=f.tellg();f.seekg(0);std::vector<float> x(static_cast<size_t>(bytes)/sizeof(float));f.read((char*)x.data(),bytes);
 double sr=std::atof(argv[4]);int ch=std::atoi(argv[5]);
 deess::Engine e;e.prepare(sr,ch);e.setControls({float(std::atof(argv[6])),float(std::atof(argv[7])),float(std::atof(argv[8])),float(std::atof(argv[9])),false});
 std::vector<float> out(x.size());std::ofstream csv(argv[3]);csv<<"time,confidence,event,level,repair,eventGain\n";
 const size_t n=x.size()/ch;for(size_t i=0;i<n+e.latencySamples();++i){float a,b; e.process(i<n?x[i*ch]:0,i<n?x[i*ch+ch-1]:0,a,b);
 if(i>=size_t(e.latencySamples())){size_t j=i-e.latencySamples();out[j*ch]=a;if(ch==2)out[j*ch+1]=b;}
 if(i%deess::hopSize==deess::hopSize-1){auto m=e.meters();csv<<(double(i)-(deess::fftSize-1)*.5)/sr<<','<<m.confidence<<','<<m.event<<','<<m.detectorDb<<','<<m.repairPeakDb<<','<<m.eventGainDb<<'\n';}}
 std::ofstream o(argv[2],std::ios::binary);o.write((char*)out.data(),out.size()*sizeof(float));
}
