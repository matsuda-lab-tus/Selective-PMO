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


 std::uint8_t *my_allocator( size_t const& size )
  {
   return (std::uint8_t *) malloc( size );
  }

int main( int argc, char *argv[] )
 {
  std::ifstream ifs( DATA_FOLDER "wikipedia.pbm", std::ios_base::binary );
  std::uint8_t *data;
  PNM::Info info;

  ifs >> PNM::load( &data, my_allocator, info );
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

