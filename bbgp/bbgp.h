struct bgp {
	u_int8_t 	bgp_marker[16];
	u_int16_t 	bgp_len;
	u_int8_t	bgp_type;
};
#define BGP_SIZE		19

#define BGP_OPEN 		1
#define BGP_UPDATE 		2
#define BGP_NOTIFICATION	3
#define BGP_KEEPALIVE		4
#define BGP_ROUTE_REFRESH	5

struct bgp_open {
	u_int8_t	bgpo_marker[16];
	u_int16_t	bgpo_len;
	u_int8_t	bgpo_type;
	u_int8_t	bgpo_version;
	u_int16_t	bgpo_myas;
	u_int16_t	bgpo_holdtime;
	u_int32_t	bgpo_id;
	u_int8_t	bgpo_optlen;
	/* options should follow */
};
#define BGP_OPEN_SIZE		29

#define BGP_OPT_AUTH			1
#define BGP_OPT_CAP			2

struct bgp_opt {
	u_int8_t	bgpopt_type;
	u_int8_t	bgpopt_len;
	/* variable length */
};
#define BGP_OPT_SIZE			2

#define BGP_CAPCODE_MP			1
#define BGP_CAPCODE_RR			2
#define BGP_CAPCODE_RESTART            64 /* draft-ietf-idr-restart-05  */
#define BGP_CAPCODE_RR_CISCO          128

struct capability_opt {
	u_int8_t	capopt_code;
	u_int8_t	capopt_len;
	/* variable length */
};

#define BGP_NOTIFY_MAJOR_MSG		1
#define BGP_NOTIFY_MAJOR_OPEN		2
#define BGP_NOTIFY_MAJOR_UPDATE		3
#define BGP_NOTIFY_MAJOR_HOLDTIME	4
#define BGP_NOTIFY_MAJOR_FSM		5
#define BGP_NOTIFY_MAJOR_CEASE		6
#define BGP_NOTIFY_MAJOR_CAP		7

#define BGP_NOTIFY_MINOR_MSG_SYN	1
#define BGP_NOTIFY_MINOR_MSG_LEN	2
#define BGP_NOTIFY_MINOR_MSG_TYPE	3

#define BGP_NOTIFY_MINOR_OPEN_BADVER	1
#define BGP_NOTIFY_MINOR_OPEN_BADAS	2
#define BGP_NOTIFY_MINOR_OPEN_BADID	3
#define BGP_NOTIFY_MINOR_OPEN_BADOPT	4
#define BGP_NOTIFY_MINOR_OPEN_AUTHFAIL	5
#define BGP_NOTIFY_MINOR_OPEN_BADHOLD	6

#define BGP_NOTIFY_MINOR_UPDATE_BADATTR	1
#define BGP_NOTIFY_MINOR_UPDATE_UNKATTR	2
#define BGP_NOTIFY_MINOR_UPDATE_MISATTR	3
#define BGP_NOTIFY_MINOR_UPDATE_ATTRFLG	4
#define BGP_NOTIFY_MINOR_UPDATE_ATTRLEN	5
#define BGP_NOTIFY_MINOR_UPDATE_INVORIG	6
#define BGP_NOTIFY_MINOR_UPDATE_ASLOOP	7
#define BGP_NOTIFY_MINOR_UPDATE_INVNH	8
#define BGP_NOTIFY_MINOR_UPDATE_OPTERR	9
#define BGP_NOTIFY_MINOR_UPDATE_INVNET	10
#define BGP_NOTIFY_MINOR_UPDATE_BADPATH	11

#define BGP_NOTIFY_MINOR_CAP_INVACT	1
#define BGP_NOTIFY_MINOR_CAP_INVCAP	2
#define BGP_NOTIFY_MINOR_CAP_BADCAPV	3
#define BGP_NOTIFY_MINOR_CAP_BADCAPC	4

#define BGP_NOTIFY_MINOR_CEASE_MAXPRFX	1
#define BGP_NOTIFY_MINOR_CEASE_ADDOWN	2
#define BGP_NOTIFY_MINOR_CEASE_NOPEER	3
#define BGP_NOTIFY_MINOR_CEASE_ADRST	4
#define BGP_NOTIFY_MINOR_CEASE_CONNREJ	5
#define BGP_NOTIFY_MINOR_CEASE_CONFCHG	6
#define BGP_NOTIFY_MINOR_CEASE_COLLRES	7

struct bgp_notification {
	u_int8_t	bgpn_marker[16];
	u_int16_t	bgpn_len;
	u_int8_t	bgpn_type;
	u_int8_t	bgpn_major;
	u_int8_t	bgpn_minor;
};
#define BGP_NOTIFICATION_SIZE		21

/*
BGP update format (too variable for a struct)

	Withdrawn Routes Length (2 octets)
	Withdrawn Routes (variable)
		Length (1 octet)
		Prefix (variable)
	Total Path Attribute Length (2 octets)
	Path Attributes (variable)
		Attribute Type (2 octets)
			Attribute Flags (1 octet)
			Attribute Type Code (1 octet)
		Attribute Length (2 octets)
		Attribute Value (variable)
	Network Layer Reachability Information (variable)
		Length (1 octet)
		Prefix (variable)
*/
		
#define BGP_STATE_IDLE			1
#define BGP_STATE_CONN			2
#define BGP_STATE_OPENSENT		3
#define BGP_STATE_OPENCONF		4
#define BGP_STATE_ACTIVE		5
#define BGP_STATE_ESTAB			6

struct bgp_peer {
	u_int8_t	bgp_state;
	u_int32_t	bgp_peer_id;
	u_int16_t	bgp_peer_as;
	struct in_addr	ip;	
	u_int16_t	max_hold_time;
	time_t		last_ack;
};

#define BGP_UPDATE_MIN_SIZE 23

struct bgp_attr_type {
	u_int8_t	unused:4;
	u_int8_t	extended:1;
	u_int8_t	partial:1;
	u_int8_t	transitive:1;
	u_int8_t	optional:1;
	u_int8_t	attr_type_code;
	union {
		u_int8_t	len;
		u_int16_t	ext_len;
	};
};

#define BGPTYPE_ORIGIN			1
#define BGPTYPE_AS_PATH			2
#define BGPTYPE_NEXT_HOP		3
#define BGPTYPE_MULTI_EXIT_DISC		4
#define BGPTYPE_LOCAL_PREF		5
#define BGPTYPE_ATOMIC_AGGREGATE	6
#define BGPTYPE_AGGREGATOR		7
#define	BGPTYPE_COMMUNITIES		8	/* RFC1997 */
#define	BGPTYPE_ORIGINATOR_ID		9	/* RFC1998 */
#define	BGPTYPE_CLUSTER_LIST		10	/* RFC1998 */
#define	BGPTYPE_DPA			11	/* draft-ietf-idr-bgp-dpa */
#define	BGPTYPE_ADVERTISERS		12	/* RFC1863 */
#define	BGPTYPE_RCID_PATH		13	/* RFC1863 */
#define BGPTYPE_MP_REACH_NLRI		14	/* RFC2283 */
#define BGPTYPE_MP_UNREACH_NLRI		15	/* RFC2283 */
#define BGPTYPE_EXTD_COMMUNITIES        16      /* draft-ietf-idr-bgp-ext-communities */

struct bgp_as_path_tuple {
	u_int8_t	segment_type;
	u_int8_t	segment_length; // number of AS's
	// variable length path segment value
	// AS's encoded a 2-byte values
};

struct bgp_capabilities {
	u_int8_t	type;
	u_int8_t	length;
	// variable length value
}; 
#define BGP_CAPABILITIES_SIZE		2

struct bgp_cap_mp {
	u_int16_t	afi;
	u_int8_t	res;
	u_int8_t	safi;
};
#define BGP_CAPABILITIES_MP_SIZE	4

#define BGP_CAPCODE_MP                  1
#define BGP_CAPCODE_RR                  2
#define BGP_CAPCODE_RESTART            64 /* draft-ietf-idr-restart-05  */
#define BGP_CAPCODE_RR_CISCO          128

#define BGP_AFI_IPV4			1
#define BGP_AFI_IPV6			2

#define BGP_SAFI_UNICAST		1
#define BGP_SAFI_MULTICAST		2

#define MAX_AS_PATH_LENGTH 	       64

struct bgp_rib {
	struct in_addr		prefix;
	struct in_addr		next_hop;
	u_int16_t		valid;
	u_int8_t		bits;
	unsigned char 		origin;
	unsigned char 		as_path[MAX_AS_PATH_LENGTH];
};

#define MAX_RIB_SIZE		      100
