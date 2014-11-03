#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <stdint.h>

#define LINEBUF_SIZE	0x1000

#define MAX_IDLE_SECS 	90

static uint32_t lastrand = 0;


typedef void (*action_fptr)( uint8_t exit_num );

action_fptr exit_func = NULL;

void mysrand( uint32_t seed )
{	
	lastrand = seed - 1;
}

uint32_t myrand(void)
{	int r;
	{	// As supplied by D McDonnell	from SAS Insititute C
		r = (((((((((((lastrand << 3) - lastrand) << 3)
        + lastrand) << 1) + lastrand) << 4)
        - lastrand) << 1) - lastrand) + 0xe60)
        & 0x7fffffff;
    lastrand = r - 1;	
	}
	return(r);
}

uint8_t randbyte(void)	{ return (uint8_t)(myrand()&0xFF);}

uint32_t randrange(uint32_t min, uint32_t max )
{
	uint32_t difference = max - min;

	return ((myrand()%(difference+1))+min);
}

static size_t get_my_line (char *buff, size_t sz) 
{
	int ch, extra;
	int counter = 0;
	int readbytes = -1;

	while ( (counter < sz) && (ch != EOF) )
	{
		ch = getc( stdin );

		if ( ch != EOF )
			buff[counter] = ch;

		if ( buff[counter] == '\n' )
		{
			counter++;
			break;
		}

		counter++;
	}

	return counter;
}

void sig_alarm_handler( int signum )
{
	// Send connection close
	printf( "Connection idle, closing.\n" );

	// Shutdown
	exit( 1 );
}


void do_exit( uint8_t exit_num )
{
	printf( "Did you forget to read the flag with your shellcode?\n" );
	printf( "Exiting\n" );

	exit( exit_num );
}

int main()
{
	size_t line_result;	
	uint32_t i;
	uint8_t line_buf[LINEBUF_SIZE+4];

	struct alloc_data
	{
		uint8_t *data_ptr;
		size_t  alloc_size;
	};	

	struct alloc_data mem_list[100];


	setvbuf( stdout, NULL, _IONBF, 0 );

	// Turn on signal alarm handler
	signal( SIGALRM, sig_alarm_handler );
	alarm( MAX_IDLE_SECS );
	
	// Seed random number generator
	mysrand( 0x1234 );	

	// Banner
	printf("\nWelcome to your first heap overflow...\n");

	// Inform the user what we are doing (hold there hands)
	printf( "I am going to allocate 20 objects...\n" );
	printf( "Using Dougle Lee Allocator 2.6.1...\nGoodluck!\n\n" );

	exit_func = do_exit;
	
	printf ( "Exit function pointer is at %X address.\n", &exit_func );

	// Run the allocate loop!
	for ( i=0; i<20; i++ )
	{
		size_t alloc_size = randrange( 0x200, 0x500 );

		if ( i == 10 )
			alloc_size = 260;

		mem_list[i].data_ptr = (uint8_t*)malloc( alloc_size );
		mem_list[i].alloc_size = alloc_size;

		printf( "[ALLOC][loc=%X][size=%d]\n", mem_list[i].data_ptr, alloc_size ); 		
	}

	// Now let them write past the object
	printf( "Write to object [size=%d]:\n", mem_list[10].alloc_size );

	line_result = get_my_line( line_buf, LINEBUF_SIZE );

	memcpy( mem_list[10].data_ptr, line_buf, line_result );

	printf( "Copied %d bytes.\n", line_result );


	
	for ( i = 0; i < 20; i++ )
	{
		printf( "[FREE][address=%X]\n", mem_list[i].data_ptr );

		free( mem_list[i].data_ptr );
	}


	(*exit_func)( 1 );

	return 0;
}
