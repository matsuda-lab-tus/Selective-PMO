#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>

#include "3rdparty/PNM/src/pnm/pnm.hpp"

using namespace std;


#ifdef _MSC_VER
#define DATA_FOLDER "../data"
#define OUT_FOLDER  "../out"
#endif

#ifdef __GNUC__
#define DATA_FOLDER  "./data"
#define OUT_FOLDER   "./out"
#endif


int main( int argc, char *argv[] )
 {
 std::ifstream ifs( DATA_FOLDER "wikipedia.pbm", std::ios_base::binary );
 PNM::Info info;

 ifs >> PNM::probe( info );
 if( true == info.valid() )
  {
   std::cout << "width   = "  << info.width ()    << std::endl;
   std::cout << "height  = "  << info.height()    << std::endl;
   std::cout << "max     = "  << info.max()       << std::endl;
   std::cout << "channel = "  << info.channel()   << std::endl;
   std::cout << "type    = "  << (int)info.type() << std::endl;
  }

  return EXIT_SUCCESS;
 }

