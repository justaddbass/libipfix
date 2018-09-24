/*
**     exporter.c - example exporter
**
**     Copyright Fraunhofer FOKUS
**
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <ipfix.h>
#include "mlog.h"

struct http_record {
    unsigned short request_method;
    unsigned short status_code;
};

int main ( int argc, char **argv )
{
    char      *optstr="hc:p:vstu";
    int       opt;
    char      chost[256];
    int       protocol = IPFIX_PROTO_TCP;
    int       j;
    char      buf[31]  = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                           11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
                           21, 22, 23, 24, 25, 26, 27, 28, 29, 30 };

    ipfix_t           *ipfixh  = NULL;
    ipfix_template_t  *http_session_template  = NULL;
    ipfix_template_t  *stl_template = NULL;
    int               sourceid = 12345;
    int               port     = IPFIX_PORTNO;
    int               verbose_level = 0;

    /* set default host */
    strcpy(chost, "localhost");

    /** process command line args
     */
    while( ( opt = getopt( argc, argv, optstr ) ) != EOF )
    {
	switch( opt )
	{
	  case 'p':
	    if ((port=atoi(optarg)) <0) {
		fprintf( stderr, "Invalid -p argument!\n" );
		exit(1);
	    }
            break;

	  case 'c':
            strcpy(chost, optarg);
	    break;

          case 's':
              protocol = IPFIX_PROTO_SCTP;
              break;

          case 't':
              protocol = IPFIX_PROTO_TCP;
              break;

          case 'u':
              protocol = IPFIX_PROTO_UDP;
              break;

          case 'v':
              verbose_level ++;
              break;

	  case 'h':
	  default:
              fprintf( stderr, "usage: %s [-hstuv] [-c collector] [-p portno]\n"
                       "  -h               this help\n"
                       "  -c <collector>   collector address\n"
                       "  -p <portno>      collector port number (default=%d)\n"
                       "  -s               send data via SCTP\n"
                       "  -t               send data via TCP (default)\n"
                       "  -u               send data via UDP\n"
                       "  -v               increase verbose level\n\n",
                       argv[0], IPFIX_PORTNO  );
              exit(1);
	}
    }

    /** init loggin
     */
    mlog_set_vlevel( verbose_level );

    /** init lib
     */
    if ( ipfix_init() <0) {
        fprintf( stderr, "cannot init ipfix module: %s\n", strerror(errno) );
        exit(1);
    }

    /** open ipfix exporter
     */
    if ( ipfix_open( &ipfixh, sourceid, IPFIX_VERSION ) <0 ) {
        fprintf( stderr, "ipfix_open() failed: %s\n", strerror(errno) );
        exit(1);
    }

    /** set collector to use
     */
    if ( ipfix_add_collector( ipfixh, chost, port, protocol ) <0 ) {
        fprintf( stderr, "ipfix_add_collector(%s,%d) failed: %s\n",
                 chost, port, strerror(errno));
        exit(1);
    }

    //http session template
    if ( ipfix_new_data_template( ipfixh, &http_session_template, 2 ) <0 ) {
        fprintf( stderr, "ipfix_new_template() failed: %s\n",
                 strerror(errno) );
        exit(1);
    }
    if((ipfix_add_field(ipfixh, http_session_template, 0, IPFIX_FT_HTTPREQUESTMETHOD, 2) < 0)
            || (ipfix_add_field(ipfixh, http_session_template, 0, IPFIX_FT_HTTPSTATUSCODE, 2) < 0)) {
        fprintf(stderr, "125 add field error %s\n", strerror(errno));
        exit(1);
    }

    //stl template
    if(ipfix_new_data_template(ipfixh, &stl_template, 2) < 0) {
        fprintf(stderr, "new template error %s\n", strerror(errno));
        exit(1);
    }
    if((ipfix_add_field(ipfixh, stl_template, 0, IPFIX_FT_SOURCEIPV4ADDRESS, 4) < 0)
            || (ipfix_add_stl(ipfixh, stl_template))) {
        fprintf(stderr, "136 add field error %s\n", strerror(errno));
        exit(1);
    }

    ipfix_set_stl_tmpl(ipfixh, stl_template, http_session_template);

    struct http_record *rec = malloc(sizeof(struct http_record) * 4);
    rec[0] = (struct http_record) {.request_method=1, .status_code=200};
    rec[1] = (struct http_record) {.request_method=2, .status_code=301};
    rec[2] = (struct http_record) {.request_method=3, .status_code=404};
    rec[3] = (struct http_record) {.request_method=4, .status_code=500};

    subtemplatelist_t *stl = malloc(sizeof(subtemplatelist_t));
    stl->addrs = malloc(sizeof(struct http_record*) * 4);
    for(int i = 0; i < 4; ++i)
        stl->addrs[i] = &rec[i];
    stl->lens = malloc(sizeof(uint32_t) * 4);
    for(int i = 0; i < 4; ++i)
        stl->lens[i] = 4;
    stl->templ = http_session_template;
    stl->elem_count = 4;

    /** export some data
     */
    for( j=0; j<4; j++ ) {

        printf( "[%d] export some data ... ", j );
        fflush( stdout) ;

        /*if ( ipfix_export( ipfixh, http_session_template, buf, &bytes ) <0 ) {
            fprintf( stderr, "ipfix_export() failed: %s\n",
                     strerror(errno) );
            exit(1);
        }*/

        if(ipfix_export(ipfixh, stl_template, buf, stl) < 0) {
            fprintf( stderr, "ipfix_export() failed: %s\n", strerror(errno) );
            exit(1);
        }
        if(ipfix_export(ipfixh, stl_template, buf, stl) < 0) {
            fprintf( stderr, "ipfix_export() failed: %s\n", strerror(errno) );
            exit(1);
        }

        if ( ipfix_export_flush( ipfixh ) <0 ) {
            fprintf( stderr, "ipfix_export_flush() failed: %s\n",
                     strerror(errno) );
            exit(1);
        }

        printf( "done.\n" );
        //bytes++;
        sleep(1);
    }

    printf( "data exported.\n" );

    /** clean up
     */
    free(rec);
    ipfix_delete_template( ipfixh, http_session_template );
    ipfix_delete_template( ipfixh, stl_template);
    ipfix_close( ipfixh );
    ipfix_cleanup();
    exit(0);
}
