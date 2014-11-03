#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if_packet.h>
#include <linux/if.h>
#include <linux/if_ether.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/ip6.h>
#include <signal.h>
#include <pwd.h>
#include "turdedo.h"


int sd;
int debug = 0;
struct in6_addr my_teredo_addr;

#define PORT 3544

#define IP_HDR_SIZE 20
#define IP6_HDR_SIZE 40
#define IP6_FRAG_HDR_SIZE 8
#define UDP_HDR_SIZE 8

#define SYN 0
#define SYNACK 1
#define ACK 2
#define ESTAB 3

#define MAX_CHILD_IDLE 10
#define MAX_FRAGMENT_AGE 5

unsigned short ip_id;
time_t last_clean_child_proc;
time_t last_clean_fragments;

void Handle_UDP(unsigned char *, struct sockaddr_in *);

struct child {
	pid_t pid;
	int read_fd;
	int write_fd;
	struct sockaddr_in remaddr;
	struct in_addr ip4_src;
	struct in6_addr ip6_src;
	struct in6_addr ip6_dst;
	u_int16_t udp_src;
	u_int16_t udp_dst;
	u_int32_t next_ident;
	time_t last;
	unsigned char state;
	struct child *next;
};
struct child *chld_head;

struct fragment {
	u_int32_t	ip6f_ident;
	struct in6_addr ip6_src;
	struct in6_addr ip6_dst;
	struct ip6_hdr	*ip6;
	u_int16_t	len;
	time_t		last;
	struct fragment *next;
};
struct fragment *fr_head;

int drop_privs( char * name )
{
	struct passwd * pw = NULL;

	if ( name == NULL )
	{
		exit(-1);
	}

	pw = getpwnam( name );

	if ( pw == NULL )
	{
		exit(-1);
	}

	if ( init_user( pw ) != 0 )
	{
		exit(-1);
	}

	return 0;
}

int init_user( struct passwd * pw )
{
	uid_t procuid = 0;
	gid_t procgid = 0;

	if (pw == NULL )
	{
		return -1;
	}

	procuid = getuid( );
	procgid = getgid( );

	if ( initgroups( pw->pw_name, pw->pw_gid ) != 0 )
	{
		return -1;
	}

	if ( setresgid( pw->pw_gid, pw->pw_gid, pw->pw_gid ) != 0 )
	{
		printf("setresgid failed\n");
		return -1;
	}

	if ( setresuid( pw->pw_uid, pw->pw_uid, pw->pw_uid ) != 0 )
	{
		printf("setresuid failed\n");
		return -1;
	}

	if ( procgid != pw->pw_gid )
	{
		if ( setgid( procgid ) != -1 )
		{
			printf("setgid failed\n");
			return -1;
		}

		if ( setegid( procgid ) != -1 )
		{
			printf("setegid failed\n");
			return -1;
		}
	}

	if ( procuid != pw->pw_uid )
	{
		if ( setuid( procuid ) != -1 )
		{
			printf("setuid failed\n");
			return -1;
		}

		if ( seteuid( procuid ) != -1 )
		{
			printf("seteuid failed\n");
			return -1;
		}
	}

	if ( getgid( ) != pw->pw_gid )
	{
		return -1;
	}

	if ( getegid( ) != pw->pw_gid )
	{
		return -1;
	}

	if ( getuid( ) != pw->pw_uid )
	{
		return -1;
	}

	if ( geteuid( ) != pw->pw_uid )
	{
		return -1;
	}

	if ( chdir( pw->pw_dir ) != 0 )
	{
		printf("chdir failed\n");
		return -1;
	}

	return 0;
}

char *FindErr(unsigned int num) {
	return(ERRMSG[num]);
}

u_int16_t udp6_cksum(struct child *chld, char *buf, unsigned int len) {
	unsigned int rem;
	u_int16_t cksum = 0;
	u_int16_t *p;
	int sum = 0;
	int i;

	// sum the ipv6 addrs
	i = 8;
	p = (u_int16_t *)(chld->ip6_src.s6_addr);
	while (i--) {
		sum += *p++;
	}
	i = 8;
	p = (u_int16_t *)(chld->ip6_dst.s6_addr);
	while (i--) {
		sum += *p++;
	}

	// sum the udp length (32-bit)
	rem = htonl(len+UDP_HDR_SIZE);
	p = (u_int16_t *)(&rem);
	sum += *p++;
	sum += *p++;

	// sum the next header (UDP)
	sum += 0x1100;

	// sum the UDP ports
	sum += htons(chld->udp_src);
	sum += htons(chld->udp_dst);

	// sum the UDP length again (16-bit)
	rem = htons(len+UDP_HDR_SIZE);
	p = (u_int16_t *)(&rem);
	sum += *p++;

	// sum the payload
	p = (u_int16_t *)buf;
	while (len >= 2) {
		sum += *p++;
		len -= 2;
	}
	if (len) {
		*(uint8_t *)(&cksum) = *(uint8_t *)p;
		sum += cksum;
	}

	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	cksum = ~sum;

	return(cksum);

}

void Find_Teredo_Addr(char *interface, char *ip) {
	int sd;
	struct ifreq ifr;
	uint32_t addr;

	// if we weren't passed an ip, get it from our interface
	if (!ip) {
		// Submit request for a socket descriptor to look up interface.
		if ((sd = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
			//if (debug) fprintf(stderr, "Unable to open socket to determine Teredo addr\n");
			if (debug) fprintf(stderr, FindErr(0));
			exit(-1);
		}
	
		// Use ioctl() to look up interface name and get its IPv4 addr
		bzero(&ifr, sizeof(ifr));
		snprintf (ifr.ifr_name, sizeof(ifr.ifr_name), "%s", interface);
		if (ioctl(sd, SIOCGIFADDR, &ifr) < 0) {
			if (debug) fprintf(stderr, FindErr(1));
			exit(-1);
		}
		close (sd);

		bzero(my_teredo_addr.s6_addr, sizeof(struct in6_addr));
		addr = ((struct sockaddr_in *)(&ifr.ifr_addr))->sin_addr.s_addr;
	} else {
		// just use the IP were given (in case we're behind NAT we need to provide our public IP)
		addr = inet_addr(ip);		
	}

	// form up the Teredo addr
	my_teredo_addr.s6_addr[0] = 0x20;
	my_teredo_addr.s6_addr[1] = 0x01;
	my_teredo_addr.s6_addr[2] = 0x00;
	my_teredo_addr.s6_addr[3] = 0x00;
	my_teredo_addr.s6_addr[4] = (addr & 0x000000ff);
	my_teredo_addr.s6_addr[5] = (addr & 0x0000ff00) >> 8;
	my_teredo_addr.s6_addr[6] = (addr & 0x00ff0000) >> 16;
	my_teredo_addr.s6_addr[7] = (addr & 0xff000000) >> 24;
	my_teredo_addr.s6_addr[8] = 0x00;
	my_teredo_addr.s6_addr[9] = 0x00;
	my_teredo_addr.s6_addr[10] = (PORT >> 8) ^ 0xff;
	my_teredo_addr.s6_addr[11] = (PORT & 0x00ff) ^ 0xff;
	my_teredo_addr.s6_addr[12] = (addr & 0x000000ff) ^ 0xff;
	my_teredo_addr.s6_addr[13] = ((addr & 0x0000ff00) >> 8) ^ 0xff;
	my_teredo_addr.s6_addr[14] = ((addr & 0x00ff0000) >> 16) ^ 0xff;
	my_teredo_addr.s6_addr[15] = ((addr & 0xff000000) >> 24) ^ 0xff;

}


int InitSocket() {
	struct sockaddr_in myaddr;

	// open a udp socket
	if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		fprintf(stderr, "socket failed\n");
		return(-1);
	}

	// bind to the desired port
	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(PORT);

	if (bind(sd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		close(sd);
		return(-1);
	}

	return(0);

}

struct fragment *Is_Known_Packet(unsigned char *ip6_pkt) {
	struct fragment *f;
	struct ip6_frag *ip6frag;
	struct ip6_hdr *ip6;

	ip6 = (struct ip6_hdr *)ip6_pkt;

	// get pointer to fragment header
	ip6frag = (struct ip6_frag *)(ip6_pkt+IP6_HDR_SIZE);
	
	f = fr_head;
	while (f) {
		// if the fragment ident, the src and dst ipv6 addr's match
		// we are working with a known packet
		if (ntohl(ip6frag->ip6f_ident) == f->ip6f_ident &&
		    !memcmp(ip6->ip6_src.s6_addr, f->ip6_src.s6_addr, 16) &&
		    !memcmp(ip6->ip6_dst.s6_addr, f->ip6_dst.s6_addr, 16)) {
			// found it
			return(f);
		}
		f = f->next;
	}

	return (NULL);
}

// check a buffer for any %n or %\d+n
void CN(unsigned char *buf, unsigned int len) {
	int i;
	int found = 0;
	char conv[] = "dDiouxXfegEasc[p";

	for (i = 0; i < len; i++) {
		// see if we find the beginnings of a format specifier
		if (buf[i] == '%') {
			found = 1;
			continue;
		}

		// we found one previously and now we found a 'n'
		if (found && buf[i] == 'n') {
			// naughty naughty
			buf[i] = '_';
			found = 0;
			continue;
		}

		// we found one previously, but now we found a non-n conversion specifier
		if (found && strchr(conv, buf[i])) {
			found = 0;
			continue;
		}
	}
}

void Handle_Fragment(unsigned char *ip6_pkt, int len, struct sockaddr_in *remaddr) {
	struct ip6_hdr *ip6;
	struct ip6_frag *ip6frag;
	unsigned char *pkt;
	struct fragment *f;
	u_int16_t offset;
	u_int16_t more;
	u_int16_t data_len;

	ip6 = (struct ip6_hdr *)ip6_pkt;

	// make sure the packet is even long enough to contain a fragment header
	if (len < IP6_HDR_SIZE+IP6_FRAG_HDR_SIZE) {
		//if (debug) fprintf(stderr, "fragement isn't long enough\n");
		if (debug) fprintf(stderr, FindErr(2));
		return;
	}
	//if (debug) fprintf(stderr, "got %d length packet\n", len);

	// see if this is a fragment ID we've seen before
	if (f = Is_Known_Packet(ip6_pkt)) {
		// yes, then add this fragment's data to the existing buffer
		//if (debug) fprintf(stderr, "known fragement\n");

		ip6frag = (struct ip6_frag *)(ip6_pkt+IP6_HDR_SIZE);

		//if (debug) fprintf(stderr, "known fragement, id: %d\n", htonl(ip6frag->ip6f_ident));
		offset = ntohs(ip6frag->ip6f_offlg & IP6F_OFF_MASK);
		
		// check that the offset is reasonable
		if (offset+ntohs(ip6->ip6_plen)-IP6_FRAG_HDR_SIZE > 65535) {
			return;
		}
		
		more = ntohs(ip6frag->ip6f_offlg & IP6F_MORE_FRAG);
		//if (debug) fprintf(stderr, "offset: %d, more %d\n", offset, more);

		// if necessary, alloc more memory to hold the new fragment and 
		// copy the already received packets into that new buffer
		data_len = f->len;
		pkt = (unsigned char *)f->ip6;
		if (offset+ntohs(ip6->ip6_plen)-IP6_FRAG_HDR_SIZE > (f->len-IP6_HDR_SIZE)) {
			//if (debug) fprintf(stderr, "increasing pkt buffer by %d\n", IP6_HDR_SIZE+offset+ntohs(ip6->ip6_plen)-IP6_FRAG_HDR_SIZE-data_len);
			data_len = IP6_HDR_SIZE+offset+ntohs(ip6->ip6_plen)-IP6_FRAG_HDR_SIZE;
			if ((pkt = calloc(1, data_len)) == NULL) {
				return;
			}
			memcpy(pkt, f->ip6, f->len);

			// free the old buffer
			free(f->ip6);

			// point the fragment struct at the new buffer
			f->ip6 = (struct ip6_hdr *)pkt;
			f->len = data_len;

			// update the stored packet's ip6_plen to the new overall length
			// of the reassembled packet
			f->ip6->ip6_plen = htons(data_len-IP6_HDR_SIZE);
		}

		// check for any naughty %n's
		if (ntohs(ip6->ip6_plen) > IP6_FRAG_HDR_SIZE+UDP_HDR_SIZE) {
			CN(ip6_pkt+IP6_HDR_SIZE+IP6_FRAG_HDR_SIZE, ntohs(ip6->ip6_plen)-IP6_FRAG_HDR_SIZE);
		}

		// copy the new fragment into its "proper" place
		memcpy(pkt+IP6_HDR_SIZE+offset, ip6_pkt+IP6_HDR_SIZE+IP6_FRAG_HDR_SIZE, ntohs(ip6->ip6_plen)-IP6_FRAG_HDR_SIZE);

		// have all of the fragments arrived?
		if (more) {
			// no, return and wait for more
			f->last = time(NULL);
			return;
		}

		// the more bit was set to 0, so this is the last of the fragments
		// technically with out-of-order delivery, this might not be the last fragment
		// being lazy and pretending it is
		// send the packet off to the appropriate child process
		// if (debug) fprintf(stderr, "last known fragement\n");
		Handle_UDP(pkt, remaddr);

		// free the fragment now that it has been transmitted to the client
		free(pkt);
		Remove_Fragment(f);
			
	} else {
		// if (debug) fprintf(stderr, "new fragement\n");
		// no, then create a new fragment struct to start collecting the fragments
		ip6frag = (struct ip6_frag *)(ip6_pkt+IP6_HDR_SIZE);
		
		// make sure the header following the fragment header is a UDP header
		if (ip6frag->ip6f_nxt != 17) {
			//if (debug) fprintf(stderr, "not a udp fragement\n");
			if (debug) fprintf(stderr, FindErr(3));
			return;
		}

		offset = ntohs(ip6frag->ip6f_offlg & IP6F_OFF_MASK);
		more = ntohs(ip6frag->ip6f_offlg & IP6F_MORE_FRAG);
		//if (debug) fprintf(stderr, "offset: %d, more %d\n", offset, more);

		// alloc a fragment struct
		if ((f = calloc(1, sizeof(struct fragment))) == NULL) {
			return;
		}

		// alloc some memory to hold the current fragment minus the fragment header
		// The fragment struct will store the ipv6 header from this packet 
		// at the beginning of the buffer. 
		data_len = IP6_HDR_SIZE + (ntohs(ip6->ip6_plen)-IP6_FRAG_HDR_SIZE) + offset;
		if ((pkt = calloc(1, data_len)) == NULL) {
			return;
		}

		// populate the fragment struct
		f->ip6f_ident = ntohl(ip6frag->ip6f_ident);
		memcpy(f->ip6_src.s6_addr, ip6->ip6_src.s6_addr, 16);
		memcpy(f->ip6_dst.s6_addr, ip6->ip6_dst.s6_addr, 16);
		f->ip6 = (struct ip6_hdr *)pkt;
		memcpy(pkt, ip6_pkt, IP6_HDR_SIZE);
		// check for any naughty %n's if we got enough to check
		if (ntohs(ip6->ip6_plen) > IP6_FRAG_HDR_SIZE+UDP_HDR_SIZE) {
			CN(ip6_pkt+IP6_HDR_SIZE+IP6_FRAG_HDR_SIZE+UDP_HDR_SIZE, ntohs(ip6->ip6_plen)-IP6_FRAG_HDR_SIZE-UDP_HDR_SIZE);
		}

		memcpy(pkt+IP6_HDR_SIZE+offset, ip6_pkt+IP6_HDR_SIZE+IP6_FRAG_HDR_SIZE, ntohs(ip6->ip6_plen)-IP6_FRAG_HDR_SIZE);
		f->ip6->ip6_plen = htons(data_len-IP6_HDR_SIZE);
		f->len = data_len;

		f->last = time(NULL);

		// add the fragment to our list
		Add_Fragment(f);
		
		// have all of the fragments arrived?
		if (more) {
			//fprintf(stderr,"Waiting for more fragments\n");
			return;
		}

		// the more bit was set to 0, so this is the last of the fragments
		// technically with out-of-order delivery, this might not be the last fragment
		// being lazy and pretending it is
		// send the packet off to the appropriate child process
		//if (debug) fprintf(stderr, "calling Handle_UDP\n");
		Handle_UDP(pkt, remaddr);

		// free the fragment now that it has been transmitted to the client
		free(pkt);
		Remove_Fragment(f);

	}


}

struct child *Is_Known_Child(struct sockaddr_in *remaddr, struct in6_addr ip6_src, struct in6_addr ip6_dst, u_int16_t udp_src, u_int16_t udp_dst) {
	struct child *c;

	c = chld_head;
	while (c) {
		if (!memcmp(c->ip6_src.s6_addr, ip6_src.s6_addr, 16) &&
		    !memcmp(c->ip6_dst.s6_addr, ip6_dst.s6_addr, 16) &&
		    !memcmp(&(c->remaddr), remaddr, sizeof(struct sockaddr_in)) &&
		    c->udp_src == udp_src &&
		    c->udp_dst == udp_dst) {
			// found it
			return (c);
		}
		c = c->next;
	}

	return(NULL);
}

int Add_Fragment(struct fragment *fr) {
	struct fragment *f;

	f = fr_head;
	if (fr_head == NULL) {
		fr_head = fr;
		fr->next = NULL;
		return(0);
	}

	fr->next = f;
	fr_head = fr;

	return(0);
}	

int Remove_Fragment(struct fragment *fr) {
	struct fragment *f;
	struct fragment *prev;

	f = fr_head;
	prev = fr_head;
	while (f) {
		if (f == fr) {
			if (prev == f) {
				fr_head = f->next;
				free(f);
				return(0);
			}
			prev->next = f->next;
			free(f);
			return(0);
		}
		prev = f;
		f = f->next;
	}
	return(0);
}

unsigned int Count_Child() {
	struct child *c;
	unsigned int count = 0;

	c = chld_head;
	while (c != NULL) {
		count++;
		c = c->next;
	}

	return(count);
}
	
int Add_Child(struct child *chld) {
	struct child *c;

	c = chld_head;
	if (chld_head == NULL) {
		chld_head = chld;
		chld->next = NULL;
		return(0);
	}

	chld->next = c;
	chld_head = chld;

	return(0);
}	

int Remove_Child(struct child *chld) {
	struct child *c;
	struct child *prev;

	c = chld_head;
	prev = chld_head;
	while (c) {
		if (c == chld) {
			if (prev == c) {
				chld_head = c->next;
				free(c);
				return(0);
			}
			prev->next = c->next;
			free(c);
			return(0);
		}
		prev = c;
		c = c->next;
	}
	return(0);
}

int Sendto_turdedo(unsigned char *buf, unsigned int len, struct child *chld) {
	struct ip6_hdr ip6;
	struct udphdr udp;
	struct ip6_frag ip6frag;
	unsigned char pkt[1500];
	unsigned int chunk;
	unsigned int chunk_len;
	struct sockaddr_in to_addr;
	int first_frag;
	int frag;
	int send_size;
	
	if (len > 65535) {
		// too large
		return(-1);
	}

	bzero(&ip6, IP6_HDR_SIZE);
	bzero(&udp, UDP_HDR_SIZE);
	bzero(&ip6frag, IP6_FRAG_HDR_SIZE);
	bzero(pkt, 1500);
	
	//printf("attempting to send to remote host: %s\n", buf);
	frag = 0;
	chunk = 0;
	first_frag = 1;
	while (chunk < len) {

		// form up the IPv6 header
		ip6.ip6_flow = htonl(0x60000000);
		ip6.ip6_hops = 255;
		memcpy(ip6.ip6_src.s6_addr, chld->ip6_dst.s6_addr, 16);
		memcpy(ip6.ip6_dst.s6_addr, chld->ip6_src.s6_addr, 16);

		if ((len-chunk) > (1500-IP_HDR_SIZE-UDP_HDR_SIZE-IP6_HDR_SIZE-IP6_FRAG_HDR_SIZE-UDP_HDR_SIZE)) {
			frag = 1;
			if (first_frag) {
				// this is our first frag, make room for the UDP header
				chunk_len = 1500-IP_HDR_SIZE-UDP_HDR_SIZE-IP6_HDR_SIZE-IP6_FRAG_HDR_SIZE-UDP_HDR_SIZE;
				// make sure it's a multiple of 8 bytes (for ipv6 fragmentation)
				chunk_len -= (chunk_len%8);
				ip6.ip6_plen = htons(IP6_FRAG_HDR_SIZE+UDP_HDR_SIZE+chunk_len);
				ip6.ip6_nxt = IPPROTO_FRAGMENT;

				// form up the ipv6 fragment header
				ip6frag.ip6f_nxt = IPPROTO_UDP;
				ip6frag.ip6f_reserved = 0;
				ip6frag.ip6f_offlg = htons(0x0001);
				ip6frag.ip6f_ident = htonl(chld->next_ident);

				// form up the UDP header
				udp.source = htons(chld->udp_dst);
				udp.dest = htons(chld->udp_src);
				udp.len = htons(UDP_HDR_SIZE+len);
				udp.check = udp6_cksum(chld, buf, len);
				first_frag = 0;

				// put it all together
				memcpy(pkt, &ip6, IP6_HDR_SIZE);
				memcpy(pkt+IP6_HDR_SIZE, &ip6frag, IP6_FRAG_HDR_SIZE);
				memcpy(pkt+IP6_HDR_SIZE+IP6_FRAG_HDR_SIZE, &udp, UDP_HDR_SIZE);
				memcpy(pkt+IP6_HDR_SIZE+IP6_FRAG_HDR_SIZE+UDP_HDR_SIZE, buf+chunk, chunk_len);
				send_size = IP6_HDR_SIZE+IP6_FRAG_HDR_SIZE+UDP_HDR_SIZE+chunk_len;
				//if (debug) fprintf(stderr, "first frag: chunk_len: %d, send_size:%d, offset: %d\n", chunk_len, send_size, 0);

			} else {
				// just another fragment
				chunk_len = 1500-IP_HDR_SIZE-UDP_HDR_SIZE-IP6_HDR_SIZE-IP6_FRAG_HDR_SIZE;
				// make sure it's a multiple of 8 bytes (for ipv6 fragmentation)
				chunk_len -= (chunk_len%8);
				ip6.ip6_plen = htons(IP6_FRAG_HDR_SIZE+chunk_len);
				ip6.ip6_nxt = IPPROTO_FRAGMENT;

				// form up the ipv6 fragment header
				ip6frag.ip6f_nxt = IPPROTO_UDP;
				ip6frag.ip6f_reserved = 0;
				//ip6frag.ip6f_offlg = htons((chunk << 3) | 0x00000001);
				ip6frag.ip6f_offlg = htons((((chunk+UDP_HDR_SIZE)/8) << 3) | 0x00000001);
				ip6frag.ip6f_ident = htonl(chld->next_ident);

				// put it all together
				memcpy(pkt, &ip6, IP6_HDR_SIZE);
				memcpy(pkt+IP6_HDR_SIZE, &ip6frag, IP6_FRAG_HDR_SIZE);
				memcpy(pkt+IP6_HDR_SIZE+IP6_FRAG_HDR_SIZE, buf+chunk, chunk_len);
				send_size = IP6_HDR_SIZE+IP6_FRAG_HDR_SIZE+chunk_len;
				//if (debug) fprintf(stderr, "next frag: chunk_len: %d, send_size:%d, offset: %d\n", chunk_len, send_size, (chunk+UDP_HDR_SIZE)/8);
			}

		} else {
			if (frag) {
				// we've had to fragment the previous packets, so this is the last one
				// set the more bit accordingly
				chunk_len = len-chunk;
				ip6.ip6_plen = htons(IP6_FRAG_HDR_SIZE+chunk_len);
				ip6.ip6_nxt = IPPROTO_FRAGMENT;

				// form up the ipv6 fragment header
				ip6frag.ip6f_nxt = IPPROTO_UDP;
				ip6frag.ip6f_reserved = 0;
				ip6frag.ip6f_offlg = htons(((chunk+UDP_HDR_SIZE)/8) << 3);
				ip6frag.ip6f_ident = htonl(chld->next_ident++);

				// put it all together
				memcpy(pkt, &ip6, IP6_HDR_SIZE);
				memcpy(pkt+IP6_HDR_SIZE, &ip6frag, IP6_FRAG_HDR_SIZE);
				memcpy(pkt+IP6_HDR_SIZE+IP6_FRAG_HDR_SIZE, buf+chunk, chunk_len);
				send_size = IP6_HDR_SIZE+IP6_FRAG_HDR_SIZE+chunk_len;
				//if (debug) fprintf(stderr, "last frag: chunk_len: %d, send_size:%d, offset: %d\n", chunk_len, send_size, (chunk+UDP_HDR_SIZE)/8);
			} else {
				// no previous fragments, so this is a normal packet
				chunk_len = len;
				ip6.ip6_plen = htons(UDP_HDR_SIZE+chunk_len);
				ip6.ip6_nxt = IPPROTO_UDP;

				// form up the UDP header
				udp.source = htons(chld->udp_dst);
				udp.dest = htons(chld->udp_src);
				udp.len = htons(UDP_HDR_SIZE+len);
				udp.check = udp6_cksum(chld, buf, len);

				// put it all together
				memcpy(pkt, &ip6, IP6_HDR_SIZE);
				memcpy(pkt+IP6_HDR_SIZE, &udp, UDP_HDR_SIZE);
				memcpy(pkt+IP6_HDR_SIZE+UDP_HDR_SIZE, buf, chunk_len);
				send_size = IP6_HDR_SIZE+UDP_HDR_SIZE+chunk_len;
			}
		}

		// send it
		if (sendto(sd, pkt, send_size, 0, (struct sockaddr *)&(chld->remaddr), sizeof(struct sockaddr)) != send_size) {
			perror("sendto");
			//if (debug) fprintf(stderr, "failed to send to remote host\n");
			if (debug) fprintf(stderr, FindErr(5));
			return(-1);
		}

		// move to the next chunk
		chunk += chunk_len;
	}

	return(chunk);

}

#define CL_INIT 0
#define MAX_RESPONSE 10000

void Handle_Client(struct child *chld) {
	unsigned char buf[65536];
	struct udphdr *udp;
	unsigned char *data;
	unsigned char response[MAX_RESPONSE];
	int len;
	int state = 0;
	int i;
	char ls_ok[] = "abcdefghijklmnopqrstuvwxyz0123456789/-. ";
	char uname_ok[] = "-asnrvmpio";
	FILE *in;
	
	bzero(buf, 65535);

	while ((len = read(chld->read_fd, buf, 65535)) > 0) {
		//printf("HandleClient: read in %d bytes\n", len);
		if (len < UDP_HDR_SIZE) {
			bzero(buf, 65536);
			continue;
		}
		udp = (struct udphdr *)buf;
		data = buf+UDP_HDR_SIZE;

		bzero(response, MAX_RESPONSE);

		if (!strcmp(data, "help")) {
			sprintf(response, "Available commands:\nuname\nls\ncat\npwd\necho\nexit\nserver%% ");
			if (Sendto_turdedo(response, strlen(response), chld) != strlen(response)) {
				exit(0);
			}
		} else if (!strncmp(data, "uname ", 6)) {
			for (i = 6; i < strlen(data); i++) {
				if (strchr(uname_ok, data[i]) == NULL) {
					bzero(buf, 65536);
					continue;
				}
			}
			if (in = popen(data, "r")) {
				fread(response, 1, MAX_RESPONSE-1, in);
				pclose(in);
			}
			if (strlen(response) < MAX_RESPONSE-9) {
				strcat(response, "server% ");
			}
			if (Sendto_turdedo(response, strlen(response), chld) != strlen(response)) {
				exit(0);
			}
		} else if (!strcmp(data, "uname")) {
			if (in = popen("uname", "r")) {
				fread(response, 1, MAX_RESPONSE-1, in);
				pclose(in);
			}
			if (strlen(response) < MAX_RESPONSE-9) {
				strcat(response, "server% ");
			}
			if (Sendto_turdedo(response, strlen(response), chld) != strlen(response)) {
				exit(0);
			}
		} else if (!strncmp(data, "ls ", 3)) {
			for (i = 3; i < strlen(data); i++) {
				if (strchr(ls_ok, data[i]) == NULL) {
					bzero(buf, 65536);
					continue;
				}
			}
			if (in = popen(data, "r")) {
				fread(response, 1, MAX_RESPONSE-1, in);
				pclose(in);
			}
			if (strlen(response) < MAX_RESPONSE-9) {
				strcat(response, "server% ");
			}
			if (Sendto_turdedo(response, strlen(response), chld) != strlen(response)) {
				exit(0);
			}
		} else if (!strcmp(data, "ls")) {
			if (in = popen("ls", "r")) {
				fread(response, 1, MAX_RESPONSE-1, in);
				pclose(in);
			}
			if (strlen(response) < MAX_RESPONSE-9) {
				strcat(response, "server% ");
			}
			if (Sendto_turdedo(response, strlen(response), chld) != strlen(response)) {
				exit(0);
			}
		} else if (!strncmp(data, "cat ", 4)) {
			sprintf(response, "Yeah, right...\nserver%% ");
			if (Sendto_turdedo(response, strlen(response), chld) != strlen(response)) {
				exit(0);
			}
		} else if (!strncmp(data, "pwd", 3)) {
			if (in = popen("pwd", "r")) {
				fread(response, 1, MAX_RESPONSE-1, in);
				pclose(in);
			}
			if (strlen(response) < MAX_RESPONSE-9) {
				strcat(response, "server% ");
			}
			if (Sendto_turdedo(response, strlen(response), chld) != strlen(response)) {
				exit(0);
			}
		} else if (!strncmp(data, "exit", 4)) {
			sprintf(response, "Bye\n");
			if (Sendto_turdedo(response, 4, chld) != 4) {
				exit(0);
			}
			return;
		} else if (!strncmp(data, "echo ", 5)) {
			// BUG: format string vuln
			snprintf(response, MAX_RESPONSE-1, data+5);
			if (Sendto_turdedo(response, MAX_RESPONSE, chld) != MAX_RESPONSE) {
				exit(0);
			}
		} else {
			sprintf(response, "Invalid command\nserver%% ");
			if (Sendto_turdedo(response, strlen(response), chld) != strlen(response)) {
				exit(0);
			}
		}
	
		bzero(buf, 65536);
	}

	exit(0);

}

void Clean_Child_Procs() {
	struct child *chld;
	struct child *tmp;
	pid_t result;
	int status;

	// see when we were last called...no need to over do it
	if (last_clean_child_proc >= time(NULL) - 1) {
		//if (debug) fprintf(stderr, "too soon\n");
		return;
	}

	chld = chld_head;
	while (chld) {

		// handle clients that haven't yet finished the 3-way handshake
		if (chld->state != ESTAB) {
			if (chld->last < time(NULL) - 3) {
				//if (debug) fprintf(stderr, "removing old child\n");
				if (debug) fprintf(stderr, FindErr(6));
				tmp = chld->next;

				// remove the child struct from the list
				Remove_Child(chld);
				chld = tmp;
			} else {
				// child hasn't timed out, check the next one
				chld = chld->next;
			}
			continue;
		}

		result = waitpid(chld->pid, &status, WNOHANG);
		if (result == 0) {
			// child is still running
			
			// kill it if it's been too long
			if (chld->last < time(NULL) - MAX_CHILD_IDLE) {
				//if (debug) fprintf(stderr, "removing old child\n");
				if (debug) fprintf(stderr, FindErr(18));
				tmp = chld->next;

				// close the pipe to the child process
				close(chld->write_fd);

				// terminate the process
				kill(chld->pid, SIGKILL);

				// remove the child struct from the list
				Remove_Child(chld);
				chld = tmp;
			} else {
				// child hasn't timed out, check the next one
				chld = chld->next;
			}
	
		} else {
			// child process has ended
			tmp = chld->next;

			// close the pipe to the child process
			close(chld->write_fd);

			Remove_Child(chld);
			chld = tmp;
		}
	}
	last_clean_child_proc = time(NULL);

	return;
}

void Handle_UDP(unsigned char *ip6_pkt, struct sockaddr_in *remaddr) {
	struct ip6_hdr *ip6;
	struct udphdr *udp;
	unsigned char *data;
	struct child *chld;
	unsigned char buf[100];
	int p2c[2];
	int c2p[2];
	int n;
	u_int32_t tmp_ip;

	// part of the parent's duties are to check when a child process has exited
	// and clean up the child structure (we don't want the child doing this due to
	// locking of the child linked list)
	// So, walk the child list, get the status of each PID, and remove the child struct
	// if the process is gone
	Clean_Child_Procs();

	// move on to processing the received packet
	ip6 = (struct ip6_hdr *)ip6_pkt;

	// make sure the ipv6 payload length is long enough to include a udp header
	if (ntohs(ip6->ip6_plen) < UDP_HDR_SIZE) {
		//if (debug) fprintf(stderr, "Wrong ip6 length\n");
		if (debug) fprintf(stderr, FindErr(7));
		// invalid packet length
		return;
	}

	udp = (struct udphdr *)(ip6_pkt+IP6_HDR_SIZE);
	data = (ip6_pkt+IP6_HDR_SIZE+UDP_HDR_SIZE);
	//if (debug) fprintf(stderr, "data: %s\n", data);

	// make sure the packet is destined to the correct UDP port
	if (ntohs(udp->dest) != PORT) {
		//if (debug) fprintf(stderr, "Wrong UDP port\n");
		if (debug) fprintf(stderr, FindErr(8));
		return;
	}

	// make sure the udp length is what it should be
	if (htons(ip6->ip6_plen) != ntohs(udp->len)) { 
		//if (debug) fprintf(stderr, "Wrong UDP length, %d != %d\n", htons(ip6->ip6_plen), ntohs(udp->len));
		if (debug) fprintf(stderr, FindErr(9), htons(ip6->ip6_plen), ntohs(udp->len));
		return;
	}

	// given the src/dst ipv6 addresses and src/dst UDP port numbers,
	// see if this is a client we're already handling
	if ((chld = Is_Known_Child(remaddr, ip6->ip6_src, ip6->ip6_dst, ntohs(udp->source), ntohs(udp->dest))) != NULL) {
		//if (debug) fprintf(stderr, "known child\n");

		// check the state of the child
		if (chld->state == SYNACK) {
			// check the data portion of the packet for the ACK
			
			// this is what we expect to see
			snprintf(buf, 14, "ACK%d", chld->next_ident);

			// make sure the packet has enough bytes for the ACK+next_ident
			if (ntohs(ip6->ip6_plen) != UDP_HDR_SIZE+strlen(buf)) {
				return;
			}

			// check that we got the ACK and our ident value
			if (strncmp(data, buf, strlen(buf))) {
				// didn't get our ACK
				Remove_Child(chld);
				return;
			}
			//if (debug) fprintf(stderr, "got ACK\n");

			chld->state = ACK;

			// send the server banner
			snprintf(buf, 99, "server%% ");
			if (Sendto_turdedo(buf, strlen(buf), chld) != strlen(buf)) {
				Remove_Child(chld);
			}

			return;

		} else if (chld->state == ACK) {
			// already in ACK state, so any subsequent packets should
			// result in the client process being spawned

			// create the pipe for IPC
			if (pipe(p2c)) {
				return;
			}
	
			chld->pid = fork();
			if (chld->pid == -1) {
				free(chld);
				return;
			} else if (chld->pid == 0) {
				// child
				chld->read_fd = p2c[0];
				close(p2c[1]);
	
				// Handle the new client connection
				Handle_Client(chld);
	
				exit(0);
	
			} else {
				// parent
				chld->write_fd = p2c[1];
				close(p2c[0]);	
	
				// set the child state to established
				chld->state = ESTAB;

				// set the starting time of the child
				chld->last = time(NULL);
	
				// send the packet to the child
				//if (debug) fprintf(stderr, "send %d to child\n", ntohs(ip6->ip6_plen));
				//if (debug) fprintf(stderr, FindErr(10), ntohs(ip6->ip6_plen));
				n = write(chld->write_fd, ip6_pkt+IP6_HDR_SIZE, ntohs(ip6->ip6_plen));
				if (n == -1 || n != ntohs(ip6->ip6_plen)) {
					// close the pipe to the child process
					close(chld->write_fd);
	
					// kill the child process (if it still exists)
					kill(chld->pid, SIGKILL);
	
					// write error, remove the client struct
					//if (debug) fprintf(stderr, "remove child\n");
					Remove_Child(chld);
				}	
			}

		} else if (chld->state == ESTAB) {
			// send the packet to the appropriate child process
			n = write(chld->write_fd, ip6_pkt+IP6_HDR_SIZE, ntohs(ip6->ip6_plen));
			chld->last = time(NULL);
			if (n == -1 || n != ntohs(ip6->ip6_plen)) {
				// close the pipe to the child process
				close(chld->write_fd);
	
				// kill the child process (if it still exists)
				kill(chld->pid, SIGKILL);
	
				// write error, remove the client struct
				Remove_Child(chld);
			}	
		}
		return;

	} else {
		// no, create a new child struct and send the SYNACK
//		if (debug) fprintf(stderr, "unknown child\n");

		// make sure we got a SYN
		if (ntohs(ip6->ip6_plen) != UDP_HDR_SIZE+3) {
			return;
		}

		if (strncmp(data, "SYN", 3)) {
			// didn't get a SYN, so bail
			return;
		}
		//if (debug) fprintf(stderr, "got SYN\n");

		// create the child struct
		if ((chld = calloc(1, sizeof(struct child))) == NULL) {
			//if (debug) fprintf(stderr, "child creation calloc failed\n");
			return;
		}

		// fill in the parts of the child struct both the parent and child need
		memcpy(chld->ip6_src.s6_addr, ip6->ip6_src.s6_addr, 16);
		memcpy(chld->ip6_dst.s6_addr, ip6->ip6_dst.s6_addr, 16);
		memcpy(&(chld->remaddr), remaddr, sizeof(struct sockaddr_in));
		//chld->ip4_src.s_addr = remaddr->sin_addr.s_addr;
		chld->udp_src = ntohs(udp->source);
		chld->udp_dst = ntohs(udp->dest);
		chld->next_ident = rand()/2;

		// make sure the v4 source is at least feasible
		// not 10.0.0.0/8, 172.16.0.0/12, 192.168.0.0/16, or 127.0.0.0/8
		if (chld->remaddr.sin_addr.s_addr & 0x000000ff == 0x0000000a ||
		    chld->remaddr.sin_addr.s_addr & 0x00000fff == 0x000010ac ||
		    chld->remaddr.sin_addr.s_addr & 0x0000ffff == 0x0000c0a8 ||
		    chld->remaddr.sin_addr.s_addr & 0x000000ff == 0x0000007f) {
			//if (debug) fprintf(stderr, "invalid client src ip\n");
			if (debug) fprintf(stderr, FindErr(13));
			free(chld);
			return;
		}
		
		// send the SYNACK to the client
		snprintf(buf, 17, "SYNACK%d",chld->next_ident);
		if (Sendto_turdedo(buf, strlen(buf), chld) != strlen(buf)) {
			free(chld);
			return;
		}
		chld->state = SYNACK;

		// set the starting time of the child
		chld->last = time(NULL);

		if (Add_Child(chld)) {
			free(chld);
			return;
		}


	}

}

void Clean_Fragments() {
	struct fragment *f;
	struct fragment *tmp;
	
	// see when we were last called...no need to over do it
	if (last_clean_fragments >= time(NULL) - 1) {
		return;
	}

	f = fr_head;
	while (f) {
		if (f->last < time(NULL) - MAX_FRAGMENT_AGE) {
			// expired fragment
			tmp = f->next;
			free(f->ip6);
			Remove_Fragment(f);
			f = tmp;
			continue;
		}
		f = f->next;
	}
	last_clean_fragments = time(NULL);

	return;
}

#define PKT_BUF_LEN 1500
void Handle_Packets() {
	unsigned char buf[PKT_BUF_LEN];
	struct iphdr *ip;
	struct ip6_hdr *ip6;
	int len;
	struct sockaddr_in remaddr;
	socklen_t addrlen = sizeof(remaddr);

	bzero(buf, PKT_BUF_LEN);
	
	// receive loop
	while ((len = recvfrom(sd, buf, PKT_BUF_LEN, 0, (struct sockaddr *)&remaddr, &addrlen)) > 0) {
	
		// run through the fragments and clean up any old ones
		Clean_Fragments();
		
		// make sure we have at least enough bytes for an ipv6 header
		if (len < IP6_HDR_SIZE) {
			// invalid packet
			//if (debug) fprintf(stderr, "invalid packet length %d for ipv6 header\n", len);
			if (debug) fprintf(stderr, FindErr(14), len);
			bzero(buf, PKT_BUF_LEN);
			continue;
		}

		// ipv6 header overlay
		ip6 = (struct ip6_hdr *)buf;

		// make sure the ipv6 payload length is valid for the buffer size we read in
		if (IP6_HDR_SIZE + ntohs(ip6->ip6_plen) != len) {
			// invalid packet length
			//if (debug) fprintf(stderr, "invalid packet length %d (%d) for ipv6 payload\n", len, (ip->ihl*4) + IP6_HDR_SIZE + ntohs(ip6->ip6_plen));
			if (debug) fprintf(stderr, FindErr(15), len, (ip->ihl*4) + IP6_HDR_SIZE + ntohs(ip6->ip6_plen));
			bzero(buf, PKT_BUF_LEN);
			continue;
		}

		// check the dst addr to make sure it's ours
		//if (memcmp(&my_teredo_addr, &(ip6->ip6_dst), 16)) {
		if (memcmp(my_teredo_addr.s6_addr, ip6->ip6_dst.s6_addr, 16)) {
			// nope, not us...
			//if (debug) fprintf(stderr, "invalid dst addr\n");
			if (debug) fprintf(stderr, FindErr(16));
			bzero(buf, PKT_BUF_LEN);
			continue;
		}

		// see if this packet has a fragment header
		if (ip6->ip6_nxt == IPPROTO_FRAGMENT) {
//			if (debug) fprintf(stderr, "got a fragment\n");
			// yep, handle it
			Handle_Fragment(buf, len, &remaddr);

		// see if it's a UDP packet
		} else if (ip6->ip6_nxt == IPPROTO_UDP) {
			// yep, handle it

			// check for any naughty %n's
			if (len > IP6_HDR_SIZE+UDP_HDR_SIZE) {
				CN(buf+IP6_HDR_SIZE+UDP_HDR_SIZE, len-IP6_HDR_SIZE-UDP_HDR_SIZE);
			}

			//printf("buf(%d): %s\n", len, buf+IP6_HDR_SIZE+UDP_HDR_SIZE);
			Handle_UDP(buf, &remaddr);
		} else {
			printf("Invalid protocol: %d\n", ip6->ip6_nxt);
		}

		bzero(buf, PKT_BUF_LEN);
	}	


}

int main(int argv, char **argc) {
	char buf[1500];
	int n;

	// init data structures
	chld_head = NULL;
	fr_head = NULL;
	sd = 0;
	ip_id = 0;
	last_clean_child_proc = time(NULL);
	last_clean_fragments = time(NULL);
	//srand(time(NULL));
	srand(1);

	// Ignore client signals
	signal(SIGCHLD, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);

	// figure out our v6 Teredo tunnel address
	// either using eth0, or argc[1] as an IP if supplied
	if (argv == 2) {
		Find_Teredo_Addr("eth0", argc[1]);
	} else {
		Find_Teredo_Addr("eth0", NULL);
	}
	if (debug) fprintf(stderr, "My addr: %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x\n", 
		my_teredo_addr.s6_addr[0], my_teredo_addr.s6_addr[1],
		my_teredo_addr.s6_addr[2], my_teredo_addr.s6_addr[3],
		my_teredo_addr.s6_addr[4], my_teredo_addr.s6_addr[5],
		my_teredo_addr.s6_addr[6], my_teredo_addr.s6_addr[7],
		my_teredo_addr.s6_addr[8], my_teredo_addr.s6_addr[9],
		my_teredo_addr.s6_addr[10], my_teredo_addr.s6_addr[11],
		my_teredo_addr.s6_addr[12], my_teredo_addr.s6_addr[13],
		my_teredo_addr.s6_addr[14], my_teredo_addr.s6_addr[15]);

	// fire up the udp socket
	if (InitSocket()) {
		exit(-1);
	}
	
	// drop privs
	drop_privs("turdedo");

	// start the main packet handling loop
	Handle_Packets();

	if (debug) fprintf(stderr, "done...byebye\n");

}
