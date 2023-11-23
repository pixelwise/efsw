#include <efsw/FileSystem.hpp>
#include <efsw/System.hpp>
#include <efsw/efsw.hpp>
#include <iostream>
#include <signal.h>

bool STOP = false;

void sigend( int ) {
	std::cout << std::endl << "Bye bye" << std::endl;
	STOP = true;
}

int main( int argc, char** argv ) {
	signal( SIGABRT, sigend );
	signal( SIGINT, sigend );
	signal( SIGTERM, sigend );

	std::cout << "Press ^C to exit demo" << std::endl;


	if ( argc >= 2 ) {
		auto path = std::string( argv[1] );
		auto guard = efsw::watch(path, [](efsw::GenericFileWatchListener::Event event){
			std::cout << event.filename << std::endl;
		});
		while ( !STOP )
			efsw::System::sleep( 100 );
	}

	return 0;
}
