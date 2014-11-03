#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/time.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include "bbgp.h"

#define BGP_PORT 179
#define MYAS 31337
#define OUR_HOLD_TIME 9

//int debug = 1;

struct bgp_peer peer;
char my_prefix[] = "10.1.1.1";
struct bgp_rib rib[MAX_RIB_SIZE];
int rib_size = 0;

int print_rib() {
	int i;
	char buf[INET_ADDRSTRLEN+1];

	for (i = 0; i < rib_size; i++) {
		if (!rib[i].valid) {
			continue;
		}
		fprintf(stderr, "rib entry: %d\n", i);
		inet_ntop(AF_INET, &(rib[i].prefix), buf, INET_ADDRSTRLEN);
		fprintf(stderr, "\tprefix:   %s/%d\n", buf, rib[i].bits);
		fprintf(stderr, "\torigin:   %d\n", rib[i].origin);
		fprintf(stderr, "\tas_path:  %s\n", rib[i].as_path);
		inet_ntop(AF_INET, &(rib[i].next_hop), buf, INET_ADDRSTRLEN);
		fprintf(stderr, "\tnext_hop: %s\n", buf);
	}

	return(0);
}

int bgp_send_notification(u_int8_t major, u_int8_t minor) {
	unsigned char buf[BGP_NOTIFICATION_SIZE];
	struct bgp_notification *bgpn_hdr;

	bgpn_hdr = (struct bgp_notification*)buf;

	// form up the notification header
	memset(bgpn_hdr, 0xFF, 16);
	bgpn_hdr->bgpn_len = htons(BGP_NOTIFICATION_SIZE);
	bgpn_hdr->bgpn_type = BGP_NOTIFICATION;
	bgpn_hdr->bgpn_major = major;	
	bgpn_hdr->bgpn_minor = minor;	

	// send it
	if (send(STDOUT_FILENO, buf, BGP_NOTIFICATION_SIZE, 0) != BGP_NOTIFICATION_SIZE) {
		return(-1);
	}
//	if (debug) fprintf(stderr, "Sent notification (%d,%d)\n", major, minor);

}

int bgp_send_keepalive() {
	unsigned char buf[BGP_SIZE];
	struct bgp *bgp_hdr;

	bgp_hdr = (struct bgp *)buf;

	// form up the bgp header
	memset(bgp_hdr, 0xFF, 16);
	bgp_hdr->bgp_len = htons(BGP_SIZE);
	bgp_hdr->bgp_type = BGP_KEEPALIVE;

	// send it
	if (send(STDOUT_FILENO, buf, BGP_SIZE, 0) != BGP_SIZE) {
		return(-1);
	}
//	if (debug) fprintf(stderr, "Sent keepalive\n");

	return(0);

}

int del_route(u_int32_t prefix, unsigned char prefix_bits) {
	int i;

	// look for the entry
	for (i = 1; i < rib_size; i++) {
		if (rib[i].prefix.s_addr == prefix && rib[i].bits == prefix_bits) {
			rib[i].valid = 0;
		}
	}

	return(0);
}

int add_route(u_int32_t prefix, unsigned char prefix_bits, unsigned int origin, char *as_path, u_int32_t next_hop) {
	int i;
	int first_available = -1;

	// see if this prefix already exists in the table
	for (i = 1; i < rib_size; i++) {
		// also find the first invalid rib entry in case we need to insert a new one
		if (rib[i].valid == 0) {
			first_available = i;
		}

		if (rib[i].prefix.s_addr == prefix && rib[i].bits == prefix_bits &&
		    rib[i].origin == origin && !strcmp(rib[i].as_path, as_path) &&
		    rib[i].next_hop.s_addr == next_hop) {
			if (rib[i].valid == 1) {
				return(0);
			}
		}
	}

	// see if we found an available entry to re-use
	if (first_available == -1) {
		// nope, just add one to the end
		i = rib_size;
	} else {
		i = first_available;
	}

	// not found, add it
	if (i < MAX_RIB_SIZE) {
		// new, zero the rib entry before setting values
		bzero(&(rib[i]), sizeof(struct bgp_rib));
	    
		rib[i].prefix.s_addr = prefix;
		rib[i].bits = prefix_bits;
		rib[i].origin = origin;
		bzero(rib[i].as_path, MAX_AS_PATH_LENGTH);
		strncpy(rib[i].as_path, as_path, MAX_AS_PATH_LENGTH-1);
		rib[i].next_hop.s_addr = next_hop;
		rib[i].valid = 1;
		rib_size++;
	} else {
		// exceeded RIB capacity, close the connection
//		if (debug) fprintf(stderr,"reached max RIB size\n");
		bgp_send_notification(BGP_NOTIFY_MAJOR_UPDATE, BGP_NOTIFY_MINOR_CEASE_MAXPRFX);
		return(-1);
	}

	return(0);
}

int handle_bgp_open() {
	unsigned char buf[BGP_OPEN_SIZE+BGP_OPT_SIZE+BGP_CAPABILITIES_SIZE+BGP_CAPABILITIES_MP_SIZE];
	struct bgp_open *open_hdr;
	int n;
	unsigned char trash[1];
	unsigned char cap[256];
	unsigned char opt[256];
	unsigned char varopt[2];
	unsigned char mpcap[256];
	struct bgp_opt *bgp_opt_hdr;
	struct bgp_capabilities *bgpc_hdr;
	struct bgp_cap_mp *bgpc_mp_hdr;
	unsigned int total_size;
	int i;
	int v4u = 0;
	struct itimerval new_value;

	// try to read in the open header
	if ((n = read(STDIN_FILENO, buf, BGP_OPEN_SIZE)) == -1) {
		return(-1);
	}

	// make sure we got enough bytes for the open header
	if (n != BGP_OPEN_SIZE) {
//		if (debug) fprintf(stderr, "Invalid open size: %d\n", n);
		// nope, bye-bye
		bgp_send_notification(BGP_NOTIFY_MAJOR_OPEN, 0);
		return(-1);
	}

	// got enough, check the basics

	// make sure we were sent an open packet
	open_hdr = (struct bgp_open *)buf;
	if (open_hdr->bgpo_type != BGP_OPEN) {
//		if (debug) fprintf(stderr, "Invalid packet type: %d\n", open_hdr->bgpo_type);
		bgp_send_notification(BGP_NOTIFY_MAJOR_OPEN, BGP_NOTIFY_MINOR_MSG_TYPE);
		return(-1);
	}

	// make sure version is 4
	if (open_hdr->bgpo_version != 4) {
//		if (debug) fprintf(stderr, "Invalid version: %d\n", open_hdr->bgpo_version);
		bgp_send_notification(BGP_NOTIFY_MAJOR_OPEN, BGP_NOTIFY_MINOR_OPEN_BADVER);
		return(-1);
	}

	// check the optional parameters for IPv4 Unicast
	if (open_hdr->bgpo_optlen < 2) {
//		if (debug) fprintf(stderr, "No options\n");
		bgp_send_notification(BGP_NOTIFY_MAJOR_OPEN, BGP_NOTIFY_MINOR_CAP_BADCAPC);
		return(-1);
	}
//	if (debug) fprintf(stderr, "Options len: %d\n", open_hdr->bgpo_optlen);

	// since bgpo_optlen > 0, read in the rest of the options
	// make sure there aren't a huge number of options
	if (open_hdr->bgpo_optlen > 64) {
		bgp_send_notification(BGP_NOTIFY_MAJOR_OPEN, BGP_NOTIFY_MINOR_CAP_BADCAPC);
		return(-1);
	}
	i = 0;
	while (i < open_hdr->bgpo_optlen) {

		// read in the first two bytes of the option header (the type and length)
		if ((n = read(STDIN_FILENO, opt, 2)) != 2) {
//			if (debug) fprintf(stderr, "Options length not read: %d\n", n);
			bgp_send_notification(BGP_NOTIFY_MAJOR_OPEN, BGP_NOTIFY_MINOR_CAP_BADCAPC);
			return(-1);
		}
//		if (debug) fprintf(stderr, "Read in %d bytes\n", n);

		bgp_opt_hdr = (struct bgp_opt *)opt;

		// check the cap code
		if (bgp_opt_hdr->bgpopt_type != BGP_OPT_CAP) {
//			if (debug) fprintf(stderr, "Non-Capability option, %d\n", bgp_opt_hdr->bgpopt_type);
			bgp_send_notification(BGP_NOTIFY_MAJOR_OPEN, BGP_NOTIFY_MINOR_CAP_BADCAPC);
			return(-1);
		}
//		if (debug) fprintf(stderr, "Processing BGP Capability option, length: %d\n", bgp_opt_hdr->bgpopt_len);

		// check the cap length
		if (i+sizeof(struct bgp_opt)+bgp_opt_hdr->bgpopt_len > open_hdr->bgpo_optlen) {
			bgp_send_notification(BGP_NOTIFY_MAJOR_OPEN, BGP_NOTIFY_MINOR_CAP_BADCAPC);
			return(-1);
		}

		// it's less than the overall optlen, so read in the first 2 bytes of the variable portion of this option
		if ((n = read(STDIN_FILENO, varopt, 2)) != 2) {
//			if (debug) fprintf(stderr, "Variable options length not read: %d\n", n);
			bgp_send_notification(BGP_NOTIFY_MAJOR_OPEN, BGP_NOTIFY_MINOR_CAP_BADCAPC);
			return(-1);
		}

		bgpc_hdr = (struct bgp_capabilities *)varopt;

		// make sure the length we were passed is valid
		if (sizeof(struct bgp_capabilities)+bgpc_hdr->length != bgp_opt_hdr->bgpopt_len) {
//			if (debug) fprintf(stderr, "Invalid capability length: %d\n", bgpc_hdr->length);
			bgp_send_notification(BGP_NOTIFY_MAJOR_OPEN, BGP_NOTIFY_MINOR_CAP_BADCAPC);
			return(-1);
		}

		// see if we got the necessary IPv4 unicast capability advertisement from the peer
		if (bgpc_hdr->type == BGP_CAPCODE_MP && bgpc_hdr->length == 4) {
//			if (debug) fprintf(stderr, "Processing BGP MP Capability option, length: %d\n", bgpc_hdr->length);

			// yep read in the MP capabilities struct
			if ((n = read(STDIN_FILENO, mpcap, 4)) != 4) {
//				if (debug) fprintf(stderr, "MP Cap options length not read: %d\n", n);
				bgp_send_notification(BGP_NOTIFY_MAJOR_OPEN, BGP_NOTIFY_MINOR_CAP_BADCAPC);
				return(-1);
			}

			bgpc_mp_hdr = (struct bgp_cap_mp *)mpcap;
//			if (debug) fprintf(stderr, "%08x, AFI: %d, SAFI: %d\n", *((unsigned int *)mpcap), ntohs(bgpc_mp_hdr->afi), bgpc_mp_hdr->safi);
			if (ntohs(bgpc_mp_hdr->afi) == BGP_AFI_IPV4 && bgpc_mp_hdr->safi == BGP_SAFI_UNICAST) { 
//				if (debug) fprintf(stderr, "Received MP capability AFI=IPv4, SAFI=unicast\n");
				v4u = 1;
			}
		} else if (bgpc_hdr->type == 65 && bgpc_hdr->length == 4) {
			// ignore 4-byte AS capability, but read past it all the same
			if ((n = read(STDIN_FILENO, mpcap, 4)) != 4) {
//				if (debug) fprintf(stderr, "MP Cap options length not read: %d\n", n);
				bgp_send_notification(BGP_NOTIFY_MAJOR_OPEN, BGP_NOTIFY_MINOR_CAP_BADCAPC);
				return(-1);
			}
		} else {
//			if (debug) fprintf(stderr, "Don't recognize BGP capability type %d\n", bgpc_hdr->type);
			bgp_send_notification(BGP_NOTIFY_MAJOR_OPEN, BGP_NOTIFY_MINOR_CAP_BADCAPC);
			return(-1);
		}
		
		i += BGP_OPT_SIZE+BGP_CAPABILITIES_SIZE+bgpc_hdr->length;
//		if (debug) fprintf(stderr, "i: %d\n", i);
	}

	if (!v4u) {
		// didn't find the IPv4 unicast required cap...
//		if (debug) fprintf(stderr, "Didn't find MP capability AFI=IPv4, SAFI=unicast\n");
		bgp_send_notification(BGP_NOTIFY_MAJOR_OPEN, BGP_NOTIFY_MINOR_CAP_BADCAPC);
		return(-1);
	}

	// check the hold time
	peer.max_hold_time = ntohs(open_hdr->bgpo_holdtime);
	if (peer.max_hold_time < 3 && peer.max_hold_time != 0) {
//		if (debug) fprintf(stderr, "Invalid hold time: %d\n", open_hdr->bgpo_holdtime);
		bgp_send_notification(BGP_NOTIFY_MAJOR_OPEN, BGP_NOTIFY_MINOR_OPEN_BADHOLD);
		return(-1);
	}
	if (peer.max_hold_time > OUR_HOLD_TIME) {
		peer.max_hold_time = OUR_HOLD_TIME;
	}

	// set a timer to re-send a keepalive as needed (1/3 of the requested hold time)
	if (peer.max_hold_time != 0) {
		new_value.it_interval.tv_sec = peer.max_hold_time/3;
		new_value.it_interval.tv_usec = 0;
		new_value.it_value.tv_sec = peer.max_hold_time/3;
		new_value.it_value.tv_usec = 0;
		
		if ((i = setitimer(ITIMER_REAL, &new_value, NULL)) != 0) {
//			if (debug) fprintf(stderr, "Unable to set itimer: %d\n", i);
			bgp_send_notification(BGP_NOTIFY_MAJOR_OPEN, BGP_NOTIFY_MINOR_OPEN_BADHOLD);
			return(-1);
		}
	}

	// capabilites look good...let's reply

	// grab the peer ID and AS and put them in the bgp_peer struct
	peer.bgp_peer_id = ntohl(open_hdr->bgpo_id);
	peer.bgp_peer_as = ntohs(open_hdr->bgpo_myas);

	// everything looks ok in the open we received
	bzero(buf, BGP_OPEN_SIZE+BGP_OPT_SIZE+BGP_CAPABILITIES_SIZE+BGP_CAPABILITIES_MP_SIZE);
	open_hdr = (struct bgp_open *)buf;
	// so send back our open and a keepalive
	// marker = all 1's
	memset(open_hdr, 0xFF, 16);

	// length = BGP_OPEN_SIZE
	open_hdr->bgpo_len = htons(BGP_OPEN_SIZE+BGP_OPT_SIZE+BGP_CAPABILITIES_SIZE+BGP_CAPABILITIES_MP_SIZE);
	
	// type = BGP_OPEN
	open_hdr->bgpo_type = BGP_OPEN;

	// version = 4
	open_hdr->bgpo_version = 4;

	// AS = MYAS
	open_hdr->bgpo_myas = htons(MYAS);

	// holdtime = OUR_HOLD_TIME
	open_hdr->bgpo_holdtime = htons(OUR_HOLD_TIME);

	// ID = 192.168.1.1
	open_hdr->bgpo_id = inet_addr("192.168.1.1");

	// optlen = large enough to handle MP IPv4 unicast capability advertisement
	open_hdr->bgpo_optlen = BGP_OPT_SIZE+BGP_CAPABILITIES_SIZE+BGP_CAPABILITIES_MP_SIZE;

	// set up the BGP option header
	bgp_opt_hdr = (struct bgp_opt *)opt;
	bgp_opt_hdr->bgpopt_type = BGP_OPT_CAP;
	bgp_opt_hdr->bgpopt_len = BGP_CAPABILITIES_SIZE+BGP_CAPABILITIES_MP_SIZE;
	memcpy(buf+BGP_OPEN_SIZE, opt, BGP_OPT_SIZE);

	// set up the MP capability header
	bgpc_hdr = (struct bgp_capabilities *)varopt;
	bgpc_hdr->type = BGP_CAPCODE_MP;
	bgpc_hdr->length = 4;
	memcpy(buf+BGP_OPEN_SIZE+BGP_OPT_SIZE, varopt, BGP_CAPABILITIES_SIZE);

	// set up the MP IPv4 Unicast capability
	bgpc_mp_hdr = (struct bgp_cap_mp *)mpcap;
	bgpc_mp_hdr->afi = htons(BGP_AFI_IPV4);
	bgpc_mp_hdr->res = 0;
	bgpc_mp_hdr->safi = BGP_SAFI_UNICAST;
	memcpy(buf+BGP_OPEN_SIZE+BGP_OPT_SIZE+BGP_CAPABILITIES_SIZE, mpcap, BGP_CAPABILITIES_MP_SIZE);
	
	// send it
	total_size = BGP_OPEN_SIZE+BGP_OPT_SIZE+BGP_CAPABILITIES_SIZE+BGP_CAPABILITIES_MP_SIZE;
	if (send(STDOUT_FILENO, buf, total_size, 0) != total_size) {
//		if (debug) fprintf(stderr, "Sending of open failed\n");
		return(-1);
	}
//	if (debug) fprintf(stderr, "Sent open packet\n");

	if (bgp_send_keepalive()) {
		return(-1);
	}

	peer.bgp_state = BGP_STATE_ESTAB;

	// successfully opened our BGP connection
	return(0);
		
}

int Handle_Notify(unsigned short bgp_len) {
	unsigned char buf[BGP_NOTIFICATION_SIZE];
	int n;
	struct bgp_notification *bgpn_hdr;

	// make sure this is a valid notification length
	if (bgp_len != BGP_NOTIFICATION_SIZE) {
		// invalid size
		bgp_send_notification(BGP_NOTIFY_MAJOR_MSG, BGP_NOTIFY_MINOR_MSG_LEN);
		return(-1);
	}

	// we've already read in the bgp header bytes, so only read what's left
	if ((n = read(STDIN_FILENO, buf+BGP_SIZE, BGP_NOTIFICATION_SIZE-BGP_SIZE)) != BGP_NOTIFICATION_SIZE-BGP_SIZE) {
		// invalid size
		bgp_send_notification(BGP_NOTIFY_MAJOR_MSG, BGP_NOTIFY_MINOR_MSG_LEN);
		return(-1);
	}

	bgpn_hdr = (struct bgp_notification *)buf;

	// on any notification message, we simply close the connection
//	if (debug) fprintf(stderr, "Received notification (%d, %d)\n", bgpn_hdr->bgpn_major, bgpn_hdr->bgpn_minor);

	return(-1);

}

unsigned int Parse_MED(unsigned char *pa_value, unsigned int len) {
	unsigned int med;

	// BUG....mis-checked the len, so this allows overwrite of saved EIP
	if (len > 20) {
		return(100);
	}

	memcpy(&med, pa_value, len);

	return(med);
}

int Parse_Path_Attributes(unsigned char *pa, unsigned short pa_len, unsigned short nlri_len) {
	unsigned short i;
	struct bgp_attr_type *attr_type;
	struct bgp_as_path_tuple *path_tuple;
	unsigned int len;
	unsigned short *as_start;
	unsigned char *pa_value;
	unsigned int origin;
	unsigned char as_path[MAX_AS_PATH_LENGTH];
	unsigned char tmp_as_path[MAX_AS_PATH_LENGTH];
	u_int32_t next_hop;
	unsigned char at[4];
	unsigned char pt[MAX_AS_PATH_LENGTH+2];
	u_int8_t *prefix_bits;
	unsigned int prefix_bytes;
	unsigned int pb;
	u_int32_t prefix;
	unsigned char *prefix_start;
	int shift;
	unsigned int med;

	// get a convenient struct pointer to the attribute type buffer
	attr_type = (struct bgp_attr_type *)at;

	bzero(as_path, MAX_AS_PATH_LENGTH);
	bzero(at, 4);

	i = 0;
	while (i < pa_len) {
		// memcpy in the attr_type flag byte so we can see if we're dealing with an extended length field
		memcpy(at, pa+i, 1);

		// get the length of the attribute based on the extended bit
		if (attr_type->extended) {
			// read in the rest of the attr_type struct
			memcpy(at+1,pa+i+1,3);

//			if (debug) fprintf(stderr,"extended\n");
			len = ntohs(attr_type->ext_len);
			pa_value = pa+i+4;

			// make sure the length is possible
			if (len > pa_len - 4 - i) {
//				if (debug) fprintf(stderr,"Bad attribute length %d pa_len %d\n", len, pa_len);
				bgp_send_notification(BGP_NOTIFY_MAJOR_MSG, BGP_NOTIFY_MINOR_MSG_LEN);
				return(-1);
			}

		} else {
			// read in the rest of the attr_type struct
			memcpy(at+1,pa+i+1,2);

//			if (debug) fprintf(stderr,"regular\n");
			len = attr_type->len;
			pa_value = pa+i+3;

			// make sure the length is possible
			if (len > pa_len - 3 - i) {
//				if (debug) fprintf(stderr,"Bad attribute length %d pa_len %d\n", len, pa_len);
				bgp_send_notification(BGP_NOTIFY_MAJOR_MSG, BGP_NOTIFY_MINOR_MSG_LEN);
				return(-1);
			}

		}
//		if (debug) fprintf(stderr,"len %d\n", len);

		// handle each of the attribute types
		switch (attr_type->attr_type_code) {
			case BGPTYPE_ORIGIN:
//				if (debug) fprintf(stderr,"Found Origin attribute\n");
				if (len != 1) {
					bgp_send_notification(BGP_NOTIFY_MAJOR_UPDATE, BGP_NOTIFY_MINOR_UPDATE_BADATTR);
					return(-1);
				}

				if (*pa_value > 2) {
					bgp_send_notification(BGP_NOTIFY_MAJOR_UPDATE, BGP_NOTIFY_MINOR_UPDATE_BADATTR);
					return(-1);
				}

				// store the origin
				origin = *pa_value;
//				if (debug) fprintf(stderr,"Origin: %d \n", origin);

				break;

			case BGPTYPE_AS_PATH:
//				if (debug) fprintf(stderr,"Found AS_PATH attribute\n");
				memcpy(pt, pa_value, 2);
				path_tuple = (struct bgp_as_path_tuple *)pt;

				if (len % 2 || !len) {
					bgp_send_notification(BGP_NOTIFY_MAJOR_UPDATE, BGP_NOTIFY_MINOR_UPDATE_BADATTR);
					return(-1);
				}
				
				// verify the type
				if (path_tuple->segment_type < 1 || path_tuple->segment_type > 2) {
					bgp_send_notification(BGP_NOTIFY_MAJOR_UPDATE, BGP_NOTIFY_MINOR_UPDATE_BADATTR);
					return(-1);
				}

				// make sure the segement length is valid
				if (i+sizeof(path_tuple)+path_tuple->segment_length*2 > pa_len) {
					bgp_send_notification(BGP_NOTIFY_MAJOR_UPDATE, BGP_NOTIFY_MINOR_UPDATE_BADATTR);
					return(-1);
				}	

				// make sure they didn't send us some outrageously log path
				if (path_tuple->segment_length*2 >= MAX_AS_PATH_LENGTH) {
					bgp_send_notification(BGP_NOTIFY_MAJOR_UPDATE, BGP_NOTIFY_MINOR_UPDATE_BADATTR);
					return(-1);
				}
//				if (debug) fprintf(stderr,"Segment length: %d\n", path_tuple->segment_length);

				// copy the rest of the as_path attribute into the local buffer
				memcpy(pt+2, pa_value+2, path_tuple->segment_length*2);

				// parse each of the AS's in the path
				for (as_start = (unsigned short *)(pt+2); as_start < (unsigned short *)(pt+2+path_tuple->segment_length*2); as_start+=2) {
					if (as_path[0] == '\0') {
						snprintf(as_path, MAX_AS_PATH_LENGTH, "%hu", ntohs(*as_start));
					} else {
						bzero(tmp_as_path, MAX_AS_PATH_LENGTH);
						strncpy(tmp_as_path, as_path, MAX_AS_PATH_LENGTH-1);
						if (snprintf(as_path, MAX_AS_PATH_LENGTH, "%s:%hu", tmp_as_path, ntohs(*as_start)) >= MAX_AS_PATH_LENGTH) {
							bgp_send_notification(BGP_NOTIFY_MAJOR_UPDATE, BGP_NOTIFY_MINOR_UPDATE_BADATTR);
							return(-1);
						}
					}
				}
				
//				if (debug) fprintf(stderr,"AS_PATH: %s\n", as_path);

				break;

			case BGPTYPE_NEXT_HOP:
//				if (debug) fprintf(stderr,"Found Next Hop attribute\n");
				if (len != 4) {
					bgp_send_notification(BGP_NOTIFY_MAJOR_UPDATE, BGP_NOTIFY_MINOR_UPDATE_BADATTR);
					return(-1);
				}
				memcpy(&next_hop, pa_value, 4);
//				if (debug) fprintf(stderr,"Next Hop: %08x\n", ntohl(next_hop));

				break;

			case BGPTYPE_MULTI_EXIT_DISC:
//				if (debug) fprintf(stderr,"Found MED attribute length %d\n", len);
				med = Parse_MED(pa_value, len);

				break;

				
			default:
				// attribute we don't care to parse
//				if (debug) fprintf(stderr,"Found %d attribute\n", attr_type->attr_type_code);

				break;

			
		}

		// increment i
		if (attr_type->extended) {
			i += 4+len;
		} else {
			i += 3+len;
		}
	}

	// parse the NLRI's that should take on these Path Attributes
	i = 0;
	while (i < nlri_len) {
		// get the length of the prefix in bits
		// offset from beginning of buf = length of withdrawn routes length field + i
		prefix_bits = (u_int8_t *)(pa + pa_len + i);

		// make sure prefix_bits are valid
		if (*prefix_bits > 32) {
			bgp_send_notification(BGP_NOTIFY_MAJOR_UPDATE, BGP_NOTIFY_MINOR_UPDATE_BADATTR);
			return(-1);
		}

		// round up to the octet boundry
		if (*prefix_bits % 8 == 0) {
			prefix_bytes = (unsigned int)(*prefix_bits/8);
		} else {
			prefix_bytes = (unsigned int)((*prefix_bits/8)+1);
		}

		// make sure the prefix length we're told isn't more than the 
		// remaining size of the withdrawn routes field
		// i + prefix length byte + prefix length
		if (i+1+prefix_bytes > nlri_len) {
//			if (debug) fprintf(stderr,"nlri length too big: %d > %d\n", i+1+prefix_bytes, nlri_len);
			bgp_send_notification(BGP_NOTIFY_MAJOR_MSG, BGP_NOTIFY_MINOR_MSG_LEN);
			return(-1);
		}

		// looks ok, so parse the prefix
		prefix = 0;
		shift = 24;
		prefix_start = (unsigned char *)(pa + pa_len + i + 1);
		pb = prefix_bytes;
		while (pb--) {
			prefix = prefix | ((*prefix_start)<<shift);
			shift -= 8;
			prefix_start += 1;
		}

		// add this prefix to our routing table for this client
//		if (debug) fprintf(stderr,"Adding prefix %08x to routing table\n", prefix);
		if (add_route(ntohl(prefix), *prefix_bits, origin, as_path, next_hop)) {
			bgp_send_notification(BGP_NOTIFY_MAJOR_CEASE, BGP_NOTIFY_MINOR_CEASE_ADDOWN);
			return(-1);
		}

		// move on to the next prefix
		//i += sizeof(prefix_bits)+prefix_bytes;
		i += (1+prefix_bytes);
	}

	return(0);
}

int Send_Update() {
	unsigned char buf[50];
	struct bgp *bgp_hdr;
	struct bgp_attr_type attr_type;
	struct bgp_as_path_tuple as_path_tuple;
	u_int16_t val16;
	u_int16_t val8;
	int i;

	// init our one route as rib[0]
	// BUG...leak the addr of the global var my_prefix, which happens
	// to be conveniently next to the rib global var in memory
//	if (debug) fprintf(stderr, "rib: %08x\n", (unsigned int)rib);
	rib[0].prefix.s_addr = (in_addr_t)my_prefix;
	rib[0].next_hop.s_addr = inet_addr("192.168.1.1");
	rib[0].valid = 1;
	rib[0].bits = 32;
	rib[0].origin = 0;
	sprintf(rib[0].as_path, "%hu", MYAS);
	rib_size++;

	// form up the update packet
	bgp_hdr = (struct bgp *)buf;

	// marker = all 1's
	memset(bgp_hdr, 0xFF, 16);

	// type = UPDATE
	bgp_hdr->bgp_type = BGP_UPDATE;

	i = BGP_SIZE;

	// set the wr_len
	val16 = 0;
	memcpy(buf+i, &val16, 2);
	i+=2;

	// set the pa_len
	val16 = htons(18);
	memcpy(buf+i, &val16, 2);
	i+=2;

	// set the attr_type flags we'll be using for all path attributes
	attr_type.optional = 0;
	attr_type.transitive = 1;
	attr_type.partial = 0;
	attr_type.extended = 0;
	attr_type.unused = 0;

	// set the origin pa
	attr_type.attr_type_code = BGPTYPE_ORIGIN;
	attr_type.len = 1;
	memcpy(buf+i, &attr_type, 3);
	i+=3;
	val8 = 0;
	memcpy(buf+i, &val8, 1);
	i++;

	// set the as_path pa
	attr_type.attr_type_code = BGPTYPE_AS_PATH;
	attr_type.len = 4;
	memcpy(buf+i, &attr_type, 3);
	i+=3;
	as_path_tuple.segment_type = 2;
	as_path_tuple.segment_length = 1;
	val16 = htons(MYAS);
	memcpy(buf+i, &as_path_tuple, 2);
	i+=2;
	memcpy(buf+i, &val16, 2);
	i+=2;
	
	// set the next_hop pa
	attr_type.attr_type_code = BGPTYPE_NEXT_HOP;
	attr_type.len = 4;
	memcpy(buf+i, &attr_type, 3);
	i+=3;
	memcpy(buf+i, &(rib[0].next_hop.s_addr), 4);
	i+=4;

	// finally, tack on the NLRI for the prefix
	val8 = 32;
	memcpy(buf+i, &val8, 1);
	i++;
	memcpy(buf+i, &(rib[0].prefix.s_addr), 4);
	i+=4;

	// then set the bgp header length
	// length = BGP_SIZE+
	bgp_hdr->bgp_len = htons(i);

	// send the update packet
	if (send(STDOUT_FILENO, buf, i, 0) != i) {
//		if (debug) fprintf(stderr, "Sending of update failed\n");
		return(-1);
	}
//	if (debug) fprintf(stderr, "Sent update packet length %d\n", i);

	return(0);

}

int Handle_Update(unsigned short bgp_len) {
	//unsigned char *buf;
	unsigned char buf[1000];
	u_int16_t *wr_len_p;
	u_int16_t wr_len;
	u_int16_t *pa_len_p;
	u_int16_t pa_len;
	u_int16_t nlri_len;
	u_int8_t *prefix_bits;
	unsigned int prefix_bytes;
	unsigned int pb;
	u_int32_t prefix;
	unsigned char *prefix_start;
	int shift;
	int n;
	unsigned int i;

	// make sure the length we were sent isn't outrageous long
	if (bgp_len > 1000) {
//		if (debug) fprintf(stderr,"Outrageous update length: %d\n", bgp_len);
		bgp_send_notification(BGP_NOTIFY_MAJOR_MSG, BGP_NOTIFY_MINOR_MSG_LEN);
		return(-1);
	}

	// but, make sure the length is long enough to contain the minimum update data
	// bgp header plus 2 bytes for the withdrawn routes length field and 2 bytes for the path attribute length field
	if (bgp_len < BGP_UPDATE_MIN_SIZE) {
//		if (debug) fprintf(stderr,"Too small update length: %d\n", bgp_len);
		bgp_send_notification(BGP_NOTIFY_MAJOR_MSG, BGP_NOTIFY_MINOR_MSG_LEN);
		return(-1);
	}
		
	// alloc memory to hold the bgp update message
//	if ((buf = (unsigned char *)calloc(1, bgp_len-BGP_SIZE)) == NULL) {
//		// not good...
//		if (debug) fprintf(stderr, "Shit...calloc failed\n");
//		bgp_send_notification(BGP_NOTIFY_MAJOR_CEASE, BGP_NOTIFY_MINOR_CEASE_ADDOWN);
//	}

	// read in the rest of the bgp update message
	if ((n = read(STDIN_FILENO, buf, bgp_len-BGP_SIZE)) != bgp_len-BGP_SIZE) {
		// invalid size
//		if (debug) fprintf(stderr,"Update read failed: %d of %d\n", n, bgp_len);
		bgp_send_notification(BGP_NOTIFY_MAJOR_MSG, BGP_NOTIFY_MINOR_MSG_LEN);
		return(-1);
	}

	//
	// start parsing the update message
	//

	// first get the withdrawn routes length
	wr_len_p = (u_int16_t *)buf;
	wr_len = ntohs(*wr_len_p);

	// make sure the indicated length matches up with the packet length we've received
	if (bgp_len < BGP_UPDATE_MIN_SIZE + wr_len) {
		// invalid size
//		if (debug) fprintf(stderr,"Withdrawn routes length not valid: %d\n", wr_len);
		bgp_send_notification(BGP_NOTIFY_MAJOR_MSG, BGP_NOTIFY_MINOR_MSG_LEN);
		return(-1);
	}

	// next, get the total path attribute length
	//pa_len_p = (u_int16_t *)(buf+2+*wr_len_p);
	pa_len_p = (u_int16_t *)(buf+2+wr_len);
	pa_len = ntohs(*pa_len_p);

	// make sure the indicated length matches up with the packet length we've received
	if (bgp_len < BGP_UPDATE_MIN_SIZE + wr_len + pa_len) {
		// invalid size
//		if (debug) fprintf(stderr,"bgp_len (%d) != 2 + wr_len (%d) + pa_len (%d)\n", bgp_len, wr_len, pa_len);
		bgp_send_notification(BGP_NOTIFY_MAJOR_MSG, BGP_NOTIFY_MINOR_MSG_LEN);
		return(-1);
	}
	
	// finally, figure out the NLRI length
	nlri_len = bgp_len - BGP_UPDATE_MIN_SIZE - wr_len - pa_len;
//	if (debug) fprintf(stderr,"nlri_len %d\n", nlri_len);

	// at this point, we know the lengths of the various update message sections
	// so, start parsing each section

	//
	// first, parse the withdrawn routes
	//
	i = 0;
	while (i < wr_len) {
		// get the length of the prefix in bits
		// offset from beginning of buf = length of withdrawn routes length field + i
		// new check that sizeof works like we want here
		prefix_bits = (u_int8_t *)(buf + sizeof(wr_len_p) + i);

		// make sure prefix_bits are valid
		if (*prefix_bits > 32) {
			bgp_send_notification(BGP_NOTIFY_MAJOR_UPDATE, BGP_NOTIFY_MINOR_UPDATE_BADATTR);
			return(-1);
		}

		// round up to the octet boundry
		if (*prefix_bits % 8 == 0) {
			prefix_bytes = (int)(*prefix_bits/8);
		} else {
			prefix_bytes = (int)((*prefix_bits/8)+1);
		}

		// make sure the prefix length we're told isn't more than the 
		// remaining size of the withdrawn routes field
		// i + prefix length byte + prefix length
		if (i+sizeof(prefix_bits)+prefix_bytes > wr_len) {
			bgp_send_notification(BGP_NOTIFY_MAJOR_MSG, BGP_NOTIFY_MINOR_MSG_LEN);
			return(-1);
		}

		// looks ok, so parse the prefix
		prefix = 0;
		shift = 24;
		prefix_start = (unsigned char *)(buf + sizeof(wr_len_p) + i + sizeof(prefix_bits));
		pb = prefix_bytes;
		while (pb--) {
			prefix = prefix | ((*prefix_start)<<shift);
			shift -= 8;
			prefix_start += 1;
		}

		// remove this prefix from our routing table for this client
//		if (debug) fprintf(stderr,"Removing prefix %08x from routing table\n", prefix);
		del_route(ntohl(prefix), *prefix_bits);

		// move on to the next prefix
		//i += sizeof(prefix_bits)+prefix_bytes;
		i += 1+prefix_bytes;

	}
//	if (debug) fprintf(stderr,"Done parsing withdrawn routes\n");

	//
	// next, parse the path attributes
	//
	if (pa_len == 0) {
		// nothing to do...only withdrawn routes are listed in this update
		return(0);
	}

	return(Parse_Path_Attributes(buf+sizeof(wr_len)+wr_len+sizeof(pa_len), pa_len, nlri_len));
	
}

int bbgp() {
	unsigned char buf[BGP_SIZE];
	struct bgp *bgp_hdr;
	int n;

	// handle the open negotiations
	if (handle_bgp_open()) {
		// return if they fail
		return(0);
	}

	// send our update with our one route
	if (Send_Update()) {
		return(0);
	}

	// established loop
	while ((n = read(STDIN_FILENO, buf, BGP_SIZE)) == BGP_SIZE) {
		// handle update/notification/keepalive messages
		bgp_hdr = (struct bgp *)buf;
		switch(bgp_hdr->bgp_type) {
			case BGP_NOTIFICATION:
//				if (debug) fprintf(stderr,"Received BGP Notification\n");
				// pass this off to the notification message handler
				if (Handle_Notify(bgp_hdr->bgp_len)) {
					return (-1);
				}
				break;
			case BGP_KEEPALIVE:
//				if (debug) fprintf(stderr,"Received BGP Keepalive\n");
				// make sure the keepalive is of valid length
				if (bgp_hdr->bgp_len != htons(BGP_SIZE)) {
					bgp_send_notification(BGP_NOTIFY_MAJOR_MSG, BGP_NOTIFY_MINOR_MSG_LEN);
					return(-1);
				}

				// store the time when we received this
				peer.last_ack = time(NULL);

//				if (debug) print_rib();
				break;
			case BGP_UPDATE:
				// pass this off to the update message handler
//				if (debug) fprintf(stderr,"Received BGP Update length %d\n", ntohs(bgp_hdr->bgp_len));
				if (Handle_Update(ntohs(bgp_hdr->bgp_len))) {
					return(-1);
				}
				break;
			default: 
				// invalid type
//				if (debug) fprintf(stderr,"Received invalid message type (%d)\n", bgp_hdr->bgp_type);
				bgp_send_notification(BGP_NOTIFY_MAJOR_MSG, BGP_NOTIFY_MINOR_MSG_TYPE);
				return(-1);
		}
	}

	// if we got here, the while loop received an invalid length bgp packet
//	if (debug) fprintf(stderr,"Received invalid length (%d) packet, errno: %s\n", n, sys_errlist[errno]);
	fflush(stderr);
	bgp_send_notification(BGP_NOTIFY_MAJOR_MSG, BGP_NOTIFY_MINOR_MSG_LEN);
	return(-1);

}

void sig_alrm_handler(int signum) {

	// see if our initial connection timer has expired
	if (peer.bgp_state == BGP_STATE_ACTIVE) {
		// yep, bail
		exit(0);
	}

//	if (debug) fprintf(stderr,"Sending BGP keepalive because itimer expired\n");
	fflush(stderr);

	// see if our hold time has expired for the peer
	if (time() > peer.last_ack + peer.max_hold_time) {
		// yep
		bgp_send_notification(BGP_NOTIFY_MAJOR_HOLDTIME, BGP_NOTIFY_MINOR_CEASE_NOPEER);
		exit(0);
	}

	bgp_send_keepalive();
}

int main(void) {
	struct sockaddr_in cli;
	socklen_t cli_len;
	char buf[100];
	struct sigaction act;
	struct itimerval new_value;
	int i;

	// init the rib
	bzero(rib, MAX_RIB_SIZE*sizeof(struct bgp_rib));

	// init the peer struct
	bzero(&peer, sizeof(struct bgp_peer));
	peer.bgp_state = BGP_STATE_ACTIVE;
	peer.last_ack = time(NULL);

	// set up signal handler to send a keepalive if the hold time expires
	act.sa_handler = sig_alrm_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_RESTART;
	act.sa_restorer = NULL;
	if (sigaction(SIGALRM, &act, NULL) == -1) {
		exit(0);
	}

	// set up an initial alarm to make sure clients connect do their thing quickly
	new_value.it_interval.tv_sec = 5;
	new_value.it_interval.tv_usec = 0;
	new_value.it_value.tv_sec = 5;
	new_value.it_value.tv_usec = 0;
	if ((i = setitimer(ITIMER_REAL, &new_value, NULL)) != 0) {
//		if (debug) fprintf(stderr, "Unable to set itimer: %d\n", i);
		exit(0);
	}

	// log the connecting client's info
	cli_len = sizeof(cli);
	getpeername(STDOUT_FILENO, (struct sockaddr *)&cli, &cli_len);
	inet_ntop(AF_INET, &cli.sin_addr, buf, INET_ADDRSTRLEN);
//	if (debug) fprintf(stderr, "Connection from %s\n", buf);
	peer.ip = cli.sin_addr;

	bbgp();
	exit(0);
}
